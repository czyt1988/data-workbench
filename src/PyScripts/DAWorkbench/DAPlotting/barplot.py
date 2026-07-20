"""Bar plot / count plot (seaborn ``barplot`` / ``countplot`` style).

Computes categorical aggregations (mean / median / sum / count / std / var)
with ``pandas.groupby``, optionally adds bootstrap or t-distribution
confidence intervals, and renders through the embedded ``da_figure`` module
(Qwt 7.3.4 ``QwtPlotBarChart`` for single groups, ``QwtPlotMultiBarChart``
for hue-grouped, ``QwtPlotIntervalCurve`` for error bars).
"""

import numpy as np

try:
    from scipy import stats
except Exception:  # pragma: no cover
    stats = None

import da_figure

from DAWorkbench.DAPlotting._palette import get_palette
from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


# Estimator name -> numpy function
ESTIMATOR_MAP = {
    "mean": np.mean,
    "median": np.median,
    "sum": np.sum,
    "count": len,
    "std": np.std,
    "var": np.var,
}


def plot(df, column, chart, params=None, **kwargs):
    """Draw a bar plot or count plot on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        X-axis (categorical) column name (passed positionally by the C++
        controller).
    chart : da_figure.ChartHandle
        Target chart.
    params : dict, optional
        Plotting parameters. When called from C++ this is the primary
        channel (a QJsonObject converted to a Python dict); when called
        from Python directly, ``**kwargs`` is more natural. If both are
        supplied, ``kwargs`` takes precedence.
    **kwargs :
        Alternative way to pass parameters.

    When ``y_column`` is empty / None the plot is a **countplot** (value
    counts of ``column``); otherwise it is a **barplot** (aggregation of
    ``y_column`` grouped by ``column``).

    See ``plan/plan10-barplot-countplot.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to barplot.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    x_column = column
    y_column = p.get("y_column") or None
    hue = p.get("hue")

    if y_column:
        _plot_barplot(df, x_column, y_column, hue, chart, p)
    else:
        _plot_countplot(df, x_column, hue, chart, p)

    # Chart decorations
    stat_label = p.get("estimator", "mean") if y_column else "count"
    if y_column:
        title = p.get("title", "%s of %s by %s" % (stat_label, y_column, x_column))
    else:
        title = p.get("title", "Count of %s" % x_column)
    _safe_set_title(chart, title)
    y_label = "%s (%s)" % (y_column, stat_label) if y_column else "Count"
    _safe_set_axis_label(chart, "xBottom", x_column)
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


def _with_alpha(hex_color, alpha):
    """Convert ``#RRGGBB`` + alpha float to ``(r, g, b, a)`` tuple."""
    r, g, b = _hex_to_rgb_tuple(hex_color)
    a = int(max(0.0, min(1.0, alpha)) * 255 + 0.5)
    return (r, g, b, a)


# ---------------------------------------------------------------------------
# Confidence interval
# ---------------------------------------------------------------------------

def _compute_ci(data, estimator_fn, ci=95, n_boot=1000):
    """Compute a confidence interval for ``estimator_fn(data)``.

    Uses the t-distribution for ``np.mean`` (fast), bootstrap for others.
    Returns ``(lower, upper)``.
    """
    n = len(data)
    if n < 2:
        val = float(estimator_fn(data)) if n == 1 else 0.0
        return val, val

    # t-distribution CI for the mean (fast path)
    if estimator_fn is np.mean and stats is not None:
        sem = stats.sem(data)
        alpha = 1 - ci / 100.0
        t_val = stats.t.ppf(1 - alpha / 2, df=n - 1)
        mean_val = float(np.mean(data))
        return mean_val - t_val * sem, mean_val + t_val * sem

    # Bootstrap CI for non-mean estimators
    rng = np.random.default_rng(seed=42)
    boot_stats = np.empty(n_boot)
    for i in range(n_boot):
        sample = rng.choice(data, size=n, replace=True)
        boot_stats[i] = estimator_fn(sample)

    alpha = 1 - ci / 100.0
    lower = np.percentile(boot_stats, alpha / 2 * 100)
    upper = np.percentile(boot_stats, (1 - alpha / 2) * 100)
    return float(lower), float(upper)


# ---------------------------------------------------------------------------
# Countplot
# ---------------------------------------------------------------------------

def _plot_countplot(df, x_column, hue, chart, params):
    """Draw a count plot (value counts of ``x_column``)."""
    if hue:
        _plot_countplot_grouped(df, x_column, hue, chart, params)
    else:
        _plot_countplot_simple(df, x_column, chart, params)


def _plot_countplot_simple(df, x_column, chart, params):
    """Single-group count plot."""
    data = extract_series(df, x_column)
    # For countplot we don't drop NaN via numpy — use pandas value_counts
    # to handle categorical/string data properly.
    try:
        import pandas as pd
        series = pd.Series(data)
    except Exception:
        series = None

    if series is not None:
        counts = series.value_counts().sort_index()
    else:
        # Fallback: numpy unique counts
        vals, cnts = np.unique(data, return_counts=True)
        counts = dict(zip(vals.tolist(), cnts.tolist()))

    categories = list(counts.index) if hasattr(counts, "index") else list(counts.keys())
    values = [int(v) for v in (counts.values if hasattr(counts, "values") else counts.values())]

    if not values:
        raise ValueError("No valid data for count plot")

    color = params.get("color", "#4C72B0")
    bar_item = chart.addBarChart(values, "Count of %s" % x_column)
    if bar_item is not None:
        da_figure.setBrush(bar_item, _with_alpha(color, 0.85))
        da_figure.setPen(bar_item, _hex_to_rgb_tuple(color), 1.0)


def _plot_countplot_grouped(df, x_column, hue, chart, params):
    """Grouped count plot using crosstab."""
    import pandas as pd

    try:
        ct = pd.crosstab(df[x_column], df[hue])
    except Exception as exc:
        raise ValueError("Cannot build crosstab for '%s' x '%s': %s" % (x_column, hue, exc))

    categories = ct.index.tolist()
    hue_categories = ct.columns.tolist()
    palette = get_palette(params.get("palette", "deep"), n=len(hue_categories))

    positions = list(range(len(categories)))
    values = [ct[h].tolist() for h in hue_categories]
    titles = [str(h) for h in hue_categories]

    bar_item = chart.addMultiBarChart(positions, values, titles)
    # setBrush on QwtPlotMultiBarChart may not be wired in the binding;
    # the chart's default colours will be used. We still try.
    if bar_item is not None:
        for i in range(len(palette)):
            try:
                da_figure.setBrush(bar_item, _hex_to_rgb_tuple(palette[i]))
            except Exception:
                break


# ---------------------------------------------------------------------------
# Barplot
# ---------------------------------------------------------------------------

def _plot_barplot(df, x_column, y_column, hue, chart, params):
    """Draw a bar plot (aggregation of ``y_column`` grouped by ``x_column``)."""
    if hue:
        _plot_barplot_grouped(df, x_column, y_column, hue, chart, params)
    else:
        _plot_barplot_simple(df, x_column, y_column, chart, params)


def _plot_barplot_simple(df, x_column, y_column, chart, params):
    """Single-group bar plot with optional CI."""
    estimator_name = params.get("estimator", "mean")
    estimator_fn = ESTIMATOR_MAP.get(estimator_name, np.mean)
    ci = params.get("ci")
    if ci is not None:
        ci = int(ci)
    n_boot = int(params.get("n_boot", 1000))
    errcolor = params.get("errcolor", "#1a1a1a")
    color = params.get("color", "#4C72B0")

    # Extract and filter NaN
    x = extract_series(df, x_column)
    y = extract_series(df, y_column)
    min_len = min(x.size, y.size)
    x = x[:min_len]
    y = y[:min_len]
    mask = ~(np.isnan(x) | np.isnan(y))
    x = x[mask]
    y = y[mask]

    if x.size == 0:
        raise ValueError("No valid data after NaN filtering")

    # Group by x and aggregate
    import pandas as pd
    tmp = pd.DataFrame({"x": x, "y": y})
    grouped = tmp.groupby("y", group_keys=False)  # placeholder
    # Actually group by x:
    tmp = pd.DataFrame({"cat": x, "val": y})
    grouped = tmp.groupby("cat")["val"]

    categories = list(grouped.groups.keys())
    values = []
    mins = []
    maxs = []

    for cat in categories:
        data = grouped.get_group(cat).values
        if len(data) == 0:
            values.append(0.0)
            mins.append(0.0)
            maxs.append(0.0)
            continue
        val = float(estimator_fn(data))
        values.append(val)
        if ci is not None:
            lo, hi = _compute_ci(data, estimator_fn, ci, n_boot)
            mins.append(lo)
            maxs.append(hi)
        else:
            mins.append(val)
            maxs.append(val)

    # Draw bars
    bar_item = chart.addBarChart(values, "%s (%s)" % (y_column, estimator_name))
    if bar_item is not None:
        da_figure.setBrush(bar_item, _with_alpha(color, 0.85))
        da_figure.setPen(bar_item, _hex_to_rgb_tuple(color), 1.0)

    # Draw error bars
    if ci is not None:
        err_item = chart.addIntervalCurve(values, mins, maxs, "CI")
        if err_item is not None:
            da_figure.setPen(err_item, _hex_to_rgb_tuple(errcolor), 2.0)


def _plot_barplot_grouped(df, x_column, y_column, hue, chart, params):
    """Grouped bar plot with optional per-group CI."""
    estimator_name = params.get("estimator", "mean")
    estimator_fn = ESTIMATOR_MAP.get(estimator_name, np.mean)
    ci = params.get("ci")
    if ci is not None:
        ci = int(ci)
    n_boot = int(params.get("n_boot", 1000))
    errcolor = params.get("errcolor", "#1a1a1a")

    # Extract and filter NaN
    x = extract_series(df, x_column)
    y = extract_series(df, y_column)
    h = extract_series(df, hue)
    min_len = min(x.size, y.size, h.size)
    x = x[:min_len]
    y = y[:min_len]
    h = h[:min_len]
    mask = ~(np.isnan(x) | np.isnan(y))
    x = x[mask]
    y = y[mask]
    h = h[mask]

    if x.size == 0:
        raise ValueError("No valid data after NaN filtering")

    import pandas as pd
    tmp = pd.DataFrame({"cat": x, "val": y, "hue": h})
    hue_categories = sorted(tmp["hue"].unique().tolist())
    categories = sorted(tmp["cat"].unique().tolist())
    palette = get_palette(params.get("palette", "deep"), n=len(hue_categories))

    positions = list(range(len(categories)))
    values = []
    titles = []
    all_mins = []
    all_maxs = []

    for h_cat in hue_categories:
        group_vals = []
        group_mins = []
        group_maxs = []
        for x_cat in categories:
            subset = tmp[(tmp["cat"] == x_cat) & (tmp["hue"] == h_cat)]["val"].values
            if len(subset) > 0:
                val = float(estimator_fn(subset))
            else:
                val = 0.0
            group_vals.append(val)

            if ci is not None and len(subset) > 1:
                lo, hi = _compute_ci(subset, estimator_fn, ci, n_boot)
                group_mins.append(lo)
                group_maxs.append(hi)
            else:
                group_mins.append(val)
                group_maxs.append(val)

        values.append(group_vals)
        titles.append(str(h_cat))
        all_mins.append(group_mins)
        all_maxs.append(group_maxs)

    # Draw grouped bars
    bar_item = chart.addMultiBarChart(positions, values, titles)
    if bar_item is not None:
        for i in range(len(palette)):
            try:
                da_figure.setBrush(bar_item, _hex_to_rgb_tuple(palette[i]))
            except Exception:
                break

    # Draw error bars per group
    if ci is not None:
        for g_idx in range(len(hue_categories)):
            err_item = chart.addIntervalCurve(
                values[g_idx], all_mins[g_idx], all_maxs[g_idx],
                "%s CI" % titles[g_idx],
            )
            if err_item is not None:
                da_figure.setPen(err_item, _hex_to_rgb_tuple(errcolor), 1.0)
