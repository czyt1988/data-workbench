# -*- coding: utf-8 -*-
"""If / Else 菱形条件节点"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, NodeDisplay, LinkPointStyle


@NodeDef(
    name="If / Else",
    category="System / Flow Control",
    icon="",
    style=NodeDisplay(
        body_shape="Diamond",
        background_color="#E3F2FD",
        border_color="#2196F3",
        input_port_style=LinkPointStyle(shape="Diamond"),
        output_port_style=LinkPointStyle(shape="Diamond"),
    ),
)
class IfElseNode:
    """
    条件分支节点。

    根据 condition 输入的布尔值，将 data 输入转发到 true 或 false 输出。
    未命中分支的输出为 None，执行器不会向下游传播数据，从而实现分支选择。
    """

    class Inputs:
        condition = Input("bool", required=True, description="条件表达式结果")
        data = Input("any", required=False, description="要转发的数据（可选）")

    class Outputs:
        true = Output("any", description="condition 为 True 时输出 data")
        false = Output("any", description="condition 为 False 时输出 data")

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}

        condition = bool(inputs.get("condition", False))
        data = inputs.get("data")

        if condition:
            self._output_data["true"] = data
            self._output_data["false"] = None
        else:
            self._output_data["true"] = None
            self._output_data["false"] = data

        return True
