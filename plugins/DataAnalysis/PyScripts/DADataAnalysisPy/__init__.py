# -*- coding: utf-8 -*-
"""
DADataAnalysisPy - 数据分析示例节点包

本包提供基于 pandas 的数据分析工作流节点，
使用 @NodeDef 装饰器定义，可通过 DAWorkbench.DAWorkFlowPy 节点注册表自动发现。

节点列表：
- DataSourceNode: CSV 数据读取
- DataFilterNode: 条件筛选
- DataTransformNode: 列变换
- DataExportNode: 数据导出
- DataPlotNode: 数据绘图（可选，依赖 matplotlib）
- DataDescribeNode: 数据描述统计
- DataDropDuplicatesNode: 去重
- DataDropnaNode: 删除缺失值
- DataEvalNode: 表达式求值
- DataFillInterpolateNode: 插值填充
- DataFillnaNode: 缺失值填充
- DataFilterByColumnNode: 按列筛选
- DataPivotTableNode: 透视表
- DataQueryNode: 查询
- DataRemoveOutliersIqrNode: IQR 异常值移除
- DataRemoveOutliersZscoreNode: Z-score 异常值移除
- DataReplaceValuesNode: 值替换
- DataSearchNode: 搜索
- DataSortNode: 排序
- DataThresholdFilterNode: 阈值筛选
- DataTransformSkewedNode: 偏态变换
"""

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
