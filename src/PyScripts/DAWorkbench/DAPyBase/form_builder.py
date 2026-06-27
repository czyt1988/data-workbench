# -*- coding: utf-8 -*-
"""
表单构建器（v2 schema）

提供链式 API :class:`FormBuilder` 用于便捷地构建 :class:`FormSpec`，
以及辅助函数 :func:`option` 用于构造 :class:`OptionSpec`。

构建出的 :class:`FormSpec` 通过 ``to_dict()`` 输出 v2 schema 纯字典，
由 C++ 端 ``getConfigValues`` 绑定接收并序列化为 JSON。

典型用法::

    from DAWorkbench.DAPyBase.form_builder import FormBuilder, option

    spec = (FormBuilder("参数设置")
            .group("basic", "基本参数")
                .string("name", label="名称", default="",
                        required_when="True")
                .int("count", label="数量", default=1, min=0, max=100)
            .end_group()
            .enum("mode", label="模式", default="a",
                  options=[option("a", "模式A"), option("b", "模式B")])
            .build())

    form_dict = spec.to_dict()  # 交由 C++ getConfigValues 处理

@see form_spec.py  （数据模型定义）
@see DAInterfacePythonBinding.cpp  （getConfigValues 绑定契约）
"""
from typing import Any, List, Optional, Union

from .form_spec import FieldSpec, FormSpec, GroupSpec, OptionSpec

__all__ = ["FormBuilder", "option"]


def option(value: Any, label: str, description: str = "") -> OptionSpec:
    """构造枚举选项

    :param value: 选项值（任意可 JSON 序列化的类型）
    :param label: 选项显示文本
    :param description: 选项描述，默认为空
    :return: :class:`OptionSpec` 实例
    """
    return OptionSpec(value=value, label=label, description=description)


class FormBuilder:
    """链式表单构建器

    通过方法链描述表单结构，最终调用 :meth:`build` 生成 :class:`FormSpec`。
    所有字段添加方法在 ``name`` 之后使用**仅关键字参数**，以提升可读性。

    分组通过 :meth:`group` / :meth:`end_group` 显式开启与关闭；
    同一时刻只允许一个分组处于开启状态。

    :param title: 表单标题
    """

    def __init__(self, title: str):
        self._spec: FormSpec = FormSpec(title=title)
        self._current_group: Optional[GroupSpec] = None

    def group(self, name: str, label: str, description: str = "") -> "FormBuilder":
        """开启一个新分组

        :param name: 分组唯一标识
        :param label: 分组显示名称
        :param description: 分组描述
        :return: 返回自身以支持链式调用
        :raises RuntimeError: 已有分组处于开启状态（需先调用 :meth:`end_group`）
        """
        if self._current_group is not None:
            raise RuntimeError("A group is already open; call end_group() before starting a new one.")
        self._current_group = GroupSpec(name=name, label=label, description=description)
        return self

    def end_group(self) -> "FormBuilder":
        """关闭当前分组并将其加入表单顶层条目

        :return: 返回自身以支持链式调用
        :raises RuntimeError: 当前没有开启的分组
        """
        if self._current_group is None:
            raise RuntimeError("No group is open; call group() before end_group().")
        self._spec.items.append(self._current_group)
        self._current_group = None
        return self

    def _add_field(self, field: FieldSpec) -> "FormBuilder":
        """将字段加入当前分组（若开启）或顶层条目"""
        if self._current_group is not None:
            self._current_group.items.append(field)
        else:
            self._spec.items.append(field)
        return self

    # ----- 字段类型方法 -----

    def string(self, name: str, *, label: str = "", description: str = "",
               default: Optional[str] = None, placeholder: str = "",
               read_only: bool = False, layout: str = "inline",
               height: Optional[int] = None, visible_when: str = "",
               enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加字符串字段（type="str"）"""
        return self._add_field(FieldSpec(
            name=name, type="str", label=label, description=description,
            default=default, placeholder=placeholder, read_only=read_only,
            layout=layout, height=height,
            visible_when=visible_when, enabled_when=enabled_when, required_when=required_when,
        ))

    def int(self, name: str, *, label: str = "", description: str = "",
            default: Optional[int] = None, min: Optional[int] = None,
            max: Optional[int] = None, step: Optional[int] = None,
            layout: str = "inline", visible_when: str = "",
            enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加整数字段（type="int"）"""
        return self._add_field(FieldSpec(
            name=name, type="int", label=label, description=description,
            default=default, min=min, max=max, step=step, layout=layout,
            visible_when=visible_when, enabled_when=enabled_when, required_when=required_when,
        ))

    def float(self, name: str, *, label: str = "", description: str = "",
              default: Optional[float] = None, min: Optional[float] = None,
              max: Optional[float] = None, step: Optional[float] = None,
              decimals: Optional[int] = None, layout: str = "inline",
              visible_when: str = "", enabled_when: str = "",
              required_when: str = "") -> "FormBuilder":
        """添加浮点数字段（type="float"）"""
        return self._add_field(FieldSpec(
            name=name, type="float", label=label, description=description,
            default=default, min=min, max=max, step=step, decimals=decimals,
            layout=layout, visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def bool(self, name: str, *, label: str = "", description: str = "",
             default: Optional[bool] = None, visible_when: str = "",
             enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加布尔字段（type="bool"）"""
        return self._add_field(FieldSpec(
            name=name, type="bool", label=label, description=description,
            default=default, visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def enum(self, name: str, *, label: str = "", description: str = "",
             default: Any = None, options: Optional[List[OptionSpec]] = None,
             layout: str = "inline", visible_when: str = "",
             enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加枚举字段（type="enum"）

        :param options: 枚举选项列表，通过 :func:`option` 构造；为 None 时使用空列表
        """
        return self._add_field(FieldSpec(
            name=name, type="enum", label=label, description=description,
            default=default, options=options or [], layout=layout,
            visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def file(self, name: str, *, label: str = "", description: str = "",
             default: Optional[str] = None, filter: str = "",
             visible_when: str = "", enabled_when: str = "",
             required_when: str = "") -> "FormBuilder":
        """添加文件选择字段（type="file"）"""
        return self._add_field(FieldSpec(
            name=name, type="file", label=label, description=description,
            default=default, filter=filter,
            visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def folder(self, name: str, *, label: str = "", description: str = "",
               default: Optional[str] = None, visible_when: str = "",
               enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加文件夹选择字段（type="folder"）"""
        return self._add_field(FieldSpec(
            name=name, type="folder", label=label, description=description,
            default=default, visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def color(self, name: str, *, label: str = "", description: str = "",
              default: Optional[str] = None, visible_when: str = "",
              enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加颜色字段（type="color"）"""
        return self._add_field(FieldSpec(
            name=name, type="color", label=label, description=description,
            default=default, visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def font(self, name: str, *, label: str = "", description: str = "",
             default: Any = None, visible_when: str = "",
             enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加字体字段（type="font"）

        :param default: 接受 ``{family,size,bold,italic,color}`` 字典或
                        ``QFont::toString`` 字符串，均由 C++ 端处理
        """
        return self._add_field(FieldSpec(
            name=name, type="font", label=label, description=description,
            default=default, visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def code(self, name: str, *, label: str = "", description: str = "",
             default: Optional[str] = None, height: Optional[int] = None,
             visible_when: str = "", enabled_when: str = "",
             required_when: str = "") -> "FormBuilder":
        """添加代码字段（type="code"）

        强制 ``layout="below"``，因为代码为多行内容。
        """
        return self._add_field(FieldSpec(
            name=name, type="code", label=label, description=description,
            default=default, height=height, layout="below",
            visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def list(self, name: str, *, label: str = "", description: str = "",
             default: Optional[List[str]] = None, visible_when: str = "",
             enabled_when: str = "", required_when: str = "") -> "FormBuilder":
        """添加字符串列表字段（type="list"）"""
        return self._add_field(FieldSpec(
            name=name, type="list", label=label, description=description,
            default=default, visible_when=visible_when, enabled_when=enabled_when,
            required_when=required_when,
        ))

    def build(self) -> FormSpec:
        """构建并返回 :class:`FormSpec`

        :return: 表单规格实例
        :raises RuntimeError: 仍有分组处于开启状态（需先调用 :meth:`end_group`）
        """
        if self._current_group is not None:
            raise RuntimeError("A group is still open; call end_group() before build().")
        return self._spec
