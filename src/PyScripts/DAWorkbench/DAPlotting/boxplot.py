"""Box plot (seaborn ``boxplot`` style).

Computes five-number statistics (Q1 / median / Q3 / whiskerLower /
whiskerUpper) with ``numpy.percentile``, draws box bodies through the
embedded ``da_figure`` module (Qwt 7.3.4 ``QwtPlotBoxChart``), and overlays
outliers and optional mean markers as scatter points (``QwtPlotCurve``).
"""

import numpy as np

import da_figure

from DAWorkbench.DAPlotting._palette import get_palette
from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


def plot(df, column, chart, params=None, **kwargs):
    """Draw a box plot on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        First selected column name (passed positionally by the C++
        controller).  The full list of columns is read from
        ``params["columns"]``.
    chart : da_figure.ChartHandle
        Target chart obtained via ``da_figure.getChartHandle(...)`` or
        ``da_figure.getCurrentChart()``.
    params : dict, optional
        Plotting parameters. When called from C++ this is the primary
        channel (a QJsonObject converted to a Python dict); when called
        from Python directly, ``**kwargs`` is more natural. If both are
        supplied, ``kwargs`` takes precedence.
    **kwargs :
        Alternative way to pass parameters. Merged on top of ``params``.

    See ``plan/plan07-boxplot.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to boxplot.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    # The full column list lives under "columns"; fall back to [column]
    columns = p.get("columns")
    if not columns:
        columns = [column]
    if isinstance(columns, str):
        columns = [columns]

    hue = p.get("hue")
    if hue:
        _plot_grouped(df, columns, hue, chart, p)
    else:
        color = p.get("color", "#4C72B0")
        _plot_simple(df, columns, chart, p, color=color)

    # Chart decorations
    title = p.get("title", "Box Plot")
    _safe_set_title(chart, title)
    _safe_enable_grid(chart, True)
    _safe_enable_legend(chart, bool(hue))
    _safe_replot(chart)


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _is_valid_chart(chart):
    """Check that the chart handle is usable."""
    try:
        return chart.isValid() if hasattr(chart, "isValid") else chart is not None
    except Exception:
        return False


def _safe_set_title(chart, title):
    try:
        chart.setChartTitle(title)
    except Exception:
        pass


def _safe_set_axis_label(chart, axis, label):
    try:
        chart.setAxisLabel(axis, label)
    except Exception:
        pass


def _safe_enable_grid(chart, on):
    try:
        chart.enableGrid(on)
    except Exception:
        pass


def _safe_enable_legend(chart, on):
    try:
        chart.enableLegend(on)
    except Exception:
        pass


def _safe_replot(chart):
    try:
        chart.replot()
    except Exception:
        pass


def _hex_to_rgb_tuple(hex_color):
    """Convert a ``#RRGGBB`` hex string to an ``(r, g, b)`` tuple (0-255).

    The DA framework's ``QColor`` pybind11 type caster only accepts Python
    tuples / lists of 3 or 4 ints (or floats 0.0-1.0), not hex strings.
    This helper bridges that gap.
    """
    if isinstance(hex_color, (tuple, list)):
        return tuple(hex_color)
    if not isinstance(hex_color, str) or len(hex_color) < 7:
        return (76, 114, 176)  # fallback: seaborn deep[0]
    r = int(hex_color[1:3], 16)
    g = int(hex_color[3:5], 16)
    b = int(hex_color[5:7], 16)
    return (r, g, b)


def _with_alpha(hex_color, alpha):
    """Convert ``#RRGGBB`` + alpha (0.0-1.0) to an ``(r, g, b, a)`` tuple.

    ``alpha`` is a float in [0, 1]; the returned alpha channel is scaled to
    the 0-255 int range that ``QColor`` expects.
    """
    r, g, b = _hex_to_rgb_tuple(hex_color)
    a = int(max(0.0, min(1.0, alpha)) * 255 + 0.5)
    return (r, g, b, a)


# ---------------------------------------------------------------------------
# Five-number statistics
# ---------------------------------------------------------------------------

def _compute_box_stats(data, whis=1.5):
    """Compute box plot five-number statistics + outliers + mean.

    Parameters
    ----------
    data : numpy.ndarray
        1-D array of floats (NaNs already removed).
    whis : float
        Whisker length as a multiple of the IQR.

    Returns
    -------
    dict
        Keys: whisker_lower, q1, median, q3, whisker_upper, outliers, mean
    """
    if data.size == 0:
        raise ValueError("No valid data for box plot")

    q1, median, q3 = np.percentile(data, [25, 50, 75])
    iqr = q3 - q1

    lower_fence = q1 - whis * iqr
    upper_fence = q3 + whis * iqr

    # Whisker endpoints: actual min/max within the fence
    in_range = data[(data >= lower_fence) & (data <= upper_fence)]
    if in_range.size > 0:
        whisker_lower = float(in_range.min())
        whisker_upper = float(in_range.max())
    else:
        whisker_lower = float(q1)
        whisker_upper = float(q3)

    # Outliers: data points outside the fence
    outliers = data[(data < lower_fence) | (data > upper_fence)]

    return {
        "whisker_lower": whisker_lower,
        "q1": float(q1),
        "median": float(median),
        "q3": float(q3),
        "whisker_upper": whisker_upper,
        "outliers": outliers.tolist(),
        "mean": float(np.mean(data)),
    }


# ---------------------------------------------------------------------------
# Plotting — simple (no hue)
# ---------------------------------------------------------------------------

def _plot_simple(df, columns, chart, params, color):
    """Draw one box per column, arranged horizontally."""
    whis = params.get("whis", 1.5)
    showfliers = params.get("showfliers", True)
    showmeans = params.get("showmeans", False)

    samples = []
    all_outliers = []
    all_means = []

    for i, col in enumerate(columns):
        data = drop_nan(extract_series(df, col))
        if data.size == 0:
            continue
        stats = _compute_box_stats(data, whis)

        samples.append({
            "position": float(i),
            "whiskerLower": stats["whisker_lower"],
            "q1": stats["q1"],
            "median": stats["median"],
            "q3": stats["q3"],
            "whiskerUpper": stats["whisker_upper"],
        })

        if showfliers:
            for val in stats["outliers"]:
                all_outliers.append([float(i), float(val)])

        if showmeans:
            all_means.append([float(i), float(stats["mean"])])

    if not samples:
        raise ValueError("No valid data columns for box plot")

    # Draw boxes
    box_item = chart.addBoxChart(samples, "Box Plot")
    if box_item is not None:
        fill_color = _with_alpha(color, 0.6)
        da_figure.setBrush(box_item, fill_color)
        da_figure.setPen(box_item, _hex_to_rgb_tuple(color), 1.0)

    # Outliers
    if all_outliers:
        outlier_item = chart.addScatter(all_outliers, "outliers")
        if outlier_item is not None:
            da_figure.setBrush(outlier_item, (231, 76, 60))
            da_figure.setSymbol(outlier_item, "Ellipse", 4, (231, 76, 60))

    # Means
    if all_means:
        mean_item = chart.addScatter(all_means, "means")
        if mean_item is not None:
            da_figure.setBrush(mean_item, (46, 204, 113))
            da_figure.setSymbol(mean_item, "Diamond", 6, (46, 204, 113))

    _safe_set_axis_label(chart, "xBottom", ", ".join(columns))
    _safe_set_axis_label(chart, "yLeft", "Value")


# ---------------------------------------------------------------------------
# Plotting — grouped (hue)
# ---------------------------------------------------------------------------

def _plot_grouped(df, columns, hue, chart, params):
    """Draw boxes grouped by ``hue`` category within each column."""
    palette_name = params.get("palette", "deep")
    whis = params.get("whis", 1.5)
    showfliers = params.get("showfliers", True)
    showmeans = params.get("showmeans", False)

    hue_categories = list(df[hue].dropna().unique())
    n_cats = len(hue_categories)
    palette = get_palette(palette_name, n=n_cats)

    samples = []
    all_outliers = []
    all_means = []

    pos = 0
    for col in enumerate(columns):
        col_idx, col_name = col
        for g_idx, cat in enumerate(hue_categories):
            mask = df[hue] == cat
            sub_df = df[mask]
            data = drop_nan(extract_series(sub_df, col_name))
            if data.size == 0:
                pos += 1
                continue

            stats = _compute_box_stats(data, whis)
            color = palette[g_idx]

            samples.append({
                "position": float(pos),
                "whiskerLower": stats["whisker_lower"],
                "q1": stats["q1"],
                "median": stats["median"],
                "q3": stats["q3"],
                "whiskerUpper": stats["whisker_upper"],
            })

            if showfliers:
                for val in stats["outliers"]:
                    all_outliers.append([float(pos), float(val)])

            if showmeans:
                all_means.append([float(pos), float(stats["mean"])])

            pos += 1
        pos += 0.5  # gap between columns

    if not samples:
        raise ValueError("No valid data for grouped box plot")

    # Draw boxes
    box_item = chart.addBoxChart(samples, "Box Plot by %s" % hue)
    if box_item is not None:
        fill_color = _with_alpha(palette[0], 0.6)
        da_figure.setBrush(box_item, fill_color)
        da_figure.setPen(box_item, _hex_to_rgb_tuple(palette[0]), 1.0)

    # Outliers
    if all_outliers:
        outlier_item = chart.addScatter(all_outliers, "outliers")
        if outlier_item is not None:
            da_figure.setBrush(outlier_item, (231, 76, 60))
            da_figure.setSymbol(outlier_item, "Ellipse", 4, (231, 76, 60))

    # Means
    if all_means:
        mean_item = chart.addScatter(all_means, "means")
        if mean_item is not None:
            da_figure.setBrush(mean_item, (46, 204, 113))
            da_figure.setSymbol(mean_item, "Diamond", 6, (46, 204, 113))

    _safe_set_axis_label(chart, "xBottom", ", ".join(columns))
    _safe_set_axis_label(chart, "yLeft", "Value")
