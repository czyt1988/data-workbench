"""Scatter plot (seaborn ``scatterplot`` style).

Directly maps X / Y data columns to points, rendered through the embedded
``da_figure`` module as a Qwt 7.3.4 ``QwtPlotCurve`` (Dots style +
``QwtSymbol``). Supports categorical ``hue`` grouping with per-group
colours, configurable marker style / size / alpha.
"""

import numpy as np

import da_figure

from DAWorkbench.DAPlotting._palette import get_palette
from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


# QwtSymbol style name mapping (seaborn marker -> Qwt symbol string)
STYLE_MAP = {
    "circle": "Ellipse",
    "square": "Rect",
    "diamond": "Diamond",
    "triangle": "Triangle",
    "cross": "Cross",
    "plus": "XCross",
    "star": "Star1",
}


def plot(df, column, chart, params=None, **kwargs):
    """Draw a scatter plot on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        X-axis column name (passed positionally by the C++ controller).
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

    The Y-axis column name must be supplied under the ``"y_column"`` key
    in ``params`` (or as ``y_column=...`` in ``kwargs``).

    See ``plan/plan09-scatterplot.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to scatterplot.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    y_column = p.get("y_column")
    if not y_column:
        raise ValueError("scatterplot requires a 'y_column' parameter")

    hue = p.get("hue")
    if hue:
        _plot_grouped(df, column, y_column, hue, chart, p)
    else:
        color = p.get("marker_color", "#4C72B0")
        _plot_simple(df, column, y_column, chart, p, color=color)

    # Chart decorations
    title = p.get("title", "%s vs %s" % (y_column, column))
    _safe_set_title(chart, title)
    _safe_set_axis_label(chart, "xBottom", column)
    _safe_set_axis_label(chart, "yLeft", y_column)
    _safe_enable_grid(chart, True)
    show_legend = bool(hue) and p.get("legend", True)
    _safe_enable_legend(chart, show_legend)
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
# Plotting — simple (no hue)
# ---------------------------------------------------------------------------

def _plot_simple(df, x_column, y_column, chart, params, color):
    """Draw a single-colour scatter plot."""
    x = extract_series(df, x_column)
    y = extract_series(df, y_column)

    # Align lengths and filter NaN
    min_len = min(x.size, y.size)
    x = x[:min_len]
    y = y[:min_len]
    mask = ~(np.isnan(x) | np.isnan(y))
    points = [[float(xi), float(yi)] for xi, yi in zip(x[mask], y[mask])]

    if not points:
        raise ValueError("No valid data points after NaN filtering")

    size = int(params.get("size", 6))
    style = params.get("style", "circle")
    alpha = params.get("alpha", 0.85)

    scatter_item = chart.addScatter(points, "%s vs %s" % (x_column, y_column))
    _apply_style(scatter_item, color, size, style, alpha)


# ---------------------------------------------------------------------------
# Plotting — grouped (hue)
# ---------------------------------------------------------------------------

def _plot_grouped(df, x_column, y_column, hue, chart, params):
    """Draw a multi-colour scatter plot grouped by ``hue``."""
    palette_name = params.get("palette", "deep")
    size = int(params.get("size", 6))
    style = params.get("style", "circle")
    alpha = params.get("alpha", 0.85)

    hue_categories = list(df[hue].dropna().unique())
    palette = get_palette(palette_name, n=len(hue_categories))

    for i, cat in enumerate(hue_categories):
        mask = df[hue] == cat
        sub_df = df[mask]

        x = extract_series(sub_df, x_column)
        y = extract_series(sub_df, y_column)

        min_len = min(x.size, y.size)
        x = x[:min_len]
        y = y[:min_len]
        valid = ~(np.isnan(x) | np.isnan(y))
        points = [[float(xi), float(yi)] for xi, yi in zip(x[valid], y[valid])]

        if not points:
            continue

        color = palette[i] if i < len(palette) else palette[-1]
        label = str(cat)

        scatter_item = chart.addScatter(points, label)
        _apply_style(scatter_item, color, size, style, alpha)


# ---------------------------------------------------------------------------
# Style application
# ---------------------------------------------------------------------------

def _apply_style(item, color, size, style_name, alpha):
    """Set scatter point colour, size, and marker style."""
    if item is None:
        return

    fill_color = _with_alpha(color, alpha)
    pen_color = _hex_to_rgb_tuple(color)

    da_figure.setBrush(item, fill_color)
    da_figure.setPen(item, pen_color, 1.0)

    qwt_style = STYLE_MAP.get(style_name, "Ellipse")
    da_figure.setSymbol(item, qwt_style, size * 2, fill_color)
