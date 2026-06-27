# -*- coding: utf-8 -*-
"""
表单规格数据模型（v2 schema）

定义统一属性表单的数据模型，使用 ``@dataclass`` 描述 v2 schema 结构。
``to_dict()`` 输出严格符合 v2 schema 的纯字典，供 C++ 端
``DAFormSchemaIO::fromJsonObject`` 解析。

v2 schema 顶层结构::

    {
        "version": 2,
        "title": "表单标题",
        "items": [ <FieldSpec|GroupSpec>.to_dict(), ... ]
    }

条目通过 ``kind`` 字段区分：
    - ``"field"``：字段，见 :class:`FieldSpec`
    - ``"group"``：分组，可递归包含字段或子分组，见 :class:`GroupSpec`

@see DAFormSchemaIO.cpp  （C++ 端容错解析器）
"""
from dataclasses import dataclass, field
from typing import Any, List, Optional, Union

__all__ = ["OptionSpec", "FieldSpec", "GroupSpec", "FormSpec"]


@dataclass
class OptionSpec:
    """枚举选项规格

    用于 :class:`FieldSpec` 的 ``options`` 列表，描述枚举字段的可选项。
    """

    value: Any
    """选项值（任意可 JSON 序列化的类型）"""

    label: str
    """选项显示文本"""

    description: str = ""
    """选项描述（可选，非空时才输出）"""

    def to_dict(self) -> dict:
        """转换为 v2 schema 字典

        始终输出 ``value`` 与 ``label``；``description`` 非空时才输出。
        """
        d: dict = {"value": self.value, "label": self.label}
        if self.description:
            d["description"] = self.description
        return d


@dataclass
class FieldSpec:
    """字段规格

    描述表单中的一个字段。``to_dict()`` 仅输出非默认/非空的键，以保持字典整洁，
    并与 C++ 端容错解析器（``DAFormSchemaIO::fromJsonObject``）的容忍策略匹配。

    :ivar name: 字段唯一标识（必填）
    :ivar type: 字段类型，如 ``"str"``/``"int"``/``"float"``/``"bool"``/``"enum"``/
                ``"file"``/``"folder"``/``"color"``/``"font"``/``"code"``/``"list"``
    :ivar label: 显示名称，为空时 C++ 端回退到 ``name``
    :ivar description: 字段描述
    :ivar default: 默认值
    :ivar placeholder: 占位提示文本
    :ivar read_only: 是否只读
    :ivar layout: 布局方式，``"inline"``（默认）或 ``"below"``
    :ivar height: 高度（多行文本/代码字段使用）
    :ivar options: 枚举选项列表（仅 ``enum`` 类型使用）
    :ivar min: 数值最小值
    :ivar max: 数值最大值
    :ivar step: 数值步进
    :ivar decimals: 小数位数（浮点字段）
    :ivar filter: 文件过滤器（文件字段）
    :ivar visible_when: 可见性联动规则表达式
    :ivar enabled_when: 启用性联动规则表达式
    :ivar required_when: 必填性联动规则表达式
    """

    name: str
    type: str
    label: str = ""
    description: str = ""
    default: Any = None
    placeholder: str = ""
    read_only: bool = False
    layout: str = "inline"
    height: Optional[int] = None
    options: List[OptionSpec] = field(default_factory=list)
    # 数值扩展属性
    min: Optional[Union[int, float]] = None
    max: Optional[Union[int, float]] = None
    step: Optional[Union[int, float]] = None
    decimals: Optional[int] = None
    # 文件/文件夹
    filter: str = ""
    # 声明性联动规则
    visible_when: str = ""
    enabled_when: str = ""
    required_when: str = ""

    def to_dict(self) -> dict:
        """转换为 v2 schema 字典

        输出规则：
            - 始终输出 ``kind:"field"``、``name``、``type``、``label``
            - ``description``/``placeholder``/``filter``/``visible_when``/``enabled_when``/``required_when`` 仅在非空时输出
            - ``default``/``height``/``min``/``max``/``step``/``decimals`` 仅在非 None 时输出
            - ``read_only`` 仅在为 True 时输出
            - ``layout`` 仅在非 ``"inline"`` 时输出
            - ``options`` 仅在非空列表时输出（每个选项通过 :meth:`OptionSpec.to_dict` 转换）
        """
        d: dict = {
            "kind": "field",
            "name": self.name,
            "type": self.type,
            "label": self.label,
        }
        if self.description:
            d["description"] = self.description
        if self.default is not None:
            d["default"] = self.default
        if self.placeholder:
            d["placeholder"] = self.placeholder
        if self.read_only:
            d["read_only"] = True
        if self.layout != "inline":
            d["layout"] = self.layout
        if self.height is not None:
            d["height"] = self.height
        if self.min is not None:
            d["min"] = self.min
        if self.max is not None:
            d["max"] = self.max
        if self.step is not None:
            d["step"] = self.step
        if self.decimals is not None:
            d["decimals"] = self.decimals
        if self.filter:
            d["filter"] = self.filter
        if self.options:
            d["options"] = [opt.to_dict() for opt in self.options]
        if self.visible_when:
            d["visible_when"] = self.visible_when
        if self.enabled_when:
            d["enabled_when"] = self.enabled_when
        if self.required_when:
            d["required_when"] = self.required_when
        return d


@dataclass
class GroupSpec:
    """分组规格

    可递归包含 :class:`FieldSpec` 或 :class:`GroupSpec` 作为子条目。
    """

    name: str
    """分组唯一标识"""

    label: str = ""
    """分组显示名称"""

    description: str = ""
    """分组描述"""

    items: List[Union["FieldSpec", "GroupSpec"]] = field(default_factory=list)
    """子条目列表（字段或子分组）"""

    def to_dict(self) -> dict:
        """转换为 v2 schema 字典

        始终输出 ``kind:"group"``、``name``、``label``、``items``，
        其中 ``items`` 通过各子条目的 ``to_dict()`` 递归转换；
        ``description`` 仅在非空时输出，与 :meth:`FieldSpec.to_dict` 保持一致。
        """
        d: dict = {
            "kind": "group",
            "name": self.name,
            "label": self.label,
        }
        if self.description:
            d["description"] = self.description
        d["items"] = [item.to_dict() for item in self.items]
        return d


@dataclass
class FormSpec:
    """表单规格（顶层容器）

    对应 v2 schema 顶层对象，``to_dict()`` 输出固定为::

        {"version": 2, "title": ..., "items": [...]}
    """

    title: str
    """表单标题"""

    items: List[Union[FieldSpec, GroupSpec]] = field(default_factory=list)
    """顶层条目列表（字段或分组）"""

    version: int = 2
    """schema 版本，固定为 2"""

    def to_dict(self) -> dict:
        """转换为 v2 schema 顶层字典

        输出固定包含 ``version``、``title``、``items`` 三个键。
        C++ 端绑定（``getConfigValues``）会调用本方法并将返回的纯字典
        通过 ``pyDictToJsonString`` 序列化为 JSON 字符串。
        """
        return {
            "version": self.version,
            "title": self.title,
            "items": [item.to_dict() for item in self.items],
        }
