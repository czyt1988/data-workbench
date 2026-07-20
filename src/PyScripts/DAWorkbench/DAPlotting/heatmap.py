"""Heatmap plot (seaborn ``heatmap`` style).

Uses ``pandas.pivot_table`` to reshape long-format data into a matrix,
optionally normalises rows or columns, and renders the result through the
embedded ``da_figure`` module as a Qwt 7.3.4 ``QwtPlotSpectrogram``.
"""

import math

import numpy as np
import pandas as pd

import da_figure

from DAWorkbench.DAPlotting._utils import (
    extract_series,
    drop_nan,
)


def plot(df, column, chart, params=None, **kwargs):
    """Draw a heatmap on ``chart``.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    column : str
        X-axis (column-direction) category column name (passed positionally
        by the C++ controller).
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

    The Y-axis (row-direction) column name must be supplied under the
    ``"y_column"`` key in ``params`` (or as ``y_column=...`` in ``kwargs``).

    See ``plan/plan08-heatmap.md`` for the full parameter table.
    """
    if chart is None or not _is_valid_chart(chart):
        raise ValueError("Invalid chart handle passed to heatmap.plot()")

    p = dict(params) if params else {}
    p.update(kwargs)

    x_column = column
    y_column = p.get("y_column")
    if not y_column:
        raise ValueError("heatmap requires a 'y_column' parameter")

    result = _compute_heatmap_matrix(df, x_column, y_column, p)
    if result is None:
        return

    _draw_heatmap(chart, result, x_column, y_column, p)

    # Chart decorations
    title = p.get("title", "Heatmap: %s x %s" % (y_column, x_column))
    _safe_set_title(chart, title)
    _safe_set_axis_label(chart, "xBottom", x_column)
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


def _is_nan(value):
    """Check if ``value`` is NaN, safely handling non-float types."""
    try:
        return math.isnan(float(value))
    except (TypeError, ValueError):
        return True


# ---------------------------------------------------------------------------
# Matrix computation
# ---------------------------------------------------------------------------

def _compute_heatmap_matrix(df, x_column, y_column, params):
    """Execute pivot_table + optional normalisation, return matrix + axes."""
    value_column = params.get("value_column")
    aggfunc = params.get("aggfunc", "mean")

    agg_map = {
        "mean": np.mean,
        "sum": np.sum,
        "count": len,
        "median": np.median,
        "max": np.max,
        "min": np.min,
    }

    if value_column is None:
        # No value column: count
        matrix = pd.pivot_table(df, index=y_column, columns=x_column,
                                aggfunc=len, fill_value=0)
    else:
        func = agg_map.get(aggfunc, np.mean)
        matrix = pd.pivot_table(df, index=y_column, columns=x_column,
                                values=value_column, aggfunc=func,
                                fill_value=0)

    # Row / column standardisation
    scale = params.get("standard_scale", None)
    if scale == "row":
        matrix = matrix.sub(matrix.min(axis=1), axis=0)
        row_range = matrix.max(axis=1) - matrix.min(axis=1)
        row_range = row_range.replace(0, 1)
        matrix = matrix.div(row_range, axis=0)
    elif scale == "column":
        matrix = matrix.sub(matrix.min(axis=0), axis=1)
        col_range = matrix.max(axis=0) - matrix.min(axis=0)
        col_range = col_range.replace(0, 1)
        matrix = matrix.div(col_range, axis=1)

    # Centering
    center = params.get("center", float("nan"))
    if not _is_nan(center):
        matrix = matrix - center

    # Value range
    vmin = params.get("vmin", float("nan"))
    vmax = params.get("vmax", float("nan"))
    if _is_nan(vmin):
        vmin = float(matrix.min().min())
    if _is_nan(vmax):
        vmax = float(matrix.max().max())

    x_labels = [str(c) for c in matrix.columns]
    y_labels = [str(i) for i in matrix.index]

    return {
        "z": matrix.values.tolist(),
        "n_rows": len(y_labels),
        "n_cols": len(x_labels),
        "x_labels": x_labels,
        "y_labels": y_labels,
        "vmin": vmin,
        "vmax": vmax,
    }


# ---------------------------------------------------------------------------
# Drawing (da_figure / Qwt)
# ---------------------------------------------------------------------------

def _draw_heatmap(chart, result, x_column, y_column, params):
    """Render the heatmap through ``da_figure``."""
    n_rows = result["n_rows"]
    n_cols = result["n_cols"]

    grid_data = {
        "z": result["z"],
        "x_interval": [0, n_cols],
        "y_interval": [0, n_rows],
        "cmap": params.get("cmap", "viridis"),
        "vmin": result["vmin"],
        "vmax": result["vmax"],
    }
    chart.addSpectrogram(grid_data, title="Heatmap")

    # Annotations — skip if da_figure doesn't expose addTextLabel
    if params.get("annot", False):
        _draw_annotations(chart, result, params)


def _draw_annotations(chart, result, params):
    """Optionally annotate each cell with its numeric value."""
    if not hasattr(chart, "addTextLabel"):
        return

    fmt = params.get("fmt", ".2f")
    z = result["z"]
    n_rows = result["n_rows"]
    n_cols = result["n_cols"]
    vmin = result["vmin"]
    vmax = result["vmax"]

    for j in range(n_rows):
        for i in range(n_cols):
            val = z[j][i]
            text = format(val, fmt)
            try:
                chart.addTextLabel(i + 0.5, j + 0.5, text)
            except Exception:
                pass
