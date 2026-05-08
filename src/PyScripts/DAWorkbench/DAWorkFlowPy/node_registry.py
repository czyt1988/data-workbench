"""
工作流节点注册表模块

本模块定义了 DANodeRegistry 类，用于管理工作流节点类型的注册和发现。
节点注册表维护所有已注册节点类型的描述符，并提供查询接口。

支持两种节点发现模式：
- 目录扫描：扫描指定路径下的 .py 文件，导入模块，查找 @NodeDef 装饰的类
- 入口点发现：通过 importlib.metadata.entry_points 查找已安装的插件包

主要功能：
- register_node(): 注册一个节点类
- discover(): 从指定路径和入口点发现节点类（双模式）
- get_all_descriptors(): 获取所有已注册节点的描述符
- get_descriptor(): 根据 qualified_name 获取指定节点的描述符
"""

import importlib
import importlib.util
import importlib.metadata
import logging
import sys
from pathlib import Path

import da_py_workflow

logger = logging.getLogger("DAWorkFlowPy.node_registry")

# 入口点分组名称
ENTRY_POINT_GROUP = "data_workbench.plugin"


class DANodeRegistry:
    """
    工作流节点注册表

    管理所有已注册的节点类型，维护节点描述符的索引。
    节点通过 qualified_name（模块名.类名）作为唯一标识进行索引。

    支持两种发现模式：

    1. 目录扫描模式：遍历指定目录中的 .py 文件，动态导入模块，
       检查类是否带有 _node_descriptor 属性（由 @NodeDef 装饰器设置），
       自动注册发现的节点类。

    2. 入口点模式：通过 importlib.metadata.entry_points 查找
       group='data_workbench.plugin' 的入口点，导入入口点指定的模块，
       检查模块中的类是否带有 _node_descriptor 属性。

    使用示例::

        registry = DANodeRegistry()

        # 双模式发现：目录扫描 + 入口点
        descriptors = registry.discover(scan_paths=["/path/to/plugins"], use_entry_points=True)

        # 仅目录扫描
        descriptors = registry.discover(scan_paths=["/path/to/plugins"])

        # 仅入口点发现
        descriptors = registry.discover(use_entry_points=True)

        # 注册节点类
        @NodeDef(name="Data Filter", category="Data Processing")
        class DataFilter:
            ...
        registry.register_node(DataFilter)

        # 获取所有描述符
        descriptors = registry.get_all_descriptors()

        # 获取特定节点的描述符
        desc = registry.get_descriptor("my_module.DataFilter")
    """

    def __init__(self):
        # 以 qualified_name 为键，DANodeDescriptor 为值
        self._registry: dict = {}

    def register_node(self, node_class: type) -> da_py_workflow.DANodeDescriptor:
        """
        注册一个节点类

        从被 NodeDef 装饰的节点类中提取描述符信息并注册到注册表中。
        如果节点类没有 _node_descriptor 属性，将抛出异常。
        如果 qualified_name 已被注册，将跳过（用于去重）。

        :param node_class: 被 NodeDef 装饰的节点类
        :return: 注册的节点描述符
        :raises ValueError: 如果节点类没有 _node_descriptor 属性
        """
        descriptor_data = getattr(node_class, "_node_descriptor", None)
        if descriptor_data is None:
            raise ValueError(
                f"类 {node_class.__name__} 没有 _node_descriptor 属性，"
                "请先使用 NodeDef 装饰器声明节点类型"
            )

        descriptor = descriptor_data
        qualified_name = descriptor.qualifiedName

        if qualified_name in self._registry:
            logger.info(
                f"节点 '{qualified_name}' 已注册，跳过重复注册"
            )
            return self._registry[qualified_name]

        self._registry[qualified_name] = descriptor
        logger.debug(f"注册节点 '{qualified_name}'")
        return descriptor

    def discover(self, scan_paths: list = None, use_entry_points: bool = False) -> list:
        """
        从指定路径和入口点发现并注册节点类

        此方法支持双模式发现节点类：

        1. 目录扫描模式（scan_paths）：
           - 遍历 scan_paths 中的目录
           - 查找 .py 文件（排除 __pycache__ 目录和 __init__.py）
           - 动态导入每个模块
           - 检查模块中的类是否带有 _node_descriptor 属性
           - 调用 register_node() 注册发现的节点

        2. 入口点发现模式（use_entry_points）：
           - 使用 importlib.metadata.entry_points(group='data_workbench.plugin')
           - 加载入口点指定的模块
           - 检查模块中的类是否带有 _node_descriptor 属性
           - 调用 register_node() 注册发现的节点

        两种模式的结果会进行去重：相同 qualified_name 的节点只注册一次。

        :param scan_paths: 要扫描的目录路径列表，默认为 None（不扫描目录）
        :param use_entry_points: 是否使用 entry_points 发现节点，默认为 False
        :return: 发现并注册的节点描述符列表
        """
        discovered = []

        # 目录扫描模式
        if scan_paths:
            for path in scan_paths:
                scan_descs = self._scan_directory(path)
                discovered.extend(scan_descs)

        # 入口点发现模式
        if use_entry_points:
            ep_descs = self._discover_from_entry_points()
            discovered.extend(ep_descs)

        # 去重：已注册过的节点不会重复添加
        unique_discovered = []
        seen = set()
        for desc in discovered:
            if desc.qualifiedName not in seen:
                seen.add(desc.qualifiedName)
                unique_discovered.append(desc)

        return unique_discovered

    def _scan_directory(self, directory: str) -> list:
        """
        扫描目录中的 Python 模块，查找 @NodeDef 装饰的节点类

        :param directory: 要扫描的目录路径
        :return: 发现的节点描述符列表
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

    def _discover_from_entry_points(self) -> list:
        """
        通过 importlib.metadata.entry_points 发现节点类

        使用 entry_points(group='data_workbench.plugin') 查找已安装的插件包，
        加载入口点指定的模块，查找 @NodeDef 装饰的节点类。

        :return: 发现的节点描述符列表
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

    def _find_node_classes_in_module(self, module) -> list:
        """
        在模块中查找带有 _node_descriptor 属性的类

        遍历模块的所有属性，找出带有 _node_descriptor 属性的类对象，
        并尝试注册到注册表中。

        :param module: Python 模块对象
        :return: 发现并注册的节点描述符列表
        """
        discovered = []
        for attr_name in dir(module):
            if attr_name.startswith("_"):
                continue
            attr_value = getattr(module, attr_name, None)
            if not isinstance(attr_value, type):
                continue
            # 检查是否带有 _node_descriptor 属性
            if hasattr(attr_value, "_node_descriptor"):
                desc = self._try_register_class(attr_value)
                if desc:
                    discovered.append(desc)
        return discovered

    def _try_register_class(self, node_class: type):
        """
        尝试注册一个节点类，如果失败则返回 None

        :param node_class: 带有 _node_descriptor 属性的类
        :return: 注册成功返回 DANodeDescriptor，失败返回 None
        """
        try:
            return self.register_node(node_class)
        except ValueError:
            # 没有 _node_descriptor 属性，跳过
            return None
        except KeyError:
            # 已注册，register_node 内部已处理去重
            try:
                d = getattr(node_class, "_node_descriptor", None)
                qname = d.qualifiedName if d else ""
            except Exception:
                qname = ""
            return self._registry.get(qname, None)

    def get_all_descriptors(self) -> list:
        """
        获取所有已注册节点的描述符

        :return: 所有节点描述符的列表
        """
        return list(self._registry.values())

    def get_descriptor(self, qualified_name: str) -> da_py_workflow.DANodeDescriptor:
        """
        根据 qualified_name 获取指定节点的描述符

        :param qualified_name: 节点的唯一标识（模块名.类名）
        :return: 对应的节点描述符
        :raises KeyError: 如果 qualified_name 未注册
        """
        if qualified_name not in self._registry:
            raise KeyError(f"节点 '{qualified_name}' 未注册")
        return self._registry[qualified_name]

    def unregister_node(self, qualified_name: str) -> da_py_workflow.DANodeDescriptor:
        """
        从注册表中移除指定节点

        :param qualified_name: 节点的唯一标识
        :return: 移除的节点描述符
        :raises KeyError: 如果 qualified_name 未注册
        """
        if qualified_name not in self._registry:
            raise KeyError(f"节点 '{qualified_name}' 未注册")
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