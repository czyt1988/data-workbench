# -*- coding: utf-8 -*-
"""DataThresholdFilter — threshold filter node."""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import threshold_filter_impl


@NodeDef(name="Threshold Filter", category=_("Data Cleaning"), icon="threshold")  # cn:数据清洗
class DataThresholdFilterNode:
    """Filter data by a column-value threshold condition."""

    column = Parameter(str, default="", description=_("Column to filter"))  # cn:要筛选的列名
    operator = Parameter(str, default=">", description=_(">, >=, <, <=, ==, !="))  # cn:>, >=, <, <=, ==, !=
    threshold_value = Parameter(float, default=0.0, description=_("Threshold"))  # cn:阈值

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        filtered = Output("DataFrame", description=_("Filtered data"))  # cn:筛选后的数据
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
        op = params.get("operator", ">")
        val = params.get("threshold_value", 0.0)
        # 将节点的 operator 映射到 Core 的 filter_type / lower / upper
        op_map = {
            ">":  ("greater_than", val, val),
            ">=": ("in_range", val, float("inf")),
            "<":  ("less_than", val, val),
            "<=": ("in_range", float("-inf"), val),
            "==": ("in_range", val, val),
            "!=": ("out_of_range", val, val),
        }
        filter_type, lower, upper = op_map.get(op, op_map[">"])
        filtered = threshold_filter_impl(df, subset=[col], filter_type=filter_type,
                                         lower=lower, upper=upper,
                                         row_logic="any", treat_nan=False, reindex=False)
        self._output_data["filtered"] = filtered
        self._output_data["removed_count"] = len(df) - len(filtered)
        return True
