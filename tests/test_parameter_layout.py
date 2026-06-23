"""
test_parameter_layout — Parameter 的 layout/height 扩展属性测试

独立于 da_py_workflow（pybind11 嵌入模块），可在纯 Python 环境下运行。
覆盖：默认值、归一化、校验、to_dict 序列化。
"""

import pytest
from DAWorkbench.DAWorkFlowPy import Parameter


class TestParameterLayout:
    """Parameter layout/height 扩展属性测试"""

    def test_layout_default_inline(self):
        """未指定 layout 时默认为 inline，并写入 _extra_kwargs"""
        p = Parameter(str, default="value", description="列名")
        assert p._extra_kwargs["layout"] == "inline"

    def test_layout_below(self):
        """layout='below' 归一化小写后写入 _extra_kwargs"""
        p = Parameter(str, default="value", layout="below")
        assert p._extra_kwargs["layout"] == "below"

    def test_layout_case_insensitive(self):
        """layout 取值大小写不敏感，统一归一化为小写"""
        p1 = Parameter(str, layout="BELOW")
        p2 = Parameter(str, layout="Inline")
        assert p1._extra_kwargs["layout"] == "below"
        assert p2._extra_kwargs["layout"] == "inline"

    def test_layout_invalid_raises(self):
        """非法 layout 值抛 ValueError"""
        with pytest.raises(ValueError):
            Parameter(str, layout="diagonal")

    def test_layout_invalid_empty_raises(self):
        """空字符串 layout 抛 ValueError"""
        with pytest.raises(ValueError):
            Parameter(str, layout="")

    def test_height_optional(self):
        """未设置 height 时不写入 _extra_kwargs；设置时以 int 存入"""
        p_no_height = Parameter(str, layout="below")
        assert "height" not in p_no_height._extra_kwargs

        p_with_height = Parameter(str, layout="below", height=120)
        assert p_with_height._extra_kwargs["height"] == 120
        assert isinstance(p_with_height._extra_kwargs["height"], int)

    def test_height_int_conversion(self):
        """height 接受可转换为 int 的值"""
        p = Parameter(str, layout="below", height="100")
        assert p._extra_kwargs["height"] == 100
        assert isinstance(p._extra_kwargs["height"], int)

    def test_to_dict_includes_layout_and_height(self):
        """to_dict 的 properties 子字典包含 layout/height"""
        p = Parameter(str, default="x", layout="below", height=90)
        d = p.to_dict("expr")
        assert d["name"] == "expr"
        assert d["properties"]["layout"] == "below"
        assert d["properties"]["height"] == 90

    def test_to_dict_layout_only(self):
        """只设 layout 时 properties 仅含 layout"""
        p = Parameter(str, default="x", layout="below")
        d = p.to_dict("expr")
        assert d["properties"]["layout"] == "below"
        assert "height" not in d["properties"]

    def test_to_dict_inline_default(self):
        """默认 inline 也会写入 properties"""
        p = Parameter(str, default="x")
        d = p.to_dict("expr")
        assert d["properties"]["layout"] == "inline"

    def test_layout_does_not_affect_type_label(self):
        """layout 不影响 type 标签"""
        p = Parameter(str, layout="below")
        assert p.get_type_label() == "str"

    def test_layout_does_not_affect_default(self):
        """layout 不影响 default 值"""
        p = Parameter(str, default="hello", layout="below")
        assert p.default == "hello"

    def test_height_with_inline_layout(self):
        """inline 模式下设置 height 也会存入（C++ 端会忽略）"""
        p = Parameter(str, layout="inline", height=80)
        assert p._extra_kwargs["layout"] == "inline"
        assert p._extra_kwargs["height"] == 80

    def test_repr_still_works(self):
        """带 layout/height 的 Parameter repr 不报错"""
        p = Parameter(str, default="x", layout="below", height=80)
        r = repr(p)
        assert "Parameter" in r
        assert "str" in r
