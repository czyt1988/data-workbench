# -*- coding: utf-8 -*-
"""If / Else diamond conditional node"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, NodeDisplay, LinkPointStyle


@NodeDef(
    name="If / Else",
    category=_("System / Flow Control"),  # cn:系统 / 流程控制
    icon="",
    description=_("Conditional branch node. Forwards input data to the 'true' or 'false' output port based on the boolean condition input. The unmatched branch outputs None, so only one path executes downstream."),  # cn:条件分支节点。根据布尔条件输入将数据转发到 true 或 false 输出端口。未匹配的分支输出 None，仅一条路径向下执行。
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
    Conditional branch node.

    Forwards the data input to the true or false output based on the
    boolean value of the condition input. The unmatched branch outputs
    None, so the executor does not propagate data downstream, achieving
    branch selection.
    """

    class Inputs:
        condition = Input("bool", required=True, description=_("Condition expression result"))  # cn:条件表达式结果
        data = Input("any", required=False, description=_("Data to forward (optional)"))  # cn:要转发的数据（可选）

    class Outputs:
        true = Output("any", description=_("Outputs data when condition is True"))  # cn:condition 为 True 时输出 data
        false = Output("any", description=_("Outputs data when condition is False"))  # cn:condition 为 False 时输出 data

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
