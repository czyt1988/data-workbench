# -*- coding: utf-8 -*-
"""DataFilterByColumn — 按列范围筛选节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import filter_by_column_range


@NodeDef(name="Filter By Column", category="数据操作", icon="filter_col")
class DataFilterByColumnNode:
    """按列值范围筛选数据"""

    column = Parameter(str, default="", description="筛选列名")
    min_value = Parameter(float, default=0.0, description="最小值（包含），0 表示不限制")
    max_value = Parameter(float, default=0.0, description="最大值（包含），0 表示不限制")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        filtered = Output("DataFrame", description="筛选结果")
        removed_count = Output("int", description="移除的行数")

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
