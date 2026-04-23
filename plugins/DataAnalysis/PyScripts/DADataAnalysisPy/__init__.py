# -*- coding: utf-8 -*-
"""
DADataAnalysisPy - 数据分析示例节点包

本包提供基于 pandas 的数据分析工作流节点，
使用 @NodeDef 装饰器定义，可通过 DAWorkFlowPy 节点注册表自动发现。

节点列表：
- DataSourceNode: CSV 数据读取
- DataFilterNode: 条件筛选
- DataTransformNode: 列变换
- DataExportNode: 数据导出
- DataPlotNode: 数据绘图（可选，依赖 matplotlib）
"""

from .data_source_node import DataSourceNode
from .data_filter_node import DataFilterNode
from .data_transform_node import DataTransformNode
from .data_export_node import DataExportNode

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
]