"""
工作流节点类型声明模块

本模块定义了工作流节点的输入、输出和参数声明类型。
这些声明类型用于在 NodeDef 装饰器中描述节点的接口信息。

- Input: 节点输入端口声明
- Output: 节点输出端口声明
- Parameter: 节点参数声明
"""


class Input:
    """
    节点输入端口声明

    用于声明节点的输入端口，指定输入数据的类型、是否必填以及描述信息。

    使用示例::

        class Inputs:
            data = Input("DataFrame", required=True)
            config = Input("dict", required=False, description="配置信息")

    :param data_type: 数据类型标签（字符串，如 "DataFrame"、"int"、"str" 等）
    :param required: 是否为必填输入，默认为 True
    :param description: 输入端口的描述信息
    """

    def __init__(self, data_type: str, required: bool = True, description: str = ""):
        # 注意：name 属性在 NodeDef 装饰器处理时通过类属性名自动设置
        self.data_type = data_type
        self.required = required
        self.description = description

    def to_dict(self, name: str) -> dict:
        """
        将输入端口声明转换为字典

        :param name: 输入端口的名称（从类属性名获取）
        :return: JSON 可序列化的字典
        """
        return {
            "name": name,
            "data_type": self.data_type,
            "required": self.required,
            "description": self.description,
        }

    def to_port_descriptor(self, name: str):
        """
        将输入端口声明转换为 C++ DAPortDescriptor 结构

        :param name: 输入端口的名称（从类属性名获取）
        :return: da_py_workflow.DAPortDescriptor 实例
        """
        import da_py_workflow

        pd = da_py_workflow.DAPortDescriptor()
        pd.name = name
        pd.dataType = self.data_type
        pd.required = self.required
        pd.description = self.description
        return pd

    def __repr__(self) -> str:
        return f"Input(data_type='{self.data_type}', required={self.required}, description='{self.description}')"


class Output:
    """
    节点输出端口声明

    用于声明节点的输出端口，指定输出数据的类型和描述信息。

    使用示例::

        class Outputs:
            result = Output("DataFrame")
            report = Output("str", description="分析报告")

    :param data_type: 数据类型标签（字符串，如 "DataFrame"、"int"、"str" 等）
    :param description: 输出端口的描述信息
    """

    def __init__(self, data_type: str, description: str = ""):
        # 注意：name 属性在 NodeDef 装饰器处理时通过类属性名自动设置
        self.data_type = data_type
        self.description = description

    def to_dict(self, name: str) -> dict:
        """
        将输出端口声明转换为字典

        :param name: 输出端口的名称（从类属性名获取）
        :return: JSON 可序列化的字典
        """
        return {
            "name": name,
            "data_type": self.data_type,
            "description": self.description,
        }

    def to_port_descriptor(self, name: str):
        """
        将输出端口声明转换为 C++ DAPortDescriptor 结构

        :param name: 输出端口的名称（从类属性名获取）
        :return: da_py_workflow.DAPortDescriptor 实例
        """
        import da_py_workflow

        pd = da_py_workflow.DAPortDescriptor()
        pd.name = name
        pd.dataType = self.data_type
        pd.required = False  # 输出端口不需要 required
        pd.description = self.description
        return pd

    def __repr__(self) -> str:
        return f"Output(data_type='{self.data_type}', description='{self.description}')"


class Parameter:
    """
    节点参数声明

    用于声明节点的可配置参数，指定参数的类型、默认值和描述信息。
    参数不同于输入端口，参数是节点自身的配置项，在执行前设置。

    使用示例::

        @NodeDef(name="Data Filter", category="Data Processing")
        class DataFilter:
            column = Parameter(str, default="value", description="要筛选的列名")
            threshold = Parameter(float, default=0.0, description="筛选阈值")

    :param param_type: 参数的 Python 类型（如 str、int、float、bool 等）或字符串类型标签（如 "file"、"enum" 等）
    :param default: 参数的默认值，默认为 None 表示无默认值
    :param description: 参数的描述信息
    :param kwargs: 扩展字段，用于支持额外属性（如 file_filter、enum_options 等）
    """

    # 支持的参数类型到字符串标签的映射
    _TYPE_LABELS = {
        str: "str",
        int: "int",
        float: "float",
        bool: "bool",
        list: "list",
        dict: "dict",
        # Extended types from DAParamTypeRegistry
        "file": "file",
        "folder": "folder",
        "enum": "enum",
        "color": "color",
        "font": "font",
        "code": "code",
    }

    def __init__(self, param_type, default=None, description: str = "", **kwargs):
        # 注意：name 属性在 NodeDef 装饰器处理时通过类属性名自动设置
        self.param_type = param_type
        self.default = default
        self.description = description
        self._extra_kwargs = kwargs

    def get_type_label(self) -> str:
        """
        获取参数类型的字符串标签

        如果参数类型在预定义映射中，返回对应标签；
        否则返回类型的 __name__ 属性。

        :return: 类型标签字符串
        """
        if isinstance(self.param_type, str):
            return self._TYPE_LABELS.get(self.param_type, self.param_type)
        return self._TYPE_LABELS.get(self.param_type, self.param_type.__name__)

    def to_dict(self, name: str) -> dict:
        """
        将参数声明转换为字典

        :param name: 参数的名称（从类属性名获取）
        :return: JSON 可序列化的字典
        """
        result = {
            "name": name,
            "type": self.get_type_label(),
            "description": self.description,
        }
        # 仅在 default 不为 None 时写入，避免将 None 与"无默认值"混淆
        if self.default is not None:
            result["default"] = self.default
        # Merge extended kwargs for extended parameter types
        result.update(self._extra_kwargs)
        return result

    def to_parameter_descriptor(self, name: str):
        """
        将参数声明转换为 C++ DAParameterDescriptor 结构

        :param name: 参数的名称（从类属性名获取）
        :return: da_py_workflow.DAParameterDescriptor 实例
        """
        import da_py_workflow

        pd = da_py_workflow.DAParameterDescriptor()
        pd.name = name
        pd.type = self.get_type_label()
        pd.description = self.description
        if self.default is not None:
            pd.setDefaultValue(self.default)
        # Build propertys dict from extra kwargs with key name mapping for C++ compatibility.
        # Python-facing kwarg names (e.g. file_filter, enum_options) are mapped to the C++
        # property keys that DAParameterDescriptor expects (filter, enum).
        _key_map = {
            "file_filter": "filter",
            "enum_options": "enum",
        }
        raw = {}
        for k, v in self._extra_kwargs.items():
            raw[_key_map.get(k, k)] = v
        if raw:
            pd.setRawDescriptor(raw)
        return pd

    def __repr__(self) -> str:
        default_str = f", default={self.default!r}" if self.default is not None else ""
        type_str = self.param_type if isinstance(self.param_type, str) else self.param_type.__name__
        return f"Parameter({type_str}{default_str}, description='{self.description}')"