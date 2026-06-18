# -*- coding: utf-8 -*-
"""延迟节点：在执行流程中等待指定秒数"""

import time
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(
    name="Delay",
    category="System / Flow Control",
    icon="",
)
class DelayNode:
    """延迟指定秒数后再向下游输出。"""

    seconds = Parameter(
        float,
        default=1.0,
        min=0.0,
        step=0.1,
        decimals=2,
        description="延迟秒数",
    )

    class Inputs:
        trigger = Input("any", required=True, description="触发信号")

    class Outputs:
        done = Output("any", description="延迟完成后的输出，原样转发 trigger")

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
