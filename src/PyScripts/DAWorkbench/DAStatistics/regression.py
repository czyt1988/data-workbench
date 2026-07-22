"""Polynomial regression fitting and bootstrap confidence intervals.

All functions return plain dicts / numpy arrays — no chart or
``da_figure`` references. Rendering is handled by the C++ layer.
"""

import numpy as np

from DAWorkbench.DAStatistics._utils import (
    extract_series,
)


def fit_polynomial(x, y, params=None, **kwargs):
    """Fit a polynomial regression to (x, y) data.

    Parameters
    ----------
    x : pandas.DataFrame | DAPyDataFrame | array-like
        X-axis data. If a DataFrame is given, ``x_column`` must be supplied
        in ``params``.
    y : pandas.DataFrame | DAPyDataFrame | array-like
        Y-axis data. If a DataFrame is given, ``y_column`` must be supplied
        in ``params``.
    params : dict, optional
        Supported keys:

        - ``x_column`` : str — column name when ``x`` is a DataFrame
        - ``y_column`` : str — column name when ``y`` is a DataFrame
        - ``order`` : int — polynomial degree (default 1)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``coefficients``: list of float — polynomial coefficients
          (highest degree first, as returned by ``numpy.polyfit``)
        - ``x_grid``: list of float — 100-point grid from x.min to x.max
        - ``y_pred``: list of float — predicted y on ``x_grid``
        - ``r_squared``: float — coefficient of determination
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    # Extract series if DataFrame-like objects are passed
    x_col = p.get("x_column")
    y_col = p.get("y_column")

    if hasattr(x, "columns") and x_col:
        x_arr = extract_series(x, x_col)
    else:
        x_arr = np.asarray(x, dtype=float).ravel()

    if hasattr(y, "columns") and y_col:
        y_arr = extract_series(y, y_col)
    else:
        y_arr = np.asarray(y, dtype=float).ravel()

    min_len = min(x_arr.size, y_arr.size)
    x_arr = x_arr[:min_len]
    y_arr = y_arr[:min_len]
    mask = ~(np.isnan(x_arr) | np.isnan(y_arr))
    x_arr = x_arr[mask]
    y_arr = y_arr[mask]

    if x_arr.size < 2:
        raise ValueError("Need at least 2 valid points for regression")

    order = int(p.get("order", 1))
    coeffs = np.polyfit(x_arr, y_arr, order)

    n_grid = 100
    x_grid = np.linspace(float(x_arr.min()), float(x_arr.max()), n_grid)
    y_pred = np.polyval(coeffs, x_grid)

    # R-squared
    y_fit = np.polyval(coeffs, x_arr)
    ss_res = np.sum((y_arr - y_fit) ** 2)
    ss_tot = np.sum((y_arr - np.mean(y_arr)) ** 2)
    r_squared = 1 - ss_res / ss_tot if ss_tot > 0 else 0.0

    return {
        "coefficients": coeffs.tolist(),
        "x_grid": x_grid.tolist(),
        "y_pred": y_pred.tolist(),
        "r_squared": float(r_squared),
    }


def compute_bootstrap_ci(x, y, params=None, **kwargs):
    """Bootstrap-resampled confidence interval for polynomial regression.

    Parameters
    ----------
    x : pandas.DataFrame | DAPyDataFrame | array-like
        X-axis data. If a DataFrame is given, ``x_column`` must be supplied
        in ``params``.
    y : pandas.DataFrame | DAPyDataFrame | array-like
        Y-axis data. If a DataFrame is given, ``y_column`` must be supplied
        in ``params``.
    params : dict, optional
        Supported keys:

        - ``x_column`` : str — column name when ``x`` is a DataFrame
        - ``y_column`` : str — column name when ``y`` is a DataFrame
        - ``order`` : int — polynomial degree (default 1)
        - ``ci`` : int — confidence level percent (default 95)
        - ``n_boot`` : int — number of bootstrap iterations (default 1000)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``x_grid``: list of float — 100-point grid from x.min to x.max
        - ``ci_lower``: list of float — lower CI bound aligned with x_grid
        - ``ci_upper``: list of float — upper CI bound aligned with x_grid

        Returns ``None`` for ci_lower/ci_upper if bootstrap fails.
    """
    p = dict(params) if params else {}
    p.update(kwargs)

    x_col = p.get("x_column")
    y_col = p.get("y_column")

    if hasattr(x, "columns") and x_col:
        x_arr = extract_series(x, x_col)
    else:
        x_arr = np.asarray(x, dtype=float).ravel()

    if hasattr(y, "columns") and y_col:
        y_arr = extract_series(y, y_col)
    else:
        y_arr = np.asarray(y, dtype=float).ravel()

    min_len = min(x_arr.size, y_arr.size)
    x_arr = x_arr[:min_len]
    y_arr = y_arr[:min_len]
    mask = ~(np.isnan(x_arr) | np.isnan(y_arr))
    x_arr = x_arr[mask]
    y_arr = y_arr[mask]

    if x_arr.size < 2:
        raise ValueError("Need at least 2 valid points for regression CI")

    order = int(p.get("order", 1))
    ci = p.get("ci", 95)
    if ci is not None:
        ci = int(ci)
    n_boot = int(p.get("n_boot", 1000))

    n_grid = 100
    x_grid = np.linspace(float(x_arr.min()), float(x_arr.max()), n_grid)

    if ci is None:
        return {
            "x_grid": x_grid.tolist(),
            "ci_lower": None,
            "ci_upper": None,
        }

    n = len(x_arr)
    rng = np.random.default_rng(seed=42)
    boot_preds = np.empty((n_boot, len(x_grid)))

    for i in range(n_boot):
        idx = rng.integers(0, n, size=n)
        x_boot = x_arr[idx]
        y_boot = y_arr[idx]
        try:
            coeffs_boot = np.polyfit(x_boot, y_boot, order)
            boot_preds[i] = np.polyval(coeffs_boot, x_grid)
        except np.linalg.LinAlgError:
            boot_preds[i] = np.nan

    valid_mask = ~np.isnan(boot_preds).any(axis=1)
    boot_preds = boot_preds[valid_mask]

    if boot_preds.shape[0] == 0:
        return {
            "x_grid": x_grid.tolist(),
            "ci_lower": None,
            "ci_upper": None,
        }

    alpha = 1 - ci / 100.0
    ci_lower = np.percentile(boot_preds, alpha / 2 * 100, axis=0)
    ci_upper = np.percentile(boot_preds, (1 - alpha / 2) * 100, axis=0)

    return {
        "x_grid": x_grid.tolist(),
        "ci_lower": ci_lower.tolist(),
        "ci_upper": ci_upper.tolist(),
    }
