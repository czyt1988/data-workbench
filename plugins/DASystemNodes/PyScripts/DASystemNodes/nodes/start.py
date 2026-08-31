# -*- coding: utf-8 -*-
"""Workflow start node"""

import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Output

# 图标目录（包根/icon），按本文件位置计算绝对路径，
# 兼容目录扫描（spec_from_file_location）与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "icon")


@NodeDef(
    name="Start",
    category=_("System / Flow Control"),  # cn:系统 / 流程控制
    icon=os.path.join(_ICON_DIR, "start.svg"),
    description=_("Marks the workflow start point. It has no inputs and emits a trigger signal (True) on execution to kick off downstream nodes."),  # cn:标记工作流起点。无输入，执行时输出触发信号（True）启动下游节点。
)
class StartNode:
    """Marks the workflow start point; sends a trigger signal downstream after execution."""

    class Outputs:
        trigger = Output("bool", description=_("Workflow start trigger signal"))  # cn:工作流启动触发信号

    def execute(self, inputs=None, params=None):
        self._output_data["trigger"] = True
        return True
