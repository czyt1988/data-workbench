"""Descriptive statistics for one or more DataFrame columns.

Computes the classic describe() set (count / mean / std / min / quartiles /
median / max) plus optional extended statistics (variance, sem, mode, mad,
range, IQR, CV, skewness, kurtosis, percentiles, outlier fences, normality
tests and the confidence interval of the mean). Box-plot five-number
summaries are always returned alongside so the caller can render box plots
without a second pass over the data.

Returns plain dicts / lists — no chart or ``da_figure`` references.
Rendering is handled by the C++ layer.
"""

import numpy as np

try:
    from scipy import stats as _sp_stats
except ImportError:  # pragma: no cover - scipy is a hard requirement of the app
    _sp_stats = None

from DAWorkbench.DAStatistics._utils import (
    drop_nan,
    extract_series,
    select_column,
    to_label,
)


# Shapiro-Wilk 的样本量上限（scipy 官方建议 3 <= n <= 5000），
# 超出时退回 D'Agostino-Pearson K² 正态性检验
_SHAPIRO_MAX_N = 5000
# D'Agostino K² 检验的最小样本量
_NORMALTEST_MIN_N = 8


def compute_describe(df, params=None, **kwargs):
    """Compute descriptive statistics for the given columns of a DataFrame.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame
        Data source.
    params : dict, optional
        Supported keys:

        - ``columns`` : list — column names or positional indices (ints
          disambiguate duplicated names).
        - ``labels`` : list of str — display labels for the table header /
          box-plot x axis (same length as ``columns``).
        - ``stats`` : list of str — statistic keys to compute (see below).
        - ``whis`` : float — whisker / outlier fence IQR multiplier
          (default 1.5).
        - ``ci_level`` : float — confidence level of the mean CI
          (default 0.95).

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        - ``labels``: list of str — display labels of the analyzed columns
        - ``stats``: list of dict, one per requested statistic:
          ``{"key": str, "values": list, "is_int": bool, "is_text": bool}``
          (``values`` entries are floats / ints / machine-readable strings,
          ``None`` when not computable for that column)
        - ``boxplot``: dict — same shape as
          :func:`compute_boxplot_stats` result (``samples`` +
          ``hue_categories``), one box per column, ready for the shared
          C++ box-plot renderer
        - ``approximate``: bool — always False here (exact computation);
          the sampled path (:func:`compute_describe_from_lists`) reports
          True so the UI can flag estimates

    Statistic keys
    --------------
    count, missing, unique, sum, mean, std, var, sem, min, max, range,
    q05, q1, median, q3, q95, iqr, mode, cv, mad, skew, kurt,
    outliers, outlier_low, outlier_high,
    norm_stat, norm_p, norm_verdict, norm_test,
    ci_low, ci_high
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    columns = p.get("columns")
    if isinstance(columns, str):
        columns = [columns]
    if not columns:
        raise ValueError("compute_describe: no columns given")

    labels = p.get("labels")
    if not labels:
        labels = [to_label(c) for c in columns]

    whis = float(p.get("whis", 1.5))
    ci_level = float(p.get("ci_level", 0.95))
    stat_keys = list(p.get("stats") or [])

    raw_columns = []   # (label, raw numpy array with NaN kept)
    for i, col in enumerate(columns):
        label = labels[i] if i < len(labels) else to_label(col)
        try:
            raw = np.asarray(select_column(df, col), dtype=float).ravel()
        except (TypeError, ValueError):
            continue  # non-numeric column — skipped, reported by caller count
        raw_columns.append((label, raw))

    if not raw_columns:
        raise ValueError("compute_describe: no numeric columns")

    stats = _describe_columns(raw_columns, stat_keys, whis, ci_level)

    return {
        "labels": [label for label, _ in raw_columns],
        "stats": stats,
        "boxplot": _boxplot_from_raw(raw_columns, whis),
        "approximate": False,
    }


def compute_describe_from_lists(columns, params=None, **kwargs):
    """Compute the same descriptive statistics from plain value lists.

    Used when the source is lazy or too large to aggregate exactly: the C++
    layer samples the columns and the same computation runs on small
    in-memory lists. ``params["columns"]`` / ``params["labels"]`` must refer
    to the keys of ``columns``.
    """
    import pandas as pd

    frame = pd.DataFrame({key: pd.Series(values) for key, values in columns.items()})
    return compute_describe(frame, params, **kwargs)


# ---------------------------------------------------------------------------
# core computation
# ---------------------------------------------------------------------------

def _describe_columns(raw_columns, stat_keys, whis, ci_level):
    """Compute the requested statistics for each (label, raw array) column.

    ``raw_columns`` keeps NaN entries so count / missing stay meaningful.
    """
    results = []  # one entry per requested stat key: {"key", "values", ...}

    # per-column precomputed values
    valid = []       # NaN-dropped float arrays
    valid_counts = []
    for _, raw in raw_columns:
        v = drop_nan(raw)
        valid.append(v)
        valid_counts.append(int(v.size))

    for key in stat_keys:
        entry = {"key": key, "values": [], "is_int": False, "is_text": False}
        for idx, (label, raw) in enumerate(raw_columns):
            v = valid[idx]
            entry["values"].append(_stat_value(key, v, raw, whis, ci_level))
        if key in ("count", "missing", "unique", "outliers"):
            entry["is_int"] = True
        if key in ("norm_verdict", "norm_test"):
            entry["is_text"] = True
        results.append(entry)
    return results


def _stat_value(key, v, raw, whis, ci_level):
    """Compute one statistic for one column.

    ``v`` is the NaN-dropped float array, ``raw`` keeps NaN. Returns None
    when the statistic is not defined for this sample.
    """
    n = int(v.size)

    def _need(min_n):
        return None if n < min_n else True

    if key == "count":
        return n
    if key == "missing":
        return int(raw.size - n)
    if key == "unique":
        return int(np.unique(v).size) if n > 0 else 0
    if n == 0:
        return None  # every other statistic needs at least one value

    if key == "sum":
        return float(np.sum(v))
    if key == "mean":
        return float(np.mean(v))
    if key == "min":
        return float(np.min(v))
    if key == "max":
        return float(np.max(v))
    if key == "range":
        return float(np.max(v) - np.min(v))
    if key == "q1":
        return float(np.percentile(v, 25))
    if key == "median":
        return float(np.percentile(v, 50))
    if key == "q3":
        return float(np.percentile(v, 75))
    if key == "q05":
        return float(np.percentile(v, 5))
    if key == "q95":
        return float(np.percentile(v, 95))
    if key == "iqr":
        q1, q3 = np.percentile(v, [25, 75])
        return float(q3 - q1)
    if key == "mode":
        vals, counts = np.unique(v, return_counts=True)
        return float(vals[int(np.argmax(counts))])
    if key == "std":
        return float(np.std(v, ddof=1)) if _need(2) else None
    if key == "var":
        return float(np.var(v, ddof=1)) if _need(2) else None
    if key == "sem":
        return float(np.std(v, ddof=1) / np.sqrt(n)) if _need(2) else None
    if key == "cv":
        if not _need(2):
            return None
        mean = float(np.mean(v))
        if mean == 0.0:
            return None
        return float(np.std(v, ddof=1) / abs(mean))
    if key == "mad":
        med = float(np.median(v))
        return float(np.median(np.abs(v - med)))
    if key == "skew":
        if _need(3) is None or _sp_stats is None:
            return None
        return float(_sp_stats.skew(v, bias=True))
    if key == "kurt":
        if _need(4) is None or _sp_stats is None:
            return None
        return float(_sp_stats.kurtosis(v, fisher=True, bias=True))
    if key in ("outliers", "outlier_low", "outlier_high"):
        q1, q3 = np.percentile(v, [25, 75])
        iqr = q3 - q1
        low, high = q1 - whis * iqr, q3 + whis * iqr
        if key == "outlier_low":
            return float(low)
        if key == "outlier_high":
            return float(high)
        return int(np.count_nonzero((v < low) | (v > high)))
    if key in ("norm_stat", "norm_p", "norm_verdict", "norm_test"):
        method, stat, pval = _normality(v)
        if key == "norm_test":
            return method  # "shapiro" / "normaltest" / "none"
        if method == "none":
            return None
        if key == "norm_stat":
            return float(stat)
        if key == "norm_p":
            return float(pval)
        return "yes" if (pval is not None and pval >= 0.05) else "no"
    if key in ("ci_low", "ci_high"):
        if _need(2) is None:
            return None
        sem = float(np.std(v, ddof=1) / np.sqrt(n))
        if _sp_stats is not None:
            t = float(_sp_stats.t.ppf(1.0 - (1.0 - ci_level) / 2.0, n - 1))
        else:  # scipy missing — fall back to the normal quantile 1.96-ish
            t = 1.959963984540054 if abs(ci_level - 0.95) < 1e-9 else 2.5758293035489004
        mean = float(np.mean(v))
        half = t * sem
        return mean - half if key == "ci_low" else mean + half
    return None  # unknown key — ignored


def _normality(v):
    """Run the normality test suited to the sample size.

    Returns ``(method, statistic, p_value)``; method is "shapiro",
    "normaltest" or "none" when the sample size admits no test.
    """
    if _sp_stats is None:
        return ("none", None, None)
    n = int(v.size)
    if 3 <= n <= _SHAPIRO_MAX_N:
        result = _sp_stats.shapiro(v)
        return ("shapiro", float(result.statistic), float(result.pvalue))
    if n >= _NORMALTEST_MIN_N:
        result = _sp_stats.normaltest(v)
        return ("normaltest", float(result.statistic), float(result.pvalue))
    return ("none", None, None)


def _boxplot_from_raw(raw_columns, whis):
    """Build the shared box-plot result from (label, raw array) columns."""
    samples = []
    for pos, (label, raw) in enumerate(raw_columns):
        data = drop_nan(raw)
        if data.size == 0:
            continue
        q1, median, q3 = np.percentile(data, [25, 50, 75])
        iqr = q3 - q1
        lower_fence, upper_fence = q1 - whis * iqr, q3 + whis * iqr
        in_range = data[(data >= lower_fence) & (data <= upper_fence)]
        if in_range.size > 0:
            whisker_lower, whisker_upper = float(in_range.min()), float(in_range.max())
        else:
            whisker_lower, whisker_upper = float(q1), float(q3)
        outliers = data[(data < lower_fence) | (data > upper_fence)]
        samples.append({
            "position": float(pos),
            "whisker_lower": whisker_lower,
            "q1": float(q1),
            "median": float(median),
            "q3": float(q3),
            "whisker_upper": whisker_upper,
            "outliers": outliers.tolist(),
            "mean": float(np.mean(data)),
            "column": label,
            "hue": None,
        })
    # 全空列（全 NaN）→ 空样本；调用方保留统计表格、跳过箱线图渲染
    return {
        "samples": samples,
        "hue_categories": None,
    }
