# -*- coding: utf-8 -*-
"""工作流起点节点"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Output


@NodeDef(
    name="Start",
    category="System / Flow Control",
    icon="",
)
class StartNode:
    """标记工作流起点，执行后向下游发送触发信号。"""

    class Outputs:
        trigger = Output("bool", description="工作流启动触发信号")

    def execute(self, inputs=None, params=None):
        self._output_data["trigger"] = True
        return True
