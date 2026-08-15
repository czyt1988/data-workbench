"""
工作流节点样式演示模块

本模块定义了 7 个非功能性节点，用于展示不同的节点样式配置。
所有节点的 execute() 返回 True，不执行任何数据处理。
通过 NodeDef 装饰器的 style 参数设置节点样式（使用 NodeDisplay dataclass）。

节点列表：
1. EllipseDemoNode — 椭圆体 + 名称下方 + 圆形端口
2. DefaultRectNode — 默认样式（无样式配置）
3. CirclePortsNode — 矩形体 + 圆形端口 + 南北布局
4. DiamondPortsNode — 矩形体 + 菱形端口 + 彩色端口填充
5. CustomColorNode — 红色背景 + 蓝色边框
6. MixedLayoutNode — 椭圆体 + 名称下方 + 菱形端口 + 蓝色边框
7. CornerRadiusNode — 矩形体 + 大圆角
"""

from ..node_def import NodeDef, NodeDisplay, LinkPointStyle
from ..types import Input, Output

# ============================================================================
# 1. EllipseDemoNode
#    椭圆体 + 名称下方 + 图标上方 + 圆形端口（输入北/输出南）
# ============================================================================


@NodeDef(name="Ellipse Demo", category="Style Demo", style=NodeDisplay(
    body_shape="Ellipse",
    name_position="Below",
    icon_position="AboveText",
    input_port_side="North",
    output_port_side="South",
    input_port_style=LinkPointStyle(shape="Circle"),
    output_port_style=LinkPointStyle(shape="Circle"),
), description=_("Demonstrates ellipse body shape with name below, icon above, and circular ports (input north / output south)."),  # cn:演示椭圆体形状，名称在下方，图标在上方，圆形端口（输入北/输出南）。
)
class EllipseDemoNode:
    """椭圆体演示节点，展示椭圆形状、名称下方和圆形端口"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self):
        return True


# ============================================================================
# 2. DefaultRectNode
#    默认样式（不设置 style 参数）
# ============================================================================
@NodeDef(name="Default Rect", category="Style Demo", description=_("Demonstrates the default node appearance with no custom style applied."),  # cn:演示默认节点外观，未应用任何自定义样式。
)
class DefaultRectNode:
    """默认矩形节点，展示默认样式行为"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self):
        return True


# ============================================================================
# 3. CirclePortsNode
#    矩形体 + 圆形端口 + 输入北/输出南
# ============================================================================


@NodeDef(name="Circle Ports", category="Style Demo", style=NodeDisplay(
    input_port_side="North",
    output_port_side="South",
    input_port_style=LinkPointStyle(shape="Circle"),
    output_port_style=LinkPointStyle(shape="Circle"),
), description=_("Demonstrates rectangular body with circular ports and north/south port layout."),  # cn:演示矩形体配圆形端口，端口南北布局。
)
class CirclePortsNode:
    """圆形端口演示节点，展示矩形体 + 圆形端口 + 南北布局"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self):
        return True


# ============================================================================
# 4. DiamondPortsNode
#    矩形体 + 菱形端口 + 彩色端口填充
# ============================================================================


@NodeDef(name="Diamond Ports", category="Style Demo", style=NodeDisplay(
    input_port_style=LinkPointStyle(shape="Diamond", fill_color=(255, 200, 200)),
    output_port_style=LinkPointStyle(shape="Diamond", fill_color=(200, 200, 255)),
), description=_("Demonstrates rectangular body with diamond-shaped ports and colored port fills."),  # cn:演示矩形体配菱形端口和彩色端口填充。
)
class DiamondPortsNode:
    """菱形端口演示节点，展示矩形体 + 菱形端口 + 彩色填充"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self):
        return True


# ============================================================================
# 5. CustomColorNode
#    红色背景 + 蓝色边框
# ============================================================================


@NodeDef(name="Custom Colors", category="Style Demo", style=NodeDisplay(
    background_color=(255, 200, 200),
    border_color=(0, 0, 255),
), description=_("Demonstrates custom background and border colors (red background, blue border)."),  # cn:演示自定义背景色和边框色（红色背景，蓝色边框）。
)
class CustomColorNode:
    """自定义颜色演示节点，展示红色背景 + 蓝色边框"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self):
        return True


# ============================================================================
# 6. MixedLayoutNode
#    椭圆体 + 名称下方 + 菱形输出端口 + 蓝色边框
# ============================================================================


@NodeDef(name="Mixed Layout", category="Style Demo", style=NodeDisplay(
    body_shape="Ellipse",
    name_position="Below",
    output_port_style=LinkPointStyle(shape="Diamond"),
    border_color=(50, 50, 200),
), description=_("Demonstrates a mixed layout: ellipse body, name below, diamond output ports, and blue border."),  # cn:演示混合布局：椭圆体、名称在下方、菱形输出端口、蓝色边框。
)
class MixedLayoutNode:
    """混合布局演示节点，展示椭圆体 + 名称下方 + 菱形端口 + 蓝色边框"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self):
        return True


# ============================================================================
# 7. CornerRadiusNode
#    矩形体 + 大圆角
# ============================================================================


@NodeDef(name="Large Radius", category="Style Demo", style=NodeDisplay(
    corner_radius=12.0,
), description=_("Demonstrates a rectangular body with large corner radius."),  # cn:演示矩形体配大圆角半径。
)
class CornerRadiusNode:
    """大圆角演示节点，展示矩形体 + 大圆角半径"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self):
        return True