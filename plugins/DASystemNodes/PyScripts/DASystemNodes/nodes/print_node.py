# -*- coding: utf-8 -*-
"""打印节点：将输入数据打印到日志/控制台，并在节点上显示文字内容"""

import logging
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter

logger = logging.getLogger("DASystemNodes.PrintNode")


@NodeDef(
    name="Print",
    category="System / Display",
    icon="",
)
class PrintNode:
    """将输入数据打印到日志（Python logging 和 Qt 输出），并在节点画面上显示文字。"""

    prefix = Parameter(
        str,
        default="",
        description="输出前缀字符串",
    )

    class Inputs:
        value = Input("any", required=True, description="要打印的数据")

    def __init__(self):
        super().__init__()
        self._last_text = "(no input)"
        self._last_prefix = ""

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

        # 缓存用于 paint() 显示
        self._last_text = text
        self._last_prefix = prefix

        if prefix:
            logger.info("%s%s", prefix, text)
        else:
            logger.info("%s", text)

        print(f"[PrintNode] {prefix}{text}")
        return True

    def paint(self, painter, body_rect):
        """自定义节点绘制：显示节点名 + 缓存的输入文字。

        由 DAPyNodeGraphicsItem::setProxy() 自动注册为绘制回调，
        节点状态变更（execute() 后）会自动触发 item->update() 重绘。

        Args:
            painter: da_py_workflow.DAPyPainterProxy 实例
            body_rect: tuple (x, y, w, h) 节点主体矩形（item 局部坐标）
        """
        x, y, w, h = body_rect

        # 背景
        painter.setNoPen()
        painter.fillRect(x, y, w, h, 250, 250, 250, 255)

        # 边框
        painter.setNoBrush()
        painter.setPenColor(150, 150, 150, 255)
        painter.setPenWidth(1)
        painter.drawRect(x, y, w, h)

        # 标题栏分割线
        painter.setPenColor(200, 200, 200, 255)
        painter.drawLine(x, y + 16, x + w, y + 16)

        # 标题 "Print"
        painter.setPenColor(80, 80, 80, 255)
        painter.setFont("Arial", 8)
        painter.drawText(x + 4, y + 12, "Print")

        # 显示输入文字（截断超长）
        display = getattr(self, "_last_text", "(no input)")
        prefix = getattr(self, "_last_prefix", "")
        if prefix:
            display = f"{prefix}{display}"

        bw, bh = painter.boundingRect(display, "Arial", 8)
        max_w = w - 8
        if bw > max_w and bw > 0:
            # 按比例截断并加省略号
            keep = max(1, int(len(display) * max_w / bw))
            display = display[:keep] + "…"

        painter.setPenColor(20, 20, 20, 255)
        painter.setFont("Arial", 8)
        painter.drawText(x + 4, y + 30, display)
