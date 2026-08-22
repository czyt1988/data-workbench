# -*- coding: utf-8 -*-
"""Workflow end node"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input


@NodeDef(
    name="End",
    category=_("System / Flow Control"),  # cn:系统 / 流程控制
    icon="",
    description=_("Marks the workflow end point. Receives upstream data but produces no output, indicating the workflow has completed."),  # cn:标记工作流终点。接收上游数据但不产生输出，表示工作流已完成。
)
class EndNode:
    """Marks the workflow end point; receives input but produces no output."""

    class Inputs:
        done = Input("any", required=True, description=_("Upstream completion signal"))  # cn:上游完成信号

    def execute(self, inputs=None, params=None):
        return True
