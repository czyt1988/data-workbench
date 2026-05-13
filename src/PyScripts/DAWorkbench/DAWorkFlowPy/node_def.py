"""
工作流节点定义装饰器模块

本模块定义了 NodeDef 装饰器，用于声明工作流节点类型。
NodeDef 装饰器会收集类中的 Input、Output、Parameter 声明，
并自动生成 _node_descriptor C++ DANodeDescriptor 结构体，
供 C++ 侧直接使用，无需再通过 JSON 中转。

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
4. 将所有声明信息汇总生成 _node_descriptor C++ DANodeDescriptor 结构体
"""

import da_py_workflow
from .types import Input, Output, Parameter


def _normalize_render_template(render_template: str) -> int:
    """
    规范化渲染模板值，将字符串映射到 da_py_workflow.RenderTemplate 枚举

    旧的 rect/svg 字符串统一映射到 NodeStyleTemplate，
    widget 映射到 WidgetTemplate，其他值默认为 NodeStyleTemplate。

    :param render_template: 原始渲染模板字符串
    :return: da_py_workflow.RenderTemplate 枚举值
    """
    if render_template in ("rect", "svg", "nodestyle"):
        return da_py_workflow.RenderTemplate.NodeStyleTemplate
    if render_template == "widget":
        return da_py_workflow.RenderTemplate.WidgetTemplate
    return da_py_workflow.RenderTemplate.NodeStyleTemplate


def _collect_parameters(cls: type) -> list:
    """
    从类属性中收集 Parameter 声明

    遍历类的所有属性，找出 Parameter 实例，
    并使用 to_parameter_descriptor() 将其转换为 C++ DAParameterDescriptor 结构体。

    :param cls: 被装饰的节点类
    :return: DAParameterDescriptor 结构体列表
    """
    params = []
    for attr_name in dir(cls):
        if attr_name.startswith("_"):
            continue
        attr_value = getattr(cls, attr_name, None)
        if isinstance(attr_value, Parameter):
            params.append(attr_value.to_parameter_descriptor(attr_name))
    return params


def _collect_from_nested_class(cls: type, nested_name: str, decl_type: type) -> list:
    """
    从嵌套类中收集 Input 或 Output 声明

    NodeDef 约定在节点类中定义 Inputs 和 Outputs 嵌套类来声明端口。
    此函数遍历嵌套类的属性，找出指定类型的声明实例，
    并使用 to_port_descriptor() 将其转换为 C++ DAPortDescriptor 结构体。

    :param cls: 被装饰的节点类
    :param nested_name: 嵌套类名（"Inputs" 或 "Outputs"）
    :param decl_type: 声明类型（Input 或 Output）
    :return: DAPortDescriptor 结构体列表
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
            items.append(attr_value.to_port_descriptor(attr_name))
    return items


def _hex_to_rgb(hex_str):
    """
    将十六进制颜色字符串转换为 (r, g, b) 整数元组

    支持 '#RRGGBB' 和 '#RGB' 两种格式。

    :param hex_str: 十六进制颜色字符串（如 '#ff0000'）
    :return: (r, g, b) 整数元组
    """
    hex_str = hex_str.lstrip('#')
    if len(hex_str) == 3:
        hex_str = ''.join(c * 2 for c in hex_str)
    return int(hex_str[0:2], 16), int(hex_str[2:4], 16), int(hex_str[4:6], 16)


def _link_point_style_from_dict(style_dict):
    """
    将 Python dict 转换为 DAPyLinkPointStyle 实例

    支持的键名（snake_case）：
    - shape: 端口形状（PortShape 枚举值）
    - fill_color: 填充颜色十六进制字符串（如 '#ff0000'）
    - border_color: 边框颜色十六进制字符串
    - border_width: 边框宽度（float）

    :param style_dict: 端口样式字典
    :return: DAPyLinkPointStyle 实例
    """
    s = da_py_workflow.DAPyLinkPointStyle()
    for key, value in style_dict.items():
        if key == 'shape':
            s.shape = value
        elif key == 'fill_color':
            if isinstance(value, str) and value.startswith('#'):
                r, g, b = _hex_to_rgb(value)
                s.setFillColor(r, g, b)
            elif isinstance(value, (tuple, list)):
                s.setFillColor(*value)
        elif key == 'border_color':
            if isinstance(value, str) and value.startswith('#'):
                r, g, b = _hex_to_rgb(value)
                s.setBorderColor(r, g, b)
            elif isinstance(value, (tuple, list)):
                s.setBorderColor(*value)
        elif key == 'border_width':
            s.borderWidth = value
    return s


def _style_from_dict(style_dict):
    """
    将 Python dict 转换为 DANodeStyle 实例

    支持的键名（snake_case）：
    - background_color: 背景色十六进制字符串
    - border_color: 边框色十六进制字符串
    - border_width: 边框宽度（float）
    - body_shape: 主体形状（BodyShape 枚举值）
    - name_position: 名称位置（NamePosition 枚举值）
    - icon_position: 图标位置（IconPosition 枚举值）
    - corner_radius: 圆角半径（float）
    - icon_size: 图标尺寸（float）
    - input_port_side: 输入端口方位（PortSide 枚举值）
    - output_port_side: 输出端口方位（PortSide 枚举值）
    - input_port_style: 输入端口样式（dict 或 DAPyLinkPointStyle）
    - output_port_style: 输出端口样式（dict 或 DAPyLinkPointStyle）
    - layout_strategy: 连接点布局策略（LinkPointLayoutStrategy 枚举值）
    - body_icon_type: 节点体图标类型（BodyIconType 枚举值）
    - body_icon_source: 图标源路径（str）
    - body_icon_scale: 图标缩放比例（float）

    :param style_dict: 样式配置字典
    :return: DANodeStyle 实例
    """
    s = da_py_workflow.DANodeStyle()
    for key, value in style_dict.items():
        if key == 'background_color':
            if isinstance(value, str) and value.startswith('#'):
                r, g, b = _hex_to_rgb(value)
                s.setBackgroundColor(r, g, b)
            elif isinstance(value, (tuple, list)):
                s.setBackgroundColor(*value)
        elif key == 'border_color':
            if isinstance(value, str) and value.startswith('#'):
                r, g, b = _hex_to_rgb(value)
                s.setBorderColor(r, g, b)
            elif isinstance(value, (tuple, list)):
                s.setBorderColor(*value)
        elif key == 'border_width':
            s.borderWidth = value
        elif key == 'body_shape':
            s.bodyShape = value
        elif key == 'name_position':
            s.namePosition = value
        elif key == 'icon_position':
            s.iconPosition = value
        elif key == 'corner_radius':
            s.cornerRadius = value
        elif key == 'icon_size':
            s.iconSize = value
        elif key == 'input_port_side':
            s.inputPortSide = value
        elif key == 'output_port_side':
            s.outputPortSide = value
        elif key == 'input_port_style':
            if isinstance(value, dict):
                s.inputPortStyle = _link_point_style_from_dict(value)
            else:
                s.inputPortStyle = value
        elif key == 'output_port_style':
            if isinstance(value, dict):
                s.outputPortStyle = _link_point_style_from_dict(value)
            else:
                s.outputPortStyle = value
        elif key == 'layout_strategy':
            s.layoutStrategy = value
        elif key == 'body_icon_type':
            s.bodyIconType = value
        elif key == 'body_icon_source':
            s.bodyIconSource = value
        elif key == 'body_icon_scale':
            s.bodyIconScale = value
    return s


def NodeDef(name: str, category: str = "", render_template: str = "nodestyle", icon:str = "" ,style=None):
    """
    工作流节点定义装饰器

    此装饰器用于声明工作流节点类型。它会收集被装饰类中的 Input、Output、Parameter
    声明，并自动在类上设置 _node_descriptor C++ DANodeDescriptor 结构体属性。

    _node_descriptor 是一个 da_py_workflow.DANodeDescriptor 结构体，包含以下字段：
    - name: 节点显示名称
    - qualifiedName: 节点的唯一标识（模块名.类名）
    - category: 节点所属分类
    - inputs: 输入端口列表（DAPortDescriptor 结构体）
    - outputs: 输出端口列表（DAPortDescriptor 结构体）
    - parameters: 参数列表（DAParameterDescriptor 结构体）
    - renderTemplate: 渲染模板类型（RenderTemplate 枚举）
    - style: 节点样式配置（DANodeStyle 结构体）

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
    :param render_template: 渲染模板类型，默认为 'rect'（映射到 NodeStyleTemplate），支持 'nodestyle'、'widget'
    :param style: 节点样式配置，可为样式参数字典或 DANodeStyle 实例，默认为 None（使用默认 DANodeStyle）
    :return: 装饰器函数
    """
    rt_enum = _normalize_render_template(render_template)

    def decorator(cls: type) -> type:
        """
        节点定义装饰器的内部函数

        收集类中的 Input、Output、Parameter 声明，
        生成 _node_descriptor C++ DANodeDescriptor 结构体并设置到类上。

        :param cls: 被装饰的节点类
        :return: 被装饰后的类（原地修改，添加 _node_descriptor 属性）
        """
        # 收集参数声明（DAParameterDescriptor 结构体列表）
        parameters = _collect_parameters(cls)

        # 收集输入端口声明（DAPortDescriptor 结构体列表）
        inputs = _collect_from_nested_class(cls, "Inputs", Input)

        # 收集输出端口声明（DAPortDescriptor 结构体列表）
        outputs = _collect_from_nested_class(cls, "Outputs", Output)

        # 生成唯一标识：模块名.类名
        qualified_name = cls.__module__ + "." + cls.__name__

        # 构建节点描述符 C++ 结构体
        desc = da_py_workflow.DANodeDescriptor()
        desc.name = name
        desc.qualifiedName = qualified_name
        desc.category = category
        desc.icon = icon
        desc.renderTemplate = rt_enum
        desc.setInputs(inputs)
        desc.setOutputs(outputs)
        desc.setParameters(parameters)

        # 处理样式参数
        if style is not None:
            if isinstance(style, dict):
                desc.style = _style_from_dict(style)
            elif isinstance(style, da_py_workflow.DANodeStyle):
                # style 已经是 DANodeStyle 实例，直接赋值
                desc.style = style

        # 在类上设置 _node_descriptor 属性（C++ DANodeDescriptor 结构体）
        cls._node_descriptor = desc

        # 设置 input_keys 和 output_keys 为类属性，供 C++ syncMetaFromPyNode 使用
        cls.input_keys = [inp.name for inp in inputs]
        cls.output_keys = [outp.name for outp in outputs]

        @classmethod
        def _get_descriptor(cls_):
            """
            获取节点描述符 C++ 结构体

            :return: da_py_workflow.DANodeDescriptor 实例，若未设置则返回 None
            """
            return getattr(cls_, "_node_descriptor", None)

        cls.get_descriptor = _get_descriptor

        return cls

    return decorator
