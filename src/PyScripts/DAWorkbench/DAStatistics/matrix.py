"""Pivot-table matrix computation for heatmaps.

Returns plain dicts of numbers — no chart or ``da_figure`` references.
Rendering is handled by the C++ layer.
"""

import math

import numpy as np
import pandas as pd

from DAWorkbench.DAStatistics._utils import (
    extract_series,
)


def compute_pivot_matrix(df, index_col, columns_col, values_col, params=None, **kwargs):
    """Compute a pivot-table matrix for heatmap rendering.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    index_col : str
        Row-direction (Y-axis) column name.
    columns_col : str
        Column-direction (X-axis) column name.
    values_col : str | None
        Value column name. If ``None``, a count matrix is produced.
    params : dict, optional
        Supported keys:

        - ``aggfunc`` : str — aggregation function (``"mean"`` (default),
          ``"sum"``, ``"count"``, ``"median"``, ``"max"``, ``"min"``)
        - ``standard_scale`` : str | None — ``"row"`` or ``"column"``
          normalisation (default None)
        - ``center`` : float — subtract a center value (default NaN = no centering)
        - ``vmin`` : float — minimum value for colour mapping (default auto)
        - ``vmax`` : float — maximum value for colour mapping (default auto)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``z``: list of list of float — matrix values (rows = index, cols = columns)
        - ``n_rows``: int
        - ``n_cols``: int
        - ``x_labels``: list of str — column labels
        - ``y_labels``: list of str — row labels
        - ``vmin``: float
        - ``vmax``: float
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    aggfunc = p.get("aggfunc", "mean")

    agg_map = {
        "mean": np.mean,
        "sum": np.sum,
        "count": len,
        "median": np.median,
        "max": np.max,
        "min": np.min,
    }

    if values_col is None:
        # No value column: count
        matrix = pd.pivot_table(df, index=index_col, columns=columns_col,
                                aggfunc=len, fill_value=0)
    else:
        func = agg_map.get(aggfunc, np.mean)
        matrix = pd.pivot_table(df, index=index_col, columns=columns_col,
                                values=values_col, aggfunc=func,
                                fill_value=0)

    # Row / column standardisation
    scale = p.get("standard_scale", None)
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
    center = p.get("center", float("nan"))
    if not _is_nan(center):
        matrix = matrix - center

    # Value range
    vmin = p.get("vmin", float("nan"))
    vmax = p.get("vmax", float("nan"))
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


def _is_nan(value):
    """Check if ``value`` is NaN, safely handling non-float types."""
    try:
        return math.isnan(float(value))
    except (TypeError, ValueError):
        return True
