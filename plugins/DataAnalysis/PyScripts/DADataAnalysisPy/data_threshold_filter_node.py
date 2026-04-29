# -*- coding: utf-8 -*-
"""DataThresholdFilter — 阈值筛选节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Threshold Filter", category="数据清洗", icon="threshold")
class DataThresholdFilterNode:
    """根据列值阈值条件筛选数据"""

    column = Parameter(str, default="", description="要筛选的列名")
    operator = Parameter(str, default=">", description=">, >=, <, <=, ==, !=")
    threshold_value = Parameter(float, default=0.0, description="阈值")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        filtered = Output("DataFrame", description="筛选后的数据")
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
        op = params.get("operator", ">")
        val = params.get("threshold_value", 0.0)
        ops = {
            ">": lambda x: x > val,
            ">=": lambda x: x >= val,
            "<": lambda x: x < val,
            "<=": lambda x: x <= val,
            "==": lambda x: x == val,
            "!=": lambda x: x != val,
        }
        fn = ops.get(op, ops[">"])
        filtered = df[fn(df[col])]
        self._output_data["filtered"] = filtered
        self._output_data["removed_count"] = len(df) - len(filtered)
        return True
