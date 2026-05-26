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

        def execute(self, inputs, params):
            # 节点执行逻辑
            ...

装饰器处理流程：
1. 扫描类属性中的 Parameter 实例
2. 扫描嵌套类 Inputs 中的 Input 实例
3. 扫描嵌套类 Outputs 中的 Output 实例
4. 将所有描述信息直接设置为类属性（纯 Python dict）
5. 构建渲染属性聚合 NodeDisplay（icon、render_template、style）
"""

from dataclasses import dataclass, field
from typing import Optional, Union

from .types import Input, Output, Parameter

# 颜色类型：支持 hex 字符串 "#rrggbb" 或 RGB 元组 (r, g, b) / (r, g, b, a)
ColorType = Union[str, tuple]


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
        转换为 C++ nodeStyleFromDict 所需的 dict 格式

        仅包含非 None 字段，C++ 侧对缺失键使用默认值。

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
class NodeDisplayStyle:
    """
    节点显示样式配置

    对应 C++ DANodeStyle 结构体，提供类型化的 Python API 替代裸 dict。
    所有字段为 Optional，None 表示使用 C++ 默认值。

    替代旧写法 ``@NodeDef(style={"background_color": "#fff", ...})``，
    新写法有 IDE 自动补全和类型检查::

        @NodeDef(name="My Node", style=NodeDisplayStyle(
            body_shape="Ellipse",
            background_color="#4A90D9",
            border_color="#3A3A5C",
            corner_radius=8.0,
            input_port_style=LinkPointStyle(shape="Circle", fill_color="#ffffff"),
        ))
        class MyNode: ...

    C++ 侧通过 DictConverter::nodeStyleFromDict() 将 to_dict() 输出的 dict
    转换为 DANodeStyle 结构体。

    :param body_shape: 节点体形状，"RoundedRect" 或 "Ellipse"；None → C++ 默认 RoundedRect
    :param name_position: 名称位置，"Inside" 或 "Below"；None → C++ 默认 Inside
    :param icon_position: 图标位置，"LeftOfText" 或 "AboveText"；None → C++ 默认 LeftOfText
    :param background_color: 背景颜色，hex "#rrggbb" 或 RGB 元组；None → C++ 默认 (240,240,240)
    :param border_color: 边框颜色，hex "#rrggbb" 或 RGB 元组；None → C++ 默认 (180,180,180)
    :param border_width: 边框宽度；None → C++ 默认 1.0
    :param corner_radius: 圆角半径；None → C++ 默认 4.0
    :param icon_size: 图标尺寸；None → C++ 默认 24.0
    :param input_port_side: 输入端口方位，"West"/"East"/"North"/"South"；None → C++ 默认 West
    :param output_port_side: 输出端口方位；None → C++ 默认 East
    :param input_port_style: 输入端口样式配置；None → C++ 默认构造
    :param output_port_style: 输出端口样式配置；None → C++ 默认构造
    :param layout_strategy: 连接点布局策略，"Auto" 或 "Manual"；None → C++ 默认 Auto
    :param body_icon_type: 节点体图标类型，"None"/"Pixmap"/"Svg"；None → C++ 默认 None
    :param body_icon_source: 图标源路径（SVG 文件路径或资源路径）；None → C++ 默认空
    :param body_icon_scale: 图标缩放比例；None → C++ 默认 0.8
    """

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

    def to_dict(self) -> dict:
        """
        转换为 C++ nodeStyleFromDict 所需的 dict 格式

        仅包含非 None 字段，C++ 侧对缺失键使用默认值。
        端口样式子对象通过 LinkPointStyle.to_dict() 递归转换。

        :return: snake_case key 的 dict，如 {"body_shape": "Ellipse", "corner_radius": 8.0}
        """
        d = {}
        if self.body_shape is not None:
            d["body_shape"] = self.body_shape
        if self.name_position is not None:
            d["name_position"] = self.name_position
        if self.icon_position is not None:
            d["icon_position"] = self.icon_position
        if self.background_color is not None:
            d["background_color"] = self.background_color
        if self.border_color is not None:
            d["border_color"] = self.border_color
        if self.border_width is not None:
            d["border_width"] = self.border_width
        if self.corner_radius is not None:
            d["corner_radius"] = self.corner_radius
        if self.icon_size is not None:
            d["icon_size"] = self.icon_size
        if self.input_port_side is not None:
            d["input_port_side"] = self.input_port_side
        if self.output_port_side is not None:
            d["output_port_side"] = self.output_port_side
        if self.input_port_style is not None:
            d["input_port_style"] = self.input_port_style.to_dict()
        if self.output_port_style is not None:
            d["output_port_style"] = self.output_port_style.to_dict()
        if self.layout_strategy is not None:
            d["layout_strategy"] = self.layout_strategy
        if self.body_icon_type is not None:
            d["body_icon_type"] = self.body_icon_type
        if self.body_icon_source is not None:
            d["body_icon_source"] = self.body_icon_source
        if self.body_icon_scale is not None:
            d["body_icon_scale"] = self.body_icon_scale
        return d


@dataclass
class NodeDisplay:
    """
    节点渲染/显示属性聚合

    将节点渲染相关的属性集中管理，实现单一职责原则：
    - DAWorkflowNode 负责节点逻辑（元数据、端口、参数）
    - NodeDisplay 负责节点渲染显示（图标、渲染模板、样式）

    NodeDisplay 使用纯 Python 类型（字符串、dict），不依赖 C++ pybind11 导出。
    C++ 侧通过 attr() 读取这些属性并内部转换为 C++ struct。

    使用示例::

        @NodeDef(name="My Node", icon=":icons/node.png", render_template="nodestyle")
        class MyNode:
            ...

        # 访问渲染属性
        display = MyNode._node_display
        print(display.icon)                  # ":icons/node.png"
        print(display.render_template)       # "nodestyle"

    :param icon: 图标路径字符串
    :param render_template: 渲染模板字符串（"nodestyle" 或 "widget"）
    :param style: 节点样式配置，支持 dict、da_py_workflow.DANodeStyle 或 NodeDisplayStyle（自动转 dict）
    """

    icon: str = ""
    render_template: str = "nodestyle"
    style: Optional[object] = None


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


def _collect_parameters(cls: type) -> list[dict]:
    """
    从类属性中收集 Parameter 声明

    遍历类的所有属性，找出 Parameter 实例，
    并使用 to_dict() 将其转换为纯 Python dict。

    :param cls: 被装饰的节点类
    :return: 参数声明字典列表，每个 dict 包含以下键：
        - name: str，参数名称（如 "column"）
        - dtype: str，参数类型名称（如 "str"、"int"）
        - default: Any，参数默认值
        - description: str，参数描述
    :rtype: list[dict[str, Any]]
    """
    params = []
    for attr_name in dir(cls):
        if attr_name.startswith("_"):
            continue
        attr_value = getattr(cls, attr_name, None)
        if isinstance(attr_value, Parameter):
            params.append(attr_value.to_dict(attr_name))
    return params


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
    :return: 端口声明字典列表，每个 dict 包含以下键：
        - name: str，端口名称（如 "data"、"filtered"）
        - dtype: str，端口数据类型名称（如 "DataFrame"）
        - required: bool，是否为必需端口（仅 Input 有此键）
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
    - parameters: 参数描述列表（list[dict]）
    - _node_display: 渲染属性聚合（NodeDisplay）
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

    # 端口与参数描述（由 @NodeDef 装饰器设置，纯 Python dict）
    inputs: list = []  # list[dict]
    outputs: list = []  # list[dict]
    parameters: list = []  # list[dict]

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


def _normalize_style(style):
    """
    规范化 style 参数为可存储的格式

    支持三种输入类型：
    - NodeDisplayStyle → 调用 to_dict() 转为 dict
    - dict → 保持原样
    - da_py_workflow.DANodeStyle → 保持原样（pybind11 对象）
    - None → 返回 None

    :param style: 原始 style 参数
    :return: dict、DANodeStyle pybind11 对象或 None
    """
    if style is None:
        return None
    if isinstance(style, NodeDisplayStyle):
        return style.to_dict()
    if isinstance(style, dict):
        return style
    # da_py_workflow.DANodeStyle 等 pybind11 对象 — 直接返回
    return style


def NodeDef(
    name: str,
    category: str = "",
    render_template: str = "nodestyle",
    icon: str = "",
    style=None,
):
    """
    工作节点定义装饰器

    此装饰器用于声明工作流节点类型。它会收集被装饰类中的 Input、Output、Parameter
    声明，并将描述信息直接设置为类属性（纯 Python dict），渲染属性聚合到 NodeDisplay。

    类属性包含：
    - name: 节点显示名称
    - qualified_name: 节点的唯一标识（模块名.类名）
    - category: 节点所属分类
    - icon: 图标路径
    - inputs: 输入端口列表（list[dict]）
    - outputs: 输出端口列表（list[dict]）
    - parameters: 参数列表（list[dict]）
    - _node_display: 渲染属性聚合（NodeDisplay，包含 render_template 和 style）

    使用示例::

        @NodeDef(name="Data Filter", category="Data Processing")
        class DataFilter:
            column = Parameter(str, default="value")
            class Inputs:
                data = Input("DataFrame", required=True)
            class Outputs:
                filtered = Output("DataFrame")
            def execute(self, inputs, params):
                ...

    使用 NodeDisplayStyle 替代裸 dict::

        @NodeDef(name="My Node", style=NodeDisplayStyle(
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
        支持三种类型：
        - NodeDisplayStyle — 类型化的样式配置（推荐，有 IDE 自动补全）
        - dict — 裸字典（snake_case 键，如 {"body_shape": "Ellipse", "corner_radius": 8.0}）
        - da_py_workflow.DANodeStyle — C++ pybind11 导出对象
    :return: 装饰器函数，接收 type 并返回继承 DAWorkflowNode 的新 type
    :rtype: Callable[[type], type]
    """
    rt_str = _normalize_render_template(render_template)

    def decorator(cls: type) -> type:
        """
        节点定义装饰器的内部函数

        收集类中的 Input、Output、Parameter 声明，
        将描述信息直接设置为类属性（纯 Python dict），渲染属性聚合到 NodeDisplay。

        :param cls: 被装饰的节点类
        :return: 继承 DAWorkflowNode 的新类
        """
        # 收集参数声明（dict 列表）
        parameters = _collect_parameters(cls)

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

        # 直接在 new_cls 上设置类属性（端口与参数描述，纯 Python dict）
        new_cls.inputs = inputs
        new_cls.outputs = outputs
        new_cls.parameters = parameters

        # 构建渲染属性聚合（NodeDisplay）
        # _normalize_style 将 NodeDisplayStyle→dict，dict→dict，DANodeStyle→保持原样
        normalized_style = _normalize_style(style)
        new_cls._node_display = NodeDisplay(
            icon=icon,
            render_template=rt_str,
            style=normalized_style,
        )

        # 设置端口名称列表，供 C++ fallback 使用
        new_cls.input_keys = [inp["name"] for inp in inputs]
        new_cls.output_keys = [outp["name"] for outp in outputs]

        return new_cls

    return decorator
