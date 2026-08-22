"""
工作流节点定义装饰器模块

本模块定义了 NodeDef 装饰器和 DAWorkflowNode 基类，用于声明工作流节点类型。
NodeDef 装饰器会收集类中的 Input、Output、Parameter 声明，
并将描述信息直接设置为类属性（节点元数据 + 端口/参数描述 dict），
渲染相关属性聚合到 NodeDisplay 中供 C++ 侧使用。

使用示例::

    @NodeDef(name="Data Filter", category="Data Processing")
    class DataFilter:
        column = Parameter(str, default="value", description="要筛选的列名")

        class Inputs:
            data = Input("DataFrame", required=True)

        class Outputs:
            filtered = Output("DataFrame")

        def execute(self, inputs=None, params=None):
            # 节点执行逻辑
            ...

使用 NodeDisplay 设置样式::

    @NodeDef(name="My Node", style=NodeDisplay(
        body_shape="Ellipse",
        background_color="#4A90D9",
        corner_radius=8.0,
    ))
    class MyNode: ...

装饰器处理流程：
1. 扫描类属性中的 Parameter 实例
2. 扫描嵌套类 Inputs 中的 Input 实例
3. 扫描嵌套类 Outputs 中的 Output 实例
4. 将所有描述信息直接设置为类属性（纯 Python dict）
5. 构建渲染属性聚合 NodeDisplay（icon、render_template、样式字段）
"""

import inspect
from dataclasses import dataclass, field
from typing import Optional, Union

from .types import Input, Output, Parameter
from ._debug import wf_dbg as _wf_dbg_

# 颜色类型：支持 hex 字符串 "#rrggbb" 或 RGB 元组 (r, g, b) / (r, g, b, a)
ColorType = Union[str, tuple]


def _wf_dbg(*args):
    _wf_dbg_("Node", *args)


@dataclass
class LinkPointStyle:
    """
    连接点（端口）样式配置

    对应 C++ DAPyLinkPointStyle 结构体。所有字段为 Optional，
    None 表示使用 C++ 默认值（Rect 形状、默认填充/边框色、borderWidth=1.0）。

    使用示例::

        port_style = LinkPointStyle(
            shape="Circle",
            fill_color="#ff0000",
            border_width=2.0,
        )

    :param shape: 端口形状，支持 "Rect"、"Circle"、"Diamond"；None → C++ 默认 Rect
    :param fill_color: 填充颜色，hex 字符串 "#rrggbb" 或 RGB 元组 (r,g,b)；None → C++ 默认色
    :param border_color: 边框颜色，hex 字符串或 RGB 元组；None → C++ 默认色
    :param border_width: 边框宽度；None → C++ 默认 1.0
    """

    shape: Optional[str] = None
    fill_color: Optional[ColorType] = None
    border_color: Optional[ColorType] = None
    border_width: Optional[float] = None

    def to_dict(self) -> dict:
        """
        转换为 C++ 所需的 dict 格式（仅包含非 None 字段）

        :return: snake_case key 的 dict，如 {"shape": "Circle", "fill_color": "#ff0000"}
        """
        d = {}
        if self.shape is not None:
            d["shape"] = self.shape
        if self.fill_color is not None:
            d["fill_color"] = self.fill_color
        if self.border_color is not None:
            d["border_color"] = self.border_color
        if self.border_width is not None:
            d["border_width"] = self.border_width
        return d


@dataclass
class NodeDisplay:
    """
    节点渲染/显示属性

    将节点渲染相关的属性集中管理，所有字段均为 Optional，
    None 表示使用 C++ 默认值。C++ 侧通过 `PY::toNodeStyle()` 直接从
    Python 对象属性读取并转换为 `DAPyNodeStyle` 结构体。

    使用示例::

        @NodeDef(name="My Node", icon=":icons/node.png",
                 style=NodeDisplay(
                     body_shape="Ellipse",
                     background_color="#4A90D9",
                     corner_radius=8.0,
                     input_port_style=LinkPointStyle(shape="Circle"),
                 ))
        class MyNode: ...

    :param icon: 图标路径字符串
    :param render_template: 渲染模板字符串（"nodestyle" 或 "widget"）
    :param body_shape: 节点体形状，"RoundedRect" 或 "Ellipse"；None → C++ 默认 RoundedRect
    :param name_position: 名称位置，"Inside" 或 "Below"；None → C++ 默认 Inside
    :param icon_position: 图标位置，"LeftOfText" 或 "AboveText"；None → C++ 默认 LeftOfText
    :param background_color: 背景颜色，hex "#rrggbb" 或 RGB 元组；None → C++ 默认色
    :param border_color: 边框颜色，hex "#rrggbb" 或 RGB 元组；None → C++ 默认色
    :param border_width: 边框宽度；None → C++ 默认 1.0
    :param corner_radius: 圆角半径；None → C++ 默认 4.0
    :param icon_size: 图标尺寸；None → C++ 默认 24.0
    :param input_port_side: 输入端口方位，"West"/"East"/"North"/"South"；None → C++ 默认 West
    :param output_port_side: 输出端口方位；None → C++ 默认 East
    :param input_port_style: 输入端口样式配置（LinkPointStyle）；None → C++ 默认构造
    :param output_port_style: 输出端口样式配置（LinkPointStyle）；None → C++ 默认构造
    :param layout_strategy: 连接点布局策略，"Auto" 或 "Manual"；None → C++ 默认 Auto
    :param body_icon_type: 节点体图标类型，"None"/"Pixmap"/"Svg"；None → C++ 默认 None
    :param body_icon_source: 图标源路径；None → C++ 默认空
    :param body_icon_scale: 图标缩放比例；None → C++ 默认 0.8
    :param min_body_width: 最小 body 宽度；None → 不限制。用于自定义 paint 回调节点预留渲染空间
    :param min_body_height: 最小 body 高度；None → 不限制。用于自定义 paint 回调节点预留渲染空间
    """

    # 渲染属性
    icon: str = ""
    render_template: str = "nodestyle"

    # 主体样式
    body_shape: Optional[str] = None
    name_position: Optional[str] = None
    icon_position: Optional[str] = None
    background_color: Optional[ColorType] = None
    border_color: Optional[ColorType] = None
    border_width: Optional[float] = None
    corner_radius: Optional[float] = None
    icon_size: Optional[float] = None

    # 端口配置
    input_port_side: Optional[str] = None
    output_port_side: Optional[str] = None
    input_port_style: Optional[LinkPointStyle] = None
    output_port_style: Optional[LinkPointStyle] = None
    layout_strategy: Optional[str] = None

    # 节点体图标（SVG 支持）
    body_icon_type: Optional[str] = None
    body_icon_source: Optional[str] = None
    body_icon_scale: Optional[float] = None

    # 最小 body 尺寸（用于自定义 paint 回调的节点预留足够渲染空间）
    min_body_width: Optional[float] = None
    min_body_height: Optional[float] = None


def _normalize_render_template(render_template: str) -> str:
    """
    规范化渲染模板值

    旧的 rect/svg 字符串统一映射到 "nodestyle"，
    widget 映射到 "widget"，其他值默认为 "nodestyle"。

    :param render_template: 原始渲染模板字符串
    :return: 规范化的渲染模板字符串
    """
    if render_template in ("rect", "svg", "nodestyle"):
        return "nodestyle"
    if render_template == "widget":
        return "widget"
    return "nodestyle"


def _collect_parameters(cls: type) -> dict:
    """
    从类属性中收集 Parameter 声明，返回 dict[str, Parameter]

    遍历类的所有属性，找出 Parameter 实例，
    为每个 Parameter 设置 name 属性，返回 name→Parameter 映射。
    dict 保持插入顺序（Python 3.7+），key 即参数名。

    :param cls: 被装饰的节点类
    :return: 参数名到Parameter实例的映射
    :rtype: dict[str, Parameter]
    """
    result = {}
    for attr_name in dir(cls):
        if attr_name.startswith("_"):
            continue
        attr_value = getattr(cls, attr_name, None)
        if isinstance(attr_value, Parameter):
            attr_value.name = attr_name
            result[attr_name] = attr_value
    return result


def _collect_from_nested_class(
    cls: type, nested_name: str, decl_type: type
) -> list[dict]:
    """
    从嵌套类中收集 Input 或 Output 声明

    NodeDef 约定在节点类中定义 Inputs 和 Outputs 嵌套类来声明端口。
    此函数遍历嵌套类的属性，找出指定类型的声明实例，
    并使用 to_dict() 将其转换为纯 Python dict。

    :param cls: 被装饰的节点类
    :param nested_name: 嵌套类名（"Inputs" 或 "Outputs"）
    :param decl_type: 声明类型（Input 或 Output）
    :return: 端口声明字典列表
    :rtype: list[dict[str, Any]]
    """
    items = []
    nested_cls = getattr(cls, nested_name, None)
    if nested_cls is None:
        return items
    for attr_name in dir(nested_cls):
        if attr_name.startswith("_"):
            continue
        attr_value = getattr(nested_cls, attr_name, None)
        if isinstance(attr_value, decl_type):
            items.append(attr_value.to_dict(attr_name))
    return items


class DAWorkflowNode:
    """
    工作流节点基类

    所有通过 @NodeDef 装饰器定义的节点类都会自动继承此类，
    提供数据输入/输出的标准接口。

    类属性由 @NodeDef 装饰器自动设置：
    - qualified_name: 节点唯一标识（模块名.类名）
    - name: 节点显示名称
    - category: 节点所属分类
    - icon: 图标路径
    - inputs: 输入端口描述列表（list[dict]）
    - outputs: 输出端口描述列表（list[dict]）
    - parameters: 参数描述映射（dict[str, Parameter]，有序）
    - _node_display: 渲染属性（NodeDisplay）
    - input_keys: 输入端口名称列表
    - output_keys: 输出端口名称列表

    _input_data 和 _output_data 用于在节点执行过程中存储和传递数据，
    由 C++ 侧通过 attr() 直接读写 Python 对象属性。
    """

    # 节点元数据（由 @NodeDef 装饰器设置）
    qualified_name: str = ""
    name: str = ""
    category: str = ""
    icon: str = ""

    # 端口描述（由 @NodeDef 装饰器设置，纯 Python dict）
    inputs: list = []  # list[dict]
    outputs: list = []  # list[dict]

    # 参数描述映射（由 @NodeDef 装饰器设置，dict[str, Parameter]，有序）
    parameters: dict = {}

    # 渲染属性聚合（由 @NodeDef 装饰器设置）
    _node_display: NodeDisplay = None

    # 端口名称列表（由 @NodeDef 装饰器设置，供 C++ fallback 使用）
    input_keys: list = []
    output_keys: list = []

    def __init__(self):
        self.node_id = (
            None  # 运行时节点 ID，由 _make_node_id() 自动生成或在序列化恢复时设置
        )
        self._input_data = {}  # dict[str, Any]，输入端口数据缓存，键为端口名称
        self._output_data = {}  # dict[str, Any]，输出端口数据缓存，键为端口名称
        self.is_global = False  # 是否为全局节点（全局节点执行但不传递数据到下游）
        self._node_state = "idle"  # 节点执行状态，与 C++ DAPyNodeState 枚举对应

        # 将参数默认值初始化为实例属性，确保 getattr(node, param_name) 始终可读
        for name, param in self.parameters.items():
            if param.default is not None:
                setattr(self, name, param.default)

        # 调用用户自定义 __init__（通过 MRO 链）。
        # @NodeDef 装饰器创建 new_cls = type(name, (DAWorkflowNode, cls), {})，
        # MRO 为 new_cls → DAWorkflowNode → 用户类 → object。
        # 若不调用 super().__init__()，用户类的 __init__ 永远不会执行，
        # 导致用户在 __init__ 中设置的实例属性（如缓存变量）缺失，引发 AttributeError。
        super().__init__()

    def set_input_data(self, key: str, data) -> None:
        """
        设置输入数据

        :param key: 数据键名
        :param data: 数据值
        """
        self._input_data[key] = data

    def get_output_data(self, key: str):
        """
        获取输出数据

        :param key: 数据键名
        :return: 对应的输出数据，若键不存在则返回 None
        """
        return self._output_data.get(key)

    def set_node_state(self, state: str) -> None:
        """
        设置节点执行状态

        状态字符串与 C++ DAPyNodeState 枚举一一对应，
        PY::getNodeState() 通过 stringToEnum() 自动映射：
        - "idle" → Idle，节点未开始执行
        - "waiting" → Waiting，节点等待依赖项完成
        - "running" → Running，节点正在执行
        - "success" → Success，节点执行成功
        - "error" → Error，节点执行失败
        - "skipped" → Skipped，节点被跳过执行

        此方法由 DAWorkflowExecutor 在执行过程中调用，
        C++ 侧通过 DAPyNode::getNodeState() 读取 _node_state 属性获取节点状态。

        :param state: 状态字符串，支持 "idle"、"waiting"、"running"、"success"、"error"、"skipped"
        """
        self._node_state = state

    @classmethod
    def get_parameter_descriptors(cls) -> dict:
        """
        获取所有 Parameter 描述符

        C++ 侧通过此方法获取参数的完整描述信息（name/type/default/properties）。

        :return: 参数名到Parameter实例的映射
        :rtype: dict[str, Parameter]
        """
        return cls.parameters

    def serialize_runtime_state(self) -> dict:
        """
        返回需要跨保存/加载周期持久化的运行时状态。

        节点可覆写此方法，将运行时衍生状态（如 execute() 缓存的显示文本、
        中间计算结果等）返回为 dict。DAWorkflowSerializer 会在保存时调用此方法，
        把返回值写入工程文件；加载时通过 deserialize_runtime_state() 恢复。

        返回的 dict 值应为 JSON 可序列化类型（str/int/float/bool/list/dict/None）。
        默认返回空 dict（不持久化任何运行时状态）。

        .. note::
            @NodeDef 装饰器创建的类 MRO 为 ``new_cls → DAWorkflowNode → 用户类 → object``，
            因此基类方法会遮盖用户类的同名覆写。这里通过 ``super()`` 转发到用户类的实现；
            若用户类未覆写此方法，``super()`` 会抛 ``AttributeError``，捕获后返回空 dict。

        :return: 运行时状态字典，空 dict 表示无需持久化
        :rtype: dict[str, Any]
        """
        try:
            return super().serialize_runtime_state() or {}
        except AttributeError:
            return {}

    def deserialize_runtime_state(self, state: dict) -> None:
        """
        从 serialize_runtime_state() 产生的字典恢复运行时状态。

        节点可覆写此方法，在工程加载后重建运行时衍生状态（如恢复缓存的显示文本），
        使得无需重新 execute() 即可在 paint() 中正确渲染。

        .. note::
            @NodeDef 装饰器创建的类 MRO 为 ``new_cls → DAWorkflowNode → 用户类 → object``，
            因此基类方法会遮盖用户类的同名覆写。这里通过 ``super()`` 转发到用户类的实现；
            若用户类未覆写此方法，``super()`` 会抛 ``AttributeError``，捕获后忽略。

        :param state: serialize_runtime_state() 返回的字典，可能为空 dict
        """
        try:
            super().deserialize_runtime_state(state)
        except AttributeError:
            pass

    def run(self) -> bool:
        """
        无参执行入口，由 DAWorkflowExecutor 调用。

        从 ``_input_data`` 构建 ``inputs`` dict，从实例属性构建 ``params`` dict，
        然后根据子类 ``execute()`` 的签名自动适配调用方式：

        - ``execute(self, inputs, params)`` → 传递 inputs 和 params
        - ``execute(self)`` → 无参调用（兼容简单节点）

        :return: ``True`` 表示执行成功
        """
        # 构建 inputs dict（兼容子类未调用 super().__init__() 的情况）
        inputs = dict(getattr(self, '_input_data', {}))

        # 构建 params dict
        params = {}
        for name in self.parameters:
            value = getattr(self, name, None)
            # 若实例属性仍是 Parameter 描述符（子类未调 super().__init__()
            # 且未被反序列化覆盖），提取其默认值
            if isinstance(value, Parameter):
                value = value.default
            params[name] = value

        _wf_dbg(f"  [run] {self.qualified_name} calling execute()")
        _wf_dbg(f"    inputs keys: {list(inputs.keys())}")
        _wf_dbg(f"    params: {params}")

        # 根据 execute() 签名自动适配调用方式
        sig = inspect.signature(self.execute)
        try:
            if len(sig.parameters) >= 2:
                result = self.execute(inputs, params)
            else:
                result = self.execute()
            success = bool(result) if result is not None else True
            _wf_dbg(f"  [run] {self.qualified_name} execute() returned: {result} -> success={success}")
            return success
        except Exception as e:
            _wf_dbg(f"  [run] {self.qualified_name} execute() raised exception: {type(e).__name__}: {e}")
            raise


def _build_node_display(icon: str, render_template: str, style) -> NodeDisplay:
    """
    从 NodeDef 参数构建 NodeDisplay 实例

    支持三种 style 输入类型：
    - NodeDisplay → 直接使用，覆盖 icon 和 render_template
    - dict → 转换为 NodeDisplay（dict 的 key 映射为 NodeDisplay 字段）
    - None → 创建默认 NodeDisplay（仅设置 icon 和 render_template）

    :param icon: 图标路径
    :param render_template: 渲染模板字符串
    :param style: 样式配置，可为 NodeDisplay、dict 或 None
    :return: NodeDisplay 实例
    """
    rt_str = _normalize_render_template(render_template)

    if style is None:
        return NodeDisplay(icon=icon, render_template=rt_str)

    if isinstance(style, NodeDisplay):
        # 如果传入 NodeDisplay，覆盖 icon 和 render_template
        style.icon = icon
        style.render_template = rt_str
        return style

    if isinstance(style, dict):
        # dict → NodeDisplay，将 dict 的 key 映射为 NodeDisplay 字段
        # 处理嵌套的端口样式 dict → LinkPointStyle
        kwargs = dict(style)
        if "input_port_style" in kwargs and isinstance(kwargs["input_port_style"], dict):
            kwargs["input_port_style"] = LinkPointStyle(**kwargs["input_port_style"])
        if "output_port_style" in kwargs and isinstance(kwargs["output_port_style"], dict):
            kwargs["output_port_style"] = LinkPointStyle(**kwargs["output_port_style"])
        kwargs["icon"] = icon
        kwargs["render_template"] = rt_str
        return NodeDisplay(**kwargs)

    # 其他类型（不应出现），直接创建默认 NodeDisplay
    return NodeDisplay(icon=icon, render_template=rt_str)


def NodeDef(
    name: str,
    category: str = "",
    render_template: str = "nodestyle",
    icon: str = "",
    style=None,
    description: str = None,
):
    """
    工作节点定义装饰器

    此装饰器用于声明工作流节点类型。它会收集被装饰类中的 Input、Output、Parameter
    声明，将端口描述转换为 dict，Parameter 实例保留在 parameters dict 中，渲染属性聚合到 NodeDisplay。

    类属性包含：
    - name: 节点显示名称
    - qualified_name: 节点的唯一标识（模块名.类名）
    - category: 节点所属分类
    - icon: 图标路径
    - inputs: 输入端口列表（list[dict]）
    - outputs: 输出端口列表（list[dict]）
    - parameters: 参数描述映射（dict[str, Parameter]，有序）
    - _node_display: 渲染属性（NodeDisplay，包含 icon、render_template 及所有样式字段）

    使用示例::

        @NodeDef(name="Data Filter", category="Data Processing")
        class DataFilter:
            column = Parameter(str, default="value")
            class Inputs:
                data = Input("DataFrame", required=True)
            class Outputs:
                filtered = Output("DataFrame")
            def execute(self, inputs=None, params=None):
                ...

    使用 NodeDisplay 设置样式::

        @NodeDef(name="My Node", style=NodeDisplay(
            body_shape="Ellipse",
            background_color="#4A90D9",
            corner_radius=8.0,
        ))
        class MyNode: ...

    :param name: 节点显示名称
    :param category: 节点所属分类，默认为空字符串
    :param render_template: 渲染模板类型，默认为 'nodestyle'，支持 'nodestyle'、'rect'、'svg'、'widget'
    :param icon: 节点图标路径
    :param style: 节点样式配置，默认为 None（使用默认样式）。
        支持两种类型：
        - NodeDisplay — 类型化的样式配置（推荐，有 IDE 自动补全）
        - dict — 裸字典（snake_case 键，自动转换为 NodeDisplay）
    :param description: 节点说明文本，显示在 tooltip 中。推荐使用 ``_("English") # cn:中文`` 模式翻译。
        若为 None，则自动回退读取类 docstring（经 inspect.cleandoc 清理）。
        docstring 降级时不经过 ``_()`` 翻译，仅为英文源文本。
    :return: 装饰器函数，接收 type 并返回继承 DAWorkflowNode 的新 type
    :rtype: Callable[[type], type]
    """
    def decorator(cls: type) -> type:
        """
        节点定义装饰器的内部函数

        收集类中的 Input、Output、Parameter 声明，
        将描述信息直接设置为类属性（纯 Python dict），渲染属性聚合到 NodeDisplay。

        :param cls: 被装饰的节点类
        :return: 继承 DAWorkflowNode 的新类
        """
        # 收集参数描述映射（dict[str, Parameter]，同时为 Parameter 实例设置 name 属性）
        parameters_dict = _collect_parameters(cls)

        # 收集输入端口声明（dict 列表）
        inputs = _collect_from_nested_class(cls, "Inputs", Input)

        # 收集输出端口声明（dict 列表）
        outputs = _collect_from_nested_class(cls, "Outputs", Output)

        # 生成唯一标识：模块名.类名
        qualified_name = cls.__module__ + "." + cls.__name__

        # 创建继承 DAWorkflowNode 的新类，确保所有 @NodeDef 节点都具备基类方法
        new_cls = type(cls.__name__, (DAWorkflowNode, cls), {})
        new_cls.__module__ = cls.__module__

        # 直接在 new_cls 上设置类属性（节点元数据）
        # 必须设置在 new_cls 上而非 cls 上，否则 DAWorkflowNode 基类的默认值会遮盖
        new_cls.qualified_name = qualified_name
        new_cls.name = name
        new_cls.category = category
        new_cls.icon = icon

        # 节点说明文本：优先使用显式 description 参数，降级读取类 docstring
        import inspect
        if description is not None:
            new_cls.__node_description = description
        elif cls.__doc__:
            new_cls.__node_description = inspect.cleandoc(cls.__doc__)
        else:
            new_cls.__node_description = ""

        # 直接在 new_cls 上设置类属性（端口描述 + 参数描述映射）
        new_cls.inputs = inputs
        new_cls.outputs = outputs
        new_cls.parameters = parameters_dict

        # 构建渲染属性聚合（NodeDisplay）
        new_cls._node_display = _build_node_display(icon, render_template, style)

        # 设置端口名称列表，供 C++ fallback 使用
        new_cls.input_keys = [inp["name"] for inp in inputs]
        new_cls.output_keys = [outp["name"] for outp in outputs]

        _wf_dbg(f"Register node: {qualified_name} (name='{name}', category='{category}', "
                f"inputs={[i['name'] for i in inputs]}, "
                f"outputs={[o['name'] for o in outputs]}, "
                f"params={list(parameters_dict.keys())})")

        return new_cls

    return decorator
