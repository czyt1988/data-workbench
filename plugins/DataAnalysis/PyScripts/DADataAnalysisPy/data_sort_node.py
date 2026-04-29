# -*- coding: utf-8 -*-
"""DataSort — 排序节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Sort", category="数据操作", icon="sort")
class DataSortNode:
    """对 DataFrame 进行排序"""

    columns = Parameter(str, default="", description="排序列名，逗号分隔")
    ascending = Parameter(bool, default=True, description="是否升序")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        sorted = Output("DataFrame", description="排序后的数据")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        cols_str = params.get("columns", "")
        if df is None or not cols_str:
            return False
        cols = [c.strip() for c in cols_str.split(",")]
        asc = params.get("ascending", True)
        result = df.sort_values(by=cols, ascending=asc)
        self._output_data["sorted"] = result
        return True
