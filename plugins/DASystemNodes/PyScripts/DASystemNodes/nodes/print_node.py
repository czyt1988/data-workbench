# -*- coding: utf-8 -*-
"""打印节点：将输入数据打印到日志/控制台"""

import logging
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter

logger = logging.getLogger("DASystemNodes.PrintNode")


@NodeDef(
    name="Print",
    category="System / Display",
    icon="",
)
class PrintNode:
    """将输入数据打印到日志（Python logging 和 Qt 输出）。"""

    prefix = Parameter(
        str,
        default="",
        description="输出前缀字符串",
    )

    class Inputs:
        value = Input("any", required=True, description="要打印的数据")

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        value = inputs.get("value")
        prefix = params.get("prefix", "")

        try:
            text = str(value)
        except Exception:
            text = "<unprintable>"

        if prefix:
            logger.info("%s%s", prefix, text)
        else:
            logger.info("%s", text)

        print(f"[PrintNode] {prefix}{text}")
        return True
