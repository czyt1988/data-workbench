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
    :param min: 最小值（int/float 类型参数适用）
    :param max: 最大值（int/float 类型参数适用）
    :param step: 步长（int/float 类型参数适用）
    :param decimals: 小数位数（float 类型参数适用）
    :param enum: 枚举选项列表（"enum" 类型参数适用，如 ["csv", "json", "excel"]）
    :param filter: 文件过滤器（"file" 类型参数适用，如 "CSV Files (*.csv);;All Files (*.*)"）
    :param layout: 编辑器布局模式，"inline"（默认，属性名在左、编辑器在右）或 "below"（属性名在上、编辑器占满整行下方）。str 类型在 below 模式下自动切换为多行 QPlainTextEdit
    :param height: 编辑器高度（像素），仅 below 模式生效。str 类型默认 80，code 类型默认 100，未设置时使用类型默认值
    :param kwargs: 扩展字段，用于支持额外属性（键名需与 C++ DANodeParameterFormAdapter 读取的 attributes 键一致：min/max/step/decimals/filter/layout/height/enum）
    """

    # 支持的参数类型到字符串标签的映射
    _TYPE_LABELS = {
        str: "str",
        int: "int",
        float: "float",
        bool: "bool",
        list: "list",
        dict: "dict",
        # Extended types handled by DAFormEditorRegistry
        "file": "file",
        "folder": "folder",
        "enum": "enum",
        "color": "color",
        "font": "font",
        "code": "code",
    }

    # 支持的 layout 取值（小写归一化后校验）
    _LAYOUT_VALUES = {"inline", "below"}

    def __init__(
        self,
        param_type,
        default=None,
        description: str = "",
        min=None,
        max=None,
        step=None,
        decimals=None,
        enum=None,
        filter=None,
        layout: str = "inline",
        height=None,
        **kwargs,
    ):
        self.name = ""  # 由 NodeDef 装饰器通过类属性名设置
        self.param_type = param_type
        self.default = default
        self.description = description

        # 校验 layout 取值，归一化为小写
        layout_norm = str(layout).lower()
        if layout_norm not in self._LAYOUT_VALUES:
            raise ValueError(
                f"layout must be one of {sorted(self._LAYOUT_VALUES)}, got {layout!r}"
            )

        # 构建扩展属性 dict，键名与 C++ DANodeParameterFormAdapter 读取的 attributes 键一致
        self._extra_kwargs = {}
        if min is not None:
            self._extra_kwargs["min"] = min
        if max is not None:
            self._extra_kwargs["max"] = max
        if step is not None:
            self._extra_kwargs["step"] = step
        if decimals is not None:
            self._extra_kwargs["decimals"] = decimals
        if enum is not None:
            self._extra_kwargs["enum"] = enum
        if filter is not None:
            self._extra_kwargs["filter"] = filter
        self._extra_kwargs["layout"] = layout_norm
        if height is not None:
            self._extra_kwargs["height"] = int(height)
        # 保留 **kwargs 用于未来扩展
        self._extra_kwargs.update(kwargs)

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

    def to_dict(self, name: str = "") -> dict:
        """
        将参数声明转换为字典

        :param name: 参数的名称，为空时使用 self.name
        :return: JSON 可序列化的字典
        """
        result = {
            "name": name or self.name,
            "type": self.get_type_label(),
            "description": self.description,
        }
        # 仅在 default 不为 None 时写入，避免将 None 与"无默认值"混淆
        if self.default is not None:
            result["default"] = self.default
        # 扩展属性嵌套到 properties 子字典，由 C++ DANodeParameterFormAdapter 读取
        if self._extra_kwargs:
            result["properties"] = dict(self._extra_kwargs)
        return result

    def __repr__(self) -> str:
        default_str = f", default={self.default!r}" if self.default is not None else ""
        type_str = (
            self.param_type
            if isinstance(self.param_type, str)
            else self.param_type.__name__
        )
        return f"Parameter({type_str}{default_str}, description='{self.description}')"
