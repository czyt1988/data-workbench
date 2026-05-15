"""
DAWorkflowNode 单元测试

注意: node_def.py 顶层 import da_py_workflow (C++ 绑定),
测试需预先注入 mock 模块.
"""

import os
import sys
import types
import unittest
import importlib.util
from unittest.mock import MagicMock


def _mock_da_py_workflow():
    mod = types.ModuleType("da_py_workflow")
    mod.DANodeDescriptor = MagicMock
    mod.DAPortDescriptor = MagicMock
    mod.DAParameterDescriptor = MagicMock
    mod.DANodeStyle = MagicMock
    mod.DAPyLinkPointStyle = MagicMock

    class RenderTemplate:
        NodeStyleTemplate = 0
        WidgetTemplate = 1

    mod.RenderTemplate = RenderTemplate
    return mod


sys.modules["da_py_workflow"] = _mock_da_py_workflow()


def _load_types_module():
    types_path = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "DAWorkbench", "DAWorkFlowPy", "types.py")
    )
    mod = types.ModuleType("DAWorkbench.DAWorkFlowPy.types")
    with open(types_path, "r", encoding="utf-8") as f:
        code = f.read()
    exec(compile(code, types_path, "exec"), mod.__dict__)
    return mod


_pkg = types.ModuleType("DAWorkbench")
sys.modules["DAWorkbench"] = _pkg
_wfpkg = types.ModuleType("DAWorkbench.DAWorkFlowPy")
sys.modules["DAWorkbench.DAWorkFlowPy"] = _wfpkg
sys.modules["DAWorkbench.DAWorkFlowPy.types"] = _load_types_module()

_node_def_path = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "DAWorkbench", "DAWorkFlowPy", "node_def.py")
)
_spec = importlib.util.spec_from_file_location("DAWorkbench.DAWorkFlowPy.node_def", _node_def_path)
_node_def_mod = importlib.util.module_from_spec(_spec)
sys.modules["DAWorkbench.DAWorkFlowPy.node_def"] = _node_def_mod
_spec.loader.exec_module(_node_def_mod)

DAWorkflowNode = _node_def_mod.DAWorkflowNode
NodeDef = _node_def_mod.NodeDef


@NodeDef(name="SampleNode", category="Test", render_template="nodestyle")
class SampleNode:

    def execute(self, inputs, params):
        pass


class TestDAWorkflowNodeDirect(unittest.TestCase):

    def test_input_data_initialized_as_empty_dict(self):
        node = DAWorkflowNode()
        self.assertEqual(node._input_data, {})

    def test_output_data_initialized_as_empty_dict(self):
        node = DAWorkflowNode()
        self.assertEqual(node._output_data, {})

    def test_is_global_initialized_as_false(self):
        node = DAWorkflowNode()
        self.assertFalse(node.is_global)

    def test_set_input_data_stores_value(self):
        node = DAWorkflowNode()
        node.set_input_data("data", [1, 2, 3])
        self.assertEqual(node._input_data["data"], [1, 2, 3])

    def test_set_input_data_overwrites_existing(self):
        node = DAWorkflowNode()
        node.set_input_data("key", "old")
        node.set_input_data("key", "new")
        self.assertEqual(node._input_data["key"], "new")

    def test_get_output_data_returns_stored_value(self):
        node = DAWorkflowNode()
        node._output_data["result"] = {"col": [1, 2]}
        self.assertEqual(node.get_output_data("result"), {"col": [1, 2]})

    def test_get_output_data_returns_none_for_missing_key(self):
        node = DAWorkflowNode()
        self.assertIsNone(node.get_output_data("nonexistent"))

    def test_get_output_data_returns_none_for_empty_output(self):
        node = DAWorkflowNode()
        self.assertIsNone(node.get_output_data("any_key"))


class TestNodeDefInheritance(unittest.TestCase):

    def test_isinstance_check_passes(self):
        node = SampleNode()
        self.assertIsInstance(node, DAWorkflowNode)

    def test_decorated_class_has_input_data(self):
        node = SampleNode()
        self.assertEqual(node._input_data, {})

    def test_decorated_class_has_output_data(self):
        node = SampleNode()
        self.assertEqual(node._output_data, {})

    def test_decorated_class_has_is_global(self):
        node = SampleNode()
        self.assertFalse(node.is_global)

    def test_decorated_class_set_input_data(self):
        node = SampleNode()
        node.set_input_data("input1", "hello")
        self.assertEqual(node._input_data["input1"], "hello")

    def test_decorated_class_get_output_data(self):
        node = SampleNode()
        node._output_data["out1"] = 42
        self.assertEqual(node.get_output_data("out1"), 42)

    def test_decorated_class_get_output_data_missing(self):
        node = SampleNode()
        self.assertIsNone(node.get_output_data("missing"))


if __name__ == "__main__":
    unittest.main()
