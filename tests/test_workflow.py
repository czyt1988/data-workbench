"""
test_workflow — DAWorkflow DAG 模型测试

覆盖：DAG 创建、节点增删、连接增删、拓扑排序、环检测、序列化往返。
"""

import pytest
from DAWorkbench.DAWorkFlowPy import DAWorkflow, DAConnection, NodeDef, Input, Output, Parameter


# ==================== 辅助：创建带 NodeDef 的节点类 ====================

@NodeDef(name="Source Node", category="Test")
class _SourceNode:
    class Outputs:
        data = Output("DataFrame", description="输出数据")

    def __init__(self):
        self._output_data = {"data": [1, 2, 3]}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Middle Node", category="Test")
class _MiddleNode:
    class Inputs:
        data = Input("DataFrame", required=True)

    class Outputs:
        processed = Output("DataFrame", description="处理结果")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"processed": None}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Sink Node", category="Test")
class _SinkNode:
    class Inputs:
        data = Input("DataFrame", required=True)

    def __init__(self):
        self._input_data = {}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        return True


# ==================== DAConnection 测试 ====================

class TestDAConnection:
    """DAConnection 连接模型测试"""

    def test_connection_creation(self):
        """连接正常创建"""
        conn = DAConnection("n1", "out", "n2", "in")
        assert conn.source_node_id == "n1"
        assert conn.source_output_channel == "out"
        assert conn.target_node_id == "n2"
        assert conn.target_input_channel == "in"
        assert conn.connection_id is not None

    def test_connection_custom_id(self):
        """连接指定 connection_id"""
        conn = DAConnection("n1", "out", "n2", "in", connection_id="my_id")
        assert conn.connection_id == "my_id"

    def test_connection_self_connection_raises(self):
        """自连接抛 ValueError"""
        with pytest.raises(ValueError, match="源节点和目标节点不能相同"):
            DAConnection("n1", "out", "n1", "in")

    def test_connection_empty_source_raises(self):
        """source_node_id 为空抛 ValueError"""
        with pytest.raises(ValueError, match="source_node_id"):
            DAConnection("", "out", "n2", "in")

    def test_connection_empty_channel_raises(self):
        """channel 为空抛 ValueError"""
        with pytest.raises(ValueError, match="source_output_channel"):
            DAConnection("n1", "", "n2", "in")

    def test_connection_to_dict(self):
        """连接 to_dict 序列化"""
        conn = DAConnection("n1", "out", "n2", "in", connection_id="cid")
        d = conn.to_dict()
        assert d["connection_id"] == "cid"
        assert d["source_node_id"] == "n1"
        assert d["target_node_id"] == "n2"

    def test_connection_from_dict(self):
        """连接 from_dict 反序列化"""
        d = {
            "connection_id": "cid",
            "source_node_id": "n1",
            "source_output_channel": "out",
            "target_node_id": "n2",
            "target_input_channel": "in",
        }
        conn = DAConnection.from_dict(d)
        assert conn.source_node_id == "n1"
        assert conn.connection_id == "cid"

    def test_connection_from_dict_missing_key_raises(self):
        """from_dict 缺少必要字段抛 ValueError"""
        d = {"source_node_id": "n1"}  # 缺少其他必要字段
        with pytest.raises(ValueError, match="缺少必要字段"):
            DAConnection.from_dict(d)

    def test_connection_equality(self):
        """连接以 connection_id 判等"""
        conn1 = DAConnection("n1", "out", "n2", "in", connection_id="same_id")
        conn2 = DAConnection("n3", "out", "n4", "in", connection_id="same_id")
        assert conn1 == conn2

    def test_connection_hash(self):
        """连接以 connection_id 为哈希"""
        conn = DAConnection("n1", "out", "n2", "in", connection_id="hash_id")
        assert hash(conn) == hash("hash_id")


# ==================== DAWorkflow 测试 ====================

class TestDAWorkflow:
    """DAWorkflow DAG 模型测试"""

    def test_workflow_creation(self):
        """工作流正常创建"""
        wf = DAWorkflow(name="test_wf")
        assert wf.name == "test_wf"
        assert len(wf) == 0

    def test_workflow_add_node(self):
        """添加节点自动分配 node_id"""
        wf = DAWorkflow()
        node = _SourceNode()
        node_id = wf.add_node(node)
        assert node_id is not None
        assert node_id in wf
        assert len(wf) == 1

    def test_workflow_add_node_auto_id_format(self):
        """自动分配的 node_id 格式为 qualified_name_数字"""
        wf = DAWorkflow()
        node = _SourceNode()
        node_id = wf.add_node(node)
        assert "_" in node_id
        # qualified_name 包含类名，node_id 应包含其
        desc = node._node_descriptor
        assert desc["qualified_name"] in node_id

    def test_workflow_add_multiple_nodes(self):
        """添加多个节点自动递增后缀"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SourceNode()
        id1 = wf.add_node(n1)
        id2 = wf.add_node(n2)
        assert id1 != id2
        assert len(wf) == 2

    def test_workflow_remove_node(self):
        """移除节点返回实例"""
        wf = DAWorkflow()
        node = _SourceNode()
        nid = wf.add_node(node)
        removed = wf.remove_node(nid)
        assert removed is node
        assert len(wf) == 0

    def test_workflow_remove_node_removes_connections(self):
        """移除节点同时移除相关连接"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        conn = DAConnection(n1.node_id, "data", n2.node_id, "data")
        wf.add_connection(conn)
        assert len(wf.get_connections()) == 1
        wf.remove_node(n1.node_id)
        assert len(wf.get_connections()) == 0

    def test_workflow_remove_nonexistent_node_raises(self):
        """移除不存在的节点抛 KeyError"""
        wf = DAWorkflow()
        with pytest.raises(KeyError, match="不存在"):
            wf.remove_node("ghost")

    def test_workflow_add_connection(self):
        """添加连接"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        conn = DAConnection(n1.node_id, "data", n2.node_id, "data")
        cid = wf.add_connection(conn)
        assert cid == conn.connection_id
        assert len(wf.get_connections()) == 1

    def test_workflow_add_connection_nonexistent_source_raises(self):
        """源节点不存在时抛 KeyError"""
        wf = DAWorkflow()
        n2 = _SinkNode()
        wf.add_node(n2)
        conn = DAConnection("ghost", "out", n2.node_id, "data")
        with pytest.raises(KeyError, match="源节点"):
            wf.add_connection(conn)

    def test_workflow_add_duplicate_connection_raises(self):
        """重复端口连接抛 ValueError"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        conn1 = DAConnection(n1.node_id, "data", n2.node_id, "data")
        wf.add_connection(conn1)
        conn2 = DAConnection(n1.node_id, "data", n2.node_id, "data",
                             connection_id="different_id")
        with pytest.raises(ValueError, match="连接已存在"):
            wf.add_connection(conn2)

    def test_workflow_remove_connection(self):
        """移除连接"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        conn = DAConnection(n1.node_id, "data", n2.node_id, "data")
        cid = wf.add_connection(conn)
        removed = wf.remove_connection(cid)
        assert removed is conn
        assert len(wf.get_connections()) == 0

    def test_workflow_remove_nonexistent_connection_raises(self):
        """移除不存在的连接抛 KeyError"""
        wf = DAWorkflow()
        with pytest.raises(KeyError, match="不存在"):
            wf.remove_connection("ghost_conn")

    def test_workflow_is_valid_dag_linear(self):
        """线性 DAG 为有效 DAG"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _MiddleNode()
        n3 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_node(n3)
        wf.add_connection(DAConnection(n1.node_id, "data", n2.node_id, "data"))
        wf.add_connection(DAConnection(
            n2.node_id, "processed", n3.node_id, "data"))
        assert wf.is_valid_dag() is True

    def test_workflow_is_valid_dag_empty(self):
        """空工作流为有效 DAG"""
        wf = DAWorkflow()
        assert wf.is_valid_dag() is True

    def test_workflow_is_valid_dag_isolated_nodes(self):
        """孤立节点（无连接）为有效 DAG"""
        wf = DAWorkflow()
        wf.add_node(_SourceNode())
        assert wf.is_valid_dag() is True

    def test_workflow_circular_detection(self):
        """环形工作流 is_valid_dag 返回 False"""
        wf = DAWorkflow()
        n1 = _MiddleNode()
        n2 = _MiddleNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_connection(DAConnection(
            n1.node_id, "processed", n2.node_id, "data"))
        wf.add_connection(DAConnection(
            n2.node_id, "processed", n1.node_id, "data"))
        assert wf.is_valid_dag() is False

    def test_workflow_topological_sort(self):
        """拓扑排序返回有序列表"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _MiddleNode()
        n3 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_node(n3)
        wf.add_connection(DAConnection(n1.node_id, "data", n2.node_id, "data"))
        wf.add_connection(DAConnection(
            n2.node_id, "processed", n3.node_id, "data"))
        order = wf.topological_sort()
        assert len(order) == 3
        assert order.index(n1.node_id) < order.index(n2.node_id)
        assert order.index(n2.node_id) < order.index(n3.node_id)

    def test_workflow_topological_sort_circular_raises(self):
        """环形工作流拓扑排序抛 ValueError"""
        wf = DAWorkflow()
        n1 = _MiddleNode()
        n2 = _MiddleNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_connection(DAConnection(
            n1.node_id, "processed", n2.node_id, "data"))
        wf.add_connection(DAConnection(
            n2.node_id, "processed", n1.node_id, "data"))
        with pytest.raises(ValueError, match="存在环"):
            wf.topological_sort()

    def test_workflow_topological_sort_empty(self):
        """空工作流拓扑排序返回空列表"""
        wf = DAWorkflow()
        assert wf.topological_sort() == []

    def test_workflow_start_nodes(self):
        """get_start_nodes 返回入度为0且有出度的节点"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_connection(DAConnection(n1.node_id, "data", n2.node_id, "data"))
        starts = wf.get_start_nodes()
        assert n1.node_id in starts
        assert n2.node_id not in starts

    def test_workflow_isolated_nodes(self):
        """get_isolated_nodes 返回无连接的节点"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_connection(DAConnection(n1.node_id, "data", n2.node_id, "data"))
        isolated = wf.get_isolated_nodes()
        assert len(isolated) == 0
        # 添加孤立节点
        n3 = _SourceNode()
        wf.add_node(n3)
        isolated = wf.get_isolated_nodes()
        assert n3.node_id in isolated

    def test_workflow_downstream_connections(self):
        """get_downstream_connections 返回下游连接"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        conn = DAConnection(n1.node_id, "data", n2.node_id, "data")
        wf.add_connection(conn)
        downstream = wf.get_downstream_connections(n1.node_id)
        assert len(downstream) == 1
        assert downstream[0] is conn

    def test_workflow_upstream_connections(self):
        """get_upstream_connections 返回上游连接"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        conn = DAConnection(n1.node_id, "data", n2.node_id, "data")
        wf.add_connection(conn)
        upstream = wf.get_upstream_connections(n2.node_id)
        assert len(upstream) == 1
        assert upstream[0] is conn

    def test_workflow_serialization_roundtrip(self):
        """to_dict / from_dict 序列化往返"""
        wf = DAWorkflow(name="serial_test")
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_connection(DAConnection(n1.node_id, "data", n2.node_id, "data"))
        d = wf.to_dict()
        assert d["name"] == "serial_test"
        assert len(d["nodes"]) == 2
        assert len(d["connections"]) == 1
        # 反序列化
        wf2 = DAWorkflow.from_dict(d)
        assert wf2.name == "serial_test"
        assert len(wf2) == 2
        assert len(wf2.get_connections()) == 1

    def test_workflow_node_no_qualified_name_raises(self):
        """添加无 qualified_name 的节点抛 ValueError"""
        wf = DAWorkflow()

        class PlainObj:
            pass
        with pytest.raises(ValueError, match="qualified_name"):
            wf.add_node(PlainObj())

    def test_workflow_contains(self):
        """__contains__ 判断节点存在"""
        wf = DAWorkflow()
        node = _SourceNode()
        nid = wf.add_node(node)
        assert nid in wf
        assert "ghost" not in wf

    def test_workflow_clear(self):
        """clear 清空所有节点和连接"""
        wf = DAWorkflow()
        n1 = _SourceNode()
        n2 = _SinkNode()
        wf.add_node(n1)
        wf.add_node(n2)
        wf.add_connection(DAConnection(n1.node_id, "data", n2.node_id, "data"))
        wf.clear()
        assert len(wf) == 0
        assert len(wf.get_connections()) == 0
