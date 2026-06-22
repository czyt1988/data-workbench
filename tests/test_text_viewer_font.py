"""
test_text_viewer_font — TextViewerNode 字体参数字典化测试

验证 text_viewer.py 将 font_color/font_size/bold/italic 四个参数
合并为单个 font 字典参数后的声明正确性。

测试覆盖：
- font 参数存在且 type label 为 "font"
- font 默认值是包含 family/size/bold/italic/color 五键的字典
- 旧的 font_color/font_size/bold/italic 参数不再存在
- max_text_length 和 wrap_text 参数保留不变
- execute() 正常执行
- serialize/deserialize_runtime_state 正常工作
"""

import os
import sys

import pytest

# 将 DASystemNodes 插件路径加入 sys.path
_PLUGIN_PYSCRIPTS = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "plugins", "DASystemNodes", "PyScripts",
)
if _PLUGIN_PYSCRIPTS not in sys.path:
    sys.path.insert(0, _PLUGIN_PYSCRIPTS)

# 尝试导入，如果 C++ 扩展模块不可用则 skip 整个模块
try:
    from DASystemNodes.nodes.text_viewer import TextViewerNode, _DEFAULT_FONT, _hex_to_rgb
except ImportError:
    pytest.skip("DAWorkbench C++ 扩展模块不可用，跳过 text_viewer 测试", allow_module_level=True)


class TestTextViewerFontParameter:
    """TextViewerNode 字体参数声明测试"""

    def test_font_parameter_exists(self):
        """font 参数存在于 parameters 字典中"""
        assert "font" in TextViewerNode.parameters

    def test_font_parameter_type_label(self):
        """font 参数的 type label 为 'font'"""
        param = TextViewerNode.parameters["font"]
        assert param.get_type_label() == "font"

    def test_font_parameter_default_is_dict(self):
        """font 参数的默认值是字典"""
        param = TextViewerNode.parameters["font"]
        assert isinstance(param.default, dict)

    def test_font_parameter_default_has_all_keys(self):
        """font 默认值包含 family/size/bold/italic/color 五个键"""
        default = TextViewerNode.parameters["font"].default
        expected_keys = {"family", "size", "bold", "italic", "color"}
        assert set(default.keys()) == expected_keys

    def test_font_parameter_default_values(self):
        """font 默认值的字段值正确"""
        default = TextViewerNode.parameters["font"].default
        assert default["family"] == "Microsoft YaHei"
        assert default["size"] == 9
        assert default["bold"] is False
        assert default["italic"] is False
        assert default["color"] == "#282828"

    def test_font_parameter_has_description(self):
        """font 参数有描述信息"""
        param = TextViewerNode.parameters["font"]
        assert param.description != ""

    def test_old_font_parameters_removed(self):
        """旧的 font_color/font_size/bold/italic 参数不再存在"""
        params = TextViewerNode.parameters
        assert "font_color" not in params
        assert "font_size" not in params
        assert "bold" not in params
        assert "italic" not in params

    def test_max_text_length_parameter_preserved(self):
        """max_text_length 参数保留"""
        assert "max_text_length" in TextViewerNode.parameters

    def test_wrap_text_parameter_preserved(self):
        """wrap_text 参数保留"""
        assert "wrap_text" in TextViewerNode.parameters

    def test_parameter_count(self):
        """参数数量为 3（font + max_text_length + wrap_text）"""
        assert len(TextViewerNode.parameters) == 3


class TestTextViewerExecute:
    """TextViewerNode execute 行为测试"""

    def test_execute_with_string_input(self):
        """execute 接收字符串输入正常"""
        node = TextViewerNode()
        node.execute(inputs={"value": "hello world"}, params=None)
        assert node._display_text == "hello world"

    def test_execute_with_none_input(self):
        """execute 接收 None 输入时显示空字符串"""
        node = TextViewerNode()
        node.execute(inputs={"value": None}, params=None)
        assert node._display_text == ""

    def test_execute_with_int_input(self):
        """execute 接收整数输入时转为字符串"""
        node = TextViewerNode()
        node.execute(inputs={"value": 42}, params=None)
        assert node._display_text == "42"

    def test_execute_with_empty_inputs(self):
        """execute 接收空 inputs 字典"""
        node = TextViewerNode()
        node.execute(inputs={}, params=None)
        assert node._display_text == ""


class TestTextViewerRuntimeState:
    """TextViewerNode 运行时状态序列化测试"""

    def test_serialize_empty_state(self):
        """serialize_runtime_state 返回包含 display_text 的字典"""
        node = TextViewerNode()
        state = node.serialize_runtime_state()
        assert isinstance(state, dict)
        assert "display_text" in state
        assert state["display_text"] == ""

    def test_serialize_after_execute(self):
        """execute 后 serialize 包含显示文本"""
        node = TextViewerNode()
        node.execute(inputs={"value": "test data"}, params=None)
        state = node.serialize_runtime_state()
        assert state["display_text"] == "test data"

    def test_deserialize_restores_state(self):
        """deserialize_runtime_state 恢复显示文本"""
        node = TextViewerNode()
        node.deserialize_runtime_state({"display_text": "restored text"})
        assert node._display_text == "restored text"

    def test_serialize_deserialize_roundtrip(self):
        """序列化→反序列化往返一致"""
        node = TextViewerNode()
        node.execute(inputs={"value": 123}, params=None)
        state = node.serialize_runtime_state()

        node2 = TextViewerNode()
        node2.deserialize_runtime_state(state)
        assert node2._display_text == node._display_text


class TestHexToRgb:
    """_hex_to_rgb 辅助函数测试"""

    def test_hex_string_with_hash(self):
        """#RRGGBB 格式"""
        assert _hex_to_rgb("#282828") == (0x28, 0x28, 0x28)

    def test_hex_string_without_hash(self):
        """RRGGBB 格式"""
        assert _hex_to_rgb("FF0000") == (255, 0, 0)

    def test_short_hex_with_hash(self):
        """#RGB 格式"""
        assert _hex_to_rgb("#F00") == (255, 0, 0)

    def test_tuple_input(self):
        """元组输入"""
        assert _hex_to_rgb((10, 20, 30)) == (10, 20, 30)

    def test_invalid_string(self):
        """无效字符串返回默认值"""
        assert _hex_to_rgb("not-a-color") == (40, 40, 40)

    def test_non_string_non_tuple(self):
        """非字符串非元组返回默认值"""
        assert _hex_to_rgb(123) == (40, 40, 40)
