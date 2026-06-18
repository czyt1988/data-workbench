"""Tests for DAWorkflowSerializer XML serialization (pure Python, no C++ bindings required)."""

import pytest
import sys
import os
import importlib.util
import xml.etree.ElementTree as ET

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


connection_mod = _load_module(
    "DAWorkbench.DAWorkFlowPy.connection",
    "PyScripts/DAWorkbench/DAWorkFlowPy/connection.py",
)
DAConnection = connection_mod.DAConnection

syntax_mod = _load_module(
    "DAWorkbench.DAWorkFlowPy.syntax",
    "PyScripts/DAWorkbench/DAWorkFlowPy/syntax.py",
)

workflow_mod = _load_module(
    "DAWorkbench.DAWorkFlowPy.workflow",
    "PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py",
)
DAWorkflow = workflow_mod.DAWorkflow

# Pre-load node_registry and node_factory before serializer (avoids circular import)
node_registry_mod = _load_module(
    "DAWorkbench.DAWorkFlowPy.node_registry",
    "PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py",
)
node_factory_mod = _load_module(
    "DAWorkbench.DAWorkFlowPy.node_factory",
    "PyScripts/DAWorkbench/DAWorkFlowPy/node_factory.py",
)

serializer_mod = _load_module(
    "DAWorkbench.DAWorkFlowPy.serializer",
    "PyScripts/DAWorkbench/DAWorkFlowPy/serializer.py",
)
DAWorkflowSerializer = serializer_mod.DAWorkflowSerializer


# ---------- Mock node factory ----------


class _MockFactory:
    """Minimal factory that creates mock nodes by qualified_name."""

    def __init__(self, node_templates):
        self._templates = node_templates

    def create_node(self, qualified_name):
        tmpl = self._templates.get(qualified_name)
        if tmpl is None:
            raise KeyError(f"Unknown node type: {qualified_name}")
        # Create a new instance with the same attributes
        node = type("MockNode", (), {})()
        node.qualified_name = qualified_name
        node.name = qualified_name.split(".")[-1]
        # Copy parameter defaults
        for k, v in tmpl.get("params", {}).items():
            setattr(node, k, v)
        node.parameters = {k: k for k in tmpl.get("params", {})}
        return node


def _make_factory():
    templates = {
        "test.FilterNode": {
            "params": {"threshold": 0.5, "label": "default"},
        },
        "test.ProcessNode": {
            "params": {"count": 10, "enabled": True, "tags": None},
        },
    }
    return _MockFactory(templates)


def _make_test_workflow():
    """Build a small workflow with two nodes and one connection."""
    wf = DAWorkflow(name="test_wf")
    factory = _make_factory()

    n1 = factory.create_node("test.FilterNode")
    n1.node_id = "test.FilterNode_1"
    n1.threshold = 0.8
    n1.label = "my_filter"
    wf.add_node(n1)

    n2 = factory.create_node("test.ProcessNode")
    n2.node_id = "test.ProcessNode_1"
    n2.count = 20
    n2.enabled = False
    n2.tags = ["a", "b"]
    wf.add_node(n2)

    conn = DAConnection(
        source_node_id="test.FilterNode_1",
        source_output_channel="result",
        target_node_id="test.ProcessNode_1",
        target_input_channel="data",
        connection_id="conn-001",
    )
    wf.add_connection(conn)
    return wf


# =========================================================
# Tests: to_xml_element / from_xml_element
# =========================================================


class TestXmlElementRoundTrip:
    """Test to_xml_element -> from_xml_element round trip."""

    def test_basic_round_trip(self):
        wf = _make_test_workflow()
        serializer = DAWorkflowSerializer()
        element = serializer.to_xml_element(wf)

        factory = _make_factory()
        wf2 = serializer.from_xml_element(element, factory)

        assert wf2.name == "test_wf"
        assert len(wf2._nodes) == 2
        assert len(wf2.get_connections()) == 1

    def test_node_ids_preserved(self):
        wf = _make_test_workflow()
        serializer = DAWorkflowSerializer()
        element = serializer.to_xml_element(wf)

        factory = _make_factory()
        wf2 = serializer.from_xml_element(element, factory)

        ids = set(wf2._nodes.keys())
        assert "test.FilterNode_1" in ids
        assert "test.ProcessNode_1" in ids

    def test_parameter_values_preserved(self):
        wf = _make_test_workflow()
        serializer = DAWorkflowSerializer()
        element = serializer.to_xml_element(wf)

        factory = _make_factory()
        wf2 = serializer.from_xml_element(element, factory)

        n1 = wf2._nodes["test.FilterNode_1"]
        assert n1.threshold == 0.8
        assert n1.label == "my_filter"

        n2 = wf2._nodes["test.ProcessNode_1"]
        assert n2.count == 20
        assert n2.enabled is False
        assert n2.tags == ["a", "b"]

    def test_connection_preserved(self):
        wf = _make_test_workflow()
        serializer = DAWorkflowSerializer()
        element = serializer.to_xml_element(wf)

        factory = _make_factory()
        wf2 = serializer.from_xml_element(element, factory)

        conns = wf2.get_connections()
        assert len(conns) == 1
        c = conns[0]
        assert c.source_node_id == "test.FilterNode_1"
        assert c.source_output_channel == "result"
        assert c.target_node_id == "test.ProcessNode_1"
        assert c.target_input_channel == "data"
        assert c.connection_id == "conn-001"


# =========================================================
# Tests: to_xml / from_xml (string level)
# =========================================================


class TestXmlStringRoundTrip:
    """Test to_xml -> from_xml string round trip."""

    def test_string_round_trip(self):
        wf = _make_test_workflow()
        serializer = DAWorkflowSerializer()
        xml_str = serializer.to_xml(wf)

        factory = _make_factory()
        wf2 = serializer.from_xml(xml_str, factory)

        assert wf2.name == "test_wf"
        assert len(wf2._nodes) == 2

        n1 = wf2._nodes["test.FilterNode_1"]
        assert n1.threshold == 0.8

    def test_xml_is_valid(self):
        wf = _make_test_workflow()
        serializer = DAWorkflowSerializer()
        xml_str = serializer.to_xml(wf)

        # Should parse without error
        root = ET.fromstring(xml_str)
        assert root.tag == "workflow"
        assert root.get("name") == "test_wf"


# =========================================================
# Tests: parameter type handling
# =========================================================


class TestParamTypeHandling:
    """Test serialization/deserialization of various Python types."""

    def test_serialize_str(self):
        t, v = DAWorkflowSerializer._serialize_param_value("hello")
        assert t == "str"
        assert v == "hello"
        assert DAWorkflowSerializer._deserialize_param_value(t, v) == "hello"

    def test_serialize_int(self):
        t, v = DAWorkflowSerializer._serialize_param_value(42)
        assert t == "int"
        assert v == "42"
        assert DAWorkflowSerializer._deserialize_param_value(t, v) == 42

    def test_serialize_float(self):
        t, v = DAWorkflowSerializer._serialize_param_value(3.14)
        assert t == "float"
        result = DAWorkflowSerializer._deserialize_param_value(t, v)
        assert abs(result - 3.14) < 1e-10

    def test_serialize_bool_true(self):
        t, v = DAWorkflowSerializer._serialize_param_value(True)
        assert t == "bool"
        assert v == "true"
        assert DAWorkflowSerializer._deserialize_param_value(t, v) is True

    def test_serialize_bool_false(self):
        t, v = DAWorkflowSerializer._serialize_param_value(False)
        assert t == "bool"
        assert v == "false"
        assert DAWorkflowSerializer._deserialize_param_value(t, v) is False

    def test_bool_not_serialized_as_int(self):
        """bool is a subclass of int; ensure bool check comes first."""
        t, _ = DAWorkflowSerializer._serialize_param_value(True)
        assert t == "bool", "True should be serialized as 'bool', not 'int'"

    def test_serialize_none(self):
        t, v = DAWorkflowSerializer._serialize_param_value(None)
        assert t == "none"
        assert DAWorkflowSerializer._deserialize_param_value(t, v) is None

    def test_serialize_list(self):
        data = [1, "two", 3.0]
        t, v = DAWorkflowSerializer._serialize_param_value(data)
        assert t == "json"
        result = DAWorkflowSerializer._deserialize_param_value(t, v)
        assert result == data

    def test_serialize_dict(self):
        data = {"key": "value", "num": 42}
        t, v = DAWorkflowSerializer._serialize_param_value(data)
        assert t == "json"
        result = DAWorkflowSerializer._deserialize_param_value(t, v)
        assert result == data

    def test_deserialize_unknown_type_returns_string(self):
        result = DAWorkflowSerializer._deserialize_param_value("unknown_type", "raw")
        assert result == "raw"


# =========================================================
# Tests: edge cases
# =========================================================


class TestXmlEdgeCases:
    """Test edge cases for XML serialization."""

    def test_empty_workflow(self):
        wf = DAWorkflow(name="empty")
        serializer = DAWorkflowSerializer()
        xml_str = serializer.to_xml(wf)

        factory = _make_factory()
        wf2 = serializer.from_xml(xml_str, factory)
        assert wf2.name == "empty"
        assert len(wf2._nodes) == 0
        assert len(wf2.get_connections()) == 0

    def test_none_params_not_serialized(self):
        """Parameters with None value should not appear in XML."""
        wf = DAWorkflow(name="test")
        factory = _make_factory()
        n1 = factory.create_node("test.ProcessNode")
        n1.node_id = "pn_1"
        n1.tags = None  # explicitly set to None
        wf.add_node(n1)

        serializer = DAWorkflowSerializer()
        element = serializer.to_xml_element(wf)
        node_ele = element.find(".//node[@node_id='pn_1']")
        param_names = [p.get("name") for p in node_ele.findall("param")]
        assert "tags" not in param_names

    def test_from_xml_element_missing_nodes_raises(self):
        element = ET.Element("workflow")
        element.set("name", "bad")
        serializer = DAWorkflowSerializer()
        factory = _make_factory()
        with pytest.raises(ValueError, match="nodes"):
            serializer.from_xml_element(element, factory)

    def test_from_xml_no_factory_raises(self):
        element = ET.Element("workflow")
        ET.SubElement(element, "nodes")
        serializer = DAWorkflowSerializer()  # no factory
        with pytest.raises(ValueError, match="node_factory"):
            serializer.from_xml_element(element)
