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

    def discover(self, scan_paths=None, use_entry_points=False):
        """
        发现并注册节点类

        委托给内部 DANodeRegistry 的 discover() 方法，
        从指定路径和入口点发现节点类并注册。

        :param scan_paths: 要扫描的目录路径列表，默认为 None
        :param use_entry_points: 是否使用 entry_points 发现节点，默认为 False
        :return: 发现并注册的节点类列表
        """
        return self._registry.discover(scan_paths=scan_paths, use_entry_points=use_entry_points)

    def create_node(self, qualified_name):
        """
        通过 qualified_name 创建节点实例

        从内部注册表中获取节点类，实例化并返回。
        节点类的 qualified_name 必须已通过 discover() 注册。

        :param qualified_name: 节点的唯一标识（模块名.类名）
        :return: 节点实例对象
        :raises KeyError: 如果 qualified_name 未注册
        """
        node_cls = self._registry.get_descriptor(qualified_name)
        return node_cls()

    def get_node_metadata(self, qualified_name):
        """
        从节点类读取元数据，返回 dict

        从内部注册表中获取节点类，读取类属性中的元数据字段，
        返回包含 name、qualified_name、category、icon、input_keys、output_keys 的 dict。
        C++ 侧通过 pybind11 调用此方法获取 Python 节点元数据。

        :param qualified_name: 节点的唯一标识
        :return: 元数据 dict，包含 name、qualified_name、category、icon、input_keys、output_keys
        :raises KeyError: 如果 qualified_name 未注册
        """
        node_cls = self._registry.get_descriptor(qualified_name)
        return _extract_metadata_from_class(node_cls)

    def get_all_metadata(self):
        """
        获取所有已注册节点的元数据列表

        遍历内部注册表中所有节点类，从类属性读取元数据，
        返回元数据 dict 列表。C++ 侧通过 pybind11 调用此方法
        获取所有 Python 节点的元数据信息。

        :return: 元数据 dict 列表，每个 dict 包含 name、qualified_name、category、icon、input_keys、output_keys
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


def _extract_metadata_from_class(node_cls):
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

    :param node_cls: 被 @NodeDef 装饰的节点类
    :return: 元数据 dict
    """
    metadata = {}
    # qualified_name 是节点唯一标识，必须存在
    metadata["qualified_name"] = getattr(node_cls, "qualified_name", "")
    metadata["name"] = getattr(node_cls, "name", "")
    # category 优先，group 作为兼容别名
    category = getattr(node_cls, "category", "")
    if category:
        metadata["category"] = category
    else:
        metadata["category"] = getattr(node_cls, "group", "")
    metadata["icon"] = getattr(node_cls, "icon", "")
    metadata["input_keys"] = list(getattr(node_cls, "input_keys", []))
    metadata["output_keys"] = list(getattr(node_cls, "output_keys", []))
    return metadata