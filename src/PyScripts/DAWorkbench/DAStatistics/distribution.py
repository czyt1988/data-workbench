"""Histogram and ECDF (empirical cumulative distribution) computation.

All functions return plain Python dicts / lists of numbers — no chart or
``da_figure`` references. Rendering is handled by the C++ layer.
"""

import numpy as np

from DAWorkbench.DAStatistics._utils import (
    extract_series,
    drop_nan,
    resolve_range,
)


def compute_histogram(series, params=None, **kwargs):
    """Compute histogram bin counts and edges.

    Parameters
    ----------
    series : pandas.DataFrame | DAPyDataFrame | pandas.Series | numpy.ndarray
        Data source. If a DataFrame is given, ``column`` must be supplied
        in ``params`` or ``kwargs`` to select the column.
    params : dict, optional
        Computation parameters. Supported keys:

        - ``column`` : str — column name when ``series`` is a DataFrame
        - ``bins`` : int — number of bins (default 10)
        - ``binwidth`` : float — bin width; takes precedence over ``bins``
        - ``binrange`` : [lo, hi] — data range; ``[None, None]`` or ``None`` for auto
        - ``stat`` : str — ``"count"`` | ``"frequency"`` | ``"probability"`` |
          ``"percent"`` | ``"density"`` (default ``"count"``)
        - ``cumulative`` : bool — accumulate counts (default False)

    **kwargs :
        Alternative way to pass parameters. Merged on top of ``params``.

    Returns
    -------
    dict
        Keys:
        - ``counts``: list of float — bin values (stat-scaled)
        - ``edges``: list of float — bin edges (len = len(counts) + 1)
        - ``stat``: str — the statistic used
        - ``n``: int — number of valid (non-NaN) data points
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    column = p.get("column")
    data = drop_nan(extract_series(series, column))
    if data.size == 0:
        raise ValueError("No valid (non-NaN) data for histogram")

    stat = p.get("stat", "count")
    bins = p.get("bins", 10)
    binwidth = p.get("binwidth", 0)
    binrange = p.get("binrange", [None, None])
    cumulative = p.get("cumulative", False)

    lo, hi = resolve_range(data, binrange)

    # Degenerate data (all values identical): expand range
    if lo == hi:
        lo = lo - 0.5
        hi = hi + 0.5

    if binwidth and binwidth > 0:
        edges = np.arange(lo, hi + binwidth, binwidth)
        if edges.size < 2:
            edges = np.array([lo, hi], dtype=float)
    else:
        n_bins = max(int(bins), 1)
        edges = np.linspace(lo, hi, n_bins + 1)

    density_flag = (stat == "density")
    counts, edges = np.histogram(data, bins=edges, density=density_flag)

    # Convert density -> other stats if needed
    if stat in ("frequency", "probability"):
        if not density_flag:
            counts = counts / float(data.size)
    elif stat == "percent":
        if not density_flag:
            counts = counts / float(data.size) * 100.0

    if cumulative:
        counts = np.cumsum(counts)

    return {
        "counts": counts.tolist(),
        "edges": edges.tolist(),
        "stat": stat,
        "n": int(data.size),
    }


def compute_ecdf(series, params=None, **kwargs):
    """Compute the empirical cumulative distribution function.

    Parameters
    ----------
    series : pandas.DataFrame | DAPyDataFrame | pandas.Series | numpy.ndarray
        Data source. If a DataFrame is given, ``column`` must be supplied
        in ``params`` or ``kwargs``.
    params : dict, optional
        Supported keys:

        - ``column`` : str — column name when ``series`` is a DataFrame
        - ``stat`` : str — ``"proportion"`` (default) or ``"count"``
        - ``complementary`` : bool — compute 1 - CDF (default False)
        - ``weights`` : str — column name for weighted ECDF (optional)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``x``: list of float — sorted data values
        - ``y``: list of float — cumulative values (proportion or count)
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    column = p.get("column")
    data = drop_nan(extract_series(series, column))
    if data.size == 0:
        raise ValueError("No valid (non-NaN) data for ECDF")

    stat = p.get("stat", "proportion")
    complementary = p.get("complementary", False)

    # Optional weights
    weights_col = p.get("weights")
    weights = None
    if weights_col:
        w = extract_series(series, weights_col)
        min_len = min(data.size, w.size)
        data = data[:min_len]
        w = w[:min_len]
        w = w[~np.isnan(data)]
        data = data[~np.isnan(data)]
        w = w[~np.isnan(w)]
        if data.size == w.size:
            weights = w

    if weights is not None:
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

    if complementary:
        if stat == "proportion":
            y_cum = 1.0 - y_cum
        else:
            y_cum = float(len(x_sorted)) - y_cum

    return {
        "x": x_sorted.tolist(),
        "y": y_cum.tolist(),
    }
