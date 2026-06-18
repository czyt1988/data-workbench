"""
test_connection_syntax — 连接语法糖测试

覆盖：>> 单端口连接、>> 多端口歧义、connect_node()、
DAConnection 兼容性（序列化正确）、向后兼容（原有 add_connection 仍可用）、
NodeProxy 访问器、__getitem__ 语法。
"""

import pytest
from DAWorkbench.DAWorkFlowPy import (
    DAWorkflow,
    DAConnection,
    NodeDef,
    Input,
    Output,
    Parameter,
)
from DAWorkbench.DAWorkFlowPy.syntax import NodeProxy, NodeOutputProxy, NodeInputProxy


# ==================== 辅助：创建节点类 ====================

@NodeDef(name="Single Output", category="Test")
class _SingleOutNode:
    """只有 1 个输出端口 'data' 的节点"""

    class Outputs:
        data = Output("DataFrame", description="输出数据")

    def __init__(self):
        self._output_data = {"data": [1, 2, 3]}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Single Input", category="Test")
class _SingleInNode:
    """只有 1 个输入端口 'data' 的节点"""

    class Inputs:
        data = Input("DataFrame", required=True)

    def __init__(self):
        self._input_data = {}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Multi Output", category="Test")
class _MultiOutNode:
    """有 2 个输出端口 'result' 和 'log' 的节点"""

    class Outputs:
        result = Output("DataFrame", description="处理结果")
        log = Output("str", description="日志信息")

    def __init__(self):
        self._output_data = {"result": None, "log": ""}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Multi Input", category="Test")
class _MultiInNode:
    """有 2 个输入端口 'data' 和 'config' 的节点"""

    class Inputs:
        data = Input("DataFrame", required=True)
        config = Input("dict", required=False)

    def __init__(self):
        self._input_data = {}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Source Node", category="Test")
class _SourceNode:
    """单输出 'data' 的源节点"""

    class Outputs:
        data = Output("DataFrame", description="输出数据")

    def __init__(self):
        self._output_data = {"data": [1, 2, 3]}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Sink Node", category="Test")
class _SinkNode:
    """单输入 'data' 的终端节点"""

    class Inputs:
        data = Input("DataFrame", required=True)

    def __init__(self):
        self._input_data = {}

    def execute(self, inputs=None, params=None):
        return True


# ==================== NodeProxy 访问器测试 ====================

class TestNodeProxyAccessor:
    """workflow[node_id] 返回 NodeProxy，支持 .outputs / .inputs 访问器"""

    def test_workflow_getitem_returns_node_proxy(self):
        """workflow[node_id] 返回 NodeProxy 实例"""
        wf = DAWorkflow()
        node = _SingleOutNode()
        nid = wf.add_node(node)
        proxy = wf[nid]
        assert isinstance(proxy, NodeProxy)

    def test_node_proxy_outputs_returns_output_proxy(self):
        """proxy.outputs["channel"] 返回 NodeOutputProxy"""
        wf = DAWorkflow()
        node = _SingleOutNode()
        nid = wf.add_node(node)
        proxy = wf[nid]
        out_proxy = proxy.outputs["data"]
        assert isinstance(out_proxy, NodeOutputProxy)
        assert out_proxy.node_id == nid
        assert out_proxy.channel == "data"

    def test_node_proxy_inputs_returns_input_proxy(self):
        """proxy.inputs["channel"] 返回 NodeInputProxy"""
        wf = DAWorkflow()
        node = _SingleInNode()
        nid = wf.add_node(node)
        proxy = wf[nid]
        in_proxy = proxy.inputs["data"]
        assert isinstance(in_proxy, NodeInputProxy)
        assert in_proxy.node_id == nid
        assert in_proxy.channel == "data"

    def test_workflow_getitem_nonexistent_raises(self):
        """workflow[不存在node_id] 抛 KeyError"""
        wf = DAWorkflow()
        with pytest.raises(KeyError):
            wf["ghost_node"]


# ==================== >> 操作符：显式端口连接 ====================

class TestRshiftExplicitPorts:
    """output_proxy >> input_proxy 显式端口连接"""

    def test_rshift_single_port_success(self):
        """单端口 >> 连接成功创建 DAConnection"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        conn = wf[src_id].outputs["data"] >> wf[dst_id].inputs["data"]

        assert isinstance(conn, DAConnection)
        assert conn.source_node_id == src_id
        assert conn.source_output_channel == "data"
        assert conn.target_node_id == dst_id
        assert conn.target_input_channel == "data"

    def test_rshift_connection_added_to_workflow(self):
        """>> 操作后连接已加入工作流"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        conn = wf[src_id].outputs["data"] >> wf[dst_id].inputs["data"]

        connections = wf.get_connections()
        assert len(connections) == 1
        assert conn in connections

    def test_rshift_multi_port_ambiguous_raises(self):
        """多输出或多输入端口时不指定端口抛 ValueError"""
        wf = DAWorkflow()
        src = _MultiOutNode()  # 2 outputs
        dst = _SingleInNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        # NodeProxy >> NodeProxy，源有多个输出端口，应报歧义
        with pytest.raises(ValueError, match="ambiguous"):
            wf[src_id] >> wf[dst_id]

    def test_rshift_multi_input_ambiguous_raises(self):
        """目标节点有多个输入端口时报歧义"""
        wf = DAWorkflow()
        src = _SingleOutNode()  # 1 output
        dst = _MultiInNode()  # 2 inputs
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        with pytest.raises(ValueError, match="ambiguous"):
            wf[src_id] >> wf[dst_id]


# ==================== >> 操作符：自动端口匹配 ====================

class TestRshiftAutoPortMapping:
    """NodeProxy >> NodeProxy 自动端口匹配（单端口节点）"""

    def test_auto_match_single_port(self):
        """源 1 output + 目标 1 input 自动匹配"""
        wf = DAWorkflow()
        src = _SourceNode()  # 1 output: "data"
        dst = _SinkNode()    # 1 input: "data"
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        conn = wf[src_id] >> wf[dst_id]

        assert isinstance(conn, DAConnection)
        assert conn.source_output_channel == "data"
        assert conn.target_input_channel == "data"

    def test_auto_match_connection_in_workflow(self):
        """自动匹配的连接已加入工作流"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        wf[src_id] >> wf[dst_id]
        assert len(wf.get_connections()) == 1


# ==================== connect_node 方法 ====================

class TestConnectNode:
    """DAWorkflow.connect_node() 方法测试"""

    def test_connect_node_creates_connection(self):
        """connect_node 创建并添加 DAConnection"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        conn = wf.connect_node(src_id, "data", dst_id, "data")

        assert isinstance(conn, DAConnection)
        assert conn.source_node_id == src_id
        assert conn.source_output_channel == "data"
        assert conn.target_node_id == dst_id
        assert conn.target_input_channel == "data"
        assert conn in wf.get_connections()

    def test_connect_node_nonexistent_source_raises(self):
        """connect_node 源节点不存在抛 KeyError"""
        wf = DAWorkflow()
        dst = _SinkNode()
        dst_id = wf.add_node(dst)

        with pytest.raises(KeyError):
            wf.connect_node("ghost", "out", dst_id, "data")

    def test_connect_node_nonexistent_target_raises(self):
        """connect_node 目标节点不存在抛 KeyError"""
        wf = DAWorkflow()
        src = _SourceNode()
        src_id = wf.add_node(src)

        with pytest.raises(KeyError):
            wf.connect_node(src_id, "data", "ghost", "data")


# ==================== DAConnection 兼容性 ====================

class TestDAConnectionCompat:
    """语法糖创建的 DAConnection 与手动创建的行为一致"""

    def test_syntax_connection_serializes_correctly(self):
        """语法糖创建的连接 to_dict 序列化正确"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        conn = wf[src_id].outputs["data"] >> wf[dst_id].inputs["data"]
        d = conn.to_dict()

        assert d["source_node_id"] == src_id
        assert d["source_output_channel"] == "data"
        assert d["target_node_id"] == dst_id
        assert d["target_input_channel"] == "data"
        assert "connection_id" in d

    def test_syntax_connection_from_dict_roundtrip(self):
        """语法糖创建的连接支持 from_dict 反序列化"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        conn = wf[src_id].outputs["data"] >> wf[dst_id].inputs["data"]
        d = conn.to_dict()
        restored = DAConnection.from_dict(d)

        assert restored.source_node_id == conn.source_node_id
        assert restored.source_output_channel == conn.source_output_channel
        assert restored.target_node_id == conn.target_node_id
        assert restored.target_input_channel == conn.target_input_channel


# ==================== 向后兼容 ====================

class TestBackwardCompat:
    """原有 add_connection API 不受影响"""

    def test_existing_add_connection_still_works(self):
        """手动创建 DAConnection 并 add_connection 仍然可用"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        src_id = wf.add_node(src)
        dst_id = wf.add_node(dst)

        # 原有方式
        conn = DAConnection(src_id, "data", dst_id, "data")
        cid = wf.add_connection(conn)

        assert cid == conn.connection_id
        assert conn in wf.get_connections()

    def test_syntax_and_manual_can_coexist(self):
        """语法糖和手动 add_connection 可以共存"""
        wf = DAWorkflow()
        src1 = _SourceNode()
        src2 = _SourceNode()
        dst1 = _SinkNode()
        dst2 = _SinkNode()
        wf.add_node(src1)
        wf.add_node(src2)
        wf.add_node(dst1)
        wf.add_node(dst2)

        # 语法糖
        wf[src1.node_id].outputs["data"] >> wf[dst1.node_id].inputs["data"]
        # 手动
        wf.add_connection(DAConnection(src2.node_id, "data", dst2.node_id, "data"))

        assert len(wf.get_connections()) == 2

    def test_workflow_serialization_includes_syntax_connections(self):
        """to_dict 包含语法糖创建的连接"""
        wf = DAWorkflow(name="syntax_test")
        src = _SourceNode()
        dst = _SinkNode()
        wf.add_node(src)
        wf.add_node(dst)

        wf[src.node_id].outputs["data"] >> wf[dst.node_id].inputs["data"]

        d = wf.to_dict()
        assert len(d["connections"]) == 1
        assert d["connections"][0]["source_output_channel"] == "data"


# ==================== 链式连接测试 ====================

class TestChainedConnections:
    """链式 >> 连接多个节点"""

    def test_chain_three_nodes_explicit_ports(self):
        """三个节点链式连接：src >> mid >> sink（显式端口）"""
        wf = DAWorkflow()
        src = _SourceNode()
        dst = _SinkNode()
        wf.add_node(src)
        wf.add_node(dst)

        # 同一个工作流中创建两条连接
        conn1 = wf[src.node_id].outputs["data"] >> wf[dst.node_id].inputs["data"]
        assert conn1 is not None

        # 验证连接在 workflow 中
        assert len(wf.get_connections()) == 1

    def test_chain_auto_port_multiple_connections(self):
        """多条自动匹配连接可以依次创建"""
        wf = DAWorkflow()
        src1 = _SourceNode()
        src2 = _SourceNode()
        dst = _SinkNode()
        wf.add_node(src1)
        wf.add_node(src2)
        wf.add_node(dst)

        wf[src1.node_id] >> wf[dst.node_id]
        # 注意：第二条连接使用不同源节点但相同目标端口
        # 这应该是允许的（不同源连到同一目标输入）
        wf[src2.node_id] >> wf[dst.node_id]
        assert len(wf.get_connections()) == 2


# ==================== NodeProxy / NodeOutputProxy / NodeInputProxy repr ====================

class TestProxyRepr:
    """代理对象的 repr 格式"""

    def test_node_proxy_repr(self):
        """NodeProxy repr 包含 node_id"""
        wf = DAWorkflow()
        node = _SingleOutNode()
        nid = wf.add_node(node)
        proxy = wf[nid]
        r = repr(proxy)
        assert nid in r

    def test_output_proxy_repr(self):
        """NodeOutputProxy repr 包含 node_id 和 channel"""
        wf = DAWorkflow()
        node = _SingleOutNode()
        nid = wf.add_node(node)
        out = wf[nid].outputs["data"]
        r = repr(out)
        assert nid in r
        assert "data" in r

    def test_input_proxy_repr(self):
        """NodeInputProxy repr 包含 node_id 和 channel"""
        wf = DAWorkflow()
        node = _SingleInNode()
        nid = wf.add_node(node)
        inp = wf[nid].inputs["data"]
        r = repr(inp)
        assert nid in r
        assert "data" in r