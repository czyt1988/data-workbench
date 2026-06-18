# -*- coding: utf-8 -*-
"""工作流终点节点"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input


@NodeDef(
    name="End",
    category="System / Flow Control",
    icon="",
)
class EndNode:
    """标记工作流终点，仅接收输入但不输出。"""

    class Inputs:
        done = Input("any", required=True, description="上游完成信号")

    def execute(self, inputs=None, params=None):
        return True
