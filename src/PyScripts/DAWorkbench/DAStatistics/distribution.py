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


def compute_histogram_by_hue(df, data_col, hue_col, params=None, **kwargs):
    """Compute histograms for each hue group with shared bin edges.

    Bin edges are determined from the **full** dataset (common_bins=True,
    matching seaborn's default behaviour). Each group's counts are then
    computed against those shared edges so the bars are directly comparable.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    data_col : str
        Numeric column whose distribution is being histogrammed.
    hue_col : str
        Column whose unique values define the groups.
    params : dict, optional
        Same keys as :func:`compute_histogram` (``bins``, ``binwidth``,
        ``binrange``, ``stat``, ``cumulative``).

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``groups``: list of dict — one per hue category, each containing
          ``counts`` (list), ``edges`` (list), ``hue_label`` (str), ``n`` (int)
        - ``hue_categories``: list of str — group labels (sorted)
        - ``common_edges``: list of float — shared bin edges
        - ``stat``: str — the statistic used
    """
    import pandas as pd

    p = dict(params) if params else {}
    p.update(kwargs)

    data = extract_series(df, data_col)
    # Hue column may be string/categorical — extract without float conversion
    if hasattr(df, "columns"):
        hue = np.asarray(df[hue_col]).ravel()
    else:
        hue = np.asarray(df).ravel()

    min_len = min(data.size, hue.size)
    data = data[:min_len]
    hue = hue[:min_len]

    # Drop rows where data or hue is NaN
    tmp = pd.DataFrame({"data": data, "hue": hue})
    tmp = tmp.dropna(subset=["hue"])
    data_all = drop_nan(tmp["data"].values.astype(float))
    if data_all.size == 0:
        raise ValueError("No valid (non-NaN) data for histogram")

    stat = p.get("stat", "count")
    bins = p.get("bins", 10)
    binwidth = p.get("binwidth", 0)
    binrange = p.get("binrange", [None, None])
    cumulative = p.get("cumulative", False)

    # Compute shared bin edges from the full dataset
    lo, hi = resolve_range(data_all, binrange)
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

    edges_list = edges.tolist()

    # Unique hue categories (sorted), converted to str for consistency
    hue_categories = sorted(tmp["hue"].unique().tolist())
    hue_categories = [str(h) for h in hue_categories]

    density_flag = (stat == "density")

    groups = []
    for h_cat in hue_categories:
        group_data = tmp[tmp["hue"].astype(str) == h_cat]["data"].values.astype(float)
        group_data = drop_nan(group_data)
        if group_data.size == 0:
            groups.append({
                "counts": [0.0] * (len(edges_list) - 1),
                "edges": edges_list,
                "hue_label": h_cat,
                "n": 0,
            })
            continue

        counts, _ = np.histogram(group_data, bins=edges, density=density_flag)

        if stat in ("frequency", "probability"):
            if not density_flag:
                counts = counts / float(group_data.size)
        elif stat == "percent":
            if not density_flag:
                counts = counts / float(group_data.size) * 100.0

        if cumulative:
            counts = np.cumsum(counts)

        groups.append({
            "counts": counts.tolist(),
            "edges": edges_list,
            "hue_label": h_cat,
            "n": int(group_data.size),
        })

    return {
        "groups": groups,
        "hue_categories": hue_categories,
        "common_edges": edges_list,
        "stat": stat,
    }
