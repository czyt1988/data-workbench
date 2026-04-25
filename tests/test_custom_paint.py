"""
test_custom_paint — DAPyPainterProxy Python 侧接口测试

DAPyPainterProxy 是 C++ 类（通过 pybind11 绑定暴露给 Python），
由于测试要求 headless（不依赖 Qt/GUI），这里测试：
1. DAPyPainterProxy 概念在 Python 模块中不存在（纯 C++ 绑定）
2. NodeDef render_template 字段支持 'rect'/'svg'/'widget'（用于 custom paint）
3. _node_descriptor 中的 render_template 可正确传递自定义渲染信息
"""

import pytest
from DAWorkbench.DAWorkFlowPy import NodeDef, Output, DANodeDescriptor


class TestCustomPaintRenderTemplate:
    """自定义渲染模板测试"""

    def test_render_template_rect(self):
        """rect 渲染模板（默认）"""
        @NodeDef(name="Rect Node", render_template="rect")
        class RectNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        desc = RectNode._node_descriptor
        assert desc["render_template"] == "rect"

    def test_render_template_svg(self):
        """svg 渲染模板（自定义 SVG 渲染）"""
        @NodeDef(name="SVG Node", render_template="svg")
        class SVGNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        desc = SVGNode._node_descriptor
        assert desc["render_template"] == "svg"

    def test_render_template_widget(self):
        """widget 渲染模板（自定义 widget 渲染）"""
        @NodeDef(name="Widget Node", render_template="widget")
        class WidgetNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        desc = WidgetNode._node_descriptor
        assert desc["render_template"] == "widget"

    def test_invalid_render_template_raises(self):
        """无效渲染模板抛 ValueError"""
        with pytest.raises(ValueError, match="无效的渲染模板"):
            @NodeDef(name="Bad Node", render_template="unknown")
            class BadNode:
                pass

    def test_descriptor_render_template_serialization(self):
        """DANodeDescriptor render_template 序列化"""
        desc = DANodeDescriptor(
            name="Paint Node", category="Paint",
            qualified_name="mod.PaintNode",
            render_template="svg",
        )
        d = desc.to_dict()
        assert d["render_template"] == "svg"

    def test_descriptor_invalid_render_template_raises(self):
        """DANodeDescriptor 无效 render_template 抛 ValueError"""
        with pytest.raises(ValueError, match="无效的渲染模板"):
            DANodeDescriptor(name="Bad", render_template="invalid")


class TestCustomPaintConcept:
    """自定义绘制概念验证测试"""

    def test_painter_proxy_is_cpp_binding_not_python(self):
        """DAPyPainterProxy 是 C++ 绑定，不在 DAWorkFlowPy Python 包中"""
        import DAWorkbench.DAWorkFlowPy as DAWorkFlowPy
        # DAPyPainterProxy 不在 Python 包导出列表中
        assert "DAPyPainterProxy" not in DAWorkFlowPy.__all__

    def test_node_descriptor_carries_paint_info(self):
        """节点描述符承载渲染信息，供 C++ DAPyPainterProxy 使用"""
        @NodeDef(name="Paintable Node", render_template="svg")
        class PaintableNode:
            class Outputs:
                data = Output("DataFrame")

            def execute(self, inputs=None, params=None):
                pass
        desc = PaintableNode._node_descriptor
        # render_template 字段存在且可被序列化
        assert "render_template" in desc
        assert desc["render_template"] in ("rect", "svg", "widget")

    def test_render_template_in_descriptor_roundtrip(self):
        """render_template 在 to_dict/from_dict 往返中保留"""
        @NodeDef(name="RT Roundtrip", render_template="widget")
        class RTRoundtripNode:
            pass
        desc = DANodeDescriptor.from_class(RTRoundtripNode)
        d = desc.to_dict()
        assert d["render_template"] == "widget"
        # 通过 descriptor_data 重建
        desc2 = DANodeDescriptor(**d)
        assert desc2.render_template == "widget"
