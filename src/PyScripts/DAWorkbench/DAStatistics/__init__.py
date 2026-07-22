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
