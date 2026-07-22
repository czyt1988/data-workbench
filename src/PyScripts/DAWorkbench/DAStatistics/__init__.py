"""DAWorkbench statistics computation package.

This package provides pure-statistics functions that compute results using
numpy / scipy / pandas and return plain Python data structures (lists,
dicts, pandas objects). No rendering or ``da_figure`` imports are present —
all chart rendering is handled by the C++ layer.

Submodules:
    _utils         — shared data extraction helpers
    distribution   — histogram and ECDF computation
    kde            — 1-D / 2-D kernel density estimation and contour extraction
    regression     — polynomial fitting and bootstrap confidence intervals
    categorical    — bar-plot aggregation and per-category CI
    boxplot_stats  — five-number summary and outlier detection
    matrix         — pivot-table matrix computation for heatmaps
"""

from DAWorkbench.DAStatistics.distribution import compute_histogram, compute_ecdf
from DAWorkbench.DAStatistics.kde import compute_kde_1d, compute_kde_2d, compute_contours
from DAWorkbench.DAStatistics.regression import fit_polynomial, compute_bootstrap_ci
from DAWorkbench.DAStatistics.categorical import aggregate_by_category, compute_ci
from DAWorkbench.DAStatistics.boxplot_stats import compute_boxplot_stats
from DAWorkbench.DAStatistics.matrix import compute_pivot_matrix
