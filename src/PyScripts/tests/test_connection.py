"""Tests for DAConnection."""

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

connection = _load_module("DAConnection", "PyScripts/DAWorkbench/DAWorkFlowPy/connection.py")
DAConnection = connection.DAConnection


class TestDAConnectionInit:
    """DAConnection.__init__ tests."""

    def test_basic_connection(self):
        conn = DAConnection(
            source_node_id="node_1",
            source_output_channel="out",
            target_node_id="node_2",
            target_input_channel="in",
        )
        assert conn.source_node_id == "node_1"
        assert conn.source_output_channel == "out"
        assert conn.target_node_id == "node_2"
        assert conn.target_input_channel == "in"
        assert conn.connection_id is not None
        assert isinstance(conn.connection_id, str)

    def test_connection_with_custom_id(self):
        conn = DAConnection(
            source_node_id="src",
            source_output_channel="data",
            target_node_id="dst",
            target_input_channel="input",
            connection_id="my-custom-id",
        )
        assert conn.connection_id == "my-custom-id"

    def test_auto_generates_unique_ids(self):
        c1 = DAConnection("a", "o1", "b", "i1")
        c2 = DAConnection("a", "o2", "b", "i2")
        assert c1.connection_id != c2.connection_id

    def test_reject_empty_source_node_id(self):
        with pytest.raises(ValueError, match="source_node_id"):
            DAConnection("", "out", "dst", "in")

    def test_reject_empty_source_output_channel(self):
        with pytest.raises(ValueError, match="source_output_channel"):
            DAConnection("src", "", "dst", "in")

    def test_reject_empty_target_node_id(self):
        with pytest.raises(ValueError, match="target_node_id"):
            DAConnection("src", "out", "", "in")

    def test_reject_empty_target_input_channel(self):
        with pytest.raises(ValueError, match="target_input_channel"):
            DAConnection("src", "out", "dst", "")

    def test_reject_self_connection(self):
        with pytest.raises(ValueError, match="self-connection"):
            DAConnection("node_1", "out", "node_1", "in")

    def test_connection_id_is_uuid_format(self):
        import re
        conn = DAConnection("src", "out", "dst", "in")
        uuid_pattern = re.compile(
            r'^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$'
        )
        assert uuid_pattern.match(conn.connection_id)


class TestDAConnectionEquality:
    """DAConnection.__eq__ and __hash__ tests."""

    def test_equal_by_connection_id(self):
        c1 = DAConnection("a", "o1", "b", "i1", connection_id="same-id")
        c2 = DAConnection("x", "o2", "y", "i2", connection_id="same-id")
        assert c1 == c2

    def test_not_equal_different_id(self):
        c1 = DAConnection("a", "o1", "b", "i1")
        c2 = DAConnection("a", "o1", "b", "i1", connection_id="different")
        assert c1 != c2

    def test_not_equal_non_connection_type(self):
        conn = DAConnection("a", "o1", "b", "i1")
        assert conn != "not-a-connection"
        assert conn != 42
        assert conn != object()

    def test_hash_consistent(self):
        c1 = DAConnection("a", "o1", "b", "i1", connection_id="hash-test")
        c2 = DAConnection("x", "o2", "y", "i2", connection_id="hash-test")
        assert hash(c1) == hash(c2)

    def test_can_use_in_set(self):
        c1 = DAConnection("a", "o1", "b", "i1", connection_id="set-test")
        c2 = DAConnection("x", "o2", "y", "i2", connection_id="set-test")
        s = {c1, c2}
        assert len(s) == 1


class TestDAConnectionRepr:
    """DAConnection.__repr__ tests."""

    def test_repr_contains_source_and_target(self):
        conn = DAConnection("src_node", "output", "dst_node", "input", connection_id="test-id")
        repr_str = repr(conn)
        assert "src_node:output" in repr_str
        assert "dst_node:input" in repr_str
        assert "test-id" in repr_str

    def test_repr_starts_with_class_name(self):
        conn = DAConnection("a", "o", "b", "i")
        assert repr(conn).startswith("DAConnection(")
