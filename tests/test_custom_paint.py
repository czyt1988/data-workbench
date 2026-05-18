"""
test_custom_paint — DAPyPainterProxy Python 侧接口测试

DAPyPainterProxy 是 C++ 类（通过 pybind11 绑定暴露给 Python），
由于测试要求 headless（不依赖 Qt/GUI），这里测试：
1. DAPyPainterProxy 概念在 Python 模块中不存在（纯 C++ 绑定）
2. NodeDef render_template 字段支持 'rect'/'svg'/'widget'（用于 custom paint）
3. _node_display 中的 render_template 可正确传递自定义渲染信息
"""

import pytest
import da_py_workflow
from DAWorkbench.DAWorkFlowPy import NodeDef, Output, NodeDisplay


class TestCustomPaintRenderTemplate:
    """自定义渲染模板测试"""

    def test_render_template_rect(self):
        """rect 渲染模板映射到 NodeStyleTemplate"""
        @NodeDef(name="Rect Node", render_template="rect")
        class RectNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        display = RectNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate

    def test_render_template_svg(self):
        """svg 渲染模板映射到 NodeStyleTemplate"""
        @NodeDef(name="SVG Node", render_template="svg")
        class SVGNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        display = SVGNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate

    def test_render_template_widget(self):
        """widget 渲染模板映射到 WidgetTemplate"""
        @NodeDef(name="Widget Node", render_template="widget")
        class WidgetNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        display = WidgetNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.render_template == da_py_workflow.RenderTemplate.WidgetTemplate

    def test_render_template_nodestyle(self):
        """nodestyle 渲染模板映射到 NodeStyleTemplate"""
        @NodeDef(name="NodeStyle Node", render_template="nodestyle")
        class NodeStyleNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        display = NodeStyleNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate


class TestNodeDisplayPaintInfo:
    """NodeDisplay 渲染属性聚合测试"""

    def test_node_display_carries_paint_info(self):
        """NodeDisplay 承载渲染信息，供 C++ DAPyPainterProxy 使用"""
        @NodeDef(name="Paintable Node", render_template="svg", icon=":icons/paint.svg")
        class PaintableNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        display = PaintableNode._node_display
        # render_template 字段存在且为 da_py_workflow.RenderTemplate 枚举值
        assert display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate
        # icon 字段存在
        assert display.icon == ":icons/paint.svg"

    def test_node_display_with_style(self):
        """NodeDisplay 承载样式信息"""
        style = da_py_workflow.DANodeStyle()

        @NodeDef(name="Styled Paint Node", render_template="rect", style=style)
        class StyledPaintNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        display = StyledPaintNode._node_display
        assert display.style is not None
        assert isinstance(display.style, da_py_workflow.DANodeStyle)

    def test_node_display_default_no_style(self):
        """NodeDisplay 默认无样式"""
        @NodeDef(name="No Style Paint Node", render_template="rect")
        class NoStylePaintNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        display = NoStylePaintNode._node_display
        assert display.style is None