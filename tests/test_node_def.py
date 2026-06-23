"""
test_node_def — NodeDef 装饰器、Input/Output/Parameter 类型、DAWorkflowNode 测试

覆盖：装饰器创建类属性、Input/Output/Parameter to_parameter_descriptor/to_port_descriptor、
NodeDisplay 显式属性字段、缺失字段、边界条件。
"""

import pytest
import da_py_workflow
from DAWorkbench.DAWorkFlowPy import (
    NodeDef,
    Input,
    Output,
    Parameter,
    NodeDisplay,
    DAWorkflowNode,
    LinkPointStyle,
)

# ==================== Input 测试 ====================


class TestInput:
    """Input 端口声明类型测试"""

    def test_input_creation(self):
        inp = Input("DataFrame", required=True, description="输入数据")
        assert inp.data_type == "DataFrame"
        assert inp.required is True
        assert inp.description == "输入数据"

    def test_input_default_required(self):
        inp = Input("int")
        assert inp.required is True

    def test_input_optional(self):
        inp = Input("dict", required=False, description="可选配置")
        assert inp.required is False

    def test_input_to_port_descriptor(self):
        inp = Input("DataFrame", required=True, description="数据输入")
        pd = inp.to_port_descriptor("data")
        assert isinstance(pd, da_py_workflow.DAPortDescriptor)
        assert pd.name == "data"

    def test_input_repr(self):
        inp = Input("DataFrame", required=True, description="数据")
        r = repr(inp)
        assert "Input" in r
        assert "DataFrame" in r


# ==================== Output 测试 ====================


class TestOutput:
    """Output 端口声明类型测试"""

    def test_output_creation(self):
        out = Output("DataFrame", description="输出数据")
        assert out.data_type == "DataFrame"
        assert out.description == "输出数据"

    def test_output_default_description(self):
        out = Output("int")
        assert out.description == ""

    def test_output_to_port_descriptor(self):
        out = Output("DataFrame", description="筛选结果")
        pd = out.to_port_descriptor("filtered")
        assert isinstance(pd, da_py_workflow.DAPortDescriptor)
        assert pd.name == "filtered"

    def test_output_repr(self):
        out = Output("DataFrame", description="结果")
        r = repr(out)
        assert "Output" in r
        assert "DataFrame" in r


# ==================== Parameter 测试 ====================


class TestParameter:
    """Parameter 参数声明类型测试"""

    def test_parameter_creation(self):
        p = Parameter(str, default="value", description="列名")
        assert p.param_type is str
        assert p.default == "value"
        assert p.description == "列名"

    def test_parameter_default_none(self):
        p = Parameter(int, description="阈值")
        assert p.default is None

    def test_parameter_type_labels(self):
        assert Parameter(str).get_type_label() == "str"
        assert Parameter(int).get_type_label() == "int"
        assert Parameter(float).get_type_label() == "float"
        assert Parameter(bool).get_type_label() == "bool"
        assert Parameter(list).get_type_label() == "list"
        assert Parameter(dict).get_type_label() == "dict"

    def test_parameter_unknown_type_label(self):
        class CustomType:
            pass

        p = Parameter(CustomType)
        assert p.get_type_label() == "CustomType"

    def test_parameter_to_parameter_descriptor_with_default(self):
        p = Parameter(float, default=0.5, description="阈值")
        pd = p.to_parameter_descriptor("threshold")
        assert isinstance(pd, da_py_workflow.DAParameterDescriptor)
        assert pd.name == "threshold"

    def test_parameter_to_parameter_descriptor_no_default(self):
        p = Parameter(int, description="计数")
        pd = p.to_parameter_descriptor("count")
        assert isinstance(pd, da_py_workflow.DAParameterDescriptor)
        assert pd.name == "count"

    def test_parameter_repr(self):
        p = Parameter(str, default="val")
        r = repr(p)
        assert "Parameter" in r
        assert "str" in r


# ==================== NodeDef 装饰器测试 ====================


class TestNodeDef:
    """NodeDef 装饰器测试"""

    def test_node_def_sets_class_attributes(self):
        @NodeDef(name="Test Node", category="Test")
        class TestNode:
            def execute(self, inputs, params):
                pass

        assert TestNode.name == "Test Node"
        assert TestNode.category == "Test"

    def test_node_def_sets_qualified_name(self):
        @NodeDef(name="QN Test")
        class QNTestNode:
            pass

        assert "QNTestNode" in QNTestNode.qualified_name

    def test_node_def_collects_inputs(self):
        @NodeDef(name="Input Test")
        class InputTestNode:
            class Inputs:
                data = Input("DataFrame", required=True)
                config = Input("dict", required=False)

            def execute(self, inputs, params):
                pass

        inputs = InputTestNode.inputs
        assert len(inputs) == 2
        names = [inp.name for inp in inputs]
        assert "data" in names
        assert "config" in names

    def test_node_def_collects_outputs(self):
        @NodeDef(name="Output Test")
        class OutputTestNode:
            class Outputs:
                result = Output("DataFrame", description="结果")

            def execute(self, inputs, params):
                pass

        outputs = OutputTestNode.outputs
        assert len(outputs) == 1
        assert outputs[0].name == "result"

    def test_node_def_collects_parameters(self):
        @NodeDef(name="Param Test")
        class ParamTestNode:
            threshold = Parameter(float, default=0.5, description="阈值")
            column = Parameter(str, default="value", description="列名")

            def execute(self, inputs, params):
                pass

        params = ParamTestNode.parameters
        assert len(params) == 2
        names = [p.name for p in params]
        assert "threshold" in names
        assert "column" in names

    def test_node_def_render_template_default(self):
        @NodeDef(name="RT Default")
        class RTDefaultNode:
            pass

        assert (
            RTDefaultNode._node_display.render_template
            == da_py_workflow.RenderTemplate.NodeStyleTemplate
        )

    def test_node_def_render_template_svg(self):
        @NodeDef(name="RT SVG", render_template="svg")
        class RTSVGNode:
            pass

        assert (
            RTSVGNode._node_display.render_template
            == da_py_workflow.RenderTemplate.NodeStyleTemplate
        )

    def test_node_def_render_template_widget(self):
        @NodeDef(name="RT Widget", render_template="widget")
        class RTWidgetNode:
            pass

        assert (
            RTWidgetNode._node_display.render_template
            == da_py_workflow.RenderTemplate.WidgetTemplate
        )

    def test_node_def_no_inputs_outputs(self):
        @NodeDef(name="Empty IO")
        class EmptyIONode:
            def execute(self, inputs, params):
                pass

        assert EmptyIONode.inputs == []
        assert EmptyIONode.outputs == []

    def test_node_def_inherits_DAWorkflow_node(self):
        @NodeDef(name="Base Test")
        class BaseTestNode:
            pass

        assert isinstance(BaseTestNode(), DAWorkflowNode)

    def test_node_def_creates_node_display(self):
        @NodeDef(name="Display Test", icon=":icons/test.png")
        class DisplayTestNode:
            pass

        assert hasattr(DisplayTestNode, "_node_display")
        display = DisplayTestNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.icon == ":icons/test.png"

    def test_node_def_with_display_style(self):
        display_style = NodeDisplay(
            body_shape="Ellipse",
            background_color="#4A90D9",
            corner_radius=8.0,
        )

        @NodeDef(name="Style Display Test", style=display_style)
        class StyleDisplayNode:
            pass

        d = StyleDisplayNode._node_display
        assert isinstance(d, NodeDisplay)
        assert d.body_shape == "Ellipse"
        assert d.background_color == "#4A90D9"
        assert d.corner_radius == 8.0

    def test_node_def_with_dict_style_backward_compat(self):
        @NodeDef(
            name="Dict Compat Test",
            style={"body_shape": "Ellipse", "corner_radius": 10.0},
        )
        class DictCompatNode:
            pass

        display = DictCompatNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.body_shape == "Ellipse"
        assert display.corner_radius == 10.0

    def test_node_def_no_style(self):
        @NodeDef(name="No Style Test")
        class NoStyleTestNode:
            pass

        d = NoStyleTestNode._node_display
        assert d.body_shape is None
        assert d.background_color is None

    def test_node_def_sets_input_keys(self):
        @NodeDef(name="Input Keys Test")
        class InputKeysTestNode:
            class Inputs:
                data = Input("DataFrame", required=True)

            def execute(self, inputs, params):
                pass

        assert InputKeysTestNode.input_keys == ["data"]

    def test_node_def_sets_output_keys(self):
        @NodeDef(name="Output Keys Test")
        class OutputKeysTestNode:
            class Outputs:
                result = Output("DataFrame")

            def execute(self, inputs, params):
                pass

        assert OutputKeysTestNode.output_keys == ["result"]


# ==================== NodeDisplay 测试 ====================


class TestNodeDisplay:
    """NodeDisplay 渲染属性聚合测试 — 所有样式字段为显式属性"""

    def test_node_display_creation(self):
        display = NodeDisplay(icon=":icons/test.png")
        assert display.icon == ":icons/test.png"
        assert display.render_template == "nodestyle"
        assert display.body_shape is None
        assert display.background_color is None
        assert display.corner_radius is None

    def test_node_display_with_render_template(self):
        rt = da_py_workflow.RenderTemplate.NodeStyleTemplate
        display = NodeDisplay(render_template=rt)
        assert (
            display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate
        )

    def test_node_display_with_style_fields(self):
        display = NodeDisplay(
            body_shape="Ellipse",
            background_color="#4A90D9",
            corner_radius=8.0,
        )
        assert display.body_shape == "Ellipse"
        assert display.background_color == "#4A90D9"
        assert display.corner_radius == 8.0

    def test_node_display_repr(self):
        display = NodeDisplay(icon="test.png")
        r = repr(display)
        assert "NodeDisplay" in r

    def test_node_display_rgb_tuple_color(self):
        display = NodeDisplay(background_color=(240, 240, 240))
        assert display.background_color == (240, 240, 240)

    def test_node_display_with_port_style(self):
        ips = LinkPointStyle(shape="Circle", fill_color="#ffffff")
        ops = LinkPointStyle(shape="Diamond", border_color="#000000")
        display = NodeDisplay(
            input_port_style=ips,
            output_port_style=ops,
        )
        assert display.input_port_style.shape == "Circle"
        assert display.output_port_style.shape == "Diamond"


# ==================== DAWorkflowNode 测试 ====================


class TestDAWorkflowNode:
    """DAWorkflowNode 基类测试"""

    def test_workflow_node_init(self):
        node = DAWorkflowNode()
        assert node.node_id is None
        assert node._input_data == {}
        assert node._output_data == {}
        assert node.is_global is False

    def test_workflow_node_set_input_data(self):
        node = DAWorkflowNode()
        node.set_input_data("data", [1, 2, 3])
        assert node._input_data["data"] == [1, 2, 3]

    def test_workflow_node_get_output_data(self):
        node = DAWorkflowNode()
        node._output_data["result"] = [4, 5, 6]
        assert node.get_output_data("result") == [4, 5, 6]
        assert node.get_output_data("nonexistent") is None


# ==================== LinkPointStyle 测试 ====================


class TestLinkPointStyle:
    """LinkPointStyle 连接点样式 dataclass 测试"""

    def test_link_point_style_default_all_none(self):
        lps = LinkPointStyle()
        assert lps.shape is None
        assert lps.fill_color is None
        assert lps.border_color is None
        assert lps.border_width is None

    def test_link_point_style_creation(self):
        lps = LinkPointStyle(shape="Circle", fill_color="#ff0000", border_width=2.0)
        assert lps.shape == "Circle"
        assert lps.fill_color == "#ff0000"
        assert lps.border_width == 2.0

    def test_link_point_style_rgb_tuple_color(self):
        lps = LinkPointStyle(fill_color=(255, 200, 200))
        assert lps.fill_color == (255, 200, 200)


# ==================== NodeDef + NodeDisplay 集成测试 ====================


class TestNodeDefWithNodeDisplay:
    """NodeDef(style=NodeDisplay) 集成测试"""

    def test_node_def_with_display_style(self):
        display_style = NodeDisplay(
            body_shape="Ellipse",
            background_color="#4A90D9",
            corner_radius=8.0,
        )

        @NodeDef(name="Style Dataclass Test", style=display_style)
        class StyleDataclassNode:
            pass

        d = StyleDataclassNode._node_display
        assert isinstance(d, NodeDisplay)
        assert d.body_shape == "Ellipse"
        assert d.background_color == "#4A90D9"
        assert d.corner_radius == 8.0

    def test_node_def_with_display_style_port_styles(self):
        display_style = NodeDisplay(
            input_port_style=LinkPointStyle(shape="Circle", fill_color="#ffffff"),
            output_port_style=LinkPointStyle(shape="Diamond"),
        )

        @NodeDef(name="Port Style Test", style=display_style)
        class PortStyleNode:
            pass

        d = PortStyleNode._node_display
        assert d.input_port_style.shape == "Circle"
        assert d.input_port_style.fill_color == "#ffffff"
        assert d.output_port_style.shape == "Diamond"

    def test_node_def_with_dict_style_backward_compat(self):
        @NodeDef(
            name="Dict Compat Test",
            style={"body_shape": "Ellipse", "corner_radius": 10.0},
        )
        class DictCompatNode:
            pass

        d = DictCompatNode._node_display
        assert isinstance(d, NodeDisplay)
        assert d.body_shape == "Ellipse"
        assert d.corner_radius == 10.0

    def test_node_def_no_style(self):
        @NodeDef(name="No Style Compat Test")
        class NoStyleCompatNode:
            pass

        d = NoStyleCompatNode._node_display
        assert d.body_shape is None
        assert d.background_color is None

    def test_node_display_full_composite(self):
        display = NodeDisplay(
            body_shape="Ellipse",
            name_position="Below",
            icon_position="AboveText",
            background_color="#4A90D9",
            border_color="#3A3A5C",
            border_width=2.0,
            corner_radius=8.0,
            icon_size=32.0,
            input_port_side="North",
            output_port_side="South",
            input_port_style=LinkPointStyle(shape="Circle", fill_color="#ffffff"),
            output_port_style=LinkPointStyle(shape="Diamond", border_color="#000000"),
            layout_strategy="Manual",
            body_icon_type="Svg",
            body_icon_source=":/icons/node.svg",
            body_icon_scale=0.6,
        )
        assert display.body_shape == "Ellipse"
        assert display.name_position == "Below"
        assert display.icon_position == "AboveText"
        assert display.background_color == "#4A90D9"
        assert display.border_color == "#3A3A5C"
        assert display.border_width == 2.0
        assert display.corner_radius == 8.0
        assert display.icon_size == 32.0
        assert display.input_port_side == "North"
        assert display.output_port_side == "South"
        assert display.layout_strategy == "Manual"
        assert display.body_icon_type == "Svg"
        assert display.body_icon_source == ":/icons/node.svg"
        assert display.body_icon_scale == 0.6
        assert display.input_port_style.shape == "Circle"
        assert display.output_port_style.shape == "Diamond"
