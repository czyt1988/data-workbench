"""
test_node_def — NodeDef 装饰器、Input/Output/Parameter 类型、DANodeDescriptor 测试

覆盖：装饰器创建 descriptor、Input/Output/Parameter to_dict、序列化、
缺失字段、无效装饰器用法、边界条件。
"""

import pytest
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DAWorkbench.DAWorkFlowPy.node_descriptor import DANodeDescriptor


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

    def test_input_to_dict(self):
        """Input.to_dict 包含所有字段"""
        inp = Input("DataFrame", required=True, description="数据输入")
        d = inp.to_dict("data")
        assert d == {
            "name": "data",
            "data_type": "DataFrame",
            "required": True,
            "description": "数据输入",
        }

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

    def test_output_to_dict(self):
        """Output.to_dict 包含所有字段"""
        out = Output("DataFrame", description="筛选结果")
        d = out.to_dict("filtered")
        assert d == {
            "name": "filtered",
            "data_type": "DataFrame",
            "description": "筛选结果",
        }

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

    def test_parameter_to_dict_with_default(self):
        """Parameter.to_dict 包含 default 字段"""
        p = Parameter(float, default=0.5, description="阈值")
        d = p.to_dict("threshold")
        assert d["name"] == "threshold"
        assert d["type"] == "float"
        assert d["default"] == 0.5
        assert d["description"] == "阈值"

    def test_parameter_to_dict_no_default(self):
        """Parameter.to_dict 不包含 default 字段当 default=None"""
        p = Parameter(int, description="计数")
        d = p.to_dict("count")
        assert "default" not in d

    def test_parameter_repr(self):
        """Parameter repr 格式"""
        p = Parameter(str, default="val")
        r = repr(p)
        assert "Parameter" in r
        assert "str" in r


# ==================== NodeDef 装饰器测试 ====================

class TestNodeDef:
    """NodeDef 装饰器测试"""

    def test_node_def_creates_descriptor(self):
        """NodeDef 装饰器在类上创建 _node_descriptor"""
        @NodeDef(name="Test Node", category="Test")
        class TestNode:
            def execute(self, inputs, params):
                pass
        assert hasattr(TestNode, "_node_descriptor")
        desc = TestNode._node_descriptor
        assert desc["name"] == "Test Node"
        assert desc["category"] == "Test"

    def test_node_def_qualified_name(self):
        """NodeDef 生成 qualified_name (模块名.类名)"""
        @NodeDef(name="QN Test")
        class QNTestNode:
            pass
        desc = QNTestNode._node_descriptor
        assert "QNTestNode" in desc["qualified_name"]

    def test_node_def_collects_inputs(self):
        """NodeDef 收集 Inputs 嵌套类中的 Input 声明"""
        @NodeDef(name="Input Test")
        class InputTestNode:
            class Inputs:
                data = Input("DataFrame", required=True)
                config = Input("dict", required=False)

            def execute(self, inputs, params):
                pass
        inputs = InputTestNode._node_descriptor["inputs"]
        assert len(inputs) == 2
        names = [i["name"] for i in inputs]
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
        outputs = OutputTestNode._node_descriptor["outputs"]
        assert len(outputs) == 1
        assert outputs[0]["name"] == "result"

    def test_node_def_collects_parameters(self):
        """NodeDef 收集类属性中的 Parameter 声明"""
        @NodeDef(name="Param Test")
        class ParamTestNode:
            threshold = Parameter(float, default=0.5, description="阈值")
            column = Parameter(str, default="value", description="列名")

            def execute(self, inputs, params):
                pass
        params = ParamTestNode._node_descriptor["parameters"]
        assert len(params) == 2
        names = [p["name"] for p in params]
        assert "threshold" in names
        assert "column" in names

    def test_node_def_render_template_default(self):
        """NodeDef 默认 render_template='rect'"""
        @NodeDef(name="RT Default")
        class RTDefaultNode:
            pass
        assert RTDefaultNode._node_descriptor["render_template"] == "rect"

    def test_node_def_render_template_svg(self):
        """NodeDef 支持 render_template='svg'"""
        @NodeDef(name="RT SVG", render_template="svg")
        class RTSVGNode:
            pass
        assert RTSVGNode._node_descriptor["render_template"] == "svg"

    def test_node_def_render_template_widget(self):
        """NodeDef 支持 render_template='widget'"""
        @NodeDef(name="RT Widget", render_template="widget")
        class RTWidgetNode:
            pass
        assert RTWidgetNode._node_descriptor["render_template"] == "widget"

    def test_node_def_invalid_render_template(self):
        """NodeDef 无效 render_template 抛 ValueError"""
        with pytest.raises(ValueError, match="无效的渲染模板"):
            @NodeDef(name="Bad RT", render_template="invalid")
            class BadRTNode:
                pass

    def test_node_def_no_inputs_outputs(self):
        """NodeDef 类无 Inputs/Outputs 时列表为空"""
        @NodeDef(name="Empty IO")
        class EmptyIONode:
            def execute(self, inputs, params):
                pass
        desc = EmptyIONode._node_descriptor
        assert desc["inputs"] == []
        assert desc["outputs"] == []


# ==================== DANodeDescriptor 测试 ====================

class TestDANodeDescriptor:
    """DANodeDescriptor 描述符类测试"""

    def test_descriptor_creation(self):
        """DANodeDescriptor 正常创建"""
        desc = DANodeDescriptor(
            name="Test", category="Cat", icon="ico",
            qualified_name="mod.Test", render_template="rect",
        )
        assert desc.name == "Test"
        assert desc.category == "Cat"
        assert desc.qualified_name == "mod.Test"

    def test_descriptor_to_dict(self):
        """DANodeDescriptor.to_dict 序列化"""
        desc = DANodeDescriptor(
            name="Filter", category="Data", icon="filter",
            qualified_name="my.Filter",
            inputs=[{"name": "data", "data_type": "DataFrame",
                     "required": True, "description": ""}],
            outputs=[{"name": "filtered",
                      "data_type": "DataFrame", "description": ""}],
            parameters=[{"name": "col", "type": "str",
                         "default": "val", "description": "列名"}],
        )
        d = desc.to_dict()
        assert d["name"] == "Filter"
        assert len(d["inputs"]) == 1
        assert len(d["outputs"]) == 1
        assert len(d["parameters"]) == 1
        assert d["render_template"] == "rect"

    def test_descriptor_from_class(self):
        """DANodeDescriptor.from_class 从 @NodeDef 装饰类创建"""
        @NodeDef(name="FromClass", category="Test")
        class FromClassNode:
            threshold = Parameter(float, default=0.5)

            class Inputs:
                data = Input("DataFrame", required=True)

            class Outputs:
                result = Output("DataFrame")

            def execute(self, inputs, params):
                pass
        desc = DANodeDescriptor.from_class(FromClassNode)
        assert desc.name == "FromClass"
        assert len(desc.inputs) == 1
        assert len(desc.outputs) == 1
        assert len(desc.parameters) == 1

    def test_descriptor_from_class_override(self):
        """DANodeDescriptor.from_class 可覆盖参数"""
        @NodeDef(name="Override Test", category="Old")
        class OverrideNode:
            pass
        desc = DANodeDescriptor.from_class(OverrideNode, category="New")
        assert desc.category == "New"

    def test_descriptor_from_class_no_descriptor_raises(self):
        """DANodeDescriptor.from_class 无 _node_descriptor 抛 ValueError"""
        class PlainClass:
            pass
        with pytest.raises(ValueError, match="没有 _node_descriptor"):
            DANodeDescriptor.from_class(PlainClass)

    def test_descriptor_invalid_render_template(self):
        """DANodeDescriptor 无效 render_template 抛 ValueError"""
        with pytest.raises(ValueError, match="无效的渲染模板"):
            DANodeDescriptor(name="Bad", render_template="unknown")

    def test_descriptor_repr(self):
        """DANodeDescriptor repr 格式"""
        desc = DANodeDescriptor(
            name="Filter", category="Data", qualified_name="mod.Filter")
        r = repr(desc)
        assert "DANodeDescriptor" in r
        assert "Filter" in r
