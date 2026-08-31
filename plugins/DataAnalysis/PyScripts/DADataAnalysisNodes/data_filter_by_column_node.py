# -*- coding: utf-8 -*-
"""DataFilterByColumn — filter by column range node"""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import filter_by_column_range


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Filter By Column",
    category=_("Data Operations"),  # cn:数据操作
    icon=os.path.join(_ICON_DIR, "filterByColumn.svg"),
    description=_("Filters DataFrame rows by a numeric column's value range. Set min and/or max thresholds (inclusive); a value of 0 means no limit on that bound."),  # cn:按数值列的值范围筛选 DataFrame 行。设置最小和/或最大阈值（包含），值为 0 表示该边界不限制。
)
class DataFilterByColumnNode:
    """Filter data by a column value range"""

    column = Parameter(str, default="", description=_("Column name to filter by"))  # cn:筛选列名
    min_value = Parameter(float, default=0.0, description=_("Minimum value (inclusive), 0 means no limit"))  # cn:最小值（包含），0 表示不限制
    max_value = Parameter(float, default=0.0, description=_("Maximum value (inclusive), 0 means no limit"))  # cn:最大值（包含），0 表示不限制

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        filtered = Output("DataFrame", description=_("Filter result"))  # cn:筛选结果
        removed_count = Output("int", description=_("Number of removed rows"))  # cn:移除的行数

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        col = params.get("column", "")
        if df is None or col not in df.columns:
            return False
        min_val = params.get("min_value", None)
        max_val = params.get("max_value", None)
        if min_val == 0.0:
            min_val = None
        if max_val == 0.0:
            max_val = None
        filtered = filter_by_column_range(df, col, min_val, max_val)
        self._output_data["filtered"] = filtered
        self._output_data["removed_count"] = len(df) - len(filtered)
        return True
