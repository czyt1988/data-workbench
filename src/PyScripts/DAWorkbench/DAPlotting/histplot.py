"""Histogram plot with optional KDE overlay (seaborn ``histplot`` style).

Computes binning statistics with numpy, optionally overlays a
``scipy.stats.gaussian_kde`` curve, and renders everything through the embedded
``da_figure`` module (Qwt 7.3.4 ``QwtPlotHistogram`` / ``QwtPlotCurve``).
"""

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
    resolve_range,
    scale_kde_to_stat,
)


def plot(df, column, chart, params=None, **kwargs):
    """Draw a histogram (optionally with KDE overlay) on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        Column name to bin.
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

    See ``plan/plan04-histplot.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to histplot.plot()")

    # Merge params dict with kwargs (kwargs take precedence for direct
    # Python callers who use plot(df, col, chart, bins=20, kde=True)).
    p = dict(params) if params else {}
    p.update(kwargs)

    # 1. Extract data and drop NaN
    data = drop_nan(extract_series(df, column))
    if data.size == 0:
        raise ValueError("Column '%s' has no valid (non-NaN) data" % column)

    stat = p.get("stat", "count")
    hue = p.get("hue")

    # 2. Grouped or single?
    if hue:
        _plot_grouped(df, column, hue, chart, p, stat)
    else:
        color = p.get("color", "#4C72B0")
        _plot_single(data, chart, p, stat, color=color, label=column)

    # 3. Chart decorations
    title = p.get("title", "Distribution of %s" % column)
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


def _plot_grouped(df, column, hue, chart, params, stat):
    """Draw one histogram per ``hue`` category."""
    try:
        groups = df.groupby(hue)[column]
    except Exception as exc:
        raise ValueError("Cannot group by '%s': %s" % (hue, exc))

    group_keys = list(groups.groups.keys())
    palette = get_palette(params.get("palette", "deep"), n=len(group_keys))

    for i, key in enumerate(group_keys):
        group_values = groups.get_group(key).values
        group_data = drop_nan(np.asarray(group_values, dtype=float))
        if group_data.size == 0:
            continue
        _plot_single(
            group_data, chart, params, stat,
            color=palette[i],
            label=str(key),
        )


def _plot_single(data, chart, params, stat, color, label):
    """Draw a single histogram bar set (and optional KDE curve) on ``chart``."""
    bins = params.get("bins", 10)
    binwidth = params.get("binwidth", 0)
    binrange = params.get("binrange", [None, None])
    cumulative = params.get("cumulative", False)
    fill = params.get("fill", True)

    # Resolve bin edges
    lo, hi = resolve_range(data, binrange)
    if binwidth and binwidth > 0:
        # binwidth takes precedence over bins
        edges = np.arange(lo, hi + binwidth, binwidth)
        if edges.size < 2:
            edges = np.array([lo, hi], dtype=float)
    else:
        n_bins = max(int(bins), 1)
        edges = np.linspace(lo, hi, n_bins + 1)

    # numpy.histogram: density=True gives PDF (area=1)
    density_flag = (stat == "density")
    counts, edges = np.histogram(data, bins=edges,
                                 density=density_flag)

    # Convert density -> other stats if needed
    if stat in ("frequency", "probability"):
        # frequency = count / n; probability = count / n (same for histogram bars
        # where each bar represents one bin — seaborn treats them slightly
        # differently for density-normalised overlays but for bars they coincide)
        if not density_flag:
            counts = counts / float(data.size)
    elif stat == "percent":
        if not density_flag:
            counts = counts / float(data.size) * 100.0

    # Cumulative
    if cumulative:
        counts = np.cumsum(counts)

    # Build QwtIntervalSample list for da_figure.addHistogram
    samples = []
    for i in range(len(counts)):
        samples.append({
            "value": float(counts[i]),
            "interval": [float(edges[i]), float(edges[i + 1])],
        })

    hist_item = chart.addHistogram(samples, label)
    if hist_item is not None:
        if fill:
            da_figure.setBrush(hist_item, color)
        da_figure.setPen(hist_item, color, 1.0)

    # KDE overlay
    if params.get("kde", False) and gaussian_kde is not None:
        _overlay_kde(data, edges, chart, params, stat, color, label,
                     data.size)


def _overlay_kde(data, edges, chart, params, stat, color, label, n_total):
    """Draw a KDE curve scaled to match the histogram Y-axis units."""
    bw = params.get("kde_bw", "scott")
    try:
        kde = gaussian_kde(data, bw_method=bw)
    except Exception:
        return

    x_kde = np.linspace(float(edges[0]), float(edges[-1]), 200)
    y_kde = kde(x_kde)

    # Scale KDE to match the histogram statistic
    bin_width = float(edges[1] - edges[0]) if edges.size > 1 else 1.0
    y_kde = scale_kde_to_stat(y_kde, stat, n_total, bin_width)

    kde_item = chart.addCurve(
        x_kde.tolist(), y_kde.tolist(), "%s (KDE)" % label
    )
    if kde_item is not None:
        da_figure.setPen(kde_item, color, 2.0)
