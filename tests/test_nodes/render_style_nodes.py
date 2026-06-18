"""
渲染样式测试节点模块

本模块定义了覆盖 DANodeStyle 所有字段的测试节点，
用于验证节点渲染配置的完整性和正确性。

测试节点按照 DANodeStyle 字段分组：

1. BodyShape — 节点体形状（RoundedRect / Ellipse）
2. NamePosition — 节点名称位置（Inside / Below）
3. IconPosition — 图标位置（LeftOfText / AboveText）
4. PortShape — 端口形状（Rect / Circle / Diamond）
5. PortSide — 端口方位（East / South / West / North）
6. Colors — 背景色、边框色、端口填充色、端口边框色
7. Dimensions — borderWidth、cornerRadius、iconSize
8. DAPyLinkPointStyle — 端口样式独立字段
9. BodyIcon — 节点体图标类型、源路径、缩放比例
10. LayoutStrategy — 连接点布局策略（Auto / Manual）
11. Composite — 组合样式，验证多字段交互

所有节点的 execute() 返回 True，不执行任何数据处理。
通过 NodeDef 装饰器的 style 参数设置 DANodeStyle 实例。

注意：da_py_workflow 是 pybind11 嵌入模块，仅在 C++ 运行时可用。
本模块仅在嵌入 Python 环境中加载，独立 pytest 无法使用 da_py_workflow。
"""

import da_py_workflow

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output

# ============================================================================
# 1. BodyShape — 节点体形状
# ============================================================================

_style_rounded_rect = da_py_workflow.DANodeStyle()
_style_rounded_rect.bodyShape = da_py_workflow.BodyShape.RoundedRect


@NodeDef(
    name="Test RoundedRect Body", category="Render/BodyShape", style=_style_rounded_rect
)
class TestRoundedRectNode:
    """显式 RoundedRect 体形状测试节点 — 验证圆角矩形渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_ellipse = da_py_workflow.DANodeStyle()
_style_ellipse.bodyShape = da_py_workflow.BodyShape.Ellipse


@NodeDef(name="Test Ellipse Body", category="Render/BodyShape", style=_style_ellipse)
class TestEllipseNode:
    """椭圆体形状测试节点 — 验证椭圆渲染及碰撞检测"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 2. NamePosition — 节点名称位置
# ============================================================================

_style_name_inside = da_py_workflow.DANodeStyle()
_style_name_inside.namePosition = da_py_workflow.NamePosition.Inside


@NodeDef(
    name="Test Name Inside", category="Render/NamePosition", style=_style_name_inside
)
class TestNameInsideNode:
    """名称在节点内部测试节点 — 验证 boundingRect 不向下扩展"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_name_below = da_py_workflow.DANodeStyle()
_style_name_below.namePosition = da_py_workflow.NamePosition.Below


@NodeDef(
    name="Test Name Below", category="Render/NamePosition", style=_style_name_below
)
class TestNameBelowNode:
    """名称在节点下方测试节点 — 验证 boundingRect 向下扩展容纳文本"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 3. IconPosition — 图标位置
# ============================================================================

_style_icon_left = da_py_workflow.DANodeStyle()
_style_icon_left.iconPosition = da_py_workflow.IconPosition.LeftOfText


@NodeDef(
    name="Test Icon LeftOfText", category="Render/IconPosition", style=_style_icon_left
)
class TestIconLeftNode:
    """图标在文本左侧测试节点 — 验证图标水平布局"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_icon_above = da_py_workflow.DANodeStyle()
_style_icon_above.iconPosition = da_py_workflow.IconPosition.AboveText


@NodeDef(
    name="Test Icon AboveText", category="Render/IconPosition", style=_style_icon_above
)
class TestIconAboveNode:
    """图标在文本上方测试节点 — 验证图标垂直布局"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 4. PortShape — 端口形状
# ============================================================================

_style_rect_ports = da_py_workflow.DANodeStyle()
_style_rect_ports.inputPortStyle.shape = da_py_workflow.PortShape.Rect
_style_rect_ports.outputPortStyle.shape = da_py_workflow.PortShape.Rect


@NodeDef(name="Test Rect Ports", category="Render/PortShape", style=_style_rect_ports)
class TestRectPortsNode:
    """矩形端口测试节点 — 验证默认端口形状渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_circle_ports = da_py_workflow.DANodeStyle()
_style_circle_ports.inputPortStyle.shape = da_py_workflow.PortShape.Circle
_style_circle_ports.outputPortStyle.shape = da_py_workflow.PortShape.Circle


@NodeDef(
    name="Test Circle Ports", category="Render/PortShape", style=_style_circle_ports
)
class TestCirclePortsNode:
    """圆形端口测试节点 — 验证圆形端口渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_diamond_ports = da_py_workflow.DANodeStyle()
_style_diamond_ports.inputPortStyle.shape = da_py_workflow.PortShape.Diamond
_style_diamond_ports.outputPortStyle.shape = da_py_workflow.PortShape.Diamond


@NodeDef(
    name="Test Diamond Ports", category="Render/PortShape", style=_style_diamond_ports
)
class TestDiamondPortsNode:
    """菱形端口测试节点 — 验证菱形端口渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 5. PortSide — 端口方位
# ============================================================================

_style_default_port_side = da_py_workflow.DANodeStyle()
_style_default_port_side.inputPortSide = da_py_workflow.AspectDirection.West
_style_default_port_side.outputPortSide = da_py_workflow.AspectDirection.East


@NodeDef(
    name="Test Default PortSide",
    category="Render/PortSide",
    style=_style_default_port_side,
)
class TestDefaultPortSideNode:
    """默认端口方位测试节点 — 显式 West/East 布局"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_north_south = da_py_workflow.DANodeStyle()
_style_north_south.inputPortSide = da_py_workflow.AspectDirection.North
_style_north_south.outputPortSide = da_py_workflow.AspectDirection.South


@NodeDef(
    name="Test North-South PortSide",
    category="Render/PortSide",
    style=_style_north_south,
)
class TestNorthSouthPortSideNode:
    """南北端口方位测试节点 — 验证输入北/输出南布局"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 6. Colors — 背景色、边框色
# ============================================================================

_style_background_color = da_py_workflow.DANodeStyle()
_style_background_color.setBackgroundColor(255, 200, 200)  # 浅红色


@NodeDef(
    name="Test BackgroundColor", category="Render/Colors", style=_style_background_color
)
class TestBackgroundColorNode:
    """自定义背景色测试节点 — 验证 setBackgroundColor(255,200,200) 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_border_color = da_py_workflow.DANodeStyle()
_style_border_color.setBorderColor(0, 0, 255)  # 蓝色边框


@NodeDef(name="Test BorderColor", category="Render/Colors", style=_style_border_color)
class TestBorderColorNode:
    """自定义边框色测试节点 — 验证 setBorderColor(0,0,255) 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 7. Dimensions — borderWidth、cornerRadius、iconSize
# ============================================================================

_style_border_width = da_py_workflow.DANodeStyle()
_style_border_width.borderWidth = 3.0


@NodeDef(
    name="Test BorderWidth", category="Render/Dimensions", style=_style_border_width
)
class TestBorderWidthNode:
    """粗边框宽度测试节点 — 验证 borderWidth=3.0 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_corner_radius = da_py_workflow.DANodeStyle()
_style_corner_radius.cornerRadius = 12.0


@NodeDef(
    name="Test CornerRadius", category="Render/Dimensions", style=_style_corner_radius
)
class TestCornerRadiusNode:
    """大圆角半径测试节点 — 验证 cornerRadius=12.0 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_icon_size = da_py_workflow.DANodeStyle()
_style_icon_size.iconSize = 48.0


@NodeDef(name="Test IconSize", category="Render/Dimensions", style=_style_icon_size)
class TestIconSizeNode:
    """大图标尺寸测试节点 — 验证 iconSize=48.0 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 8. DAPyLinkPointStyle — 端口样式颜色
# ============================================================================

_style_port_fill_colors = da_py_workflow.DANodeStyle()
_style_port_fill_colors.inputPortStyle.setFillColor(255, 200, 200)  # 浅红色输入端口
_style_port_fill_colors.outputPortStyle.setFillColor(200, 200, 255)  # 浅蓝色输出端口


@NodeDef(
    name="Test Port FillColors",
    category="Render/PortStyle",
    style=_style_port_fill_colors,
)
class TestPortFillColorsNode:
    """端口填充色测试节点 — 验证 inputPortStyle/outputPortStyle.setFillColor 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_port_border_colors = da_py_workflow.DANodeStyle()
_style_port_border_colors.inputPortStyle.setBorderColor(255, 0, 0)  # 红色输入端口边框
_style_port_border_colors.outputPortStyle.setBorderColor(0, 0, 255)  # 蓝色输出端口边框


@NodeDef(
    name="Test Port BorderColors",
    category="Render/PortStyle",
    style=_style_port_border_colors,
)
class TestPortBorderColorNode:
    """端口边框色测试节点 — 验证 inputPortStyle/outputPortStyle.setBorderColor 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_port_border_width = da_py_workflow.DANodeStyle()
_style_port_border_width.inputPortStyle.borderWidth = 2.0
_style_port_border_width.outputPortStyle.borderWidth = 2.0


@NodeDef(
    name="Test Port BorderWidth",
    category="Render/PortStyle",
    style=_style_port_border_width,
)
class TestPortBorderWidthNode:
    """端口边框宽度测试节点 — 验证 inputPortStyle/outputPortStyle.borderWidth 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 9. BodyIcon — 节点体图标
# ============================================================================

_style_body_icon_pixmap = da_py_workflow.DANodeStyle()
_style_body_icon_pixmap.bodyIconType = da_py_workflow.BodyIconType.Pixmap
_style_body_icon_pixmap.bodyIconSource = ":/test/test_icon.png"
_style_body_icon_pixmap.bodyIconScale = 0.8


@NodeDef(
    name="Test BodyIcon Pixmap",
    category="Render/BodyIcon",
    style=_style_body_icon_pixmap,
)
class TestBodyIconPixmapNode:
    """位图体图标测试节点 — 验证 BodyIconType.Pixmap 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_body_icon_svg = da_py_workflow.DANodeStyle()
_style_body_icon_svg.bodyIconType = da_py_workflow.BodyIconType.Svg
_style_body_icon_svg.bodyIconSource = "./test_icon.svg"
_style_body_icon_svg.bodyIconScale = 0.6


@NodeDef(
    name="Test BodyIcon Svg", category="Render/BodyIcon", style=_style_body_icon_svg
)
class TestBodyIconSvgNode:
    """SVG 体图标测试节点 — 验证 BodyIconType.Svg 渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 10. LayoutStrategy — 连接点布局策略
# ============================================================================

_style_auto_layout = da_py_workflow.DANodeStyle()
_style_auto_layout.layoutStrategy = da_py_workflow.LinkPointLayoutStrategy.Auto


@NodeDef(
    name="Test Auto Layout", category="Render/LayoutStrategy", style=_style_auto_layout
)
class TestAutoLayoutNode:
    """自动布局策略测试节点 — 验证 LinkPointLayoutStrategy.Auto"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


_style_manual_layout = da_py_workflow.DANodeStyle()
_style_manual_layout.layoutStrategy = da_py_workflow.LinkPointLayoutStrategy.Manual


@NodeDef(
    name="Test Manual Layout",
    category="Render/LayoutStrategy",
    style=_style_manual_layout,
)
class TestManualLayoutNode:
    """手动布局策略测试节点 — 验证 LinkPointLayoutStrategy.Manual"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 11. Composite — 组合样式，验证多字段交互
# ============================================================================

# 椭圆 + 名称下方 + 圆形端口 + 南北布局 + 自定义颜色
_style_ellipse_circle_ns = da_py_workflow.DANodeStyle()
_style_ellipse_circle_ns.bodyShape = da_py_workflow.BodyShape.Ellipse
_style_ellipse_circle_ns.namePosition = da_py_workflow.NamePosition.Below
_style_ellipse_circle_ns.iconPosition = da_py_workflow.IconPosition.AboveText
_style_ellipse_circle_ns.inputPortSide = da_py_workflow.AspectDirection.North
_style_ellipse_circle_ns.outputPortSide = da_py_workflow.AspectDirection.South
_style_ellipse_circle_ns.inputPortStyle.shape = da_py_workflow.PortShape.Circle
_style_ellipse_circle_ns.outputPortStyle.shape = da_py_workflow.PortShape.Circle
_style_ellipse_circle_ns.setBackgroundColor(240, 255, 240)  # 浅绿色背景
_style_ellipse_circle_ns.setBorderColor(0, 128, 0)  # 绿色边框


@NodeDef(
    name="Test Ellipse+CirclePorts+NS",
    category="Render/Composite",
    style=_style_ellipse_circle_ns,
)
class TestEllipseCirclePortsNSNode:
    """椭圆+圆形端口+南北布局组合测试节点 — 验证多字段交互渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# 菱形端口 + 彩色端口填充 + 粗边框 + 大圆角
_style_diamond_color_border = da_py_workflow.DANodeStyle()
_style_diamond_color_border.inputPortStyle.shape = da_py_workflow.PortShape.Diamond
_style_diamond_color_border.outputPortStyle.shape = da_py_workflow.PortShape.Diamond
_style_diamond_color_border.inputPortStyle.setFillColor(255, 200, 200)
_style_diamond_color_border.outputPortStyle.setFillColor(200, 200, 255)
_style_diamond_color_border.inputPortStyle.setBorderColor(128, 0, 0)
_style_diamond_color_border.outputPortStyle.setBorderColor(0, 0, 128)
_style_diamond_color_border.borderWidth = 2.0
_style_diamond_color_border.cornerRadius = 8.0


@NodeDef(
    name="Test Diamond+Colors+Border",
    category="Render/Composite",
    style=_style_diamond_color_border,
)
class TestDiamondColorsBorderNode:
    """菱形端口+彩色填充+粗边框+大圆角组合测试节点"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# 全字段非默认值 — 验证所有字段同时偏离默认值的渲染
_style_full = da_py_workflow.DANodeStyle()
_style_full.bodyShape = da_py_workflow.BodyShape.Ellipse
_style_full.namePosition = da_py_workflow.NamePosition.Below
_style_full.iconPosition = da_py_workflow.IconPosition.AboveText
_style_full.setBackgroundColor(100, 150, 200)  # 蓝灰色背景
_style_full.setBorderColor(50, 50, 200)  # 深蓝色边框
_style_full.borderWidth = 2.5
_style_full.cornerRadius = 10.0
_style_full.iconSize = 32.0
_style_full.inputPortSide = da_py_workflow.AspectDirection.North
_style_full.outputPortSide = da_py_workflow.AspectDirection.South
_style_full.inputPortStyle.shape = da_py_workflow.PortShape.Circle
_style_full.outputPortStyle.shape = da_py_workflow.PortShape.Diamond
_style_full.inputPortStyle.setFillColor(255, 255, 200)
_style_full.outputPortStyle.setFillColor(200, 200, 255)
_style_full.inputPortStyle.setBorderColor(200, 200, 0)
_style_full.outputPortStyle.setBorderColor(0, 0, 200)
_style_full.inputPortStyle.borderWidth = 1.5
_style_full.outputPortStyle.borderWidth = 2.0
_style_full.layoutStrategy = da_py_workflow.LinkPointLayoutStrategy.Auto
_style_full.bodyIconType = da_py_workflow.BodyIconType.Pixmap
_style_full.bodyIconSource = ":/test/full_style_icon.png"
_style_full.bodyIconScale = 0.6


@NodeDef(name="Test Full Style", category="Render/Composite", style=_style_full)
class TestFullStyleNode:
    """全字段非默认值测试节点 — 验证所有字段同时偏离默认值的渲染"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True
