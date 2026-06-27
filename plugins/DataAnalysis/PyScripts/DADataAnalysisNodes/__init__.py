# -*- coding: utf-8 -*-
"""
DADataAnalysisNodes - Data analysis workflow node package

This package provides pandas-based data analysis workflow nodes,
defined using the @NodeDef decorator and auto-discovered via the
DAWorkbench.DAWorkFlowPy node registry.

Node list:
- DataSourceNode: CSV data reading
- DataFilterNode: Conditional filtering
- DataTransformNode: Column transformation
- DataExportNode: Data export
- DataPlotNode: Data plotting (optional, requires matplotlib)
- DataDescribeNode: Data description statistics
- DataDropDuplicatesNode: Deduplication
- DataDropnaNode: Drop missing values
- DataEvalNode: Expression evaluation
- DataFillInterpolateNode: Interpolation filling
- DataFillnaNode: Missing value filling
- DataFilterByColumnNode: Filter by column
- DataPivotTableNode: Pivot table
- DataQueryNode: Query
- DataRemoveOutliersIqrNode: IQR outlier removal
- DataRemoveOutliersZscoreNode: Z-score outlier removal
- DataReplaceValuesNode: Value replacement
- DataSearchNode: Search
- DataSortNode: Sorting
- DataThresholdFilterNode: Threshold filtering
- DataTransformSkewedNode: Skewed transformation
"""

# ⚠️ Must call setup_i18n() before importing node modules.
# Node modules use _() in @NodeDef(category=_(...)) which executes at import time.
from .i18n.core import setup_i18n
setup_i18n()

from .data_source_node import DataSourceNode
from .data_filter_node import DataFilterNode
from .data_transform_node import DataTransformNode
from .data_export_node import DataExportNode
from .data_describe_node import DataDescribeNode
from .data_drop_duplicates_node import DataDropDuplicatesNode
from .data_dropna_node import DataDropNaNode
from .data_eval_node import DataEvalNode
from .data_fill_interpolate_node import DataFillInterpolateNode
from .data_fillna_node import DataFillNaNode
from .data_filter_by_column_node import DataFilterByColumnNode
from .data_pivot_table_node import DataPivotTableNode
from .data_query_node import DataQueryNode
from .data_remove_outliers_iqr_node import DataRemoveOutliersIQRNode
from .data_remove_outliers_zscore_node import DataRemoveOutliersZScoreNode
from .data_replace_values_node import DataReplaceValuesNode
from .data_search_node import DataSearchNode
from .data_sort_node import DataSortNode
from .data_threshold_filter_node import DataThresholdFilterNode
from .data_transform_skewed_node import DataTransformSkewedNode

try:
    from .data_plot_node import DataPlotNode
except ImportError:
    # matplotlib 不可用时跳过 DataPlotNode
    pass

__all__ = [
    "DataSourceNode",
    "DataFilterNode",
    "DataTransformNode",
    "DataExportNode",
    "DataDescribeNode",
    "DataDropDuplicatesNode",
    "DataDropNaNode",
    "DataEvalNode",
    "DataFillInterpolateNode",
    "DataFillNaNode",
    "DataFilterByColumnNode",
    "DataPivotTableNode",
    "DataQueryNode",
    "DataRemoveOutliersIQRNode",
    "DataRemoveOutliersZScoreNode",
    "DataReplaceValuesNode",
    "DataSearchNode",
    "DataSortNode",
    "DataThresholdFilterNode",
    "DataTransformSkewedNode",
]
