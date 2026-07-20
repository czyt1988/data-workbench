"""Two-dimensional kernel density estimate plot (seaborn ``kdeplot`` style).

Uses ``scipy.stats.gaussian_kde`` to estimate the joint probability density of
two numeric columns, renders the resulting density surface through the embedded
``da_figure`` module as a Qwt 7.3.4 ``QwtPlotSpectrogram`` (heatmap), and
optionally overlays contour lines (``QwtPlotSpectroCurve``) or filled contour
polygons (``QwtPlotShapeItem``).
"""

import numpy as np

try:
    from scipy.stats import gaussian_kde
except Exception:  # pragma: no cover - scipy is a hard dep but guard import
    gaussian_kde = None

import da_figure

from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


def plot(df, column, chart, params=None, **kwargs):
    """Draw a 2-D kernel density estimate heatmap on ``chart``.

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

    The Y-axis column name must be supplied under the ``"y_column"`` key in
    ``params`` (or as ``y_column=...`` in ``kwargs``).

    See ``plan/plan06-kdeplot-2d.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to kdeplot_2d.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    y_column = p.get("y_column")
    if not y_column:
        raise ValueError("kdeplot_2d requires a 'y_column' parameter")

    x_data = drop_nan(extract_series(df, column))
    y_data = drop_nan(extract_series(df, y_column))

    # Align x and y by dropping rows where either is NaN
    min_len = min(x_data.size, y_data.size)
    if min_len < 2:
        raise ValueError(
            "2D KDE requires at least 2 valid points, got %d" % min_len)
    x_data = x_data[:min_len]
    y_data = y_data[:min_len]

    result = _compute_kde_2d(x_data, y_data, p)
    if result is None:
        return

    _draw_kde_2d(chart, result, column, y_column, p)

    # Chart decorations
    title = p.get("title", "2D KDE: %s vs %s" % (column, y_column))
    _safe_set_title(chart, title)
    _safe_set_axis_label(chart, "xBottom", column)
    _safe_set_axis_label(chart, "yLeft", y_column)
    _safe_enable_grid(chart, False)
    _safe_enable_legend(chart, False)
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
# 2-D KDE computation
# ---------------------------------------------------------------------------

def _compute_kde_2d(x_data, y_data, params):
    """Execute 2-D kernel density estimation, returning grid + contour data.

    Returns ``None`` if KDE cannot be computed (e.g. scipy unavailable).
    """
    if gaussian_kde is None:
        return None
    if x_data.size < 2:
        raise ValueError(
            "KDE requires at least 2 valid data points, got %d" % x_data.size
        )

    bw = params.get("bw_method", "scott")
    if bw == "custom":
        bw = params.get("bw_value", 0.5)

    try:
        values = np.vstack([x_data, y_data])
        kde = gaussian_kde(values, bw_method=bw)
    except Exception:
        return None

    # Generate grid
    grid_size = int(params.get("grid_size", 100))
    grid_size = max(grid_size, 10)

    x_margin = (x_data.max() - x_data.min()) * 0.1
    y_margin = (y_data.max() - y_data.min()) * 0.1
    if x_margin == 0:
        x_margin = 1.0
    if y_margin == 0:
        y_margin = 1.0

    x_grid = np.linspace(x_data.min() - x_margin,
                         x_data.max() + x_margin, grid_size)
    y_grid = np.linspace(y_data.min() - y_margin,
                         y_data.max() + y_margin, grid_size)
    X, Y = np.meshgrid(x_grid, y_grid)

    # Flatten, evaluate, reshape
    positions = np.vstack([X.ravel(), Y.ravel()])
    Z = kde(positions).reshape(X.shape)

    # Threshold filtering
    thresh = float(params.get("thresh", 0.05))
    z_max = Z.max()
    if thresh > 0 and z_max > 0:
        Z[Z < thresh * z_max] = np.nan

    # Contour levels
    n_levels = int(params.get("levels", 5))
    z_valid = Z[~np.isnan(Z)]
    if z_valid.size > 0 and n_levels > 0:
        levels = np.linspace(float(z_valid.min()),
                             float(z_valid.max()),
                             n_levels).tolist()
    else:
        levels = []

    return {
        "Z": Z.tolist(),
        "x_grid": x_grid.tolist(),
        "y_grid": y_grid.tolist(),
        "x_interval": [float(x_grid[0]), float(x_grid[-1])],
        "y_interval": [float(y_grid[0]), float(y_grid[-1])],
        "levels": levels,
        "z_max": float(z_max),
        "bw": float(kde.factor),
    }


# ---------------------------------------------------------------------------
# Drawing (da_figure / Qwt)
# ---------------------------------------------------------------------------

def _draw_kde_2d(chart, result, x_column, y_column, params):
    """Render the 2-D KDE heatmap and optional contours through ``da_figure``."""
    cmap = params.get("cmap", "viridis")
    show_heatmap = params.get("show_heatmap", True)
    fill_contours = params.get("fill_contours", False)
    levels = result["levels"]

    # 1. Heatmap
    if show_heatmap:
        grid_data = {
            "z": result["Z"],
            "x_interval": result["x_interval"],
            "y_interval": result["y_interval"],
            "cmap": cmap,
            "vmin": 0.0,
            "vmax": result["z_max"],
        }
        chart.addSpectrogram(grid_data, title="2D KDE")

    # 2. Contours
    if levels:
        if fill_contours:
            _draw_filled_contours(chart, result, levels, cmap)
        else:
            _draw_line_contours(chart, result, levels)


def _draw_line_contours(chart, result, levels):
    """Draw line contours via ``chart.addContour`` (QwtPlotSpectroCurve)."""
    X = np.array(result["x_grid"])
    Y = np.array(result["y_grid"])
    Z = np.array(result["Z"])

    # Flatten grid to 3-D point list, skipping NaN cells
    points = []
    for j in range(len(Y)):
        for i in range(len(X)):
            val = Z[j, i]
            if not np.isnan(val):
                points.append([float(X[i]), float(Y[j]), float(val)])

    chart.addContour(points, levels, title="contours")


def _draw_filled_contours(chart, result, levels, cmap_name):
    """Draw filled contour polygons via ``chart.addShapeItem``.

    Uses matplotlib's ``contourf`` (Agg backend, no rendering) solely to
    extract polygon vertices. Colour assignment samples the same colour
    map by level ratio so filled regions match the heatmap.
    """
    import matplotlib
    matplotlib.use('Agg')  # non-interactive backend, no window popup
    import matplotlib.pyplot as plt

    X = np.array(result["x_grid"])
    Y = np.array(result["y_grid"])
    Z = np.array(result["Z"])

    try:
        cmap = plt.get_cmap(cmap_name)
    except Exception:
        cmap = plt.get_cmap("viridis")

    fig, ax = plt.subplots()
    try:
        cs = ax.contourf(X, Y, Z, levels=levels)

        for i in range(len(levels) - 1):
            ratio = (float(levels[i]) / float(levels[-1])
                     if levels[-1] > 0 else 0.5)
            rgba = cmap(ratio)
            color = (int(rgba[0] * 255), int(rgba[1] * 255),
                     int(rgba[2] * 255), int(rgba[3] * 255 * 0.6))

            # matplotlib 3.8+ uses cs.get_paths(), older uses cs.collections
            if hasattr(cs, 'collections'):
                paths = cs.collections[i].get_paths()
            else:
                all_paths = cs.get_paths()
                paths = [all_paths[i]] if i < len(all_paths) else []

            for path in paths:
                polygon = [[float(p[0]), float(p[1])] for p in path.vertices]
                if len(polygon) >= 3:
                    shape = chart.addShapeItem(
                        polygon, title="contour %d" % i)
                    if shape is not None:
                        da_figure.setBrush(shape, color)
    finally:
        plt.close(fig)
