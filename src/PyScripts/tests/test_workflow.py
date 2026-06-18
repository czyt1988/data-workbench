"""Tests for DAWorkflow (pure Python, no C++ bindings required)."""

import pytest
import sys
import os
import importlib.util

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_SRC_DIR = os.path.join(_TESTS_DIR, "..", "..")

def _load_module(name, rel_path):
    spec = importlib.util.spec_from_file_location(
        name, os.path.join(_SRC_DIR, rel_path)
    )
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod

connection_mod = _load_module("DAWorkbench.DAWorkFlowPy.connection", "PyScripts/DAWorkbench/DAWorkFlowPy/connection.py")
DAConnection = connection_mod.DAConnection

syntax_mod = _load_module("DAWorkbench.DAWorkFlowPy.syntax", "PyScripts/DAWorkbench/DAWorkFlowPy/syntax.py")
NodeProxy = syntax_mod.NodeProxy

workflow_mod = _load_module("DAWorkbench.DAWorkFlowPy.workflow", "PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py")
DAWorkflow = workflow_mod.DAWorkflow


def make_node(qualified_name, node_id=None):
    """Create a minimal test node with required attributes."""
    node = type("TestNode", (), {
        "qualified_name": qualified_name,
        "name": qualified_name.split(".")[-1],
    })()
    if node_id is not None:
        node.node_id = node_id
    return node


def make_node_with_ports(qualified_name, node_id=None, inputs=None, outputs=None):
    node = type("TestNode", (), {
        "qualified_name": qualified_name,
        "name": qualified_name.split(".")[-1],
        "input_keys": list(inputs or []),
        "output_keys": list(outputs or []),
    })()
    if node_id is not None:
        node.node_id = node_id
    return node


class TestWorkflowInit:
    """DAWorkflow.__init__ tests."""

    def test_default_name_empty(self):
        wf = DAWorkflow()
        assert wf.name == ""

    def test_custom_name(self):
        wf = DAWorkflow(name="my-workflow")
        assert wf.name == "my-workflow"

    def test_initial_state_empty(self):
        wf = DAWorkflow()
        assert len(wf) == 0
        assert wf.get_nodes() == []
        assert wf.get_connections() == []


class TestAddNode:
    """DAWorkflow.add_node tests."""

    def test_add_single_node_auto_id(self):
        wf = DAWorkflow()
        node = make_node("test.DataNode")
        node_id = wf.add_node(node)
        assert node_id == "test.DataNode_1"
        assert node.node_id == "test.DataNode_1"
        assert len(wf) == 1

    def test_add_multiple_nodes_auto_increment(self):
        wf = DAWorkflow()
        n1 = wf.add_node(make_node("test.Filter"))
        n2 = wf.add_node(make_node("test.Filter"))
        assert n1 == "test.Filter_1"
        assert n2 == "test.Filter_2"
        assert len(wf) == 2

    def test_add_node_with_existing_id(self):
        wf = DAWorkflow()
        node = make_node("test.Node", node_id="explicit-id")
        node_id = wf.add_node(node)
        assert node_id == "explicit-id"
        assert node.node_id == "explicit-id"
        assert len(wf) == 1

    def test_duplicate_node_id_raises(self):
        wf = DAWorkflow()
        node1 = make_node("test.Node", node_id="dup-id")
        node2 = make_node("test.OtherNode", node_id="dup-id")
        wf.add_node(node1)
        with pytest.raises(KeyError, match="dup-id"):
            wf.add_node(node2)

    def test_add_node_missing_qualified_name_raises(self):
        wf = DAWorkflow()
        node = type("BadNode", (), {})()
        with pytest.raises(ValueError, match="qualified_name"):
            wf.add_node(node)

    def test_add_node_preserves_instance(self):
        wf = DAWorkflow()
        node = make_node("test.Node")
        node.custom_attr = "hello"
        wf.add_node(node)
        retrieved = wf.get_node_by_id(node.node_id)
        assert retrieved.custom_attr == "hello"


class TestRemoveNode:
    """DAWorkflow.remove_node tests."""

    def test_remove_existing_node(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.Node", node_id="n1"))
        removed = wf.remove_node("n1")
        assert removed.node_id == "n1"
        assert len(wf) == 0

    def test_remove_nonexistent_raises(self):
        wf = DAWorkflow()
        with pytest.raises(KeyError, match="not-exist"):
            wf.remove_node("not-exist")

    def test_remove_node_also_removes_connections(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        conn = DAConnection("a", "out", "b", "in")
        wf.add_connection(conn)
        assert len(wf.get_connections()) == 1

        wf.remove_node("a")
        assert len(wf) == 1
        assert len(wf.get_connections()) == 0

    def test_remove_node_preserves_unrelated_connections(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.add_connection(DAConnection("b", "out", "c", "in"))
        wf.remove_node("a")
        assert len(wf.get_connections()) == 1


class TestAddConnection:
    """DAWorkflow.add_connection tests."""

    def test_add_valid_connection(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        conn = DAConnection("a", "out", "b", "in")
        conn_id = wf.add_connection(conn)
        assert conn_id == conn.connection_id
        assert len(wf.get_connections()) == 1

    def test_add_connection_missing_source_raises(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.B", node_id="b"))
        conn = DAConnection("nonexistent", "out", "b", "in")
        with pytest.raises(KeyError, match="nonexistent"):
            wf.add_connection(conn)

    def test_add_connection_missing_target_raises(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        conn = DAConnection("a", "out", "nonexistent", "in")
        with pytest.raises(KeyError, match="nonexistent"):
            wf.add_connection(conn)

    def test_add_duplicate_port_connection_raises(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        c1 = DAConnection("a", "out", "b", "in")
        c2 = DAConnection("a", "out", "b", "in")
        wf.add_connection(c1)
        with pytest.raises(ValueError, match="已存在"):
            wf.add_connection(c2)

    def test_add_connection_different_channels_allowed(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_connection(DAConnection("a", "out1", "b", "in"))
        wf.add_connection(DAConnection("a", "out2", "b", "in"))
        assert len(wf.get_connections()) == 2

    def test_duplicate_connection_id_raises(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        c1 = DAConnection("a", "out1", "b", "in", connection_id="dup-conn")
        c2 = DAConnection("a", "out2", "c", "in", connection_id="dup-conn")
        wf.add_connection(c1)
        with pytest.raises(KeyError, match="dup-conn"):
            wf.add_connection(c2)


class TestRemoveConnection:
    """DAWorkflow.remove_connection tests."""

    def test_remove_existing_connection(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        conn = DAConnection("a", "out", "b", "in")
        wf.add_connection(conn)
        removed = wf.remove_connection(conn.connection_id)
        assert removed.connection_id == conn.connection_id
        assert len(wf.get_connections()) == 0

    def test_remove_nonexistent_connection_raises(self):
        wf = DAWorkflow()
        with pytest.raises(KeyError, match="no-such"):
            wf.remove_connection("no-such")


class TestConnectNode:
    """DAWorkflow.connect_node convenience method tests."""

    def test_connect_node_creates_connection(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        conn = wf.connect_node("a", "result", "b", "data")
        assert isinstance(conn, DAConnection)
        assert conn.source_node_id == "a"
        assert conn.target_node_id == "b"
        assert len(wf.get_connections()) == 1

    def test_connect_node_validates_nodes_exist(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        with pytest.raises(KeyError):
            wf.connect_node("a", "out", "missing", "in")
        with pytest.raises(KeyError):
            wf.connect_node("missing", "out", "a", "in")


class TestGetNodeById:
    """DAWorkflow.get_node_by_id tests."""

    def test_get_existing_node(self):
        wf = DAWorkflow()
        node = make_node("test.Node", node_id="n1")
        wf.add_node(node)
        assert wf.get_node_by_id("n1") is node

    def test_get_nonexistent_raises(self):
        wf = DAWorkflow()
        with pytest.raises(KeyError, match="no-such"):
            wf.get_node_by_id("no-such")


class TestGetNodes:
    """DAWorkflow.get_nodes tests."""

    def test_returns_list_of_nodes(self):
        wf = DAWorkflow()
        n1 = make_node("test.A", node_id="a")
        n2 = make_node("test.B", node_id="b")
        wf.add_node(n1)
        wf.add_node(n2)
        nodes = wf.get_nodes()
        assert len(nodes) == 2
        assert n1 in nodes
        assert n2 in nodes

    def test_returns_copy_not_internal_ref(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        nodes = wf.get_nodes()
        nodes.clear()
        assert len(wf) == 1


class TestGetConnections:
    """DAWorkflow.get_connections tests."""

    def test_returns_list_of_connections(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        conn = DAConnection("a", "out", "b", "in")
        wf.add_connection(conn)
        conns = wf.get_connections()
        assert len(conns) == 1
        assert conns[0] is conn

    def test_empty_workflow_returns_empty_list(self):
        wf = DAWorkflow()
        assert wf.get_connections() == []


class TestIsValidDAG:
    """DAWorkflow.is_valid_dag tests."""

    def test_empty_workflow_is_valid(self):
        wf = DAWorkflow()
        assert wf.is_valid_dag() is True

    def test_single_node_is_valid(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        assert wf.is_valid_dag() is True

    def test_linear_chain_is_valid(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("b", "out", "c", "in")
        assert wf.is_valid_dag() is True

    def test_cycle_is_invalid(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("b", "out", "c", "in")
        wf.connect_node("c", "out", "a", "in")
        assert wf.is_valid_dag() is False

    def test_disconnected_nodes_is_valid(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        assert wf.is_valid_dag() is True

    def test_diamond_shape_is_valid(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.add_node(make_node("test.D", node_id="d"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("a", "out", "c", "in")
        wf.connect_node("b", "out", "d", "in")
        wf.connect_node("c", "out", "d", "in")
        assert wf.is_valid_dag() is True


class TestTopologicalSort:
    """DAWorkflow.topological_sort tests."""

    def test_empty_workflow_returns_empty(self):
        wf = DAWorkflow()
        assert wf.topological_sort() == []

    def test_single_node(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        result = wf.topological_sort()
        assert result == ["a"]

    def test_linear_chain_order(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("b", "out", "c", "in")
        result = wf.topological_sort()
        assert result.index("a") < result.index("b")
        assert result.index("b") < result.index("c")

    def test_cycle_raises_value_error(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("b", "out", "a", "in")
        with pytest.raises(ValueError, match="环"):
            wf.topological_sort()

    def test_diamond_shape_sort(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.add_node(make_node("test.D", node_id="d"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("a", "out", "c", "in")
        wf.connect_node("b", "out", "d", "in")
        wf.connect_node("c", "out", "d", "in")
        result = wf.topological_sort()
        assert result.index("a") < result.index("b")
        assert result.index("a") < result.index("c")
        assert result.index("b") < result.index("d")
        assert result.index("c") < result.index("d")
        assert len(result) == 4


class TestClear:
    """DAWorkflow.clear tests."""

    def test_clear_removes_all_nodes(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.clear()
        assert len(wf) == 0

    def test_clear_removes_all_connections(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.connect_node("a", "out", "b", "in")
        wf.clear()
        assert len(wf.get_connections()) == 0

    def test_clear_resets_to_empty(self):
        wf = DAWorkflow(name="cleared-wf")
        wf.add_node(make_node("test.A", node_id="a"))
        wf.clear()
        assert wf.get_nodes() == []
        assert wf.get_connections() == []


class TestLenAndContains:
    """DAWorkflow.__len__ and __contains__ tests."""

    def test_len_empty(self):
        assert len(DAWorkflow()) == 0

    def test_len_after_add(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        assert len(wf) == 1

    def test_len_after_remove(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.remove_node("a")
        assert len(wf) == 1

    def test_contains_existing_node(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        assert "a" in wf

    def test_contains_nonexistent_node(self):
        wf = DAWorkflow()
        assert "no-such" not in wf

    def test_contains_after_remove(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.remove_node("a")
        assert "a" not in wf


class TestGetItem:
    """DAWorkflow.__getitem__ tests (NodeProxy access)."""

    def test_getitem_returns_node_proxy(self):
        wf = DAWorkflow()
        node = make_node("test.A", node_id="a")
        wf.add_node(node)
        proxy = wf["a"]
        assert proxy.workflow is wf
        assert proxy.node_id == "a"

    def test_getitem_nonexistent_raises(self):
        wf = DAWorkflow()
        with pytest.raises(KeyError, match="missing"):
            _ = wf["missing"]


class TestGetConnectionsForNode:
    """DAWorkflow.get_connections_for_node tests."""

    def test_returns_connections_for_source_node(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("a", "out", "c", "in")
        conns = wf.get_connections_for_node("a")
        assert len(conns) == 2

    def test_returns_connections_for_target_node(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.connect_node("a", "out", "b", "in")
        conns = wf.get_connections_for_node("b")
        assert len(conns) == 1

    def test_returns_empty_for_isolated_node(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        conns = wf.get_connections_for_node("a")
        assert conns == []

    def test_nonexistent_node_returns_empty(self):
        wf = DAWorkflow()
        conns = wf.get_connections_for_node("no-such")
        assert conns == []


class TestGetDownstreamUpstreamConnections:
    """DAWorkflow.get_downstream_connections and get_upstream_connections tests."""

    def test_downstream_from_source(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.connect_node("a", "result", "b", "data")
        downstream = wf.get_downstream_connections("a")
        assert len(downstream) == 1
        assert downstream[0].target_node_id == "b"

    def test_upstream_for_target(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.connect_node("a", "result", "b", "data")
        upstream = wf.get_upstream_connections("b")
        assert len(upstream) == 1
        assert upstream[0].source_node_id == "a"

    def test_downstream_channel_filter(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out1", "b", "in")
        wf.connect_node("a", "out2", "c", "in")
        filtered = wf.get_downstream_connections("a", output_channel="out1")
        assert len(filtered) == 1
        assert filtered[0].target_node_id == "b"

    def test_upstream_channel_filter(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out", "b", "data")
        wf.connect_node("c", "out", "b", "config")
        filtered = wf.get_upstream_connections("b", input_channel="data")
        assert len(filtered) == 1
        assert filtered[0].source_node_id == "a"


class TestGetStartNodes:
    """DAWorkflow.get_start_nodes tests."""

    def test_linear_chain_start_nodes(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out", "b", "in")
        wf.connect_node("b", "out", "c", "in")
        starts = wf.get_start_nodes()
        assert starts == ["a"]

    def test_empty_workflow_no_start_nodes(self):
        wf = DAWorkflow()
        assert wf.get_start_nodes() == []

    def test_no_connections_no_start_nodes(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        assert wf.get_start_nodes() == []


class TestGetIsolatedNodes:
    """DAWorkflow.get_isolated_nodes tests."""

    def test_all_isolated(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        isolated = wf.get_isolated_nodes()
        assert set(isolated) == {"a", "b"}

    def test_no_isolated_in_connected_graph(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.connect_node("a", "out", "b", "in")
        assert wf.get_isolated_nodes() == []

    def test_mixed_isolated_and_connected(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.add_node(make_node("test.C", node_id="c"))
        wf.connect_node("a", "out", "b", "in")
        isolated = wf.get_isolated_nodes()
        assert isolated == ["c"]

    def test_empty_workflow(self):
        wf = DAWorkflow()
        assert wf.get_isolated_nodes() == []


class TestRepr:
    """DAWorkflow.__repr__ tests."""

    def test_repr_format(self):
        wf = DAWorkflow(name="my-wf")
        r = repr(wf)
        assert "my-wf" in r
        assert "nodes=0" in r

    def test_repr_with_nodes(self):
        wf = DAWorkflow()
        wf.add_node(make_node("test.A", node_id="a"))
        wf.add_node(make_node("test.B", node_id="b"))
        wf.connect_node("a", "out", "b", "in")
        r = repr(wf)
        assert "nodes=2" in r
        assert "connections=1" in r
