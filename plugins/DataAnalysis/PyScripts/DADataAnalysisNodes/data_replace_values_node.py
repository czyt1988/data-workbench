# -*- coding: utf-8 -*-
"""DataReplaceValues — value replacement node."""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import replace_values_impl


@NodeDef(
    name="Replace Values",
    category=_("Data Cleaning"),  # cn:数据清洗
    icon="replace",
    description=_("Replaces a specific value with a new value in a selected column. Matching is case-sensitive. Outputs the modified DataFrame and the count of replacements made."),  # cn:在选定列中将指定旧值替换为新值。区分大小写。输出修改后的 DataFrame 和替换次数。
)
class DataReplaceValuesNode:
    """Replace specific values in a given column."""

    column = Parameter(str, default="", description=_("Column to replace"))  # cn:要替换的列名
    old_value = Parameter(str, default="", description=_("Old value"))  # cn:旧值
    new_value = Parameter(str, default="", description=_("New value"))  # cn:新值

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        replaced = Output("DataFrame", description=_("Replaced data"))  # cn:替换后的数据
        replaced_count = Output("int", description=_("Number of replacements"))  # cn:替换次数

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
