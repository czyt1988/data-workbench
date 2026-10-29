"""Categorical aggregation (bar-plot statistics) and confidence intervals.

All functions return plain dicts / lists — no chart or ``da_figure``
references. Rendering is handled by the C++ layer.
"""

import numpy as np

try:
    from scipy import stats
except Exception:  # pragma: no cover
    stats = None

from DAWorkbench.DAStatistics._utils import (
    extract_series,
    extract_values,
    to_label,
)


# Estimator name -> numpy function
# NaN must already be filtered by the caller; ``np.min`` / ``np.max`` are used
# instead of their ``nan*`` variants so a stale NaN can never poison the result.
ESTIMATOR_MAP = {
    "mean": np.mean,
    "median": np.median,
    "sum": np.sum,
    "count": len,
    "std": np.std,
    "var": np.var,
    "min": np.min,
    "max": np.max,
}


def _is_number(value):
    """True for real numeric category keys (bools are treated as labels)."""
    return isinstance(value, (int, float, np.number)) and not isinstance(value, (bool, np.bool_))


def _apply_sort_and_limit(raw_categories, values, params):
    """Order aggregated results and optionally keep only the top N entries.

    ``sort`` accepts:

    - ``"category"`` (default): ascending category order — keeps the
      historical statistics-guide behavior. Numeric keys sort numerically,
      other keys sort by their label.
    - ``"value_desc"``: descending by aggregated value — used by the quick
      plot preview so the tallest bar comes first.

    ``top_n`` > 0 keeps only the first ``top_n`` entries after sorting.

    Returns ``(labels, values, total)``; ``labels`` are display strings.
    """
    sort_mode = params.get("sort", "category")
    order = list(range(len(raw_categories)))
    if sort_mode == "value_desc":
        order.sort(key=lambda i: (-float(values[i]), to_label(raw_categories[i])))
    elif all(_is_number(c) for c in raw_categories):
        order.sort(key=lambda i: float(raw_categories[i]))
    else:
        order.sort(key=lambda i: to_label(raw_categories[i]))

    total = len(order)
    top_n = int(params.get("top_n", 0) or 0)
    if top_n > 0 and total > top_n:
        order = order[:top_n]

    return (
        [to_label(raw_categories[i]) for i in order],
        [values[i] for i in order],
        total,
    )


def aggregate_by_category(df, x_col, y_col=None, params=None, **kwargs):
    """Aggregate data by category for bar / count plots.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    x_col : str
        Categorical (X-axis) column name.
    y_col : str | None
        Value column name. If ``None``, a **count plot** is produced
        (value counts of ``x_col``).
    params : dict, optional
        Supported keys:

        - ``estimator`` : str — aggregation function name
          (``"mean"``, ``"median"``, ``"sum"``, ``"count"``, ``"std"``,
          ``"var"``, ``"min"``, ``"max"``). Only used when ``y_col`` is not
          None. Default ``"mean"``.
        - ``hue`` : str — grouping column for sub-categories (optional)
        - ``sort`` : str — ``"category"`` (default, ascending label order) or
          ``"value_desc"`` (descending aggregated value, used by the quick
          plot preview)
        - ``top_n`` : int — keep only the first N entries after sorting
          (0 / omitted means keep everything)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        When ``hue`` is None:
            - ``categories``: list — x-axis category labels
            - ``values``: list of float — aggregated values
            - ``hue_categories``: None
            - ``total_categories``: int — category count before ``top_n``

        When ``hue`` is set:
            - ``categories``: list — x-axis category labels
            - ``values``: list of list of float — one list per hue group
            - ``hue_categories``: list — hue group labels
            - ``total_categories``: int — category count before ``top_n``
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    hue = p.get("hue")

    if y_col is None:
        # Count plot
        return _aggregate_countplot(df, x_col, hue, p)
    else:
        # Bar plot
        return _aggregate_barplot(df, x_col, y_col, hue, p)


def _aggregate_countplot(df, x_col, hue, params):
    """Compute value counts for a count plot.

    The X column keeps its native dtype (string / object / category columns
    included); missing keys are dropped before counting.
    """
    import pandas as pd

    x = extract_values(df, x_col)

    if hue is None:
        series = pd.Series(x).dropna()
        counts = series.value_counts()
        values = [int(v) for v in counts.values]
        categories, values, total = _apply_sort_and_limit(list(counts.index), values, params)

        return {
            "categories": categories,
            "values": values,
            "hue_categories": None,
            "total_categories": total,
        }

    h = extract_values(df, hue)
    n = min(x.size, h.size)
    frame = pd.DataFrame({"cat": x[:n], "hue": h[:n]}).dropna()
    if frame.empty:
        raise ValueError("No valid data after NaN filtering")
    ct = pd.crosstab(frame["cat"], frame["hue"])

    categories = [to_label(v) for v in ct.index]
    hue_categories = [to_label(v) for v in ct.columns]
    values = [ct[h_name].tolist() for h_name in ct.columns]

    return {
        "categories": categories,
        "values": values,
        "hue_categories": hue_categories,
        "total_categories": len(categories),
    }


def _aggregate_barplot(df, x_col, y_col, hue, params):
    """Compute grouped aggregation for a bar plot.

    ``x_col`` is treated as a categorical key and keeps its native dtype, so
    plain text columns work. ``y_col`` must be numeric. Rows with a missing
    key or value are dropped (``dropna`` semantics; ``np.isnan`` is invalid
    for string columns).
    """
    import pandas as pd

    estimator_name = params.get("estimator", "mean")
    estimator_fn = ESTIMATOR_MAP.get(estimator_name, np.mean)

    x = extract_values(df, x_col)
    y = extract_series(df, y_col, as_float=True)
    n = min(x.size, y.size)
    frame = pd.DataFrame({"cat": x[:n], "val": y[:n]})

    if hue is None:
        frame = frame.dropna(subset=["cat", "val"])
        if frame.empty:
            raise ValueError("No valid data after NaN filtering")

        grouped = frame.groupby("cat", sort=False)["val"]
        raw_categories = list(grouped.groups.keys())
        values = []
        for cat in raw_categories:
            data = grouped.get_group(cat).values
            values.append(0.0 if len(data) == 0 else float(estimator_fn(data)))

        categories, values, total = _apply_sort_and_limit(raw_categories, values, params)

        return {
            "categories": categories,
            "values": values,
            "hue_categories": None,
            "total_categories": total,
        }

    h = extract_values(df, hue)
    n = min(n, h.size)
    frame = frame.iloc[:n].copy()
    frame["hue"] = h[:n]
    frame = frame.dropna(subset=["cat", "val", "hue"])
    if frame.empty:
        raise ValueError("No valid data after NaN filtering")

    raw_categories = list(frame["cat"].unique())
    raw_hues = list(frame["hue"].unique())
    categories = sorted((to_label(c) for c in raw_categories))
    hue_categories = sorted((to_label(c) for c in raw_hues))

    label_to_cat = {to_label(c): c for c in raw_categories}
    label_to_hue = {to_label(c): c for c in raw_hues}

    all_values = []
    for h_label in hue_categories:
        h_cat = label_to_hue[h_label]
        group_vals = []
        for x_label in categories:
            x_cat = label_to_cat[x_label]
            subset = frame[(frame["cat"] == x_cat) & (frame["hue"] == h_cat)]["val"].values
            if len(subset) > 0:
                group_vals.append(float(estimator_fn(subset)))
            else:
                group_vals.append(0.0)
        all_values.append(group_vals)

    return {
        "categories": categories,
        "values": all_values,
        "hue_categories": hue_categories,
        "total_categories": len(categories),
    }


def aggregate_by_category_from_lists(columns, x_col, y_col=None, params=None, **kwargs):
    """Aggregate categories from plain lists instead of a DataFrame.

    Used by the quick-plot path when the source is lazy or too large to
    aggregate exactly: the C++ layer samples the columns, hands the sampled
    values here, and the same aggregation code runs on a small in-memory
    frame — no second group-by implementation.

    Parameters
    ----------
    columns : dict
        ``{internal column name: list of values}``.
    x_col, y_col : str
        Keys inside ``columns``.
    params : dict, optional
        Same parameters as :func:`aggregate_by_category`.
    """
    import pandas as pd

    frame = pd.DataFrame({key: pd.Series(values) for key, values in columns.items()})
    return aggregate_by_category(frame, x_col, y_col, params, **kwargs)


def compute_ci(aggregated_df, params=None, **kwargs):
    """Compute confidence intervals for aggregated bar-plot data.

    This function takes raw per-group data arrays and computes CI bounds
    for each category's estimator.

    Parameters
    ----------
    aggregated_df : dict
        A dict with keys ``categories``, ``data`` where ``data`` is a list
        of numpy arrays (one per category). Alternatively, pass a DataFrame
        with ``x_col`` and ``y_col`` in params.
    params : dict, optional
        Supported keys:

        - ``estimator`` : str — aggregation function (default ``"mean"``)
        - ``ci`` : int — confidence level percent (default 95)
        - ``n_boot`` : int — bootstrap iterations (default 1000)
        - ``x_col`` : str — if passing a DataFrame
        - ``y_col`` : str — if passing a DataFrame

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``ci_lower``: list of float — lower CI bound per category
        - ``ci_upper``: list of float — upper CI bound per category
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    estimator_name = p.get("estimator", "mean")
    estimator_fn = ESTIMATOR_MAP.get(estimator_name, np.mean)
    ci = int(p.get("ci", 95))
    n_boot = int(p.get("n_boot", 1000))

    # Accept either a pre-grouped dict or a DataFrame
    if hasattr(aggregated_df, "columns"):
        # DataFrame input
        x_col = p.get("x_col")
        y_col = p.get("y_col")
        x = extract_series(aggregated_df, x_col)
        y = extract_series(aggregated_df, y_col)
        min_len = min(x.size, y.size)
        x = x[:min_len]
        y = y[:min_len]
        mask = ~(np.isnan(x) | np.isnan(y))
        x = x[mask]
        y = y[mask]

        import pandas as pd
        tmp = pd.DataFrame({"cat": x, "val": y})
        grouped = tmp.groupby("cat")["val"]
        categories = sorted(grouped.groups.keys())
        data_list = [grouped.get_group(cat).values for cat in categories]
    else:
        # Dict input
        data_list = aggregated_df.get("data", [])

    ci_lower = []
    ci_upper = []

    for data in data_list:
        data = np.asarray(data, dtype=float)
        n = len(data)
        if n < 2:
            val = float(estimator_fn(data)) if n == 1 else 0.0
            ci_lower.append(val)
            ci_upper.append(val)
            continue

        # t-distribution CI for the mean (fast path)
        if estimator_fn is np.mean and stats is not None:
            sem = stats.sem(data)
            alpha = 1 - ci / 100.0
            t_val = stats.t.ppf(1 - alpha / 2, df=n - 1)
            mean_val = float(np.mean(data))
            ci_lower.append(mean_val - t_val * sem)
            ci_upper.append(mean_val + t_val * sem)
        else:
            # Bootstrap CI for non-mean estimators
            rng = np.random.default_rng(seed=42)
            boot_stats = np.empty(n_boot)
            for i in range(n_boot):
                sample = rng.choice(data, size=n, replace=True)
                boot_stats[i] = estimator_fn(sample)

            alpha = 1 - ci / 100.0
            ci_lower.append(float(np.percentile(boot_stats, alpha / 2 * 100)))
            ci_upper.append(float(np.percentile(boot_stats, (1 - alpha / 2) * 100)))

    return {
        "ci_lower": ci_lower,
        "ci_upper": ci_upper,
    }
