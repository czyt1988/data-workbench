"""Shared utility functions for the DAStatistics package.

These helpers are intentionally tiny and dependency-light so that every
statistics module (distribution, kde, boxplot_stats, ...) can reuse them
without pulling in seaborn/matplotlib at import time.
"""

import numpy as np


def select_column(df, column):
    """Select a column by name or by positional index.

    Integer keys are interpreted as **positions** (``iloc``) so callers can
    disambiguate duplicated column names without copying the whole frame.
    Keyword callers keep using column labels.
    """
    if column is None:
        return df
    if not hasattr(df, "columns"):
        return df
    if isinstance(column, int) and not isinstance(column, bool):
        return df.iloc[:, column]
    return df[column]


def extract_values(df, column):
    """Extract a single column from a DataFrame as a raw numpy array.

    Unlike :func:`extract_series` this keeps the native dtype, so string /
    object / category columns remain usable as categorical keys.

    Parameters
    ----------
    df : pandas.DataFrame | DAPyDataFrame | pandas.Series
        The data source. If a DataFrame is given, ``column`` selects the
        column; if a Series is given, ``column`` is ignored.
    column : str | int | None
        Column name, or positional index when disambiguating duplicates.

    Returns
    -------
    numpy.ndarray
        1-D array of the column's native dtype.
    """
    if column is None:
        data = df
    else:
        data = select_column(df, column)
    arr = np.asarray(data)
    return arr.ravel()


def extract_series(df, column, as_float=True):
    """Extract a single column from a DataFrame as a numpy array.

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
    as_float : bool
        When ``True`` (default) the result is cast to ``float64``. When
        ``False`` the native dtype is preserved, which is required for
        categorical (string / object / bool) key columns.

    Returns
    -------
    numpy.ndarray
        1-D array. NaN values are **not** filtered here; callers should mask
        them as needed (e.g. via ``data[~np.isnan(data)]``).

    Raises
    ------
    KeyError
        If ``column`` does not exist in ``df``.
    ValueError
        If ``as_float`` is true and the data cannot be converted to float.
    """
    if not as_float:
        return extract_values(df, column)

    if column is None:
        data = df
    else:
        # DataFrame-like: select the named column / positional index
        data = select_column(df, column)

    try:
        arr = np.asarray(data, dtype=float)
    except (TypeError, ValueError) as exc:
        raise ValueError(
            "Cannot convert column '%s' to float array: %s" % (column, exc)
        )

    # Flatten to 1-D (handles single-column DataFrame slices etc.)
    arr = arr.ravel()
    return arr


def to_label(value):
    """Convert a categorical key (numpy scalar / Python object) to a label."""
    if isinstance(value, str):
        return value
    if isinstance(value, (bool, np.bool_)):
        return "True" if bool(value) else "False"
    try:
        # numpy scalars: str(np.int64(3)) == "3", str(np.float64(3.5)) == "3.5"
        return str(value)
    except Exception:
        return repr(value)


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
