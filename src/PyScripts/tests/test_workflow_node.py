"""
DAWorkflowNode 单元测试

NodeDef 装饰器和 DAWorkflowNode 基类已改为纯 Python（无 da_py_workflow 依赖），
测试不再需要 C++ 绑定 mock。
"""

import os
import sys
import types
import unittest
import importlib.util


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
NodeDisplay = _node_def_mod.NodeDisplay
Input = sys.modules["DAWorkbench.DAWorkFlowPy.types"].Input
Output = sys.modules["DAWorkbench.DAWorkFlowPy.types"].Output


@NodeDef(name="SampleNode", category="Test", render_template="nodestyle")
class SampleNode:

    class Inputs:
        data = Input("DataFrame", required=True)

    class Outputs:
        result = Output("DataFrame")

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


class TestNodeDefPurePython(unittest.TestCase):
    """验证 NodeDef 装饰器产出的类属性均为纯 Python 类型（无 C++ 依赖）"""

    def test_inputs_are_dicts(self):
        self.assertIsInstance(SampleNode.inputs, list)
        for inp in SampleNode.inputs:
            self.assertIsInstance(inp, dict)

    def test_outputs_are_dicts(self):
        self.assertIsInstance(SampleNode.outputs, list)
        for outp in SampleNode.outputs:
            self.assertIsInstance(outp, dict)

    def test_parameters_are_dicts(self):
        self.assertIsInstance(SampleNode.parameters, list)
        # SampleNode has no Parameter declarations, so it's an empty list
        self.assertEqual(SampleNode.parameters, [])

    def test_node_display_render_template_is_string(self):
        self.assertIsInstance(SampleNode._node_display, NodeDisplay)
        self.assertIsInstance(SampleNode._node_display.render_template, str)
        self.assertEqual(SampleNode._node_display.render_template, "nodestyle")

    def test_node_display_style_is_dict_or_none(self):
        self.assertIsInstance(SampleNode._node_display, NodeDisplay)
        # No style provided → should be None
        self.assertIsNone(SampleNode._node_display.style)

    def test_input_keys_are_strings(self):
        self.assertIsInstance(SampleNode.input_keys, list)
        for key in SampleNode.input_keys:
            self.assertIsInstance(key, str)

    def test_output_keys_are_strings(self):
        self.assertIsInstance(SampleNode.output_keys, list)
        for key in SampleNode.output_keys:
            self.assertIsInstance(key, str)

    def test_input_dict_has_required_keys(self):
        for inp in SampleNode.inputs:
            self.assertIn("name", inp)
            self.assertIn("data_type", inp)
            self.assertIn("required", inp)

    def test_output_dict_has_required_keys(self):
        for outp in SampleNode.outputs:
            self.assertIn("name", outp)
            self.assertIn("data_type", outp)


if __name__ == "__main__":
    unittest.main()