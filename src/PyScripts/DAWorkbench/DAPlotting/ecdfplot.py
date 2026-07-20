"""Empirical cumulative distribution function plot (seaborn ``ecdfplot``).

Sorts data, computes cumulative proportions or counts, and renders as a
step curve through the embedded ``da_figure`` module (Qwt 7.3.4
``QwtPlotCurve`` with ``Steps`` style). Supports ``hue`` grouping,
complementary CDF, and weighted ECDF.
"""

import numpy as np

import da_figure

from DAWorkbench.DAPlotting._palette import get_palette
from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


def plot(df, column, chart, params=None, **kwargs):
    """Draw an empirical CDF plot on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        Data column name (passed positionally by the C++ controller).
    chart : da_figure.ChartHandle
        Target chart.
    params : dict, optional
        Plotting parameters. When called from C++ this is the primary
        channel (a QJsonObject converted to a Python dict); when called
        from Python directly, ``**kwargs`` is more natural. If both are
        supplied, ``kwargs`` takes precedence.
    **kwargs :
        Alternative way to pass parameters.

    See ``plan/plan12-ecdfplot.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to ecdfplot.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    hue = p.get("hue")

    if hue:
        _plot_grouped(df, column, hue, chart, p)
    else:
        color = p.get("color", "#4C72B0")
        _plot_single(df, column, chart, p, color=color, label=column)

    # Chart decorations
    stat = p.get("stat", "proportion")
    complementary = p.get("complementary", False)
    y_label = "Cumulative Count" if stat == "count" else "Cumulative Proportion"
    if complementary:
        y_label = "Complementary " + y_label

    title = p.get("title", "ECDF of %s" % column)
    if hue:
        title += " by %s" % hue
    if complementary:
        title = "Complementary ECDF of %s" % column
        if hue:
            title += " by %s" % hue

    _safe_set_title(chart, title)
    _safe_set_axis_label(chart, "xBottom", column)
    _safe_set_axis_label(chart, "yLeft", y_label)
    _safe_enable_grid(chart, True)
    show_legend = bool(hue) and p.get("legend", True)
    _safe_enable_legend(chart, show_legend)
    _safe_replot(chart)


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _is_valid_chart(chart):
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
    """Convert ``#RRGGBB`` to ``(r, g, b)`` tuple (0-255)."""
    if isinstance(hex_color, (tuple, list)):
        return tuple(hex_color)
    if not isinstance(hex_color, str) or len(hex_color) < 7:
        return (76, 114, 176)
    r = int(hex_color[1:3], 16)
    g = int(hex_color[3:5], 16)
    b = int(hex_color[5:7], 16)
    return (r, g, b)


# ---------------------------------------------------------------------------
# ECDF computation
# ---------------------------------------------------------------------------

def _compute_ecdf(data, params, weights=None):
    """Compute the empirical CDF.

    Parameters
    ----------
    data : numpy.ndarray
        1-D float array (NaNs already removed).
    params : dict
        Plotting parameters (stat, complementary).
    weights : numpy.ndarray, optional
        1-D float array of weights, aligned with ``data`` before sorting.

    Returns
    -------
    (x_sorted, y_cum) : (numpy.ndarray, numpy.ndarray)
    """
    stat = params.get("stat", "proportion")
    complementary = params.get("complementary", False)

    if weights is not None:
        # Weighted ECDF: sort by data, cumulate weights
        sort_idx = np.argsort(data)
        x_sorted = data[sort_idx]
        w_sorted = weights[sort_idx]
        cum_w = np.cumsum(w_sorted)
        total_w = cum_w[-1]
        if total_w == 0:
            y_cum = np.arange(1, len(x_sorted) + 1, dtype=float) / len(x_sorted)
        else:
            y_cum = cum_w / total_w
    else:
        x_sorted = np.sort(data)
        n = len(x_sorted)
        if stat == "proportion":
            y_cum = np.arange(1, n + 1, dtype=float) / n
        else:  # count
            y_cum = np.arange(1, n + 1, dtype=float)

    # Complementary CDF
    if complementary:
        if stat == "proportion":
            y_cum = 1.0 - y_cum
        else:  # count
            y_cum = float(len(x_sorted)) - y_cum

    return x_sorted, y_cum


# ---------------------------------------------------------------------------
# Plotting — single (no hue)
# ---------------------------------------------------------------------------

def _plot_single(df, column, chart, params, color, label):
    """Draw a single ECDF step curve."""
    data = drop_nan(extract_series(df, column))
    if data.size == 0:
        raise ValueError("Column '%s' has no valid data" % column)

    # Optional weights
    weights_col = params.get("weights")
    weights = None
    if weights_col:
        w = extract_series(df, weights_col)
        min_len = min(data.size, w.size)
        data = data[:min_len]
        w = w[:min_len]
        w = w[~np.isnan(data)]
        data = data[~np.isnan(data)]
        w = w[~np.isnan(w)]
        if data.size == w.size:
            weights = w

    x_sorted, y_cum = _compute_ecdf(data, params, weights)
    line_width = float(params.get("line_width", 2.0))

    curve_item = chart.addCurve(
        x_sorted.tolist(), y_cum.tolist(),
        "ECDF of %s" % label,
    )
    if curve_item is not None:
        da_figure.setPen(curve_item, _hex_to_rgb_tuple(color), line_width)
        da_figure.setCurveStyle(curve_item, "Steps")


# ---------------------------------------------------------------------------
# Plotting — grouped (hue)
# ---------------------------------------------------------------------------

def _plot_grouped(df, column, hue, chart, params):
    """Draw one ECDF step curve per ``hue`` category."""
    palette_name = params.get("palette", "deep")
    line_width = float(params.get("line_width", 2.0))

    hue_categories = list(df[hue].dropna().unique())
    palette = get_palette(palette_name, n=len(hue_categories))

    for i, cat in enumerate(hue_categories):
        mask = df[hue] == cat
        sub_df = df[mask]

        data = drop_nan(extract_series(sub_df, column))
        if data.size == 0:
            continue

        # Optional weights for this subgroup
        weights_col = params.get("weights")
        weights = None
        if weights_col:
            w = extract_series(sub_df, weights_col)
            min_len = min(data.size, w.size)
            data = data[:min_len]
            w = w[:min_len]
            w = w[~np.isnan(data)]
            data = data[~np.isnan(data)]
            w = w[~np.isnan(w)]
            if data.size == w.size:
                weights = w

        x_sorted, y_cum = _compute_ecdf(data, params, weights)
        color = palette[i] if i < len(palette) else palette[-1]
        label = str(cat)

        curve_item = chart.addCurve(
            x_sorted.tolist(), y_cum.tolist(),
            label,
        )
        if curve_item is not None:
            da_figure.setPen(curve_item, _hex_to_rgb_tuple(color), line_width)
            da_figure.setCurveStyle(curve_item, "Steps")
