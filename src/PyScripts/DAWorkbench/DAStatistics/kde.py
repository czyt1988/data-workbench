"""Kernel density estimation (1-D and 2-D) and contour extraction.

All functions return plain dicts with numpy arrays converted to lists —
no chart or ``da_figure`` references. Rendering is handled by the C++ layer.
"""

import numpy as np

try:
    from scipy.stats import gaussian_kde
except Exception:  # pragma: no cover - scipy is a hard dep but guard import
    gaussian_kde = None

from DAWorkbench.DAStatistics._utils import (
    extract_series,
    drop_nan,
)


def compute_kde_1d(series, params=None, **kwargs):
    """Compute a 1-D kernel density estimate.

    Parameters
    ----------
    series : pandas.DataFrame | DAPyDataFrame | pandas.Series | numpy.ndarray
        Data source. If a DataFrame is given, ``column`` must be supplied
        in ``params`` or ``kwargs``.
    params : dict, optional
        Supported keys:

        - ``column`` : str — column name when ``series`` is a DataFrame
        - ``bw_method`` : str | float — bandwidth method (default ``"scott"``);
          use ``"custom"`` to set via ``bw_value``
        - ``bw_value`` : float — bandwidth when ``bw_method`` is ``"custom"``
        - ``grid_size`` : int — number of grid points (default 200)
        - ``cumulative`` : bool — integrate the KDE (default False)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``x``: list of float — grid x values
        - ``y``: list of float — density values
        - ``bw``: float — bandwidth factor used

        Returns ``None`` if KDE cannot be computed (e.g. scipy unavailable).
    """
    if gaussian_kde is None:
        return None

    p = dict(params) if params else {}
    p.update(kwargs)

    column = p.get("column")
    data = drop_nan(extract_series(series, column))
    if data.size < 2:
        raise ValueError(
            "KDE requires at least 2 valid data points, got %d" % data.size
        )

    bw = p.get("bw_method", "scott")
    if bw == "custom":
        bw = p.get("bw_value", 0.5)

    try:
        kde = gaussian_kde(data, bw_method=bw)
    except Exception:
        return None

    grid_size = int(p.get("grid_size", 200))
    grid_size = max(grid_size, 10)

    d_min, d_max = float(data.min()), float(data.max())
    margin = (d_max - d_min) * 0.1 if d_max > d_min else 1.0
    x_grid = np.linspace(d_min - margin, d_max + margin, grid_size)
    y_kde = kde(x_grid)

    if p.get("cumulative", False):
        dx = float(x_grid[1] - x_grid[0]) if grid_size > 1 else 1.0
        y_kde = np.cumsum(y_kde) * dx
        if y_kde[-1] > 0:
            y_kde = y_kde / y_kde[-1]  # normalise to [0, 1]

    return {
        "x": x_grid.tolist(),
        "y": y_kde.tolist(),
        "bw": float(kde.factor),
    }


def compute_kde_2d(x, y, params=None, **kwargs):
    """Compute a 2-D kernel density estimate on a grid.

    Parameters
    ----------
    x : array-like
        X-axis data (1-D).
    y : array-like
        Y-axis data (1-D).
    params : dict, optional
        Supported keys:

        - ``bw_method`` : str | float — bandwidth method (default ``"scott"``)
        - ``bw_value`` : float — bandwidth when ``bw_method`` is ``"custom"``
        - ``grid_size`` : int — number of grid points per axis (default 100)
        - ``thresh`` : float — density threshold fraction of max (default 0.05)
        - ``levels`` : int — number of contour levels (default 5)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``Z``: list of list of float — density matrix (rows=y, cols=x)
        - ``x_grid``: list of float — x grid values
        - ``y_grid``: list of float — y grid values
        - ``levels``: list of float — contour levels
        - ``z_max``: float — maximum density value
        - ``bw``: float — bandwidth factor used

        Returns ``None`` if KDE cannot be computed.
    """
    if gaussian_kde is None:
        return None

    p = dict(params) if params else {}
    p.update(kwargs)

    x_data = drop_nan(np.asarray(x, dtype=float))
    y_data = drop_nan(np.asarray(y, dtype=float))

    min_len = min(x_data.size, y_data.size)
    if min_len < 2:
        raise ValueError(
            "2D KDE requires at least 2 valid points, got %d" % min_len)
    x_data = x_data[:min_len]
    y_data = y_data[:min_len]

    bw = p.get("bw_method", "scott")
    if bw == "custom":
        bw = p.get("bw_value", 0.5)

    try:
        values = np.vstack([x_data, y_data])
        kde = gaussian_kde(values, bw_method=bw)
    except Exception:
        return None

    grid_size = int(p.get("grid_size", 100))
    grid_size = max(grid_size, 10)

    x_margin = (x_data.max() - x_data.min()) * 0.1
    y_margin = (y_data.max() - y_data.min()) * 0.1
    if x_margin == 0:
        x_margin = 1.0
    if y_margin == 0:
        y_margin = 1.0

    x_grid = np.linspace(x_data.min() - x_margin,
                         x_data.max() + x_margin, grid_size)
    y_grid = np.linspace(y_data.min() - y_margin,
                         y_data.max() + y_margin, grid_size)
    X, Y = np.meshgrid(x_grid, y_grid)

    positions = np.vstack([X.ravel(), Y.ravel()])
    Z = kde(positions).reshape(X.shape)

    thresh = float(p.get("thresh", 0.05))
    z_max = Z.max()
    if thresh > 0 and z_max > 0:
        Z[Z < thresh * z_max] = np.nan

    n_levels = int(p.get("levels", 5))
    z_valid = Z[~np.isnan(Z)]
    if z_valid.size > 0 and n_levels > 0:
        levels = np.linspace(float(z_valid.min()),
                             float(z_valid.max()),
                             n_levels).tolist()
    else:
        levels = []

    return {
        "Z": Z.tolist(),
        "x_grid": x_grid.tolist(),
        "y_grid": y_grid.tolist(),
        "levels": levels,
        "z_max": float(z_max),
        "bw": float(kde.factor),
    }


def compute_contours(density_2d, x_grid, y_grid, params=None, **kwargs):
    """Extract contour polygons from a 2-D density matrix.

    Uses matplotlib's ``contourf`` (Agg backend, no window) solely to
    extract polygon vertices. No rendering is performed.

    Parameters
    ----------
    density_2d : numpy.ndarray | list of list of float
        2-D density matrix (rows = y, cols = x).
    x_grid : array-like
        X-axis grid values (1-D, length = ncols).
    y_grid : array-like
        Y-axis grid values (1-D, length = nrows).
    params : dict, optional
        Supported keys:

        - ``levels`` : list of float — contour levels. If not supplied,
          levels are auto-generated from the density matrix.
        - ``cmap`` : str — colour map name (only used to assign colours
          to contours; default ``"viridis"``)

    **kwargs :
        Alternative way to pass parameters.

    Returns
    -------
    dict
        Keys:
        - ``polygons``: list of list of [x, y] — each inner list is a
          closed polygon's vertices
        - ``levels``: list of float — the levels used
        - ``colors``: list of [r, g, b, a] — one colour per contour band
    """
    import matplotlib
    matplotlib.use('Agg')  # non-interactive backend, no window popup
    import matplotlib.pyplot as plt

    p = dict(params) if params else {}
    p.update(kwargs)

    X = np.array(x_grid)
    Y = np.array(y_grid)
    Z = np.array(density_2d)

    levels = p.get("levels")
    if not levels:
        n_levels = 5
        z_valid = Z[~np.isnan(Z)]
        if z_valid.size > 0:
            levels = np.linspace(float(z_valid.min()),
                                 float(z_valid.max()),
                                 n_levels).tolist()
        else:
            levels = []

    cmap_name = p.get("cmap", "viridis")
    try:
        cmap = plt.get_cmap(cmap_name)
    except Exception:
        cmap = plt.get_cmap("viridis")

    fig, ax = plt.subplots()
    polygons = []
    colors = []
    try:
        cs = ax.contourf(X, Y, Z, levels=levels)

        for i in range(len(levels) - 1):
            ratio = (float(levels[i]) / float(levels[-1])
                     if levels[-1] > 0 else 0.5)
            rgba = cmap(ratio)
            colors.append([
                int(rgba[0] * 255),
                int(rgba[1] * 255),
                int(rgba[2] * 255),
                int(rgba[3] * 255 * 0.6),
            ])

            # matplotlib 3.8+ uses cs.get_paths(), older uses cs.collections
            if hasattr(cs, 'collections'):
                paths = cs.collections[i].get_paths()
            else:
                all_paths = cs.get_paths()
                paths = [all_paths[i]] if i < len(all_paths) else []

            for path in paths:
                poly = [[float(pt[0]), float(pt[1])] for pt in path.vertices]
                if len(poly) >= 3:
                    polygons.append(poly)
    finally:
        plt.close(fig)

    return {
        "polygons": polygons,
        "levels": levels,
        "colors": colors,
    }
