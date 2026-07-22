"""Box plot five-number statistics and outlier detection.

Returns plain dicts of numbers — no chart or ``da_figure`` references.
Rendering is handled by the C++ layer.
"""

import numpy as np

from DAWorkbench.DAStatistics._utils import (
    extract_series,
    drop_nan,
)


def compute_boxplot_stats(df, params=None, **kwargs):
    """Compute box plot statistics (five-number summary + outliers + mean).

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    params : dict, optional
        Supported keys:

        - ``columns`` : list of str — column names to compute box stats for.
          If not supplied, ``column`` (singular) is used.
        - ``column`` : str — single column name (fallback when ``columns``
          is absent).
        - ``whis`` : float — whisker length as IQR multiplier (default 1.5)
        - ``hue`` : str — grouping column for sub-categories (optional)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        When ``hue`` is None:
            - ``samples``: list of dict — one per column, each containing:
                - ``position``: float — column index (0-based)
                - ``whisker_lower``: float
                - ``q1``: float
                - ``median``: float
                - ``q3``: float
                - ``whisker_upper``: float
                - ``outliers``: list of float
                - ``mean``: float

        When ``hue`` is set:
            - ``samples``: list of dict (same structure, ``position`` reflects
              the grouped layout)
            - ``hue_categories``: list — unique hue values
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    columns = p.get("columns")
    if not columns:
        col = p.get("column")
        columns = [col] if col else []
    if isinstance(columns, str):
        columns = [columns]

    whis = p.get("whis", 1.5)
    hue = p.get("hue")

    if hue:
        return _compute_grouped(df, columns, hue, whis, p)
    else:
        return _compute_simple(df, columns, whis, p)


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

    in_range = data[(data >= lower_fence) & (data <= upper_fence)]
    if in_range.size > 0:
        whisker_lower = float(in_range.min())
        whisker_upper = float(in_range.max())
    else:
        whisker_lower = float(q1)
        whisker_upper = float(q3)

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


def _compute_simple(df, columns, whis, params):
    """Compute one box per column."""
    samples = []

    for i, col in enumerate(columns):
        data = drop_nan(extract_series(df, col))
        if data.size == 0:
            continue
        stats = _compute_box_stats(data, whis)
        stats["position"] = float(i)
        samples.append(stats)

    if not samples:
        raise ValueError("No valid data columns for box plot")

    return {
        "samples": samples,
        "hue_categories": None,
    }


def _compute_grouped(df, columns, hue, whis, params):
    """Compute boxes grouped by hue category within each column."""
    hue_categories = list(df[hue].dropna().unique())

    samples = []
    pos = 0

    for col_idx, col_name in enumerate(columns):
        for g_idx, cat in enumerate(hue_categories):
            mask = df[hue] == cat
            sub_df = df[mask]
            data = drop_nan(extract_series(sub_df, col_name))
            if data.size == 0:
                pos += 1
                continue

            stats = _compute_box_stats(data, whis)
            stats["position"] = float(pos)
            samples.append(stats)
            pos += 1
        pos += 0.5  # gap between columns

    if not samples:
        raise ValueError("No valid data for grouped box plot")

    return {
        "samples": samples,
        "hue_categories": hue_categories,
    }
