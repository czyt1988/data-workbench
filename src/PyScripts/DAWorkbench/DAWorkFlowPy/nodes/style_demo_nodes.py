"""
工作流节点样式演示模块

本模块定义了 7 个非功能性节点，用于展示不同的节点样式配置。
所有节点的 execute() 返回 True，不执行任何数据处理。
通过 NodeDef 装饰器的 style 参数设置节点样式。

节点列表：
1. EllipseDemoNode — 椭圆体 + 名称下方 + 圆形端口
2. DefaultRectNode — 默认样式（无样式配置）
3. CirclePortsNode — 矩形体 + 圆形端口 + 南北布局
4. DiamondPortsNode — 矩形体 + 菱形端口 + 彩色端口填充
5. CustomColorNode — 红色背景 + 蓝色边框
6. MixedLayoutNode — 椭圆体 + 名称下方 + 菱形端口 + 蓝色边框
7. CornerRadiusNode — 矩形体 + 大圆角
"""

import da_py_workflow

from ..node_def import NodeDef
from ..types import Input, Output

# ============================================================================
# 1. EllipseDemoNode
#    椭圆体 + 名称下方 + 图标上方 + 圆形端口（输入北/输出南）
# ============================================================================
_ellipse_style = da_py_workflow.DANodeStyle()
_ellipse_style.bodyShape = da_py_workflow.BodyShape.Ellipse
_ellipse_style.namePosition = da_py_workflow.NamePosition.Below
_ellipse_style.iconPosition = da_py_workflow.IconPosition.AboveText
_ellipse_style.inputPortSide = da_py_workflow.AspectDirection.North
_ellipse_style.outputPortSide = da_py_workflow.AspectDirection.South
_ellipse_style.inputPortStyle.shape = da_py_workflow.PortShape.Circle
_ellipse_style.outputPortStyle.shape = da_py_workflow.PortShape.Circle


@NodeDef(name="Ellipse Demo", category="Style Demo", style=_ellipse_style)
class EllipseDemoNode:
    """椭圆体演示节点，展示椭圆形状、名称下方和圆形端口"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 2. DefaultRectNode
#    默认样式（不设置 style 参数）
# ============================================================================
@NodeDef(name="Default Rect", category="Style Demo")
class DefaultRectNode:
    """默认矩形节点，展示默认样式行为"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 3. CirclePortsNode
#    矩形体 + 圆形端口 + 输入北/输出南
# ============================================================================
_circle_ports_style = da_py_workflow.DANodeStyle()
_circle_ports_style.inputPortSide = da_py_workflow.AspectDirection.North
_circle_ports_style.outputPortSide = da_py_workflow.AspectDirection.South
_circle_ports_style.inputPortStyle.shape = da_py_workflow.PortShape.Circle
_circle_ports_style.outputPortStyle.shape = da_py_workflow.PortShape.Circle


@NodeDef(name="Circle Ports", category="Style Demo", style=_circle_ports_style)
class CirclePortsNode:
    """圆形端口演示节点，展示矩形体 + 圆形端口 + 南北布局"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 4. DiamondPortsNode
#    矩形体 + 菱形端口 + 彩色端口填充
# ============================================================================
_diamond_ports_style = da_py_workflow.DANodeStyle()
_diamond_ports_style.inputPortStyle.shape = da_py_workflow.PortShape.Diamond
_diamond_ports_style.outputPortStyle.shape = da_py_workflow.PortShape.Diamond
_diamond_ports_style.inputPortStyle.setFillColor(255, 200, 200)
_diamond_ports_style.outputPortStyle.setFillColor(200, 200, 255)


@NodeDef(name="Diamond Ports", category="Style Demo", style=_diamond_ports_style)
class DiamondPortsNode:
    """菱形端口演示节点，展示矩形体 + 菱形端口 + 彩色填充"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 5. CustomColorNode
#    红色背景 + 蓝色边框
# ============================================================================
_custom_color_style = da_py_workflow.DANodeStyle()
_custom_color_style.setBackgroundColor(255, 200, 200)
_custom_color_style.setBorderColor(0, 0, 255)


@NodeDef(name="Custom Colors", category="Style Demo", style=_custom_color_style)
class CustomColorNode:
    """自定义颜色演示节点，展示红色背景 + 蓝色边框"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 6. MixedLayoutNode
#    椭圆体 + 名称下方 + 菱形输出端口 + 蓝色边框
# ============================================================================
_mixed_layout_style = da_py_workflow.DANodeStyle()
_mixed_layout_style.bodyShape = da_py_workflow.BodyShape.Ellipse
_mixed_layout_style.namePosition = da_py_workflow.NamePosition.Below
_mixed_layout_style.outputPortStyle.shape = da_py_workflow.PortShape.Diamond
_mixed_layout_style.setBorderColor(50, 50, 200)


@NodeDef(name="Mixed Layout", category="Style Demo", style=_mixed_layout_style)
class MixedLayoutNode:
    """混合布局演示节点，展示椭圆体 + 名称下方 + 菱形端口 + 蓝色边框"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True


# ============================================================================
# 7. CornerRadiusNode
#    矩形体 + 大圆角
# ============================================================================
_corner_radius_style = da_py_workflow.DANodeStyle()
_corner_radius_style.cornerRadius = 12.0


@NodeDef(name="Large Radius", category="Style Demo", style=_corner_radius_style)
class CornerRadiusNode:
    """大圆角演示节点，展示矩形体 + 大圆角半径"""

    class Inputs:
        data = Input("any")

    class Outputs:
        result = Output("any")

    def execute(self, inputs, params):
        return True
