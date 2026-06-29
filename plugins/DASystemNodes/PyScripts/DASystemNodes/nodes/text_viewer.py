# -*- coding: utf-8 -*-
"""Text viewer node: display input data as text on the node body"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter, NodeDisplay

_DEFAULT_FONT = {
    "family": "Microsoft YaHei",
    "size": 9,
    "bold": False,
    "italic": False,
    "color": "#282828",
}


def _hex_to_rgb(color):
    """
    将颜色值转换为 (r, g, b) 元组。

    支持格式：
      - "#RRGGBB" / "RRGGBB" / "#RGB" / "RGB" 十六进制字符串
      - (r, g, b) 元组或列表

    :param color: 颜色值
    :return: (r, g, b) 元组，解析失败返回 (40, 40, 40)
    """
    if isinstance(color, (tuple, list)) and len(color) >= 3:
        try:
            return (int(color[0]), int(color[1]), int(color[2]))
        except (TypeError, ValueError):
            return (40, 40, 40)
    if not isinstance(color, str):
        return (40, 40, 40)
    s = color.strip().lstrip("#")
    if len(s) == 3:
        s = s[0] * 2 + s[1] * 2 + s[2] * 2
    if len(s) != 6:
        return (40, 40, 40)
    try:
        return (int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16))
    except ValueError:
        return (40, 40, 40)


def _wrap_text_by_width(painter, text, max_width, font):
    """
    按像素宽度逐字符测量切分文本为多行，遇到换行符 \\n 也强制换行。

    :param painter: DAPyPainterProxy 实例
    :param text: 原始文本
    :param max_width: 单行最大像素宽度
    :param font: 字体字典（含 family/size/bold/italic/color 键）
    :return: 切分后的行列表
    """
    if not text:
        return [""]
    lines = []
    current = ""
    for ch in text:
        if ch == "\n":
            lines.append(current)
            current = ""
            continue
        candidate = current + ch
        w, _ = painter.textBoundingRectWithFont(candidate, font)
        if w > max_width and current:
            lines.append(current)
            current = ch
        else:
            current = candidate
    lines.append(current)
    return lines


@NodeDef(
    name="Text Viewer",
    category=_("System / Display"),  # cn:系统 / 显示
    icon="",
    style=NodeDisplay(
        background_color="#FDFDFD",
        border_color="#AAAAAA",
        name_position="Below",
    ),
)
class TextViewerNode:
    """Receive any data, stringify it, and display the text on the node body."""

    font = Parameter(
        "font",
        default=_DEFAULT_FONT,
        description=_("Text font (family/size/bold/italic/color)"),  # cn:文本字体（族/字号/粗体/斜体/颜色）
    )
    max_text_length = Parameter(
        int,
        default=200,
        min=1,
        description=_("Maximum text character count, excess is truncated with …"),  # cn:文字最大字符数，超出部分以 … 截断
    )
    wrap_text = Parameter(
        bool,
        default=True,
        description=_("Auto-wrap text by node body width; off for single line, excess is clipped"),  # cn:是否按节点体宽度自动换行；关闭则单行显示，超出部分被裁剪
    )

    class Inputs:
        value = Input("any", required=True, description=_("Data to display"))  # cn:要显示的数据

    def __init__(self):
        super().__init__()
        self._display_text = ""

    def execute(self, inputs=None, params=None):
        """缓存输入数据的字符串形式，实际绘制在 paint() 中完成。"""
        if inputs is None:
            inputs = {}
        value = inputs.get("value")
        try:
            self._display_text = str(value) if value is not None else ""
        except Exception:
            self._display_text = _("<unprintable>")  # cn:<不可打印>
        return True

    def serialize_runtime_state(self) -> dict:
        """持久化 execute() 缓存的显示文本，使得工程重新加载后无需运行即可显示。"""
        return {"display_text": getattr(self, "_display_text", "")}

    def deserialize_runtime_state(self, state: dict) -> None:
        """从工程文件恢复显示文本缓存。"""
        self._display_text = state.get("display_text", "")

    def paint(self, painter, body_rect):
        """
        自定义绘制回调：在节点体上按参数渲染缓存的文本。

        由 DAPyNodeGraphicsItem::setProxy() 自动注册，节点状态变更后触发重绘。
        """
        x, y, w, h = body_rect
        font = getattr(self, "font", _DEFAULT_FONT)
        max_text_length = getattr(self, "max_text_length", 200)
        wrap_text = getattr(self, "wrap_text", True)

        color = font.get("color", "#282828") if isinstance(font, dict) else "#282828"
        r, g, b = _hex_to_rgb(color)
        margin = 6

        # 限制绘制区域，避免超出节点体
        painter.setClipRect(x + margin, y + margin, w - 2 * margin, h - 2 * margin)

        # 设置字体和颜色
        painter.setFontFromDict(font)
        painter.setPenColor(r, g, b)

        # 取缓存文本
        text = getattr(self, "_display_text", "") or _("(no data)")  # cn:(无数据)

        # 按字符数截断
        if len(text) > max_text_length:
            text = text[:max_text_length] + "…"

        # 测量行高
        _, line_height = painter.textBoundingRectWithFont("A", font)
        size = font.get("size", 9) if isinstance(font, dict) else 9
        line_height = max(line_height, size + 2)

        # 行切分
        if wrap_text:
            max_width = max(1, w - 2 * margin)
            lines = _wrap_text_by_width(painter, text, max_width, font)
        else:
            lines = text.split("\n")

        # 逐行绘制
        draw_y = y + margin + line_height
        for line in lines:
            if draw_y > y + h - margin:
                break
            painter.drawText(x + margin, draw_y, line)
            draw_y += line_height

        painter.clearClip()
