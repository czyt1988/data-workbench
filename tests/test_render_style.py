"""
test_render_style — 节点渲染样式配置测试

覆盖 DANodeStyle 通过 @NodeDef(style=...) 装饰器的序列化和传递正确性。

测试分为两个层级：
1. NodeDef 装饰器层 — 验证 style 参数正确序列化到 _node_descriptor["style"] 中
2. 节点发现层 — 验证 render_style_nodes 模块中的所有节点可被 DANodeRegistry 发现

注意：涉及 da_py_workflow（pybind11 嵌入模块）的测试需要 C++ 运行时，
独立 pytest 运行时自动跳过这些测试。
"""

import os
import pytest

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output
from DAWorkbench.DAWorkFlowPy.node_descriptor import DANodeDescriptor
from DAWorkbench.DAWorkFlowPy.node_registry import DANodeRegistry


# ==================== 辅助工具 ====================

def _da_py_workflow_available():
    """检查 da_py_workflow 模块是否可用"""
    try:
        import da_py_workflow
        return True
    except ImportError:
        return False


requires_da_py_workflow = pytest.mark.skipif(
    not _da_py_workflow_available(),
    reason="da_py_workflow 模块仅在 C++ 嵌入 Python 环境中可用",
)


# ==================== NodeDef 装饰器 style 参数测试 ====================

class TestNodeDefStyleParameter:
    """NodeDef 装饰器 style 参数处理测试"""

    @requires_da_py_workflow
    def test_style_creates_descriptor_style_key(self):
        """NodeDef(style=...) 在 _node_descriptor 中创建 'style' 键"""
        import da_py_workflow

        style = da_py_workflow.DANodeStyle()
        style.bodyShape = da_py_workflow.BodyShape.Ellipse

        @NodeDef(name="Style Test", category="Test", style=style)
        class StyleTestNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        desc = StyleTestNode._node_descriptor
        assert "style" in desc
        assert isinstance(desc["style"], dict)

    @requires_da_py_workflow
    def test_style_tojson_sparse_strategy(self):
        """DANodeStyle.toJson() 使用稀疏策略 — 仅包含非默认字段"""
        import da_py_workflow

        # 仅修改 bodyShape，其余保持默认
        style = da_py_workflow.DANodeStyle()
        style.bodyShape = da_py_workflow.BodyShape.Ellipse

        @NodeDef(name="Sparse Test", category="Test", style=style)
        class SparseTestNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        style_dict = SparseTestNode._node_descriptor["style"]
        # 稀疏策略：仅 bodyShape 应出现在序列化结果中
        assert "bodyShape" in style_dict
        # 默认值字段不应出现
        assert "borderWidth" not in style_dict  # 默认 1.0
        assert "iconSize" not in style_dict      # 默认 24.0

    @requires_da_py_workflow
    def test_style_default_values_not_serialized(self):
        """默认值的字段不出现在 style 序列化结果中"""
        import da_py_workflow

        # 全默认的 DANodeStyle
        style = da_py_workflow.DANodeStyle()

        @NodeDef(name="Default Style Test", category="Test", style=style)
        class DefaultStyleNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        # 稀疏策略：全默认样式序列化应为空或接近空
        style_dict = DefaultStyleNode._node_descriptor["style"]
        assert isinstance(style_dict, dict)

    @requires_da_py_workflow
    def test_style_dict_parameter(self):
        """NodeDef(style=dict) 直接使用字典作为样式配置"""
        style_dict = {"bodyShape": "Ellipse", "cornerRadius": 10.0}

        @NodeDef(name="Dict Style Test", category="Test", style=style_dict)
        class DictStyleNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        desc = DictStyleNode._node_descriptor
        assert "style" in desc
        assert desc["style"]["bodyShape"] == "Ellipse"
        assert desc["style"]["cornerRadius"] == 10.0

    @requires_da_py_workflow
    def test_style_no_style_parameter(self):
        """NodeDef 无 style 参数时不包含 'style' 键"""
        @NodeDef(name="No Style Test", category="Test")
        class NoStyleNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        desc = NoStyleNode._node_descriptor
        assert "style" not in desc

    @requires_da_py_workflow
    def test_style_all_body_shape_values(self):
        """覆盖所有 BodyShape 枚举值的序列化"""
        import da_py_workflow

        for shape_val, shape_name in [
            (da_py_workflow.BodyShape.RoundedRect, "RoundedRect"),
            (da_py_workflow.BodyShape.Ellipse, "Ellipse"),
        ]:
            style = da_py_workflow.DANodeStyle()
            style.bodyShape = shape_val

            @NodeDef(name=f"BodyShape {shape_name}", category="Test/BodyShape", style=style)
            class BodyShapeNode:
                class Inputs:
                    data = Input("any")
                class Outputs:
                    result = Output("any")
                def execute(self, inputs, params):
                    return True

            style_dict = BodyShapeNode._node_descriptor["style"]
            assert "bodyShape" in style_dict

    @requires_da_py_workflow
    def test_style_all_port_shape_values(self):
        """覆盖所有 PortShape 枚举值的序列化"""
        import da_py_workflow

        for shape_val, shape_name in [
            (da_py_workflow.PortShape.Rect, "Rect"),
            (da_py_workflow.PortShape.Circle, "Circle"),
            (da_py_workflow.PortShape.Diamond, "Diamond"),
        ]:
            style = da_py_workflow.DANodeStyle()
            style.inputPortStyle.shape = shape_val

            @NodeDef(name=f"PortShape {shape_name}", category="Test/PortShape", style=style)
            class PortShapeNode:
                class Inputs:
                    data = Input("any")
                class Outputs:
                    result = Output("any")
                def execute(self, inputs, params):
                    return True

            style_dict = PortShapeNode._node_descriptor["style"]
            # inputPortStyle 应包含 shape 字段
            assert "inputPortStyle" in style_dict

    @requires_da_py_workflow
    def test_style_color_setters(self):
        """验证 setBackgroundColor/setBorderColor 序列化"""
        import da_py_workflow

        style = da_py_workflow.DANodeStyle()
        style.setBackgroundColor(255, 200, 200)
        style.setBorderColor(0, 0, 255)

        @NodeDef(name="Color Setter Test", category="Test/Colors", style=style)
        class ColorSetterNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        style_dict = ColorSetterNode._node_descriptor["style"]
        assert "backgroundColor" in style_dict
        assert "borderColor" in style_dict

    @requires_da_py_workflow
    def test_style_port_fill_border_colors(self):
        """验证端口 fillColor/borderColor 序列化"""
        import da_py_workflow

        style = da_py_workflow.DANodeStyle()
        style.inputPortStyle.setFillColor(255, 200, 200)
        style.outputPortStyle.setBorderColor(0, 0, 255)

        @NodeDef(name="Port Color Test", category="Test/PortColors", style=style)
        class PortColorNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        style_dict = PortColorNode._node_descriptor["style"]
        assert "inputPortStyle" in style_dict
        assert "outputPortStyle" in style_dict

    @requires_da_py_workflow
    def test_style_dimensions_fields(self):
        """验证 borderWidth/cornerRadius/iconSize 序列化"""
        import da_py_workflow

        style = da_py_workflow.DANodeStyle()
        style.borderWidth = 3.0
        style.cornerRadius = 12.0
        style.iconSize = 48.0

        @NodeDef(name="Dimension Test", category="Test/Dimensions", style=style)
        class DimensionNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        style_dict = DimensionNode._node_descriptor["style"]
        assert style_dict["borderWidth"] == 3.0
        assert style_dict["cornerRadius"] == 12.0
        assert style_dict["iconSize"] == 48.0

    @requires_da_py_workflow
    def test_style_body_icon_fields(self):
        """验证 bodyIconType/bodyIconSource/bodyIconScale 序列化"""
        import da_py_workflow

        style = da_py_workflow.DANodeStyle()
        style.bodyIconType = da_py_workflow.BodyIconType.Pixmap
        style.bodyIconSource = ":/test/icon.png"
        style.bodyIconScale = 0.6

        @NodeDef(name="BodyIcon Test", category="Test/BodyIcon", style=style)
        class BodyIconNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        style_dict = BodyIconNode._node_descriptor["style"]
        assert "bodyIconType" in style_dict
        assert "bodyIconSource" in style_dict
        assert "bodyIconScale" in style_dict

    @requires_da_py_workflow
    def test_style_layout_strategy_values(self):
        """覆盖所有 LinkPointLayoutStrategy 枚举值"""
        import da_py_workflow

        for strategy_val, strategy_name in [
            (da_py_workflow.LinkPointLayoutStrategy.Auto, "Auto"),
            (da_py_workflow.LinkPointLayoutStrategy.Manual, "Manual"),
        ]:
            style = da_py_workflow.DANodeStyle()
            style.layoutStrategy = strategy_val

            @NodeDef(name=f"Layout {strategy_name}", category="Test/Layout", style=style)
            class LayoutNode:
                class Inputs:
                    data = Input("any")
                class Outputs:
                    result = Output("any")
                def execute(self, inputs, params):
                    return True

            style_dict = LayoutNode._node_descriptor["style"]
            assert "layoutStrategy" in style_dict

    @requires_da_py_workflow
    def test_style_port_side_values(self):
        """覆盖所有 AspectDirection (PortSide) 枚举值"""
        import da_py_workflow

        for side_val, side_name in [
            (da_py_workflow.AspectDirection.East, "East"),
            (da_py_workflow.AspectDirection.South, "South"),
            (da_py_workflow.AspectDirection.West, "West"),
            (da_py_workflow.AspectDirection.North, "North"),
        ]:
            style = da_py_workflow.DANodeStyle()
            style.inputPortSide = side_val

            @NodeDef(name=f"PortSide {side_name}", category="Test/PortSide", style=style)
            class PortSideNode:
                class Inputs:
                    data = Input("any")
                class Outputs:
                    result = Output("any")
                def execute(self, inputs, params):
                    return True

            style_dict = PortSideNode._node_descriptor["style"]
            assert "inputPortSide" in style_dict

    @requires_da_py_workflow
    def test_style_composite_all_fields(self):
        """验证组合样式（所有字段非默认值）完整序列化"""
        import da_py_workflow

        style = da_py_workflow.DANodeStyle()
        style.bodyShape = da_py_workflow.BodyShape.Ellipse
        style.namePosition = da_py_workflow.NamePosition.Below
        style.iconPosition = da_py_workflow.IconPosition.AboveText
        style.setBackgroundColor(100, 150, 200)
        style.setBorderColor(50, 50, 200)
        style.borderWidth = 2.5
        style.cornerRadius = 10.0
        style.iconSize = 32.0
        style.inputPortSide = da_py_workflow.AspectDirection.North
        style.outputPortSide = da_py_workflow.AspectDirection.South
        style.inputPortStyle.shape = da_py_workflow.PortShape.Circle
        style.outputPortStyle.shape = da_py_workflow.PortShape.Diamond
        style.inputPortStyle.setFillColor(255, 255, 200)
        style.outputPortStyle.setFillColor(200, 200, 255)
        style.layoutStrategy = da_py_workflow.LinkPointLayoutStrategy.Auto
        style.bodyIconType = da_py_workflow.BodyIconType.Pixmap
        style.bodyIconSource = ":/test/icon.png"
        style.bodyIconScale = 0.6

        @NodeDef(name="Full Style Test", category="Test/Composite", style=style)
        class FullStyleNode:
            class Inputs:
                data = Input("any")
            class Outputs:
                result = Output("any")
            def execute(self, inputs, params):
                return True

        style_dict = FullStyleNode._node_descriptor["style"]
        # 所有非默认字段都应出现在序列化结果中
        expected_keys = [
            "bodyShape", "namePosition", "iconPosition",
            "backgroundColor", "borderColor", "borderWidth",
            "cornerRadius", "iconSize", "inputPortSide", "outputPortSide",
            "inputPortStyle", "outputPortStyle", "layoutStrategy",
            "bodyIconType", "bodyIconSource", "bodyIconScale",
        ]
        for key in expected_keys:
            assert key in style_dict, f"Composite style should contain '{key}'"


# ==================== render_style_nodes 发现测试 ====================

class TestRenderStyleNodesDiscovery:
    """render_style_nodes 模块节点发现测试"""

    @requires_da_py_workflow
    def test_discover_render_style_nodes(self):
        """DANodeRegistry 能发现 render_style_nodes 模块中的节点"""
        # scan_nodes 目录
        test_nodes_dir = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "test_nodes",
        )

        registry = DANodeRegistry()
        discovered = registry.discover(scan_paths=[test_nodes_dir])

        # 应发现 render_style_nodes.py 和 sample_nodes.py 中的所有节点
        # render_style_nodes 有 19 个节点 + sample_nodes 有 7 个 = 26
        assert len(discovered) >= 7, "Should discover at least sample_nodes"

        names = [d.name for d in discovered]
        # 验证关键渲染节点被发现
        assert "Test Ellipse Body" in names, "TestEllipseNode should be discovered"
        assert "Test Name Below" in names, "TestNameBelowNode should be discovered"
        assert "Test Circle Ports" in names, "TestCirclePortsNode should be discovered"
        assert "Test BackgroundColor" in names, "TestBackgroundColorNode should be discovered"
        assert "Test Full Style" in names, "TestFullStyleNode should be discovered"

    @requires_da_py_workflow
    def test_render_style_nodes_descriptor_has_style(self):
        """render_style_nodes 中所有带 style 的节点描述符包含 style 键"""
        test_nodes_dir = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "test_nodes",
        )

        registry = DANodeRegistry()
        discovered = registry.discover(scan_paths=[test_nodes_dir])

        # 查找渲染样式节点（category 包含 "Render/"）
        render_nodes = [d for d in discovered if "Render" in d.category]
        assert len(render_nodes) >= 10, "Should discover at least 10 render style nodes"

        for desc in render_nodes:
            d = desc.to_dict()
            # 所有 Render/ 分类节点应有 style 键
            assert "style" in d, f"Node '{desc.name}' should have 'style' in descriptor"


# ==================== pytest.ini marker 注册 ====================

# 注册 render_style marker（用于标记需要 C++ 运行时的测试）
pytest_render_style = pytest.mark.render_style