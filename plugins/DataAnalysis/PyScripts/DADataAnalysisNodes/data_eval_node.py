# -*- coding: utf-8 -*-
"""DataEval — expression evaluation node"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import eval_expression


@NodeDef(name="Eval Expression", category=_("Data Operations"), icon="eval")  # cn:数据操作
class DataEvalNode:
    """Evaluate an expression on a DataFrame"""

    expression = Parameter(
        str,
        default="",
        description=_("pandas eval expression, e.g. 'C = A + B'"),  # cn:pandas eval 表达式，如 'C = A + B'
        layout="below",
    )

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        result = Output("DataFrame", description=_("Computation result"))  # cn:计算结果

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        expr = params.get("expression", "")
        if df is None or not expr:
            return False
        result = eval_expression(df, expr)
        self._output_data["result"] = result
        return True
