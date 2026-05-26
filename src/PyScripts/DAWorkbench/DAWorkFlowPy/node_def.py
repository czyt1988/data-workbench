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
from typing import Optional

from .types import Input, Output, Parameter


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
    :param style: 节点样式配置 dict 或 None
    """

    icon: str = ""
    render_template: str = "nodestyle"
    style: Optional[dict] = None


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

    :param name: 节点显示名称
    :param category: 节点所属分类，默认为空字符串
    :param render_template: 渲染模板类型，默认为 'nodestyle'，支持 'nodestyle'、'rect'、'svg'、'widget'
    :param icon: 节点图标路径
    :param style: 节点样式配置 dict，默认为 None（使用默认样式）。
        支持的键包括边框颜色、填充色等渲染属性（具体键值由 C++ NodeStyle 定义）
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
        new_cls._node_display = NodeDisplay(
            icon=icon,
            render_template=rt_str,
            style=style if isinstance(style, dict) else None,
        )

        # 设置端口名称列表，供 C++ fallback 使用
        new_cls.input_keys = [inp["name"] for inp in inputs]
        new_cls.output_keys = [outp["name"] for outp in outputs]

        return new_cls

    return decorator
