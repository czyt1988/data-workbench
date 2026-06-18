# -*- coding: utf-8 -*-
"""常量节点：输出一个固定的常量值"""

import ast
from DAWorkbench.DAWorkFlowPy import NodeDef, Output, Parameter


@NodeDef(
    name="Constant",
    category="System / Data",
    icon="",
)
class ConstantNode:
    """输出用户指定的常量值，可作为工作流中的配置参数源。"""

    value = Parameter(
        "code",
        default="1",
        description="常量值，支持 Python 字面量表达式（如 1、'hello'、[1,2,3]）",
    )

    class Outputs:
        value = Output("any", description="常量值输出")

    def execute(self, inputs=None, params=None):
        if params is None:
            params = {}

        raw = params.get("value", "1")
        try:
            # 安全求值：仅允许 Python 字面量
            value = ast.literal_eval(raw)
        except Exception:
            value = raw

        self._output_data["value"] = value
        return True
