"""
工作流节点定义装饰器模块

本模块定义了 NodeDef 装饰器，用于声明工作流节点类型。
NodeDef 装饰器会收集类中的 Input、Output、Parameter 声明，
并自动生成 _node_descriptor 字典，供 C++ 侧通过 DAPyJsonCast 读取。

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
4. 将所有声明信息汇总生成 _node_descriptor 字典
"""

from .types import Input, Output, Parameter


def _normalize_render_template(render_template: str) -> str:
    """
    规范化渲染模板值，将旧的 rect/svg 映射到 nodestyle。

    :param render_template: 原始渲染模板字符串
    :return: 规范化后的模板字符串（'nodestyle' 或 'widget'）
    """
    if render_template in ("rect", "svg"):
        return "nodestyle"
    if render_template == "widget":
        return "widget"
    return "nodestyle"


def _collect_parameters(cls: type) -> list:
    """
    从类属性中收集 Parameter 声明

    遍历类的所有属性，找出 Parameter 实例，并使用属性名作为参数名。

    :param cls: 被装饰的节点类
    :return: 参数信息字典列表
    """
    params = []
    for attr_name in dir(cls):
        if attr_name.startswith("_"):
            continue
        attr_value = getattr(cls, attr_name, None)
        if isinstance(attr_value, Parameter):
            params.append(attr_value.to_dict(attr_name))
    return params


def _collect_from_nested_class(cls: type, nested_name: str, decl_type: type) -> list:
    """
    从嵌套类中收集 Input 或 Output 声明

    NodeDef 约定在节点类中定义 Inputs 和 Outputs 嵌套类来声明端口。
    此函数遍历嵌套类的属性，找出指定类型的声明实例。

    :param cls: 被装饰的节点类
    :param nested_name: 嵌套类名（"Inputs" 或 "Outputs"）
    :param decl_type: 声明类型（Input 或 Output）
    :return: 端口信息字典列表
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


def NodeDef(name: str, category: str = "", render_template: str = "rect", style=None):
    """
    工作流节点定义装饰器

    此装饰器用于声明工作流节点类型。它会收集被装饰类中的 Input、Output、Parameter
    声明，并自动在类上设置 _node_descriptor 字典属性。

    _node_descriptor 字典包含以下键：
    - name: 节点显示名称
    - category: 节点所属分类
    - qualified_name: 节点的唯一标识（模块名.类名）
    - inputs: 输入端口列表
    - outputs: 输出端口列表
    - parameters: 参数列表
    - render_template: 渲染模板类型
    - style: 节点样式配置（DANodeStyle 实例或字典）

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
    :param render_template: 渲染模板类型，默认为 'rect'（映射到 'nodestyle'），支持 'nodestyle'、'widget'
    :param style: 节点样式配置，可为 DANodeStyle 实例或样式参数字典，默认为 None
    :return: 装饰器函数
    """
    normalized_template = _normalize_render_template(render_template)

    def decorator(cls: type) -> type:
        """
        节点定义装饰器的内部函数

        收集类中的 Input、Output、Parameter 声明，
        生成 _node_descriptor 字典并设置到类上。

        :param cls: 被装饰的节点类
        :return: 被装饰后的类（原地修改，添加 _node_descriptor 属性）
        """
        # 收集参数声明
        parameters = _collect_parameters(cls)

        # 收集输入端口声明
        inputs = _collect_from_nested_class(cls, "Inputs", Input)

        # 收集输出端口声明
        outputs = _collect_from_nested_class(cls, "Outputs", Output)

        # 生成唯一标识：模块名.类限定名
        qualified_name = f"{cls.__module__}.{cls.__qualname__}"

        # 构建节点描述符字典
        descriptor = {
            "name": name,
            "category": category,
            "qualified_name": qualified_name,
            "inputs": inputs,
            "outputs": outputs,
            "parameters": parameters,
            "render_template": normalized_template,
        }

        # 处理样式参数
        if style is not None:
            # 尝试将样式转换为可序列化的字典
            style_dict = None
            if hasattr(style, "toJson"):
                style_dict = style.toJson()
            elif isinstance(style, dict):
                style_dict = style
            elif hasattr(style, "__dict__"):
                style_dict = {
                    k: v for k, v in vars(style).items() if not k.startswith("_")
                }
            if style_dict is not None:
                descriptor["style"] = style_dict

        # 在类上设置 _node_descriptor 属性
        cls._node_descriptor = descriptor

        # Also set input_keys and output_keys as class attributes for C++ syncMetaFromPyNode
        cls.input_keys = [inp["name"] for inp in inputs]
        cls.output_keys = [outp["name"] for outp in outputs]

        @classmethod
        def _get_descriptor(cls):
            return getattr(cls, "_node_descriptor", {})

        cls.get_descriptor = _get_descriptor

        return cls

    return decorator
