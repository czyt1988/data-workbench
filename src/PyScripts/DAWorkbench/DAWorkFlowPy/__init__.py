"""
DAWorkFlowPy - DA 工作流 Python 模块

本模块提供工作流节点类型的定义、描述、注册、DAG 模型、信号传播、执行编排和序列化功能。
通过 NodeDef 装饰器声明节点类型，使用 Input/Output/Parameter
描述节点接口，通过 DANodeRegistry 注册和发现节点。
DAWorkflow 管理 DAG 模型，DASignalManager 管理基于事件驱动的数据传播，
DAConnection 描述节点间的数据连接关系，
DAWorkflowExecutor 提供工作流执行编排引擎，
DAWorkflowSerializer 提供工作流序列化/反序列化功能。

模块架构（调用关系）::

    ┌──────────────────────────────────────────────────────┐
    │ 定义层：NodeDef + Input/Output/Parameter + DAWorkflowNode │  ← 声明节点类型
    ├──────────────────────────────────────────────────────┤
    │ 注册层：DANodeRegistry ← discover() ← scan_paths/entry_points │  ← 发现并注册节点类
    │         DANodeFactory  ← discover()/create_node()/get_metadata() │  ← C++ 调用入口
    ├──────────────────────────────────────────────────────┤
    │ 模型层：DAWorkflow ← add_node()/add_connection()/get_connections() │  ← DAG 拓扑管理
    │         DAConnection ← source→target 端口映射           │  ← 有向图边
    ├──────────────────────────────────────────────────────┤
    │ 执行层：DAWorkflowExecutor ← execute()/execute_async()  │  ← 拓扑排序执行引擎
    │         DASignalManager ← send_output()/process_pending() │  ← 数据传播
    ├──────────────────────────────────────────────────────┤
    │ 序列层：DAWorkflowSerializer ← to_dict()/from_dict()    │  ← JSON 序列化/反序列化
    └──────────────────────────────────────────────────────┤

主要导出：
- NodeDef: 节点定义装饰器，收集 Input/Output/Parameter 声明并设置类属性
- Input: 输入端口声明，描述节点接收数据的端口
- Output: 输出端口声明，描述节点输出数据的端口
- Parameter: 参数声明，描述节点的可配置参数
- NodeDisplay: 节点渲染/显示属性 dataclass（icon、render_template、body_shape 等所有样式字段）
- LinkPointStyle: 连接点（端口）样式配置 dataclass
- DAWorkflowNode: 工作流节点基类，提供 set_input_data/get_output_data 等方法
- DANodeRegistry: 节点注册表，管理节点类的发现、注册和查询
- DANodeFactory: 节点工厂，封装 DANodeRegistry 的发现和实例化功能（C++ 调用入口）
- DAWorkflow: 工作流 DAG 模型，管理节点和连接的拓扑关系
- DASignalManager: 信号管理器，基于事件驱动实现节点间数据传播
- DAWorkflowState: 工作流运行状态枚举（Idle/Running/Completed/Error/Stopped）
- DAConnection: 节点间连接关系，描述 source→target 的端口映射
- DAWorkflowExecutor: 工作流执行编排引擎，基于拓扑排序执行节点
- DAExecutorState: 执行器状态枚举（Idle/Running/Paused/Error/Finished）
- DAWorkflowSerializer: 工作流序列化器，提供 JSON 序列化/反序列化
- NodeProxy: 节点代理（语法糖），支持 A >> B 链式连接语法
- NodeOutputProxy: 输出端口代理，支持 A.out >> B.in 端口级连接
- NodeInputProxy: 输入端口代理，配合 NodeOutputProxy 使用
"""

__version__ = "1.0.0"
__author__ = "DA WorkBench Team"

from .types import Input, Output, Parameter
from .node_def import NodeDef, NodeDisplay, DAWorkflowNode, LinkPointStyle
from .node_registry import DANodeRegistry
from .node_factory import DANodeFactory
from .connection import DAConnection
from .workflow import DAWorkflow
from .signal_manager import DASignalManager, DAWorkflowState
from .executor import DAWorkflowExecutor, DAExecutorState
from .syntax import NodeProxy, NodeOutputProxy, NodeInputProxy
from .serializer import DAWorkflowSerializer

# 导入节点模块（供 DANodeRegistry 发现）
from . import nodes

__all__ = [
    "NodeDef",
    "Input",
    "Output",
    "Parameter",
    "NodeDisplay",
    "LinkPointStyle",
    "DAWorkflowNode",
    "DANodeRegistry",
    "DAWorkflow",
    "DASignalManager",
    "DAWorkflowState",
    "DAConnection",
    "DAWorkflowExecutor",
    "DAExecutorState",
    "DANodeFactory",
    "DAWorkflowSerializer",
    "NodeProxy",
    "NodeOutputProxy",
    "NodeInputProxy",
]