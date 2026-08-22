# -*- coding: utf-8 -*-
"""
DASystemNodes - DAWorkbench 系统级工作流节点包

This package provides system-level workflow nodes for UI interaction,
flow control, and data display in DAWorkbench.
All nodes are defined using the @NodeDef decorator and auto-discovered
via the DAWorkbench.DAWorkFlowPy node registry.

Node list:
- DataToManagerNode: Publish data to DataManager panel
- TextViewerNode: Display input data as text on the node body (with optional console logging)
- IfElseNode: Diamond-shaped conditional node, selects true/false branch by bool input
- ConstantNode: Output a constant value
- DelayNode: Delay for a specified number of seconds
- StartNode: Workflow start point
- EndNode: Workflow end point
"""

# ⚠️ Must call setup_i18n() before importing node modules.
# Node modules use _() in @NodeDef(category=_(...)) which executes at import time.
from .i18n.core import setup_i18n
setup_i18n()

from .nodes.data_to_manager import DataToManagerNode
from .nodes.text_viewer import TextViewerNode
from .nodes.condition_if import IfElseNode
from .nodes.constant import ConstantNode
from .nodes.delay import DelayNode
from .nodes.start import StartNode
from .nodes.end import EndNode

__all__ = [
    "DataToManagerNode",
    "TextViewerNode",
    "IfElseNode",
    "ConstantNode",
    "DelayNode",
    "StartNode",
    "EndNode",
]
