"""
工作流节点注册表模块

本模块定义了 DANodeRegistry 类，用于管理工作流节点类型的注册和发现。
节点注册表维护所有已注册节点类型的索引，以 qualified_name 为键。

支持两种节点发现模式：
- 目录扫描：扫描指定路径下的 .py 文件，导入模块，查找 @NodeDef 装饰的类
- 入口点发现：通过 importlib.metadata.entry_points 查找已安装的插件包

主要功能：
- register_node(): 注册一个节点类
- discover(): 从指定路径和入口点发现节点类（双模式）
- get_all_descriptors(): 获取所有已注册节点类
- get_descriptor(): 根据 qualified_name 获取指定节点类
"""

import importlib
import importlib.util
import importlib.metadata
import logging
import sys
from pathlib import Path

logger = logging.getLogger("DAWorkFlowPy.node_registry")

# 入口点分组名称
ENTRY_POINT_GROUP = "data_workbench.plugin"


class DANodeRegistry:
    """
    工作流节点注册表

    管理所有已注册的节点类型，维护节点类（继承 DAWorkflowNode）的索引。
    节点通过 qualified_name（模块名.类名）作为唯一标识进行索引。

    支持两种发现模式：

    1. 目录扫描模式：遍历指定目录中的 .py 文件，动态导入模块，
       检查类是否带有 qualified_name 属性（由 @NodeDef 装饰器设置），
       自动注册发现的节点类。

    2. 入口点模式：通过 importlib.metadata.entry_points 查找
       group='data_workbench.plugin' 的入口点，导入入口点指定的模块，
       检查模块中的类是否带有 qualified_name 属性。

    使用示例::

        registry = DANodeRegistry()

        # 双模式发现：目录扫描 + 入口点
        node_classes = registry.discover(scan_paths=["/path/to/plugins"], use_entry_points=True)

        # 仅目录扫描
        node_classes = registry.discover(scan_paths=["/path/to/plugins"])

        # 仅入口点发现
        node_classes = registry.discover(use_entry_points=True)

        # 注册节点类
        @NodeDef(name="Data Filter", category="Data Processing")
        class DataFilter:
            ...
        registry.register_node(DataFilter)

        # 获取所有已注册节点类
        node_classes = registry.get_all_descriptors()

        # 获取特定节点类
        node_cls = registry.get_descriptor("my_module.DataFilter")
    """

    def __init__(self):
        # 以 qualified_name 为键，节点类（type）为值
        self._registry: dict = {}

    def register_node(self, node_class: type) -> type:
        """
        注册一个节点类

        从被 NodeDef 装饰的节点类中提取 qualified_name 并注册到注册表中。
        如果节点类没有 qualified_name 属性，将抛出异常。
        如果 qualified_name 已被注册，将跳过（用于去重）。

        节点类是由 @NodeDef 装饰器修饰的 Python 类（type 对象），
        注册后可通过 qualified_name 查询。节点类具有以下由 @NodeDef 注入的属性：
        - qualified_name: str，节点的唯一类型标识（模块名.类名）
        - name: str，节点的显示名称
        - category: str，节点的分类路径
        - description: str，节点的功能描述
        - inputs: list[dict]，输入端口声明列表
        - outputs: list[dict]，输出端口声明列表
        - parameters: list[dict]，参数声明列表
        - is_global: bool，是否为全局节点

        :param node_class: 被 NodeDef 装饰的节点类（type 对象，需具有 qualified_name 属性）
        :return: 注册成功的节点类（type 对象）。若 qualified_name 已存在，返回先前注册的同类对象
        :rtype: type
        :raises ValueError: 如果节点类没有 qualified_name 属性
        """
        qualified_name = getattr(node_class, "qualified_name", None)
        if not qualified_name:
            raise ValueError(
                f"Class {node_class.__name__} has no qualified_name attribute, "
                "please declare the node type with the NodeDef decorator first"
            )

        if qualified_name in self._registry:
            logger.info(
                f"节点 '{qualified_name}' 已注册，跳过重复注册"
            )
            return self._registry[qualified_name]

        self._registry[qualified_name] = node_class
        logger.debug(f"注册节点 '{qualified_name}'")
        return node_class

    def discover(self, scan_paths: list[str] = None, use_entry_points: bool = False) -> list[type]:
        """
        从指定路径和入口点发现并注册节点类

        此方法支持双模式发现节点类：

        1. 目录扫描模式（scan_paths）：
           - 遍历 scan_paths 中的目录
           - 查找 .py 文件（排除 __pycache__ 目录和 __init__.py）
           - 动态导入每个模块
           - 检查模块中的类是否带有 qualified_name 属性
           - 调用 register_node() 注册发现的节点

        2. 入口点发现模式（use_entry_points）：
           - 使用 importlib.metadata.entry_points(group='data_workbench.plugin')
           - 加载入口点指定的模块
           - 检查模块中的类是否带有 qualified_name 属性
           - 调用 register_node() 注册发现的节点

        两种模式的结果会进行去重：相同 qualified_name 的节点只注册一次。

        :param scan_paths: 要扫描的目录绝对路径列表，每个元素为 str 类型的目录路径。
            默认为 None（不执行目录扫描）
        :param use_entry_points: 是否通过 importlib.metadata.entry_points 发现已安装的插件节点，默认为 False
        :return: 去重后的节点类列表，每个元素为被 @NodeDef 装饰的 type 对象，
            拥有 qualified_name、name、category、inputs、outputs、parameters 等属性。
            列表中的节点类同时已被注册到注册表中（已调用 register_node）
        :rtype: list[type]
        """
        discovered = []

        # 目录扫描模式
        if scan_paths:
            for path in scan_paths:
                scan_classes = self._scan_directory(path)
                discovered.extend(scan_classes)

        # 入口点发现模式
        if use_entry_points:
            ep_classes = self._discover_from_entry_points()
            discovered.extend(ep_classes)

        # 去重：已注册过的节点不会重复添加
        unique_discovered = []
        seen = set()
        for node_cls in discovered:
            qn = getattr(node_cls, "qualified_name", None)
            if qn and qn not in seen:
                seen.add(qn)
                unique_discovered.append(node_cls)

        return unique_discovered

    def _scan_directory(self, directory: str) -> list[type]:
        """
        扫描目录中的 Python 模块，查找 @NodeDef 装饰的节点类

        遍历目录下所有 .py 文件（排除 __pycache__ 和 __init__.py 等），
        动态导入模块并查找带有 qualified_name 属性的类。

        :param directory: 要扫描的目录绝对路径
        :return: 发现的节点类列表，每个元素为被 @NodeDef 装饰的 type 对象
        :rtype: list[type]
        """
        discovered = []
        dir_path = Path(directory)

        if not dir_path.is_dir():
            logger.warning(f"扫描路径 '{directory}' 不是有效目录，跳过")
            return discovered

        # 遍历目录中的 .py 文件
        for py_file in dir_path.rglob("*.py"):
            # 排除 __pycache__ 目录
            if "__pycache__" in py_file.parts:
                continue
            # 排除 __init__.py、setup.py、conftest.py 等非节点文件
            if py_file.name in ("__init__.py", "setup.py", "conftest.py"):
                continue

            module_name = _module_name_from_path(py_file, dir_path)
            try:
                module = _import_module_from_file(module_name, str(py_file))
                if module is None:
                    continue
                found = self._find_node_classes_in_module(module)
                discovered.extend(found)
            except Exception as e:
                logger.warning(
                    f"导入模块 '{module_name}' (文件: {py_file}) 失败: {e}"
                )

        return discovered

    def _discover_from_entry_points(self) -> list[type]:
        """
        通过 importlib.metadata.entry_points 发现节点类

        使用 entry_points(group='data_workbench.plugin') 查找已安装的插件包，
        加载入口点指定的模块，查找 @NodeDef 装饰的节点类。

        入口点可以指向模块（则扫描模块中所有类）或直接指向类（则直接注册该类）。

        :return: 发现的节点类列表，每个元素为被 @NodeDef 装饰的 type 对象
        :rtype: list[type]
        """
        discovered = []

        try:
            # Python 3.12+ 返回 SelectableGroups，Python 3.9- 返回 dict
            eps = importlib.metadata.entry_points()
            if hasattr(eps, "select"):
                # Python 3.12+ 风格
                group_eps = eps.select(group=ENTRY_POINT_GROUP)
            else:
                # Python 3.9- 风格
                group_eps = eps.get(ENTRY_POINT_GROUP, [])

            for ep in group_eps:
                try:
                    module = ep.load()
                    # 如果入口点加载的是模块，直接扫描模块
                    if isinstance(module, type):
                        # 入口点直接指向类
                        found = self._try_register_class(module)
                        if found:
                            discovered.append(found)
                    else:
                        # 入口点指向模块，扫描模块中的类
                        found = self._find_node_classes_in_module(module)
                        discovered.extend(found)
                except Exception as e:
                    logger.warning(
                        f"加载入口点 '{ep.name}' 失败: {e}"
                    )
        except Exception as e:
            logger.warning(f"查询 entry_points 失败: {e}")

        return discovered

    def _find_node_classes_in_module(self, module) -> list[type]:
        """
        在模块中查找带有 qualified_name 属性的类

        遍历模块的所有属性，找出带有 qualified_name 属性的类对象（即被 @NodeDef 装饰的节点类），
        并尝试注册到注册表中。

        :param module: Python 模块对象（由 importlib 导入的模块实例）
        :return: 发现并注册的节点类列表，每个元素为被 @NodeDef 装饰的 type 对象
        :rtype: list[type]
        """
        discovered = []
        for attr_name in dir(module):
            if attr_name.startswith("_"):
                continue
            attr_value = getattr(module, attr_name, None)
            if not isinstance(attr_value, type):
                continue
            # 检查是否带有 qualified_name 属性
            if hasattr(attr_value, "qualified_name") and attr_value.qualified_name:
                desc = self._try_register_class(attr_value)
                if desc:
                    discovered.append(desc)
        return discovered

    def _try_register_class(self, node_class: type) -> type | None:
        """
        尝试注册一个节点类，如果失败则返回 None

        :param node_class: 带有 qualified_name 属性的节点类（type 对象）
        :return: 注册成功返回节点类（type 对象），失败或无 qualified_name 时返回 None
        :rtype: type | None
        """
        try:
            return self.register_node(node_class)
        except ValueError:
            # 没有 qualified_name 属性，跳过
            return None
        except KeyError:
            # 已注册，register_node 内部已处理去重
            qname = getattr(node_class, "qualified_name", "")
            return self._registry.get(qname, None)

    def get_all_descriptors(self) -> list[type]:
        """
        获取所有已注册节点类

        :return: 所有已注册节点类的列表，每个元素为被 @NodeDef 装饰的 type 对象，
            拥有 qualified_name、name、category、inputs、outputs、parameters 等属性
        :rtype: list[type]
        """
        return list(self._registry.values())

    def get_descriptor(self, qualified_name: str) -> type:
        """
        根据 qualified_name 获取指定节点类

        :param qualified_name: 节点的唯一类型标识（格式为 "模块名.类名"，如 "my_module.DataFilter")
        :return: 对应的节点类（type 对象），拥有 qualified_name、name、category、inputs、outputs、parameters 等属性
        :rtype: type
        :raises KeyError: 如果 qualified_name 未注册
        """
        if qualified_name not in self._registry:
            raise KeyError(f"Node '{qualified_name}' is not registered")
        return self._registry[qualified_name]

    def unregister_node(self, qualified_name: str) -> type:
        """
        从注册表中移除指定节点

        :param qualified_name: 节点的唯一类型标识（格式为 "模块名.类名"）
        :return: 移除的节点类（type 对象）
        :rtype: type
        :raises KeyError: 如果 qualified_name 未注册
        """
        if qualified_name not in self._registry:
            raise KeyError(f"Node '{qualified_name}' is not registered")
        return self._registry.pop(qualified_name)

    def clear(self):
        """
        清空注册表中的所有节点
        """
        self._registry.clear()

    def __len__(self) -> int:
        return len(self._registry)

    def __contains__(self, qualified_name: str) -> bool:
        return qualified_name in self._registry

    def __repr__(self) -> str:
        return f"DANodeRegistry(nodes={len(self._registry)})"


def _module_name_from_path(py_file: Path, base_dir: Path) -> str:
    """
    从文件路径生成模块名

    根据文件相对于基础目录的路径，生成符合 Python 模块命名规范的模块名。

    :param py_file: Python 文件路径
    :param base_dir: 基础目录路径
    :return: 模块名（如 'pkg.sub.module'）
    """
    relative = py_file.relative_to(base_dir)
    # 将路径中的 / 替换为 .，去掉 .py 后缀
    parts = list(relative.parts)
    # 去掉 .py 后缀（使用 endswith 检查 + 切片，避免 rstrip 误删字符）
    filename = parts[-1]
    if filename.endswith(".py"):
        parts[-1] = filename[:-3]
    return ".".join(parts)


def _import_module_from_file(module_name: str, file_path: str):
    """
    从文件路径动态导入 Python 模块

    使用 importlib.util.spec_from_file_location 创建模块规格，
    再使用 importlib.util.module_from_spec 创建模块对象并执行。

    :param module_name: 模块名
    :param file_path: 文件绝对路径
    :return: 导入的模块对象，如果失败返回 None
    """
    try:
        spec = importlib.util.spec_from_file_location(module_name, file_path)
        if spec is None or spec.loader is None:
            logger.warning(f"无法创建模块 '{module_name}' 的规格")
            return None
        module = importlib.util.module_from_spec(spec)
        # 将模块添加到 sys.modules，以便模块内相对导入能正常工作
        sys.modules[module_name] = module
        spec.loader.exec_module(module)
        return module
    except Exception as e:
        logger.warning(f"执行模块 '{module_name}' (文件: {file_path}) 失败: {e}")
        # 清理 sys.modules 中失败的模块
        if module_name in sys.modules:
            del sys.modules[module_name]
        return None