"""
节点工厂模块

本模块定义了 DANodeFactory 类，封装 DANodeRegistry 的发现和实例化功能。
DANodeFactory 是 C++ 侧 DAPyNodeFactory 的 Python 对应物，
C++ 侧通过 pybind11 调用 DANodeFactory 的方法完成节点发现和创建。

主要功能：
- discover(): 发现并注册节点类（委托给 DANodeRegistry）
- create_node(): 通过 qualified_name 创建节点实例
- get_node_metadata(): 从节点类读取元数据 dict
- get_all_metadata(): 获取所有已注册节点的元数据列表

使用示例::

    factory = DANodeFactory()
    node_classes = factory.discover(scan_paths=["/path/to/plugins"], use_entry_points=True)

    # 创建节点实例
    node = factory.create_node("my_module.DataFilter")

    # 获取元数据
    metadata = factory.get_node_metadata("my_module.DataFilter")
    all_metadata = factory.get_all_metadata()
"""

from .node_registry import DANodeRegistry


class DANodeFactory:
    """
    节点工厂

    封装 DANodeRegistry 的发现和实例化功能，供 C++ 侧 DAPyNodeFactory 调用。
    C++ 侧通过 pybind11 获取 DANodeFactory 类引用，创建实例，
    然后调用 discover() 和 create_node() 方法完成节点发现和创建。

    职责：
    - discover(): 委托给 DANodeRegistry.discover()，返回发现的节点类列表
    - create_node(): 通过 DANodeRegistry.get_descriptor() 获取类，实例化返回
    - get_node_metadata(): 从节点类属性直接读取元数据，返回 dict
    - get_all_metadata(): 遍历所有已注册节点类，返回元数据 dict 列表

    使用示例::

        factory = DANodeFactory()
        node_classes = factory.discover(scan_paths=["/path/to/plugins"], use_entry_points=True)
        node = factory.create_node("my_module.DataFilter")
    """

    def __init__(self):
        self._registry = DANodeRegistry()

    def discover(self, scan_paths: list[str] = None, use_entry_points: bool = False) -> list[type]:
        """
        发现并注册节点类

        委托给内部 DANodeRegistry 的 discover() 方法，
        从指定路径和入口点发现节点类并注册。

        :param scan_paths: 要扫描的目录绝对路径列表，每个元素为 str 类型的目录路径。
            默认为 None（不执行目录扫描）
        :param use_entry_points: 是否通过 importlib.metadata.entry_points 发现已安装的插件节点，默认为 False
        :return: 去重后的节点类列表，每个元素为被 @NodeDef 装饰的 type 对象，
            拥有 qualified_name、name、category、inputs、outputs、parameters 等属性。
            列表中的节点类同时已被注册到内部注册表中
        :rtype: list[type]
        """
        return self._registry.discover(scan_paths=scan_paths, use_entry_points=use_entry_points)

    def create_node(self, qualified_name: str) -> object:
        """
        通过 qualified_name 创建节点实例

        从内部注册表中获取节点类，实例化并返回。
        节点类的 qualified_name 必须已通过 discover() 注册。

        返回的节点实例是由 @NodeDef 装饰的类实例，具有以下关键属性：
        - node_id: str，节点的唯一运行时标识（实例化时自动生成）
        - qualified_name: str，节点类型标识（与传入参数相同）
        - name: str，节点的显示名称
        - inputs: list[dict]，输入端口声明列表
        - outputs: list[dict]，输出端口声明列表
        - parameters: list[dict]，参数声明列表
        - execute(): 方法，执行节点的业务逻辑

        :param qualified_name: 节点的唯一类型标识（格式为 "模块名.类名"，如 "my_module.DataFilter")
        :return: @NodeDef 装饰的节点类实例，拥有 node_id、qualified_name、execute() 等属性和方法
        :raises KeyError: 如果 qualified_name 未注册
        """
        node_cls = self._registry.get_descriptor(qualified_name)
        return node_cls()

    def get_node_metadata(self, qualified_name: str) -> dict:
        """
        从节点类读取元数据，返回 dict

        从内部注册表中获取节点类，读取类属性中的元数据字段，
        返回包含 name、qualified_name、category、icon、input_keys、output_keys 的 dict。
        C++ 侧通过 pybind11 调用此方法获取 Python 节点元数据。

        返回的 dict 包含以下键值（所有值均为 str 或 list[str] 类型）：
        - qualified_name: str，节点类型唯一标识
        - name: str，节点显示名称
        - category: str，节点分类路径
        - icon: str，节点图标路径
        - input_keys: list[str]，输入端口名称列表
        - output_keys: list[str]，输出端口名称列表

        :param qualified_name: 节点的唯一类型标识（格式为 "模块名.类名"）
        :return: 元数据字典，键为 str，值为 str 或 list[str]
        :rtype: dict[str, str | list[str]]
        :raises KeyError: 如果 qualified_name 未注册
        """
        node_cls = self._registry.get_descriptor(qualified_name)
        return _extract_metadata_from_class(node_cls)

    def get_all_metadata(self) -> list[dict]:
        """
        获取所有已注册节点的元数据列表

        遍历内部注册表中所有节点类，从类属性读取元数据，
        返回元数据 dict 列表。C++ 侧通过 pybind11 调用此方法
        获取所有 Python 节点的元数据信息。

        :return: 元数据字典列表，每个元素为 dict，包含以下键值：
            - qualified_name: str，节点类型唯一标识
            - name: str，节点显示名称
            - category: str，节点分类路径
            - icon: str，节点图标路径
            - input_keys: list[str]，输入端口名称列表
            - output_keys: list[str]，输出端口名称列表
        :rtype: list[dict[str, str | list[str]]
        """
        result = []
        for node_cls in self._registry.get_all_descriptors():
            metadata = _extract_metadata_from_class(node_cls)
            if metadata and metadata.get("qualified_name"):
                result.append(metadata)
        return result

    def get_registry(self):
        """
        获取内部注册表引用

        :return: DANodeRegistry 实例
        """
        return self._registry

    def __repr__(self):
        return f"DANodeFactory(nodes={len(self._registry)})"


def _extract_metadata_from_class(node_cls: type) -> dict:
    """
    从节点类属性中提取元数据 dict

    读取 @NodeDef 装饰器设置的类属性，构建元数据 dict。
    dict 的 key 与 C++ DAPyNodeMetaData 的字段一一对应：
    - name → DAPyNodeMetaData::name
    - qualified_name → DAPyNodeMetaData::qualifiedName
    - category → DAPyNodeMetaData::group
    - icon → DAPyNodeMetaData::iconPath
    - input_keys → DAPyNodeMetaData::inputKeys
    - output_keys → DAPyNodeMetaData::outputKeys

    :param node_cls: 被 @NodeDef 装饰的节点类（type 对象）
    :return: 元数据字典，键为 str，值为 str 或 list[str]。
        包含 qualified_name、name、category、icon、input_keys、output_keys
    :rtype: dict[str, str | list[str]]
    """
    metadata = {}
    # qualified_name 是节点唯一标识，必须存在
    metadata["qualified_name"] = getattr(node_cls, "qualified_name", "")
    metadata["name"] = getattr(node_cls, "name", "")
    metadata["category"] = getattr(node_cls, "category", "")
    metadata["icon"] = getattr(node_cls, "icon", "")
    metadata["input_keys"] = list(getattr(node_cls, "input_keys", []))
    metadata["output_keys"] = list(getattr(node_cls, "output_keys", []))
    metadata["node_description"] = getattr(node_cls, "__node_description", "")
    return metadata