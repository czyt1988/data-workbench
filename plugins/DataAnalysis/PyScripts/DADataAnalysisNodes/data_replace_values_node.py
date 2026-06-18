# -*- coding: utf-8 -*-
"""DataReplaceValues — 替换值节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import replace_values_impl


@NodeDef(name="Replace Values", category="数据清洗", icon="replace")
class DataReplaceValuesNode:
    """替换指定列中的特定值"""

    column = Parameter(str, default="", description="要替换的列名")
    old_value = Parameter(str, default="", description="旧值")
    new_value = Parameter(str, default="", description="新值")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        replaced = Output("DataFrame", description="替换后的数据")
        replaced_count = Output("int", description="替换次数")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        col = params.get("column", "")
        old = params.get("old_value", "")
        new = params.get("new_value", "")
        if df is None or not col or col not in df.columns:
            return False
        # 统计替换次数
        count = int((df[col] == old).sum())
        result = replace_values_impl(df, subset=[col], old_values=[old], new_value=new, case_sensitive=True)
        self._output_data["replaced"] = result
        self._output_data["replaced_count"] = count
        return True
