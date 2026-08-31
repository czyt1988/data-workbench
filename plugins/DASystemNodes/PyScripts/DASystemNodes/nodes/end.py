# -*- coding: utf-8 -*-
"""Workflow end node"""

import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input

# 图标目录（包根/icon），按本文件位置计算绝对路径，
# 兼容目录扫描（spec_from_file_location）与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "icon")


@NodeDef(
    name="End",
    category=_("System / Flow Control"),  # cn:系统 / 流程控制
    icon=os.path.join(_ICON_DIR, "end.svg"),
    description=_("Marks the workflow end point. Receives upstream data but produces no output, indicating the workflow has completed."),  # cn:标记工作流终点。接收上游数据但不产生输出，表示工作流已完成。
)
class EndNode:
    """Marks the workflow end point; receives input but produces no output."""

    class Inputs:
        done = Input("any", required=True, description=_("Upstream completion signal"))  # cn:上游完成信号

    def execute(self, inputs=None, params=None):
        return True
