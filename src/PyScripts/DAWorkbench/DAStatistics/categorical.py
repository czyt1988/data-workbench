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
          ``"var"``). Only used when ``y_col`` is not None. Default ``"mean"``.
        - ``hue`` : str — grouping column for sub-categories (optional)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        When ``hue`` is None:
            - ``categories``: list — x-axis category labels
            - ``values``: list of float — aggregated values
            - ``hue_categories``: None

        When ``hue`` is set:
            - ``categories``: list — x-axis category labels
            - ``values``: list of list of float — one list per hue group
            - ``hue_categories``: list — hue group labels
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
    """Compute value counts for a count plot."""
    x = extract_series(df, x_col)

    try:
        import pandas as pd
        series = pd.Series(x)
    except Exception:
        series = None

    if hue is None:
        if series is not None:
            counts = series.value_counts().sort_index()
        else:
            vals, cnts = np.unique(x, return_counts=True)
            counts = dict(zip(vals.tolist(), cnts.tolist()))

        categories = list(counts.index) if hasattr(counts, "index") else list(counts.keys())
        values = [int(v) for v in (counts.values if hasattr(counts, "values") else counts.values())]

        return {
            "categories": categories,
            "values": values,
            "hue_categories": None,
        }
    else:
        import pandas as pd
        h = extract_series(df, hue)
        min_len = min(x.size, h.size)
        x = x[:min_len]
        h = h[:min_len]
        tmp = pd.DataFrame({"cat": x, "hue": h})
        ct = pd.crosstab(tmp["cat"], tmp["hue"])

        categories = ct.index.tolist()
        hue_categories = ct.columns.tolist()
        values = [ct[h_name].tolist() for h_name in hue_categories]

        return {
            "categories": categories,
            "values": values,
            "hue_categories": hue_categories,
        }


def _aggregate_barplot(df, x_col, y_col, hue, params):
    """Compute grouped aggregation for a bar plot."""
    estimator_name = params.get("estimator", "mean")
    estimator_fn = ESTIMATOR_MAP.get(estimator_name, np.mean)

    x = extract_series(df, x_col)
    y = extract_series(df, y_col)
    min_len = min(x.size, y.size)
    x = x[:min_len]
    y = y[:min_len]
    mask = ~(np.isnan(x) | np.isnan(y))
    x = x[mask]
    y = y[mask]

    if x.size == 0:
        raise ValueError("No valid data after NaN filtering")

    import pandas as pd

    if hue is None:
        tmp = pd.DataFrame({"cat": x, "val": y})
        grouped = tmp.groupby("cat")["val"]

        categories = sorted(grouped.groups.keys())
        values = []
        for cat in categories:
            data = grouped.get_group(cat).values
            if len(data) == 0:
                values.append(0.0)
            else:
                values.append(float(estimator_fn(data)))

        return {
            "categories": categories,
            "values": values,
            "hue_categories": None,
        }
    else:
        h = extract_series(df, hue)
        min_len = min(x.size, y.size, h.size)
        x = x[:min_len]
        y = y[:min_len]
        h = h[:min_len]
        mask = ~(np.isnan(x) | np.isnan(y))
        x = x[mask]
        y = y[mask]
        h = h[mask]

        tmp = pd.DataFrame({"cat": x, "val": y, "hue": h})
        hue_categories = sorted(tmp["hue"].unique().tolist())
        categories = sorted(tmp["cat"].unique().tolist())

        all_values = []
        for h_cat in hue_categories:
            group_vals = []
            for x_cat in categories:
                subset = tmp[(tmp["cat"] == x_cat) & (tmp["hue"] == h_cat)]["val"].values
                if len(subset) > 0:
                    group_vals.append(float(estimator_fn(subset)))
                else:
                    group_vals.append(0.0)
            all_values.append(group_vals)

        return {
            "categories": categories,
            "values": all_values,
            "hue_categories": hue_categories,
        }


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
