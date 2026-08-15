# -*- coding: utf-8 -*-
"""Constant node: output a fixed constant value"""

import ast
from DAWorkbench.DAWorkFlowPy import NodeDef, Output, Parameter


@NodeDef(
    name="Constant",
    category=_("System / Data"),  # cn:系统 / 数据
    icon="",
    description=_("Outputs a user-specified constant value. Supports Python literal expressions such as numbers (1), strings ('hello'), lists ([1,2,3]), and dicts. Useful as a parameter source for downstream nodes."),  # cn:输出用户指定的常量值。支持 Python 字面量表达式，如数字(1)、字符串('hello')、列表([1,2,3])和字典。用作下游节点的参数来源。
)
class ConstantNode:
    """Output a user-specified constant value, usable as a configuration parameter source in workflows."""

    value = Parameter(
        "code",
        default="1",
        description=_("Constant value, supports Python literal expressions (e.g. 1, 'hello', [1,2,3])"),  # cn:常量值，支持 Python 字面量表达式（如 1、'hello'、[1,2,3]）
    )

    class Outputs:
        value = Output("any", description=_("Constant value output"))  # cn:常量值输出

    def execute(self, inputs=None, params=None):
        if params is None:
            params = {}

        raw = params.get("value", "1")
        try:
            value = ast.literal_eval(raw)
        except Exception:
            value = raw

        self._output_data["value"] = value
        return True
