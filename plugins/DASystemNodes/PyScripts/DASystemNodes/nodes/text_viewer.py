# -*- coding: utf-8 -*-
"""文本显示节点：将输入数据以文本形式绘制在节点体上"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter, NodeDisplay


def data_to_text(value, max_lines: int = 8, max_width: int = 40) -> list[str]:
    """
    将任意数据对象转换为用于节点显示的文本行列表。

    :param value: 任意输入数据
    :param max_lines: 最大行数
    :param max_width: 每行最大字符数（按字符数截断）
    :return: 文本行列表
    """
    if value is None:
        return ["None"]

    try:
        text = str(value)
    except Exception:
        text = "<unprintable>"

    lines = text.splitlines()
    result = []
    for line in lines:
        # 按字符数截断，避免单行过长
        while len(line) > max_width:
            result.append(line[:max_width])
            line = line[max_width:]
            if len(result) >= max_lines:
                break
        if result:
            if len(result) >= max_lines:
                break
        result.append(line)
        if len(result) >= max_lines:
            break

    if len(result) > max_lines:
        result = result[:max_lines]

    return result if result else [""]


@NodeDef(
    name="Text Viewer",
    category="System / Display",
    icon="",
    style=NodeDisplay(
        background_color="#FDFDFD",
        border_color="#AAAAAA",
        name_position="Below",
    ),
)
class TextViewerNode:
    """接收任意数据，将其字符串化后在节点体上绘制显示。"""

    max_lines = Parameter(
        int,
        default=8,
        description="节点体上最多显示的行数",
    )
    font_size = Parameter(
        int,
        default=9,
        description="显示文本的字号",
    )
    line_length = Parameter(
        int,
        default=40,
        description="每行最大字符数，超出自动截断",
    )

    class Inputs:
        value = Input("any", required=True, description="要显示的数据")

    def __init__(self):
        super().__init__()
        self._display_lines = []

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        value = inputs.get("value")
        max_lines = params.get("max_lines", 8)
        line_length = params.get("line_length", 40)

        self._display_lines = data_to_text(value, max_lines=max_lines, max_width=line_length)
        return True

    def paint(self, painter, body_rect):
        """自定义绘制回调：在节点体上绘制缓存的文本行。"""
        x, y, w, h = body_rect
        font_size = getattr(self, "font_size", 9)

        # 边距
        margin = 6
        painter.setPenColor(40, 40, 40)
        painter.setFont("Microsoft YaHei", font_size)

        # 测量行高
        _, line_height = painter.boundingRect("A", "Microsoft YaHei", font_size)
        line_height = max(line_height, font_size + 2)

        # 限制绘制区域，避免超出节点体
        painter.setClipRect(x + margin, y + margin, w - 2 * margin, h - 2 * margin)

        lines = getattr(self, "_display_lines", [])
        if not lines:
            lines = ["<no data>"]

        draw_y = y + margin + line_height
        for line in lines:
            if draw_y > y + h - margin:
                break
            painter.drawText(x + margin, draw_y, str(line))
            draw_y += line_height

        painter.clearClip()
