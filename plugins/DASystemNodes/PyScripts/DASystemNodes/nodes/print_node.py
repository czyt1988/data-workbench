# -*- coding: utf-8 -*-
"""Print node: print input data to log/console and display text on the node body"""

import logging
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter

logger = logging.getLogger("DASystemNodes.PrintNode")


@NodeDef(
    name="Print",
    category=_("System / Display"),  # cn:系统 / 显示
    icon="",
)
class PrintNode:
    """Print input data to log (Python logging and Qt output) and display text on the node body."""

    title = Parameter(
        str,
        default="",
        description=_("Node title (displayed at node top); empty defaults to 'Print'"),  # cn:节点标题（显示在节点顶部）；为空时默认显示 'Print'
    )

    class Inputs:
        value = Input("any", required=True, description=_("Data to print"))  # cn:要打印的数据

    def __init__(self):
        super().__init__()
        self._last_text = _("(no input)")  # cn:(无输入)
        self._last_title = ""

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        value = inputs.get("value")
        title = params.get("title", "")

        try:
            text = str(value)
        except Exception:
            text = _("<unprintable>")  # cn:<不可打印>

        # 缓存用于 paint() 显示
        self._last_text = text
        self._last_title = title

        if title:
            logger.info("[%s] %s", title, text)
        else:
            logger.info("%s", text)

        print(f"[PrintNode][{title}] {text}" if title else f"[PrintNode] {text}")
        return True

    def serialize_runtime_state(self) -> dict:
        """持久化 execute() 缓存的显示文本，使得工程重新加载后无需运行即可显示。"""
        return {
            "last_text": getattr(self, "_last_text", _("(no input)")),  # cn:(无输入)
            "last_title": getattr(self, "_last_title", ""),
        }

    def deserialize_runtime_state(self, state: dict) -> None:
        """从工程文件恢复显示文本缓存。"""
        self._last_text = state.get("last_text", _("(no input)"))  # cn:(无输入)
        self._last_title = state.get("last_title", "")

    def paint(self, painter, body_rect):
        """自定义节点绘制：显示节点名 + 缓存的输入文字。

        由 DAPyNodeGraphicsItem::setProxy() 自动注册为绘制回调，
        节点状态变更（execute() 后）会自动触发 item->update() 重绘。

        Args:
            painter: da_py_workflow.DAPyPainterProxy 实例
            body_rect: tuple (x, y, w, h) 节点主体矩形（item 局部坐标）
        """
        x, y, w, h = body_rect

        # 背景（fillRect 不受画笔状态影响，无需 setNoPen）
        painter.fillRect(x, y, w, h, 250, 250, 250, 255)

        # 边框（setNoBrush 让 drawRect 只描边不填充）
        painter.setNoBrush()
        painter.setPenColor(150, 150, 150, 255)
        painter.setPenWidth(1)
        painter.drawRect(x, y, w, h)

        # 标题栏分割线
        painter.setPenColor(200, 200, 200, 255)
        painter.drawLine(x, y + 16, x + w, y + 16)

        # 顶部标题：优先显示 title 参数，为空时回退到 "Print"
        title = getattr(self, "_last_title", "") or _("Print")  # cn:Print
        painter.setPenColor(80, 80, 80, 255)
        painter.setFont("Arial", 8)
        painter.drawText(x + 4, y + 12, title)

        # 显示输入文字（按宽度换行，超出高度截断）
        display = getattr(self, "_last_text", _("(no input)"))  # cn:(无输入)

        font_family = "Arial"
        font_size = 8
        margin_x = 4
        text_x = x + margin_x
        text_top = y + 20
        text_bottom = y + h - margin_x
        max_w = w - 2 * margin_x

        painter.setFont(font_family, font_size)
        painter.setPenColor(20, 20, 20, 255)

        # 测量行高
        _, line_height = painter.textBoundingRect("A", font_family, font_size)
        line_height = max(line_height, font_size + 2)

        # 限制绘制区域在边框内
        painter.setClipRect(text_x, text_top, max_w, text_bottom - text_top)

        # 按像素宽度逐字符切分换行
        lines = []
        current = ""
        for ch in display:
            if ch == "\n":
                lines.append(current)
                current = ""
                continue
            candidate = current + ch
            cw, _ = painter.textBoundingRect(candidate, font_family, font_size)
            if cw > max_w and current:
                lines.append(current)
                current = ch
            else:
                current = candidate
        lines.append(current)

        # 逐行绘制，超出底部则停止
        draw_y = text_top + line_height
        for line in lines:
            if draw_y > text_bottom:
                break
            painter.drawText(text_x, draw_y, line)
            draw_y += line_height

        painter.clearClip()
