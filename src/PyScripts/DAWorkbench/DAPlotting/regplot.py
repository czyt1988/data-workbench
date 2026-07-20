"""Regression plot (seaborn ``regplot`` style).

Fits a polynomial regression (``numpy.polyfit``) on X / Y data, optionally
computes bootstrap confidence intervals, and renders scatter + regression
line + CI band through the embedded ``da_figure`` module (Qwt 7.3.4
``QwtPlotCurve`` for scatter and regression line, ``QwtPlotIntervalCurve``
for CI boundaries, ``QwtPlotShapeItem`` for CI fill).
"""

import numpy as np

import da_figure

from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


def plot(df, column, chart, params=None, **kwargs):
    """Draw a regression plot on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        X-axis column name (passed positionally by the C++ controller).
    chart : da_figure.ChartHandle
        Target chart.
    params : dict, optional
        Plotting parameters. When called from C++ this is the primary
        channel (a QJsonObject converted to a Python dict); when called
        from Python directly, ``**kwargs`` is more natural. If both are
        supplied, ``kwargs`` takes precedence.
    **kwargs :
        Alternative way to pass parameters.

    The Y-axis column name must be supplied under the ``"y_column"`` key.

    See ``plan/plan11-regplot.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to regplot.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    x_column = column
    y_column = p.get("y_column")
    if not y_column:
        raise ValueError("regplot requires a 'y_column' parameter")

    # 1. Extract and filter NaN
    x = extract_series(df, x_column)
    y = extract_series(df, y_column)
    min_len = min(x.size, y.size)
    x = x[:min_len]
    y = y[:min_len]
    mask = ~(np.isnan(x) | np.isnan(y))
    x = x[mask]
    y = y[mask]

    if x.size < 2:
        raise ValueError("Need at least 2 valid points for regression")

    # 2. Polynomial fit
    order = int(p.get("order", 1))
    coeffs = np.polyfit(x, y, order)

    # 3. Generate grid for regression line and CI
    n_grid = 100
    x_grid = np.linspace(float(x.min()), float(x.max()), n_grid)
    y_pred = np.polyval(coeffs, x_grid)

    # 4. R-squared
    y_fit = np.polyval(coeffs, x)
    ss_res = np.sum((y - y_fit) ** 2)
    ss_tot = np.sum((y - np.mean(y)) ** 2)
    r_squared = 1 - ss_res / ss_tot if ss_tot > 0 else 0.0

    # 5. Bootstrap CI
    ci = p.get("ci")
    if ci is not None:
        ci = int(ci)
    n_boot = int(p.get("n_boot", 1000))

    ci_lower = None
    ci_upper = None
    if ci is not None:
        ci_lower, ci_upper = _bootstrap_ci(x, y, order, x_grid, ci, n_boot)

    # 6. Draw
    _do_plotting(x, y, x_grid, y_pred, ci_lower, ci_upper,
                 r_squared, x_column, y_column, chart, p)

    # 7. Chart decorations
    title = p.get("title", "%s vs %s (R²=%.3f)" % (y_column, x_column, r_squared))
    _safe_set_title(chart, title)
    _safe_set_axis_label(chart, "xBottom", x_column)
    _safe_set_axis_label(chart, "yLeft", y_column)
    _safe_enable_grid(chart, True)
    _safe_enable_legend(chart, True)
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


def _with_alpha(hex_color, alpha):
    """Convert ``#RRGGBB`` + alpha float to ``(r, g, b, a)`` tuple."""
    r, g, b = _hex_to_rgb_tuple(hex_color)
    a = int(max(0.0, min(1.0, alpha)) * 255 + 0.5)
    return (r, g, b, a)


# ---------------------------------------------------------------------------
# Bootstrap confidence interval
# ---------------------------------------------------------------------------

def _bootstrap_ci(x, y, order, x_grid, ci=95, n_boot=1000):
    """Bootstrap-resampled CI for polynomial regression predictions.

    Returns ``(ci_lower, ci_upper)`` arrays aligned with ``x_grid``.
    """
    n = len(x)
    rng = np.random.default_rng(seed=42)
    boot_preds = np.empty((n_boot, len(x_grid)))

    for i in range(n_boot):
        idx = rng.integers(0, n, size=n)
        x_boot = x[idx]
        y_boot = y[idx]
        try:
            coeffs_boot = np.polyfit(x_boot, y_boot, order)
            boot_preds[i] = np.polyval(coeffs_boot, x_grid)
        except np.linalg.LinAlgError:
            boot_preds[i] = np.nan

    # Filter failed fits
    valid_mask = ~np.isnan(boot_preds).any(axis=1)
    boot_preds = boot_preds[valid_mask]

    if boot_preds.shape[0] == 0:
        return None, None

    alpha = 1 - ci / 100.0
    ci_lower = np.percentile(boot_preds, alpha / 2 * 100, axis=0)
    ci_upper = np.percentile(boot_preds, (1 - alpha / 2) * 100, axis=0)
    return ci_lower, ci_upper


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------

def _do_plotting(x, y, x_grid, y_pred, ci_lower, ci_upper,
                 r_squared, x_column, y_column, chart, params):
    """Render scatter, regression line, and CI band on ``chart``."""
    color = params.get("color", "#4C72B0")
    scatter_kws = params.get("scatter_kws", {})
    reg_kws = params.get("reg_kws", {})

    # Scatter
    if params.get("scatter", True):
        points = [[float(xi), float(yi)] for xi, yi in zip(x, y)]
        scatter_item = chart.addScatter(points, "Data")
        if scatter_item is not None:
            marker_color = scatter_kws.get("marker_color", color)
            marker_size = int(scatter_kws.get("marker_size", 6))
            alpha = float(scatter_kws.get("alpha", 0.85))
            fill_color = _with_alpha(marker_color, alpha)
            da_figure.setBrush(scatter_item, fill_color)
            da_figure.setPen(scatter_item, _hex_to_rgb_tuple(marker_color), 1.0)
            da_figure.setSymbol(scatter_item, "Ellipse", marker_size * 2, fill_color)

    # Regression line
    if params.get("fit_reg", True):
        line_color = reg_kws.get("line_color", color)
        line_width = float(reg_kws.get("line_width", 2.0))
        reg_item = chart.addCurve(
            x_grid.tolist(), y_pred.tolist(),
            "Regression (order=%d, R²=%.3f)" % (int(params.get("order", 1)), r_squared),
        )
        if reg_item is not None:
            da_figure.setPen(reg_item, _hex_to_rgb_tuple(line_color), line_width)

    # Confidence interval
    if ci_lower is not None and ci_upper is not None:
        ci_color = reg_kws.get("line_color", color)
        ci_val = int(params.get("ci", 95))

        # CI boundary lines (upper / lower as interval curve)
        ci_item = chart.addIntervalCurve(
            y_pred.tolist(), ci_lower.tolist(), ci_upper.tolist(),
            "%d%% CI" % ci_val,
        )
        if ci_item is not None:
            da_figure.setPen(ci_item, _with_alpha(ci_color, 0.3), 1.0)

        # CI fill polygon
        polygon = (
            [[float(xg), float(cl)] for xg, cl in zip(x_grid, ci_lower)]
            + [[float(xg), float(cu)] for xg, cu in
               zip(reversed(x_grid.tolist()), reversed(ci_upper.tolist()))]
        )
        shape_item = chart.addShapeItem(polygon, "CI band")
        if shape_item is not None:
            da_figure.setBrush(shape_item, _with_alpha(ci_color, 0.15))
