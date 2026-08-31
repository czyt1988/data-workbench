# -*- coding: utf-8 -*-
"""Delay node: wait a specified number of seconds before passing data downstream"""

import os
import time
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter

# 图标目录（包根/icon），按本文件位置计算绝对路径，
# 兼容目录扫描（spec_from_file_location）与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "icon")


@NodeDef(
    name="Delay",
    category=_("System / Flow Control"),  # cn:系统 / 流程控制
    icon=os.path.join(_ICON_DIR, "delay.svg"),
    description=_("Pauses execution for a specified number of seconds, then forwards the trigger input to the output unchanged. Useful for timing control or rate-limiting in workflows."),  # cn:暂停执行指定秒数，然后将触发输入原样转发到输出。用于工作流中的定时控制或限流。
)
class DelayNode:
    """Delay for a specified number of seconds before passing data downstream."""

    seconds = Parameter(
        float,
        default=1.0,
        min=0.0,
        step=0.1,
        decimals=2,
        description=_("Delay in seconds"),  # cn:延迟秒数
    )

    class Inputs:
        trigger = Input("any", required=True, description=_("Trigger signal"))  # cn:触发信号

    class Outputs:
        done = Output("any", description=_("Output after delay, forwards trigger as-is"))  # cn:延迟完成后的输出，原样转发 trigger

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        seconds = params.get("seconds", 1.0)
        seconds = max(0.0, float(seconds))

        if seconds > 0:
            time.sleep(seconds)

        self._output_data["done"] = inputs.get("trigger")
        return True
