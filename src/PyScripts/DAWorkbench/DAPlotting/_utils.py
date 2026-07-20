"""Shared utility functions for the DAPlotting package.

These helpers are intentionally tiny and dependency-light so that every
plot module (histplot, kdeplot, boxplot, ...) can reuse them without pulling
in seaborn/matplotlib at import time.
"""

import numpy as np


def extract_series(df, column):
    """Extract a single column from a DataFrame as a float numpy array.

    Accepts both a plain ``pandas.DataFrame`` (or ``pandas.Series``) and the
    C++-backed ``DAPyDataFrame`` / ``DAPySeries`` wrappers used inside
    data-workbench — they all support ``__getitem__`` returning an object that
    ``numpy.asarray`` can coerce.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame | pandas.Series
        The data source. If a DataFrame is given, ``column`` selects the
        column; if a Series is given, ``column`` is ignored and the values
        are returned directly.
    column : str | None
        Column name to extract when ``df`` is a DataFrame. ``None`` means
        ``df`` is already a Series-like object.

    Returns
    -------
    numpy.ndarray
        1-D array of dtype ``float64``. NaN values are **not** filtered here;
        callers should mask them as needed (e.g. via ``data[~np.isnan(data)]``).

    Raises
    ------
    KeyError
        If ``column`` does not exist in ``df``.
    ValueError
        If the extracted data cannot be converted to float.
    """
    if column is None:
        data = df
    elif hasattr(df, "columns"):
        # DataFrame-like: select the named column
        data = df[column]
    else:
        # Already a Series-like object (has .values but no .columns)
        data = df

    try:
        arr = np.asarray(data, dtype=float)
    except (TypeError, ValueError) as exc:
        raise ValueError(
            "Cannot convert column '%s' to float array: %s" % (column, exc)
        )

    # Flatten to 1-D (handles single-column DataFrame slices etc.)
    arr = arr.ravel()
    return arr


def drop_nan(arr):
    """Return a 1-D float array with all NaN entries removed."""
    arr = np.asarray(arr, dtype=float).ravel()
    return arr[~np.isnan(arr)]


def resolve_range(data, binrange):
    """Resolve a ``(min, max)`` range tuple, falling back to data extents.

    Parameters
    ----------
    data : numpy.ndarray
        1-D array of floats (NaNs already removed).
    binrange : list | tuple | None
        Either ``None``, ``[None, None]`` (auto), or ``[lo, hi]``.

    Returns
    -------
    tuple(float, float)
    """
    if binrange is None:
        return (float(data.min()), float(data.max()))

    lo = binrange[0]
    hi = binrange[1] if len(binrange) > 1 else None
    if lo is None:
        lo = float(data.min())
    if hi is None:
        hi = float(data.max())
    return (float(lo), float(hi))


def scale_kde_to_stat(y_kde, stat, n_total, bin_width):
    """Scale a KDE density curve so it visually matches the histogram bars.

    ``gaussian_kde`` always returns probability density (area = 1). To overlay
    it on a histogram whose Y axis uses a different statistic, we multiply:

    - ``density``    -> scale = 1 (KDE is already density)
    - ``count``      -> scale = n * bin_width
    - ``frequency``  -> scale = bin_width  (count / n * n = count, then / n * n? actually frequency = count/n)
    - ``probability``-> scale = bin_width  (count / n; KDE area = 1, each bin has width w, so KDE * w = prob)
    - ``percent``    -> scale = bin_width * 100
    """
    if stat == "density":
        return y_kde
    elif stat == "count":
        return y_kde * n_total * bin_width
    elif stat in ("frequency", "probability"):
        return y_kde * bin_width
    elif stat == "percent":
        return y_kde * bin_width * 100.0
    return y_kde
