"""One-dimensional kernel density estimate plot (seaborn ``kdeplot`` style).

Uses ``scipy.stats.gaussian_kde`` to estimate the probability density of a
numeric column, renders the resulting smooth curve through the embedded
``da_figure`` module (Qwt 7.3.4 ``QwtPlotCurve``), and optionally fills the
area beneath the curve (``QwtPlotShapeItem``).
"""

import math

import numpy as np

try:
    from scipy.stats import gaussian_kde
except Exception:  # pragma: no cover - scipy is a hard dep but guard import
    gaussian_kde = None

import da_figure

from DAWorkbench.DAPlotting._palette import get_palette
from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


def plot(df, column, chart, params=None, **kwargs):
    """Draw a 1-D kernel density estimate curve on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        Column name to estimate the density of.
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

    See ``plan/plan05-kdeplot-1d.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to kdeplot_1d.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    hue = p.get("hue")
    if hue:
        _plot_grouped(df, column, hue, chart, p)
    else:
        color = p.get("color", "#4C72B0")
        _plot_single(df, column, chart, p, color=color, label=column)

    # Chart decorations
    if hue:
        title = p.get("title", "KDE of %s by %s" % (column, hue))
    else:
        title = p.get("title", "KDE of %s" % column)
    _safe_set_title(chart, title)
    _safe_set_axis_label(chart, "xBottom", column)
    _safe_set_axis_label(chart, "yLeft", "Density")
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
# Plotting
# ---------------------------------------------------------------------------

def _plot_grouped(df, column, hue, chart, params):
    """Draw one KDE curve per ``hue`` category."""
    try:
        groups = df.groupby(hue)[column]
    except Exception as exc:
        raise ValueError("Cannot group by '%s': %s" % (hue, exc))

    group_keys = list(groups.groups.keys())
    palette = get_palette(params.get("palette", "deep"), n=len(group_keys))

    common_norm = params.get("common_norm", True)
    total = 0
    group_data_list = []
    for key in group_keys:
        group_values = groups.get_group(key).values
        group_data = drop_nan(np.asarray(group_values, dtype=float))
        if group_data.size < 2:
            continue
        group_data_list.append((key, group_data))
        total += group_data.size

    for i, (key, group_data) in enumerate(group_data_list):
        result = _compute_kde_1d(group_data, params)
        if result is None:
            continue

        # Group normalisation
        if common_norm and total > 0:
            scale = group_data.size / total
            result["y"] = (np.array(result["y"]) * scale).tolist()

        color = palette[i] if i < len(palette) else palette[-1]
        _draw_kde(chart, result, color, label=str(key), params=params)


def _plot_single(df, column, chart, params, color, label):
    """Draw a single KDE curve on ``chart``."""
    data = drop_nan(extract_series(df, column))
    if data.size == 0:
        raise ValueError("Column '%s' has no valid (non-NaN) data" % column)

    result = _compute_kde_1d(data, params)
    if result is None:
        return

    _draw_kde(chart, result, color, label=label, params=params)


# ---------------------------------------------------------------------------
# KDE computation
# ---------------------------------------------------------------------------

def _compute_kde_1d(data, params):
    """Execute kernel density estimation, returning ``{x, y, bw}``.

    Returns ``None`` if KDE cannot be computed (e.g. insufficient data or
    scipy unavailable).
    """
    if gaussian_kde is None:
        return None
    if data.size < 2:
        raise ValueError(
            "KDE requires at least 2 valid data points, got %d" % data.size
        )

    bw = params.get("bw_method", "scott")
    if bw == "custom":
        bw = params.get("bw_value", 0.5)

    try:
        kde = gaussian_kde(data, bw_method=bw)
    except Exception:
        return None

    grid_size = int(params.get("grid_size", 200))
    grid_size = max(grid_size, 10)

    d_min, d_max = float(data.min()), float(data.max())
    margin = (d_max - d_min) * 0.1 if d_max > d_min else 1.0
    x_grid = np.linspace(d_min - margin, d_max + margin, grid_size)
    y_kde = kde(x_grid)

    if params.get("cumulative", False):
        dx = float(x_grid[1] - x_grid[0]) if grid_size > 1 else 1.0
        y_kde = np.cumsum(y_kde) * dx
        if y_kde[-1] > 0:
            y_kde = y_kde / y_kde[-1]  # normalise to [0, 1]

    return {
        "x": x_grid.tolist(),
        "y": y_kde.tolist(),
        "bw": float(kde.factor),
    }


# ---------------------------------------------------------------------------
# Drawing (da_figure / Qwt)
# ---------------------------------------------------------------------------

def _draw_kde(chart, result, color, label, params):
    """Render the KDE curve and optional fill through ``da_figure``."""
    x, y = result["x"], result["y"]

    # 1. KDE curve
    curve_item = chart.addCurve(x, y, label)
    if curve_item is not None:
        pen_color = _hex_to_rgb_tuple(color)
        da_figure.setPen(curve_item, pen_color, 2.0)

    # 2. Fill area beneath the curve
    fill = params.get("fill", True)
    shade = params.get("shade", False)
    if fill or shade:
        alpha = 0.25 if shade else 0.4
        fill_color = _with_alpha(color, alpha)

        below = params.get("below", float("nan"))
        above = params.get("above", float("nan"))
        polygon = _build_fill_polygon(x, y, below, above)
        if polygon:
            shape_item = chart.addShapeItem(polygon, "%s (fill)" % label)
            if shape_item is not None:
                da_figure.setBrush(shape_item, fill_color)


# ---------------------------------------------------------------------------
# Geometry helpers
# ---------------------------------------------------------------------------

def _build_fill_polygon(x, y, below, above, baseline=0.0):
    """Construct the closed polygon vertices for the fill area.

    Supports ``below`` / ``above`` threshold clipping: only the portion of
    the curve where ``x <= below`` and / or ``x >= above`` is filled. When
    both are ``NaN`` the entire area is filled.

    Returns ``None`` if no points satisfy the filter.
    """
    nan_below = _is_nan(below)
    nan_above = _is_nan(above)

    # No filter: include all points
    if nan_below and nan_above:
        polygon = [[float(x[i]), float(y[i])] for i in range(len(x))]
        polygon.append([float(x[-1]), baseline])
        polygon.append([float(x[0]), baseline])
        return polygon

    # Build mask
    mask = [True] * len(x)
    if not nan_below:
        mask = [xi <= below for xi in x]
    if not nan_above:
        mask = [m and xi >= above for xi, m in zip(x, mask)]

    if not any(mask):
        return None

    indices = [i for i, m in enumerate(mask) if m]
    polygon = [[float(x[i]), float(y[i])] for i in indices]
    polygon.append([float(x[indices[-1]]), baseline])
    polygon.append([float(x[indices[0]]), baseline])
    return polygon


def _is_nan(value):
    """Check if ``value`` is NaN, safely handling non-float types."""
    try:
        return math.isnan(float(value))
    except (TypeError, ValueError):
        return True
