"""
test_node_def — NodeDef 装饰器、Input/Output/Parameter 类型、DAWorkflowNode 测试

覆盖：装饰器创建类属性、Input/Output/Parameter to_parameter_descriptor/to_port_descriptor、
NodeDisplay 渲染属性聚合、缺失字段、边界条件。
"""

import pytest
import da_py_workflow
from DAWorkbench.DAWorkFlowPy import (
    NodeDef, Input, Output, Parameter, NodeDisplay, DAWorkflowNode,
    NodeDisplayStyle, LinkPointStyle,
)


# ==================== Input 测试 ====================

class TestInput:
    """Input 端口声明类型测试"""

    def test_input_creation(self):
        """Input 正常创建"""
        inp = Input("DataFrame", required=True, description="输入数据")
        assert inp.data_type == "DataFrame"
        assert inp.required is True
        assert inp.description == "输入数据"

    def test_input_default_required(self):
        """Input 默认 required=True"""
        inp = Input("int")
        assert inp.required is True

    def test_input_optional(self):
        """Input 可选端口 required=False"""
        inp = Input("dict", required=False, description="可选配置")
        assert inp.required is False

    def test_input_to_port_descriptor(self):
        """Input.to_port_descriptor 返回 DAPortDescriptor"""
        inp = Input("DataFrame", required=True, description="数据输入")
        pd = inp.to_port_descriptor("data")
        assert isinstance(pd, da_py_workflow.DAPortDescriptor)
        assert pd.name == "data"

    def test_input_repr(self):
        """Input repr 格式"""
        inp = Input("DataFrame", required=True, description="数据")
        r = repr(inp)
        assert "Input" in r
        assert "DataFrame" in r


# ==================== Output 测试 ====================

class TestOutput:
    """Output 端口声明类型测试"""

    def test_output_creation(self):
        """Output 正常创建"""
        out = Output("DataFrame", description="输出数据")
        assert out.data_type == "DataFrame"
        assert out.description == "输出数据"

    def test_output_default_description(self):
        """Output 默认 description 为空"""
        out = Output("int")
        assert out.description == ""

    def test_output_to_port_descriptor(self):
        """Output.to_port_descriptor 返回 DAPortDescriptor"""
        out = Output("DataFrame", description="筛选结果")
        pd = out.to_port_descriptor("filtered")
        assert isinstance(pd, da_py_workflow.DAPortDescriptor)
        assert pd.name == "filtered"

    def test_output_repr(self):
        """Output repr 格式"""
        out = Output("DataFrame", description="结果")
        r = repr(out)
        assert "Output" in r
        assert "DataFrame" in r


# ==================== Parameter 测试 ====================

class TestParameter:
    """Parameter 参数声明类型测试"""

    def test_parameter_creation(self):
        """Parameter 正常创建"""
        p = Parameter(str, default="value", description="列名")
        assert p.param_type is str
        assert p.default == "value"
        assert p.description == "列名"

    def test_parameter_default_none(self):
        """Parameter 默认 default=None"""
        p = Parameter(int, description="阈值")
        assert p.default is None

    def test_parameter_type_labels(self):
        """Parameter.get_type_label 返回已知类型标签"""
        assert Parameter(str).get_type_label() == "str"
        assert Parameter(int).get_type_label() == "int"
        assert Parameter(float).get_type_label() == "float"
        assert Parameter(bool).get_type_label() == "bool"
        assert Parameter(list).get_type_label() == "list"
        assert Parameter(dict).get_type_label() == "dict"

    def test_parameter_unknown_type_label(self):
        """Parameter.get_type_label 对未知类型返回 __name__"""
        class CustomType:
            pass
        p = Parameter(CustomType)
        assert p.get_type_label() == "CustomType"

    def test_parameter_to_parameter_descriptor_with_default(self):
        """Parameter.to_parameter_descriptor 包含 default 字段"""
        p = Parameter(float, default=0.5, description="阈值")
        pd = p.to_parameter_descriptor("threshold")
        assert isinstance(pd, da_py_workflow.DAParameterDescriptor)
        assert pd.name == "threshold"

    def test_parameter_to_parameter_descriptor_no_default(self):
        """Parameter.to_parameter_descriptor 无 default 时 default 为空"""
        p = Parameter(int, description="计数")
        pd = p.to_parameter_descriptor("count")
        assert isinstance(pd, da_py_workflow.DAParameterDescriptor)
        assert pd.name == "count"

    def test_parameter_repr(self):
        """Parameter repr 格式"""
        p = Parameter(str, default="val")
        r = repr(p)
        assert "Parameter" in r
        assert "str" in r


# ==================== NodeDef 装饰器测试 ====================

class TestNodeDef:
    """NodeDef 装饰器测试"""

    def test_node_def_sets_class_attributes(self):
        """NodeDef 装饰器在类上设置 name/category 等类属性"""
        @NodeDef(name="Test Node", category="Test")
        class TestNode:
            def execute(self, inputs, params):
                pass
        assert TestNode.name == "Test Node"
        assert TestNode.category == "Test"

    def test_node_def_sets_qualified_name(self):
        """NodeDef 生成 qualified_name (模块名.类名)"""
        @NodeDef(name="QN Test")
        class QNTestNode:
            pass
        assert "QNTestNode" in QNTestNode.qualified_name

    def test_node_def_collects_inputs(self):
        """NodeDef 收集 Inputs 嵌套类中的 Input 声明"""
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
        """NodeDef 收集 Outputs 嵌套类中的 Output 声明"""
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
        """NodeDef 收集类属性中的 Parameter 声明"""
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
        """NodeDef 默认 render_template 为 NodeStyleTemplate"""
        @NodeDef(name="RT Default")
        class RTDefaultNode:
            pass
        assert RTDefaultNode._node_display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate

    def test_node_def_render_template_svg(self):
        """NodeDef 支持 render_template='svg'（映射到 NodeStyleTemplate）"""
        @NodeDef(name="RT SVG", render_template="svg")
        class RTSVGNode:
            pass
        assert RTSVGNode._node_display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate

    def test_node_def_render_template_widget(self):
        """NodeDef 支持 render_template='widget'"""
        @NodeDef(name="RT Widget", render_template="widget")
        class RTWidgetNode:
            pass
        assert RTWidgetNode._node_display.render_template == da_py_workflow.RenderTemplate.WidgetTemplate

    def test_node_def_no_inputs_outputs(self):
        """NodeDef 类无 Inputs/Outputs 时列表为空"""
        @NodeDef(name="Empty IO")
        class EmptyIONode:
            def execute(self, inputs, params):
                pass
        assert EmptyIONode.inputs == []
        assert EmptyIONode.outputs == []

    def test_node_def_inherits_DAWorkflowNode(self):
        """NodeDef 装饰的类继承 DAWorkflowNode"""
        @NodeDef(name="Base Test")
        class BaseTestNode:
            pass
        assert isinstance(BaseTestNode(), DAWorkflowNode)

    def test_node_def_creates_node_display(self):
        """NodeDef 装饰器在类上创建 _node_display"""
        @NodeDef(name="Display Test", icon=":icons/test.png")
        class DisplayTestNode:
            pass
        assert hasattr(DisplayTestNode, "_node_display")
        display = DisplayTestNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.icon == ":icons/test.png"

    def test_node_def_node_display_with_style_dict(self):
        """NodeDef(style=dict) 在 _node_display 中保持 dict 格式"""
        @NodeDef(name="Style Dict Test", style={"background_color": "#ffffff"})
        class StyleDictTestNode:
            pass
        assert StyleDictTestNode._node_display.style is not None
        assert isinstance(StyleDictTestNode._node_display.style, dict)

    def test_node_def_node_display_with_style_object(self):
        """NodeDef(style=DANodeStyle) 在 _node_display 中设置 DANodeStyle"""
        node_style = da_py_workflow.DANodeStyle()
        @NodeDef(name="Style Obj Test", style=node_style)
        class StyleObjTestNode:
            pass
        assert StyleObjTestNode._node_display.style is not None
        assert isinstance(StyleObjTestNode._node_display.style, da_py_workflow.DANodeStyle)

    def test_node_def_node_display_no_style(self):
        """NodeDef 无 style 参数时 _node_display.style 为 None"""
        @NodeDef(name="No Style Test")
        class NoStyleTestNode:
            pass
        assert NoStyleTestNode._node_display.style is None

    def test_node_def_sets_input_keys(self):
        """NodeDef 设置 input_keys 列表"""
        @NodeDef(name="Input Keys Test")
        class InputKeysTestNode:
            class Inputs:
                data = Input("DataFrame", required=True)

            def execute(self, inputs, params):
                pass
        assert InputKeysTestNode.input_keys == ["data"]

    def test_node_def_sets_output_keys(self):
        """NodeDef 设置 output_keys 列表"""
        @NodeDef(name="Output Keys Test")
        class OutputKeysTestNode:
            class Outputs:
                result = Output("DataFrame")

            def execute(self, inputs, params):
                pass
        assert OutputKeysTestNode.output_keys == ["result"]

# ==================== NodeDisplay 测试 ====================

class TestNodeDisplay:
    """NodeDisplay 渲染属性聚合测试"""

    def test_node_display_creation(self):
        """NodeDisplay 正常创建"""
        display = NodeDisplay(icon=":icons/test.png")
        assert display.icon == ":icons/test.png"
        assert display.render_template == "nodestyle"
        assert display.style is None

    def test_node_display_with_render_template(self):
        """NodeDisplay 设置 render_template"""
        rt = da_py_workflow.RenderTemplate.NodeStyleTemplate
        display = NodeDisplay(render_template=rt)
        assert display.render_template == da_py_workflow.RenderTemplate.NodeStyleTemplate

    def test_node_display_with_style(self):
        """NodeDisplay 设置 style"""
        style = da_py_workflow.DANodeStyle()
        display = NodeDisplay(style=style)
        assert display.style is not None
        assert isinstance(display.style, da_py_workflow.DANodeStyle)

    def test_node_display_repr(self):
        """NodeDisplay dataclass repr"""
        display = NodeDisplay(icon="test.png")
        r = repr(display)
        assert "NodeDisplay" in r


# ==================== DAWorkflowNode 测试 ====================

class TestDAWorkflowNode:
    """DAWorkflowNode 基类测试"""

    def test_workflow_node_init(self):
        """DAWorkflowNode 初始化"""
        node = DAWorkflowNode()
        assert node.node_id is None
        assert node._input_data == {}
        assert node._output_data == {}
        assert node.is_global is False

    def test_workflow_node_set_input_data(self):
        """DAWorkflowNode.set_input_data 设置输入数据"""
        node = DAWorkflowNode()
        node.set_input_data("data", [1, 2, 3])
        assert node._input_data["data"] == [1, 2, 3]

    def test_workflow_node_get_output_data(self):
        """DAWorkflowNode.get_output_data 获取输出数据"""
        node = DAWorkflowNode()
        node._output_data["result"] = [4, 5, 6]
        assert node.get_output_data("result") == [4, 5, 6]
        assert node.get_output_data("nonexistent") is None


# ==================== LinkPointStyle 测试 ====================

class TestLinkPointStyle:
    """LinkPointStyle 连接点样式 dataclass 测试"""

    def test_link_point_style_default_all_none(self):
        """LinkPointStyle 默认所有字段为 None"""
        lps = LinkPointStyle()
        assert lps.shape is None
        assert lps.fill_color is None
        assert lps.border_color is None
        assert lps.border_width is None

    def test_link_point_style_creation(self):
        """LinkPointStyle 正常创建"""
        lps = LinkPointStyle(shape="Circle", fill_color="#ff0000", border_width=2.0)
        assert lps.shape == "Circle"
        assert lps.fill_color == "#ff0000"
        assert lps.border_width == 2.0

    def test_link_point_style_to_dict_skips_none(self):
        """LinkPointStyle.to_dict() 仅包含非 None 字段"""
        lps = LinkPointStyle(shape="Circle")
        d = lps.to_dict()
        assert d == {"shape": "Circle"}

    def test_link_point_style_to_dict_all_fields(self):
        """LinkPointStyle.to_dict() 包含所有非 None 字段"""
        lps = LinkPointStyle(shape="Diamond", fill_color="#ffffff", border_color="#000000", border_width=3.0)
        d = lps.to_dict()
        assert d == {
            "shape": "Diamond",
            "fill_color": "#ffffff",
            "border_color": "#000000",
            "border_width": 3.0,
        }

    def test_link_point_style_to_dict_empty(self):
        """全默认 LinkPointStyle.to_dict() 返回空 dict"""
        lps = LinkPointStyle()
        assert lps.to_dict() == {}

    def test_link_point_style_rgb_tuple_color(self):
        """LinkPointStyle 支持 RGB 元组颜色"""
        lps = LinkPointStyle(fill_color=(255, 200, 200))
        d = lps.to_dict()
        assert d["fill_color"] == (255, 200, 200)


# ==================== NodeDisplayStyle 测试 ====================

class TestNodeDisplayStyle:
    """NodeDisplayStyle 节点样式 dataclass 测试"""

    def test_node_display_style_default_all_none(self):
        """NodeDisplayStyle 默认所有字段为 None"""
        nds = NodeDisplayStyle()
        assert nds.body_shape is None
        assert nds.background_color is None
        assert nds.corner_radius is None
        assert nds.input_port_style is None

    def test_node_display_style_creation(self):
        """NodeDisplayStyle 正常创建"""
        nds = NodeDisplayStyle(
            body_shape="Ellipse",
            background_color="#4A90D9",
            corner_radius=8.0,
        )
        assert nds.body_shape == "Ellipse"
        assert nds.background_color == "#4A90D9"
        assert nds.corner_radius == 8.0

    def test_node_display_style_to_dict_sparse(self):
        """NodeDisplayStyle.to_dict() 仅包含非 None 字段"""
        nds = NodeDisplayStyle(body_shape="Ellipse", corner_radius=10.0)
        d = nds.to_dict()
        assert d == {"body_shape": "Ellipse", "corner_radius": 10.0}

    def test_node_display_style_to_dict_with_port_style(self):
        """NodeDisplayStyle.to_dict() 递归转换 LinkPointStyle"""
        nds = NodeDisplayStyle(
            body_shape="Ellipse",
            input_port_style=LinkPointStyle(shape="Circle", fill_color="#ffffff"),
        )
        d = nds.to_dict()
        assert d["body_shape"] == "Ellipse"
        assert d["input_port_style"] == {"shape": "Circle", "fill_color": "#ffffff"}

    def test_node_display_style_to_dict_all_body_fields(self):
        """NodeDisplayStyle.to_dict() 包含所有主体样式字段"""
        nds = NodeDisplayStyle(
            body_shape="Ellipse",
            name_position="Below",
            icon_position="AboveText",
            background_color="#f0f0f0",
            border_color="#b4b4b4",
            border_width=2.0,
            corner_radius=8.0,
            icon_size=32.0,
        )
        d = nds.to_dict()
        assert len(d) == 8
        assert d["body_shape"] == "Ellipse"
        assert d["name_position"] == "Below"
        assert d["border_width"] == 2.0

    def test_node_display_style_to_dict_port_fields(self):
        """NodeDisplayStyle.to_dict() 包含端口配置字段"""
        nds = NodeDisplayStyle(
            input_port_side="North",
            output_port_side="South",
            layout_strategy="Manual",
        )
        d = nds.to_dict()
        assert d["input_port_side"] == "North"
        assert d["output_port_side"] == "South"
        assert d["layout_strategy"] == "Manual"

    def test_node_display_style_to_dict_body_icon_fields(self):
        """NodeDisplayStyle.to_dict() 包含节点体图标字段"""
        nds = NodeDisplayStyle(
            body_icon_type="Svg",
            body_icon_source=":/icons/node.svg",
            body_icon_scale=0.6,
        )
        d = nds.to_dict()
        assert d["body_icon_type"] == "Svg"
        assert d["body_icon_source"] == ":/icons/node.svg"
        assert d["body_icon_scale"] == 0.6

    def test_node_display_style_to_dict_empty(self):
        """全默认 NodeDisplayStyle.to_dict() 返回空 dict"""
        nds = NodeDisplayStyle()
        assert nds.to_dict() == {}

    def test_node_display_style_rgb_tuple_color(self):
        """NodeDisplayStyle 支持 RGB 元组颜色"""
        nds = NodeDisplayStyle(background_color=(240, 240, 240))
        d = nds.to_dict()
        assert d["background_color"] == (240, 240, 240)

    def test_node_display_style_repr(self):
        """NodeDisplayStyle dataclass repr"""
        nds = NodeDisplayStyle(body_shape="Ellipse")
        r = repr(nds)
        assert "NodeDisplayStyle" in r
        assert "Ellipse" in r


# ==================== NodeDef + NodeDisplayStyle 集成测试 ====================

class TestNodeDefWithNodeDisplayStyle:
    """NodeDef(style=NodeDisplayStyle) 集成测试"""

    def test_node_def_with_node_display_style(self):
        """NodeDef(style=NodeDisplayStyle) 将样式转为 dict 存入 _node_display"""
        nds = NodeDisplayStyle(
            body_shape="Ellipse",
            background_color="#4A90D9",
            corner_radius=8.0,
        )
        @NodeDef(name="Style Dataclass Test", style=nds)
        class StyleDataclassNode:
            pass
        display = StyleDataclassNode._node_display
        assert isinstance(display, NodeDisplay)
        assert display.style is not None
        # NodeDisplayStyle 被转为 dict
        assert isinstance(display.style, dict)
        assert display.style["body_shape"] == "Ellipse"
        assert display.style["background_color"] == "#4A90D9"
        assert display.style["corner_radius"] == 8.0

    def test_node_def_with_node_display_style_port_styles(self):
        """NodeDef(style=NodeDisplayStyle) 包含端口样式"""
        nds = NodeDisplayStyle(
            input_port_style=LinkPointStyle(shape="Circle", fill_color="#ffffff"),
            output_port_style=LinkPointStyle(shape="Diamond"),
        )
        @NodeDef(name="Port Style Test", style=nds)
        class PortStyleNode:
            pass
        style_dict = PortStyleNode._node_display.style
        assert style_dict["input_port_style"] == {"shape": "Circle", "fill_color": "#ffffff"}
        assert style_dict["output_port_style"] == {"shape": "Diamond"}

    def test_node_def_with_dict_style_backward_compat(self):
        """NodeDef(style=dict) 向后兼容"""
        @NodeDef(name="Dict Compat Test", style={"body_shape": "Ellipse", "corner_radius": 10.0})
        class DictCompatNode:
            pass
        display = DictCompatNode._node_display
        assert isinstance(display.style, dict)
        assert display.style["body_shape"] == "Ellipse"

    def test_node_def_with_da_py_workflow_style_backward_compat(self):
        """NodeDef(style=da_py_workflow.DANodeStyle) 向后兼容"""
        node_style = da_py_workflow.DANodeStyle()
        @NodeDef(name="DANodeStyle Compat Test", style=node_style)
        class DANodeStyleCompatNode:
            pass
        display = DANodeStyleCompatNode._node_display
        assert display.style is not None
        assert isinstance(display.style, da_py_workflow.DANodeStyle)

    def test_node_def_no_style(self):
        """NodeDef 无 style 参数时 _node_display.style 为 None"""
        @NodeDef(name="No Style Compat Test")
        class NoStyleCompatNode:
            pass
        assert NoStyleCompatNode._node_display.style is None

    def test_node_display_style_full_composite(self):
        """NodeDisplayStyle 所有字段组合"""
        nds = NodeDisplayStyle(
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
        d = nds.to_dict()
        expected_keys = [
            "body_shape", "name_position", "icon_position",
            "background_color", "border_color", "border_width",
            "corner_radius", "icon_size", "input_port_side", "output_port_side",
            "input_port_style", "output_port_style", "layout_strategy",
            "body_icon_type", "body_icon_source", "body_icon_scale",
        ]
        for key in expected_keys:
            assert key in d, f"Composite style dict should contain '{key}'"
