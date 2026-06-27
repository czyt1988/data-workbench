# -*- coding: utf-8 -*-
"""Workflow start node"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Output


@NodeDef(
    name="Start",
    category=_("System / Flow Control"),  # cn:系统 / 流程控制
    icon="",
)
class StartNode:
    """Marks the workflow start point; sends a trigger signal downstream after execution."""

    class Outputs:
        trigger = Output("bool", description=_("Workflow start trigger signal"))  # cn:工作流启动触发信号

    def execute(self, inputs=None, params=None):
        self._output_data["trigger"] = True
        return True
