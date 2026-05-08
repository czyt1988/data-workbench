"""
DAWorkFlowPy - DA 工作流 Python 模块

本模块提供工作流节点类型的定义、描述、注册、DAG 模型、信号传播和执行编排功能。
通过 NodeDef 装饰器声明节点类型，使用 Input/Output/Parameter
描述节点接口，通过 DANodeRegistry 注册和发现节点。
DAWorkflow 管理 DAG 模型，DASignalManager 管理基于事件驱动的数据传播，
DAConnection 描述节点间的数据连接关系，
DAWorkflowExecutor 提供工作流执行编排引擎。

主要导出：
- NodeDef: 节点定义装饰器
- Input: 输出端口声明
- Output: 输出端口声明
- Parameter: 参数声明
- DANodeDescriptor: 节点描述符
- DAPortDescriptor: 端口描述符
- DAParameterDescriptor: 参数描述符
- DANodeRegistry: 节点注册表
- DAWorkflow: 工作流 DAG 模型
- DASignalManager: 信号管理器（事件驱动数据传播）
- DAWorkflowState: 工作流运行状态枚举
- DAConnection: 节点间连接关系
- DAWorkflowExecutor: 工作流执行编排引擎
- DAExecutorState: 执行器状态枚举
"""

__version__ = "1.0.0"
__author__ = "DA WorkBench Team"

from .types import Input, Output, Parameter
from .node_def import NodeDef
from da_py_workflow import DANodeDescriptor
from da_py_workflow import DAPortDescriptor, DAParameterDescriptor
from .node_registry import DANodeRegistry
from .connection import DAConnection
from .workflow import DAWorkflow
from .signal_manager import DASignalManager, DAWorkflowState
from .executor import DAWorkflowExecutor, DAExecutorState
from .syntax import NodeProxy, NodeOutputProxy, NodeInputProxy

# 导入节点模块（供 DANodeRegistry 发现）
from . import nodes

__all__ = [
    "NodeDef",
    "Input",
    "Output",
    "Parameter",
    "DANodeDescriptor",
    "DAPortDescriptor",
    "DAParameterDescriptor",
    "DANodeRegistry",
    "DAWorkflow",
    "DASignalManager",
    "DAWorkflowState",
    "DAConnection",
    "DAWorkflowExecutor",
    "DAExecutorState",
    "NodeProxy",
    "NodeOutputProxy",
    "NodeInputProxy",
]