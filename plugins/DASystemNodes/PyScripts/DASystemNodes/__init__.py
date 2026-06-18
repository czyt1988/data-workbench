# -*- coding: utf-8 -*-
"""
DASystemNodes - DAWorkbench 系统级工作流节点包

本包提供与 DAWorkbench UI 交互、流程控制和数据展示的通用节点。
所有节点使用 @NodeDef 装饰器定义，可通过 DAWorkbench.DAWorkFlowPy 节点注册表自动发现。

节点列表：
- DataToManagerNode: 将数据发布到 DataManager 面板
- TextViewerNode: 在节点体上显示输入数据的文本形式
- IfElseNode: 菱形条件节点，根据 bool 输入选择 true/false 分支
- ConstantNode: 输出常量值
- PrintNode: 打印输入数据到日志/控制台
- DelayNode: 延迟指定秒数
- StartNode: 工作流起点
- EndNode: 工作流终点
"""

from .nodes.data_to_manager import DataToManagerNode
from .nodes.text_viewer import TextViewerNode
from .nodes.condition_if import IfElseNode
from .nodes.constant import ConstantNode
from .nodes.print_node import PrintNode
from .nodes.delay import DelayNode
from .nodes.start import StartNode
from .nodes.end import EndNode

__all__ = [
    "DataToManagerNode",
    "TextViewerNode",
    "IfElseNode",
    "ConstantNode",
    "PrintNode",
    "DelayNode",
    "StartNode",
    "EndNode",
]
