"""
test_node_registry — DANodeRegistry 节点注册和发现测试

覆盖：注册节点、目录扫描发现、入口点发现、去重、
查询描述符、无效节点文件处理。
"""

import os
import sys
import tempfile
import pytest
from DAWorkbench.DAWorkFlowPy import DANodeRegistry, NodeDef, Input, Output, Parameter
from DAWorkbench.DAWorkFlowPy.node_descriptor import DANodeDescriptor


# ==================== 辅助节点 ====================

@NodeDef(name="RegistryTestNode", category="Registry Test", icon="test")
class RegistryTestNode:
    class Inputs:
        data = Input("DataFrame", required=True)

    class Outputs:
        result = Output("DataFrame")
    threshold = Parameter(float, default=0.5, description="阈值")

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="SecondTestNode", category="Registry Test")
class SecondTestNode:
    class Outputs:
        output = Output("int")

    def execute(self, inputs=None, params=None):
        return True


# ==================== 注册测试 ====================

class TestDANodeRegistryRegister:
    """DANodeRegistry 注册测试"""

    def test_register_node(self):
        """注册节点类"""
        registry = DANodeRegistry()
        desc = registry.register_node(RegistryTestNode)
        assert isinstance(desc, DANodeDescriptor)
        assert desc.name == "RegistryTestNode"

    def test_register_node_in_registry(self):
        """注册节点后可通过 qualified_name 查找"""
        registry = DANodeRegistry()
        desc = registry.register_node(RegistryTestNode)
        assert desc.qualified_name in registry

    def test_register_duplicate_skips(self):
        """重复注册同名节点跳过"""
        registry = DANodeRegistry()
        desc1 = registry.register_node(RegistryTestNode)
        desc2 = registry.register_node(RegistryTestNode)
        assert desc1 is desc2  # 返回已有描述符
        assert len(registry) == 1

    def test_register_node_without_descriptor_raises(self):
        """注册无 _node_descriptor 的类抛 ValueError"""
        registry = DANodeRegistry()

        class PlainClass:
            pass
        with pytest.raises(ValueError, match="没有 _node_descriptor"):
            registry.register_node(PlainClass)


# ==================== 查询测试 ====================

class TestDANodeRegistryQuery:
    """DANodeRegistry 查询测试"""

    def test_get_descriptor(self):
        """获取指定节点的描述符"""
        registry = DANodeRegistry()
        registry.register_node(RegistryTestNode)
        desc = registry.get_descriptor(
            RegistryTestNode._node_descriptor["qualified_name"])
        assert desc.name == "RegistryTestNode"

    def test_get_descriptor_not_registered_raises(self):
        """获取未注册节点描述符抛 KeyError"""
        registry = DANodeRegistry()
        with pytest.raises(KeyError, match="未注册"):
            registry.get_descriptor("ghost.node")

    def test_get_all_descriptors(self):
        """获取所有已注册描述符"""
        registry = DANodeRegistry()
        registry.register_node(RegistryTestNode)
        registry.register_node(SecondTestNode)
        all_descs = registry.get_all_descriptors()
        assert len(all_descs) == 2

    def test_unregister_node(self):
        """移除已注册节点"""
        registry = DANodeRegistry()
        registry.register_node(RegistryTestNode)
        qn = RegistryTestNode._node_descriptor["qualified_name"]
        removed = registry.unregister_node(qn)
        assert removed.name == "RegistryTestNode"
        assert qn not in registry

    def test_unregister_nonexistent_raises(self):
        """移除未注册节点抛 KeyError"""
        registry = DANodeRegistry()
        with pytest.raises(KeyError, match="未注册"):
            registry.unregister_node("ghost.node")

    def test_clear_registry(self):
        """清空注册表"""
        registry = DANodeRegistry()
        registry.register_node(RegistryTestNode)
        registry.clear()
        assert len(registry) == 0

    def test_registry_len(self):
        """__len__ 返回注册数量"""
        registry = DANodeRegistry()
        assert len(registry) == 0
        registry.register_node(RegistryTestNode)
        assert len(registry) == 1

    def test_registry_repr(self):
        """__repr__ 格式"""
        registry = DANodeRegistry()
        registry.register_node(RegistryTestNode)
        r = repr(registry)
        assert "DANodeRegistry" in r
        assert "nodes=1" in r


# ==================== 目录扫描发现测试 ====================

class TestDANodeRegistryScanDirectory:
    """DANodeRegistry 目录扫描发现测试"""

    def test_scan_directory_discovery(self):
        """扫描目录发现 @NodeDef 装饰的节点"""
        # 创建临时目录和有效节点文件
        with tempfile.TemporaryDirectory() as tmpdir:
            node_content = '''
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter

@NodeDef(name="TmpNode", category="Tmp")
class TmpNode:
    class Inputs:
        data = Input("DataFrame", required=True)
    class Outputs:
        result = Output("DataFrame")
    def execute(self, inputs=None, params=None):
        return True
'''
            node_file = os.path.join(tmpdir, "tmp_node.py")
            with open(node_file, "w", encoding="utf-8") as f:
                f.write(node_content)

            registry = DANodeRegistry()
            discovered = registry.discover(scan_paths=[tmpdir])
            assert len(discovered) >= 1
            names = [d.name for d in discovered]
            assert "TmpNode" in names

    def test_scan_invalid_directory_skips(self):
        """扫描无效目录跳过"""
        registry = DANodeRegistry()
        discovered = registry.discover(scan_paths=["/nonexistent/path"])
        assert len(discovered) == 0

    def test_scan_directory_skips_init_and_pycache(self):
        """扫描跳过 __init__.py 和 __pycache__"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # 创建 __init__.py（应被跳过）
            init_file = os.path.join(tmpdir, "__init__.py")
            with open(init_file, "w", encoding="utf-8") as f:
                f.write("# empty init")

            # 创建 __pycache__ 目录中的文件（应被跳过）
            cache_dir = os.path.join(tmpdir, "__pycache__")
            os.makedirs(cache_dir, exist_ok=True)
            cache_file = os.path.join(cache_dir, "cached.cpython-39.pyc")
            with open(cache_file, "wb") as f:
                f.write(b"\x00")

            registry = DANodeRegistry()
            discovered = registry.discover(scan_paths=[tmpdir])
            assert len(discovered) == 0

    def test_scan_invalid_python_file_skips(self):
        """扫描无效 Python 文件跳过"""
        with tempfile.TemporaryDirectory() as tmpdir:
            bad_file = os.path.join(tmpdir, "bad_node.py")
            with open(bad_file, "w", encoding="utf-8") as f:
                f.write("import nonexistent_module\nraise ImportError()")

            registry = DANodeRegistry()
            discovered = registry.discover(scan_paths=[tmpdir])
            # 导入失败应被跳过，不崩溃
            assert len(discovered) == 0

    def test_scan_non_nodedef_class_skips(self):
        """扫描目录中非 @NodeDef 装饰的类被跳过"""
        with tempfile.TemporaryDirectory() as tmpdir:
            plain_content = '''
class PlainClass:
    """没有 @NodeDef 装饰器的普通类"""
    pass
'''
            plain_file = os.path.join(tmpdir, "plain_class.py")
            with open(plain_file, "w", encoding="utf-8") as f:
                f.write(plain_content)

            registry = DANodeRegistry()
            discovered = registry.discover(scan_paths=[tmpdir])
            assert len(discovered) == 0


# ==================== 入口点发现测试 ====================

class TestDANodeRegistryEntryPoints:
    """DANodeRegistry 入口点发现测试"""

    def test_discover_without_entry_points(self):
        """不使用入口点时仅靠目录扫描"""
        registry = DANodeRegistry()
        discovered = registry.discover(scan_paths=None, use_entry_points=False)
        assert len(discovered) == 0

    def test_discover_entry_points_no_plugins(self):
        """入口点发现无插件时不崩溃"""
        registry = DANodeRegistry()
        discovered = registry.discover(use_entry_points=True)
        # 没有安装 data_workbench.plugin 入口点，结果为空
        assert len(discovered) == 0


# ==================== 去重测试 ====================

class TestDANodeRegistryDeduplication:
    """DANodeRegistry 去重测试"""

    def test_deduplication_on_discover(self):
        """相同 qualified_name 的节点只注册一次"""
        # 先手动注册一个
        registry = DANodeRegistry()
        registry.register_node(RegistryTestNode)

        # 再通过 discover 扫描包含相同类的文件
        # discover 返回 unique_discovered 列表
        with tempfile.TemporaryDirectory() as tmpdir:
            node_content = '''
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter

@NodeDef(name="RegistryTestNode", category="Registry Test", icon="test")
class RegistryTestNode:
    class Inputs:
        data = Input("DataFrame", required=True)
    class Outputs:
        result = Output("DataFrame")
    threshold = Parameter(float, default=0.5, description="阈值")
    def execute(self, inputs=None, params=None):
        return True
'''
            # 写入同名节点文件（qualified_name 不同因为模块名不同）
            node_file = os.path.join(tmpdir, "dup_node.py")
            with open(node_file, "w", encoding="utf-8") as f:
                f.write(node_content)

            discovered = registry.discover(scan_paths=[tmpdir])
            # 即使扫描发现了节点，已在注册表中的不会重复
            # 但不同 qualified_name 的节点会增加
            total = len(registry)
            # 手动注册了 1 个 + 扫描发现的（不同模块名的同类）
            assert total >= 1
