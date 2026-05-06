"""
工作流节点描述符模块

本模块定义了 DANodeDescriptor 类，用于存储节点的完整元数据信息。
DANodeDescriptor 是节点类型定义的序列化载体，通过 to_dict() 方法
生成 JSON 可序列化的字典，供 C++ 侧通过 DAPyJsonCast 读取。

节点描述符包含以下信息：
- name: 节点显示名称
- category: 节点所属分类
- qualified_name: 节点的唯一标识（模块名+类名）
- inputs: 输入端口列表
- outputs: 输出端口列表
- parameters: 参数列表
- render_template: 渲染模板类型（'nodestyle'、'widget'）
"""

from .types import Input, Output, Parameter


class DANodeDescriptor:
    """
    工作流节点描述符

    存储节点类型的完整元数据，是 NodeDef 装饰器收集声明后生成的数据对象。
    通过 to_dict() 方法可生成 JSON 可序列化的字典，传递给 C++ 侧的 DAPyJsonCast。

    使用示例::

        descriptor = DANodeDescriptor(
            name="Data Filter",
            category="Data Processing",
            qualified_name="my_module.DataFilter",
            inputs=[{"name": "data", "data_type": "DataFrame", "required": True, "description": ""}],
            outputs=[{"name": "filtered", "data_type": "DataFrame", "description": ""}],
            parameters=[{"name": "column", "type": "str", "default": "value", "description": "列名"}],
            render_template="rect",
        )
        # 序列化为字典
        d = descriptor.to_dict()

    :param name: 节点显示名称
    :param category: 节点所属分类
    :param qualified_name: 节点唯一标识（模块名.类名）
    :param inputs: 输入端口信息列表（每个元素为 dict）
    :param outputs: 输出端口信息列表（每个元素为 dict）
    :param parameters: 参数信息列表（每个元素为 dict）
    :param render_template: 渲染模板类型，默认为 'nodestyle'
    """

    # 支持的渲染模板类型
    VALID_RENDER_TEMPLATES = ("nodestyle", "widget")

    def __init__(
        self,
        name: str,
        category: str = "",
        qualified_name: str = "",
        inputs: list = None,
        outputs: list = None,
        parameters: list = None,
        render_template: str = "nodestyle",
        style: dict = None,
    ):
        self.name = name
        self.category = category
        self.qualified_name = qualified_name
        self.inputs = inputs if inputs is not None else []
        self.outputs = outputs if outputs is not None else []
        self.parameters = parameters if parameters is not None else []
        self.render_template = render_template
        self.style = style  # 样式参数字典

        # 校验渲染模板
        if self.render_template not in self.VALID_RENDER_TEMPLATES:
            raise ValueError(
                f"无效的渲染模板 '{self.render_template}'，"
                f"支持的模板类型为: {', '.join(self.VALID_RENDER_TEMPLATES)}"
            )

    def to_dict(self) -> dict:
        """
        将节点描述符转换为 JSON 可序列化的字典

        生成的字典所有值均为 JSON 兼容类型（str、int、float、bool、list、dict、None），
        可直接传递给 C++ 侧通过 DAPyJsonCast (QJsonObject ↔ py::dict) 进行转换。

        :return: JSON 可序列化的字典，包含节点的完整元数据
        """
        d = {
            "name": self.name,
            "category": self.category,
            "qualified_name": self.qualified_name,
            "inputs": self.inputs,
            "outputs": self.outputs,
            "parameters": self.parameters,
            "render_template": self.render_template,
        }
        if self.style is not None:
            d["style"] = self.style
        return d

    @classmethod
    def from_class(cls, node_class: type, **kwargs) -> "DANodeDescriptor":
        """
        从带有 _node_descriptor 属性的节点类创建描述符

        NodeDef 装饰器会在被装饰的类上设置 _node_descriptor 属性，
        此方法从该属性中提取信息创建 DANodeDescriptor 实例。

        :param node_class: 被 NodeDef 装饰的节点类
        :param kwargs: 可覆盖的额外参数
        :return: DANodeDescriptor 实例
        """
        descriptor_data = getattr(node_class, "_node_descriptor", {})
        if not descriptor_data:
            raise ValueError(
                f"类 {node_class.__name__} 没有 _node_descriptor 属性，请先使用 NodeDef 装饰器"
            )
        # 合并覆盖参数
        for key, value in kwargs.items():
            descriptor_data[key] = value
        return cls(**descriptor_data)

    def __repr__(self) -> str:
        return (
            f"DANodeDescriptor(name='{self.name}', category='{self.category}', "
            f"qualified_name='{self.qualified_name}', "
            f"inputs={len(self.inputs)}, outputs={len(self.outputs)}, "
            f"parameters={len(self.parameters)}, "
            f"render_template='{self.render_template}')"
        )
