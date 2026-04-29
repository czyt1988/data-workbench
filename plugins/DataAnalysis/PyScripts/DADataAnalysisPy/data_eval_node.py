# -*- coding: utf-8 -*-
"""DataEval — 表达式计算节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Eval Expression", category="数据操作", icon="eval")
class DataEvalNode:
    """在 DataFrame 上执行表达式计算"""

    expression = Parameter(str, default="", description="pandas eval 表达式，如 'C = A + B'")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        result = Output("DataFrame", description="计算结果")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        expr = params.get("expression", "")
        if df is None or not expr:
            return False
        result = df.eval(expr)
        self._output_data["result"] = result
        return True
