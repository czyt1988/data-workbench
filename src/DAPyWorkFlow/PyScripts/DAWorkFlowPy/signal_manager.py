"""
工作流信号管理器模块

本模块定义了 DASignalManager 类，用于管理工作流中基于事件驱动的数据传播。
DASignalManager 维护信号队列，负责将节点输出数据传递到下游节点的输入端口。

DASignalManager 是纯 Python 实现，不依赖 Qt。状态变更通知通过
DAPythonSignalHandler::callInMainThread（来自 da_interface 模块）传递到 C++ 侧，
而非自定义桥接类。

状态模型：
- Stopped: 工作流未运行
- Paused: 工作流暂停，信号队列暂停处理
- Running: 工作流运行中，信号队列正常处理

使用示例::

    workflow = DAWorkflow()
    workflow.add_node(node1)
    workflow.add_node(node2)
    workflow.add_connection(conn)

    manager = DASignalManager(workflow)
    manager.start()
    # 节点执行后发送输出
    manager.send_output(node1.node_id, "result", data)
    # 处理待传递的信号
    manager.process_pending()
    # 暂停
    manager.pause()
    # 停止
    manager.stop()
"""

from collections import deque
from enum import Enum


class DAWorkflowState(Enum):
    """
    工作流运行状态枚举

    - Stopped: 工作流未运行，信号队列清空
    - Paused: 工作流暂停，信号队列暂停处理但不清空
    - Running: 工作流运行中，信号队列正常处理
    """

    Stopped = "stopped"
    Paused = "paused"
    Running = "running"


class _Signal:
    """
    内部信号对象

    封装节点输出数据及传播路径信息，存储在信号队列中等待处理。

    :param source_node_id: 源节点 ID
    :param output_channel: 输出端口名称
    :param data: 输出数据
    :param target_connections: 目标连接列表（DAConnection 实例）
    """

    __slots__ = ("source_node_id", "output_channel", "data", "target_connections")

    def __init__(self, source_node_id: str, output_channel: str, data: object, target_connections: list):
        self.source_node_id = source_node_id
        self.output_channel = output_channel
        self.data = data
        self.target_connections = target_connections


class DASignalManager:
    """
    工作流信号管理器

    管理工作流中节点之间的数据传播。维护信号队列，将节点的输出数据
    通过连接传递到下游节点的输入端口。

    状态变更回调：
    当工作流状态发生变更时，通过 DAPythonSignalHandler::callInMainThread
    （来自 da_interface 模块）将状态变更通知传递到 C++ 侧。
    本类不创建自定义桥接类，而是依赖已有的 da_interface 机制。

    .. note::
        状态变更通知将使用 DAPythonSignalHandler::callInMainThread，
        此方法来自 da_interface 模块，负责将 Python 侧的回调安全地
        在 Qt 主线程中执行。无需自定义桥接类。

    使用示例::

        manager = DASignalManager(workflow)
        manager.start()
        manager.send_output("node_1", "result", dataframe)
        manager.process_pending()

    :param workflow: DAWorkflow 实例
    :param on_state_change: 可选的状态变更回调函数，参数为 (old_state, new_state)
    """

    def __init__(self, workflow: "DAWorkflow", on_state_change=None):
        self._workflow = workflow
        self._state = DAWorkflowState.Stopped
        # 信号队列，使用 deque 实现 FIFO
        self._pending_signals: deque = deque()
        # 入度计数器，记录每个节点已收到的输入数据数量
        # 参考 C++ DAWorkFlowExecuter 的 mNodeIndegreeSetCount 模式
        self._indegree_received: dict = {}
        # 状态变更回调
        # 注意：实际的状态变更通知将使用 DAPythonSignalHandler::callInMainThread
        # 此回调为可选的 Python 侧回调
        self._on_state_change = on_state_change

    @property
    def state(self) -> DAWorkflowState:
        """
        获取当前工作流状态

        :return: 当前状态（Stopped / Paused / Running）
        """
        return self._state

    def start(self):
        """
        启动工作流信号管理器

        将状态切换为 Running，初始化入度计数器。
        如果当前状态为 Stopped，会触发状态变更回调。

        .. note::
            状态变更通知通过 DAPythonSignalHandler::callInMainThread
            （da_interface 模块）传递到 C++ 侧。
        """
        old_state = self._state
        self._state = DAWorkflowState.Running
        # 初始化入度计数器
        self._indegree_received.clear()
        for node_id in self._workflow._nodes:
            self._indegree_received[node_id] = 0
        self._notify_state_change(old_state, self._state)

    def stop(self):
        """
        停止工作流信号管理器

        将状态切换为 Stopped，清空信号队列和入度计数器。
        如果当前状态为 Running 或 Paused，会触发状态变更回调。

        .. note::
            状态变更通知通过 DAPythonSignalHandler::callInMainThread
            （da_interface 模块）传递到 C++ 侧。
        """
        old_state = self._state
        self._state = DAWorkflowState.Stopped
        self._pending_signals.clear()
        self._indegree_received.clear()
        self._notify_state_change(old_state, self._state)

    def pause(self):
        """
        暂停工作流信号管理器

        将状态切换为 Paused，信号队列暂停处理但不清空。
        只有在 Running 状态下才能暂停。

        .. note::
            状态变更通知通过 DAPythonSignalHandler::callInMainThread
            （da_interface 模块）传递到 C++ 侧。
        """
        if self._state != DAWorkflowState.Running:
            return
        old_state = self._state
        self._state = DAWorkflowState.Paused
        self._notify_state_change(old_state, self._state)

    def resume(self):
        """
        恢复工作流信号管理器

        将状态从 Paused 切换为 Running，恢复信号队列处理。
        只有在 Paused 状态下才能恢复。

        .. note::
            状态变更通知通过 DAPythonSignalHandler::callInMainThread
            （da_interface 模块）传递到 C++ 侧。
        """
        if self._state != DAWorkflowState.Paused:
            return
        old_state = self._state
        self._state = DAWorkflowState.Running
        self._notify_state_change(old_state, self._state)

    def send_output(self, node_id: str, output_channel: str, data: object):
        """
        将节点输出数据加入信号队列

        查找该节点输出端口对应的下游连接，将数据封装为信号对象
        加入待处理队列。如果当前状态为 Stopped，信号不会被加入。

        参考 C++ DAWorkFlowExecuter 的 sendParam 模式：
        先将数据放入队列，后续通过 process_pending() 传递到下游节点。

        :param node_id: 输出节点的 ID
        :param output_channel: 输出端口名称
        :param data: 输出数据（任意 Python 对象）
        :raises ValueError: 如果当前状态为 Stopped
        """
        if self._state == DAWorkflowState.Stopped:
            raise ValueError("工作流未运行，无法发送输出信号")

        # 查找下游连接
        downstream_connections = self._workflow.get_downstream_connections(node_id, output_channel)
        if not downstream_connections:
            # 无下游连接，数据无需传递
            return

        # 创建信号对象加入队列
        signal = _Signal(
            source_node_id=node_id,
            output_channel=output_channel,
            data=data,
            target_connections=downstream_connections,
        )
        self._pending_signals.append(signal)

    def process_pending(self) -> int:
        """
        处理信号队列中的待传递信号

        从信号队列中逐个取出信号对象，将数据传递到下游节点的输入端口，
        并更新入度计数器。参考 C++ DAWorkFlowExecuter 的 sendParam 和
        transmit 模式：

        - sendParam: 将节点输出数据设置到下游节点的输入端口
        - transmit: 检查下游节点是否满足执行条件（入度计数 == 总入度数）

        只有在 Running 状态下才能处理信号。

        :return: 本次处理的信号数量
        """
        if self._state != DAWorkflowState.Running:
            return 0

        processed_count = 0
        while self._pending_signals:
            signal = self._pending_signals.popleft()
            self._deliver_signal(signal)
            processed_count += 1

        return processed_count

    def _deliver_signal(self, signal: _Signal):
        """
        将信号数据传递到目标节点

        对每条目标连接，将数据设置到目标节点实例的输入端口，
        并递增入度计数器。

        参考 C++ DAWorkFlowExecuter::PrivateData::sendParam 的实现模式：
        1. 获取节点的输出数据
        2. 对每条连接，将数据设置到目标节点的输入端口
        3. 递增目标节点的入度计数

        :param signal: 内部信号对象
        """
        for conn in signal.target_connections:
            target_node = self._workflow.get_node_by_id(conn.target_node_id)
            # 将数据设置到目标节点的输入端口
            # 使用 set_input_data 方法，如果节点实例没有此方法，
            # 则设置到 _input_data 字典中
            if hasattr(target_node, "set_input_data"):
                target_node.set_input_data(conn.target_input_channel, signal.data)
            else:
                # 节点实例没有 set_input_data 方法，使用内部字典存储
                if not hasattr(target_node, "_input_data"):
                    target_node._input_data = {}
                target_node._input_data[conn.target_input_channel] = signal.data

            # 递增入度计数器
            self._indegree_received[conn.target_node_id] = (
                self._indegree_received.get(conn.target_node_id, 0) + 1
            )

    def is_node_ready(self, node_id: str) -> bool:
        """
        检查节点是否满足执行条件

        判断节点已收到的输入数据数量是否等于其总入度数（上游连接数）。
        参考 C++ DAWorkFlowExecuter::PrivateData::transmit 的实现模式：
        只有当 mNodeIndegreeSetCount[node] == getInputNodesCount(node) 时，
        节点才满足执行条件。

        :param node_id: 节点 ID
        :return: True 表示节点已收到所有必要输入数据
        """
        total_indegree = len(self._workflow.get_upstream_connections(node_id))
        received = self._indegree_received.get(node_id, 0)
        return received == total_indegree

    def get_pending_count(self) -> int:
        """
        获取信号队列中待处理的信号数量

        :return: 待处理信号数量
        """
        return len(self._pending_signals)

    def _notify_state_change(self, old_state: DAWorkflowState, new_state: DAWorkflowState):
        """
        通知状态变更

        触发 Python 侧回调，并通过 DAPythonSignalHandler::callInMainThread
        将状态变更通知传递到 C++ 侧。

        .. note::
            状态变更到 C++ 侧的通知使用 DAPythonSignalHandler::callInMainThread，
            此方法来自 da_interface 模块。无需创建自定义桥接类。
            Python 侧通过 self._on_state_change 回调接收通知。

        :param old_state: 原状态
        :param new_state: 新状态
        """
        # Python 侧回调
        if self._on_state_change is not None:
            self._on_state_change(old_state.value, new_state.value)

        # C++ 侧通知通过 da_interface 的 DAPythonSignalHandler::callInMainThread
        # 此处不直接调用，而是在 Python binding 层面由 C++ 调用 Python 时
        # 通过 da_interface 模块提供的 callInMainThread 机制实现
        # 实际实现方式：在 DAPyModuleWorkflow 或绑定层中，
        # 将 DASignalManager 的状态变更回调与 da_interface 的
        # DAPythonSignalHandler::callInMainThread 连接

    def __repr__(self) -> str:
        return (
            f"DASignalManager(state='{self._state.value}', "
            f"pending={len(self._pending_signals)}, "
            f"workflow={self._workflow})"
        )