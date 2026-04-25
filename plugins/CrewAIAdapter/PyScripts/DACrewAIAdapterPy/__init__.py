# -*- coding: utf-8 -*-
"""
DACrewAIAdapterPy — AI Agent CrewAI 适配器节点包

本包提供基于 CrewAI 概念的 AI Agent 工作流可视化节点，
用于在 DAPyWorkFlow 中编排智能体协作流程。

节点类型：
- AgentNode: 定义智能体角色、目标和背景故事
- TaskNode: 定义任务描述和期望输出
- CrewNode: 编排多个 Agent 执行 Task 集合
- ToolNode: 为 Agent 提供工具能力

使用示例::

    from DACrewAIAdapterPy import AgentNode, TaskNode, CrewNode, ToolNode

    # 获取节点描述符
    descriptor = AgentNode._node_descriptor

依赖：
- DAWorkbench.DAWorkFlowPy: 提供 NodeDef/Input/Output/Parameter 基础类型
- crewai: 可选依赖，若未安装则节点以占位模式运行
"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter

from .agent_node import AgentNode
from .task_node import TaskNode
from .crew_node import CrewNode
from .tool_node import ToolNode

__all__ = [
    "AgentNode",
    "TaskNode",
    "CrewNode",
    "ToolNode",
]

__version__ = "1.0.0"
