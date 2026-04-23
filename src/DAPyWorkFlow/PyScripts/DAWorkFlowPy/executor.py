"""
工作流执行器模块

本模块定义了 DAWorkflowExecutor 类，提供 Python 侧的工作流执行编排功能。
DAWorkflowExecutor 基于拓扑排序和入度计数模式，按序执行工作流节点，
并通过 DASignalManager 进行数据传播。

DAWorkflowExecutor 支持：
- 拓扑排序执行：按入度计数确定节点执行顺序
- 异步执行模式：通过 threading 在后台线程中执行工作流
- 状态变更通知：通过 DAPythonSignalHandler::callInMainThread 传递到 C++ 侧
- 终止/暂停支持：可中断工作流执行

状态变更通知使用 DAPythonSignalHandler::callInMainThread（来自 da_interface 模块），
不创建自定义桥接类。

使用示例::

    workflow = DAWorkflow()
    workflow.add_node(node1)
    workflow.add_node(node2)
    workflow.add_connection(conn)

    executor = DAWorkflowExecutor(workflow)
    # 同步执行
    success = executor.execute()

    # 异步执行
    executor.execute_async()
    executor.wait_completion(timeout=30)

    # 暂停/恢复
    executor.pause()
    executor.resume()

    # 终止
    executor.terminate()
"""

import threading
from enum import Enum
from .workflow import DAWorkflow
from .signal_manager import DASignalManager, DAWorkflowState


class DAExecutorState(Enum):
    """
    执行器状态枚举

    与 C++ DAPyWorkFlowExecuter::ExecState 对应：
    - Idle: 空闲，未开始执行
    - Running: 运行中
    - Paused: 已暂停
    - Error: 执行出错
    - Finished: 执行完成
    """

    Idle = "idle"
    Running = "running"
    Paused = "paused"
    Error = "error"
    Finished = "finished"


class DAWorkflowExecutor:
    """
    工作流执行器

    基于拓扑排序和入度计数的工作流执行引擎，参考 C++ DAWorkFlowExecuter
    的实现模式：
    1. 分类节点为全局节点、孤立节点、开始节点
    2. 按序执行全局节点（执行但不传递数据）
    3. 按序执行孤立节点和开始节点（执行并传递数据）
    4. 下游节点入度满足时触发执行

    状态变更通知通过 DAPythonSignalHandler::callInMainThread
    （da_interface 模块）传递到 C++ 侧，不创建自定义桥接类。

    使用示例::

        executor = DAWorkflowExecutor(workflow)
        success = executor.execute()

        # 异步执行
        executor.execute_async()
        executor.wait_completion()

    :param workflow: DAWorkflow 实例
    :param on_node_finished: 可选的节点完成回调，参数为 (node_id, success)
    :param on_state_change: 可选的状态变更回调，参数为 (old_state, new_state)
    :param on_progress: 可选的进度回调，参数为 (current, total)
    """

    def __init__(
        self,
        workflow: DAWorkflow,
        on_node_finished=None,
        on_state_change=None,
        on_progress=None,
    ):
        self._workflow = workflow
        self._signal_manager = DASignalManager(workflow)
        self._state = DAExecutorState.Idle
        self._terminate_requested = False
        self._pause_requested = False
        self._pause_event = threading.Event()
        self._pause_event.set()  # 初始为非暂停状态
        self._execution_thread = None
        self._lock = threading.Lock()
        self._result = None

        # 回调函数
        self._on_node_finished = on_node_finished
        self._on_state_change = on_state_change
        self._on_progress = on_progress

        # 执行统计
        self._executed_count = 0
        self._total_count = 0
        self._error_messages = []

    @property
    def state(self) -> DAExecutorState:
        """
        获取当前执行器状态

        :return: 当前状态
        """
        return self._state

    @property
    def signal_manager(self) -> DASignalManager:
        """
        获取关联的信号管理器

        :return: DASignalManager 实例
        """
        return self._signal_manager

    @property
    def result(self) -> bool:
        """
        获取执行结果

        :return: True 表示成功，False 表示失败，None 表示未执行
        """
        return self._result

    @property
    def error_messages(self) -> list:
        """
        获取执行过程中的错误信息列表

        :return: 错误信息字符串列表
        """
        return self._error_messages

    def execute(self) -> bool:
        """
        同步执行工作流

        在当前线程中执行工作流，按照拓扑排序顺序逐个执行节点。
        执行过程中会检查终止和暂停请求。

        :return: True 表示执行成功，False 表示执行失败或被终止
        """
        with self._lock:
            self._terminate_requested = False
            self._pause_requested = False
            self._pause_event.set()
            self._executed_count = 0
            self._total_count = len(self._workflow._nodes)
            self._error_messages.clear()
            self._result = None

        self._set_state(DAExecutorState.Running)

        # 启动信号管理器
        self._signal_manager.start()

        try:
            success = self._run_workflow()
            self._result = success
            return success
        except Exception as e:
            self._error_messages.append(str(e))
            self._set_state(DAExecutorState.Error)
            self._result = False
            return False
        finally:
            # 停止信号管理器
            self._signal_manager.stop()
            if self._state != DAExecutorState.Error:
                self._set_state(DAExecutorState.Finished)

    def execute_async(self):
        """
        异步执行工作流

        在后台线程中执行工作流，不阻塞当前线程。
        可通过 wait_completion() 等待执行完成，
        或通过 state 属性检查执行状态。

        .. note::
            状态变更通知通过 DAPythonSignalHandler::callInMainThread
            （da_interface 模块）传递到 C++ 侧。
        """
        if self._execution_thread is not None and self._execution_thread.is_alive():
            return  # 已有执行线程在运行

        self._execution_thread = threading.Thread(
            target=self._execute_thread_func,
            daemon=True,
        )
        self._execution_thread.start()

    def wait_completion(self, timeout: float = None) -> bool:
        """
        等待异步执行完成

        :param timeout: 最大等待时间（秒），None 表示无限等待
        :return: True 表示执行完成，False 表示超时
        """
        if self._execution_thread is None:
            return True
        self._execution_thread.join(timeout=timeout)
        return not self._execution_thread.is_alive()

    def terminate(self):
        """
        请求终止执行

        设置终止标记，当前正在执行的节点完成后将停止后续节点执行。
        不会中断正在执行中的节点，而是等待其完成后再停止。
        """
        with self._lock:
            self._terminate_requested = True
            # 如果处于暂停状态，唤醒以让终止生效
            self._pause_requested = False
            self._pause_event.set()

    def pause(self):
        """
        暂停执行

        设置暂停标记，在当前节点完成后暂停后续节点执行。
        只有在 Running 状态下才能暂停。
        """
        if self._state != DAExecutorState.Running:
            return
        with self._lock:
            self._pause_requested = True
            self._pause_event.clear()
        self._set_state(DAExecutorState.Paused)

    def resume(self):
        """
        恢复执行

        清除暂停标记并唤醒等待的线程，
        恢复到 Running 状态继续执行后续节点。
        """
        if self._state != DAExecutorState.Paused:
            return
        with self._lock:
            self._pause_requested = False
            self._pause_event.set()
        self._set_state(DAExecutorState.Running)

    def get_progress(self) -> tuple:
        """
        获取执行进度

        :return: (已执行节点数, 总节点数) 的元组
        """
        return (self._executed_count, self._total_count)

    # ==================== 内部方法 ====================

    def _execute_thread_func(self):
        """
        异步执行线程函数

        在后台线程中调用 execute()，执行完成后设置结果。
        """
        self.execute()

    def _set_state(self, new_state: DAExecutorState):
        """
        设置执行器状态并触发回调

        状态变更回调流程：
        1. Python 侧回调 on_state_change
        2. C++ 侧通知通过 DAPythonSignalHandler::callInMainThread

        :param new_state: 新状态
        """
        old_state = self._state
        if old_state == new_state:
            return
        self._state = new_state

        # Python 侧回调
        if self._on_state_change is not None:
            self._on_state_change(old_state.value, new_state.value)

        # C++ 侧通知：通过 da_interface 的 DAPythonSignalHandler::callInMainThread
        # 在 Python binding 层面由 C++ 调用 Python 时实现连接
        # 此处仅触发 Python 侧回调，C++ 侧连接在绑定层实现

    def _run_workflow(self) -> bool:
        """
        执行工作流核心逻辑

        参考 C++ DAWorkFlowExecuter::startExecute 的实现模式：
        1. 分类节点为全局节点、孤立节点、开始节点
        2. 执行全局节点（不传递数据）
        3. 执行孤立节点（传递数据）
        4. 执行开始节点（传递数据到下游）
        5. 下游节点通过入度计数触发执行

        :return: True 表示全部节点执行成功，False 表示有节点失败或被终止
        """
        # 分类节点
        global_nodes, isolated_nodes, begin_nodes = self._classify_nodes()

        # 执行全局节点（执行但不传递数据）
        for node_id in global_nodes:
            if self._check_interrupt():
                return False
            success = self._execute_node(node_id, transmit=False)
            if not success:
                self._error_messages.append(
                    f"全局节点 '{node_id}' 执行失败"
                )

        # 执行孤立节点（执行并传递数据）
        for node_id in isolated_nodes:
            if self._check_interrupt():
                return False
            success = self._execute_node(node_id, transmit=True)
            if not success:
                self._error_messages.append(
                    f"孤立节点 '{node_id}' 执行失败"
                )

        # 执行开始节点（执行并传递数据到下游）
        for node_id in begin_nodes:
            if self._check_interrupt():
                return False
            # 如果开始节点也是全局节点，只传递数据
            if node_id in global_nodes:
                self._propagate_and_transmit(node_id)
            else:
                success = self._execute_node(node_id, transmit=True)
                if not success:
                    self._error_messages.append(
                        f"开始节点 '{node_id}' 执行失败"
                    )

        return len(self._error_messages) == 0

    def _classify_nodes(self) -> tuple:
        """
        分类节点为全局节点、孤立节点、开始节点

        参考 C++ DAWorkFlowExecuter::PrivateData::prepareStartExec 的实现模式：
        - 入度=0 且 出度=0 的节点为孤立节点（非全局）
        - 入度=0 且 出度>0 的节点为开始节点
        - 有 is_global 属性的节点为全局节点

        :return: (全局节点ID列表, 孤立节点ID列表, 开始节点ID列表) 的元组
        """
        global_nodes = []
        isolated_nodes = []
        begin_nodes = []

        # 计算入度和出度
        connections = self._workflow.get_connections()
        in_degree = {}
        out_degree = {}

        for node_id in self._workflow._nodes:
            in_degree[node_id] = 0
            out_degree[node_id] = 0

        for conn in connections:
            out_degree[conn.source_node_id] = out_degree.get(conn.source_node_id, 0) + 1
            in_degree[conn.target_node_id] = in_degree.get(conn.target_node_id, 0) + 1

        # 分类节点
        for node_id, node_instance in self._workflow._nodes.items():
            is_global = getattr(node_instance, "is_global", False)

            if is_global:
                global_nodes.append(node_id)

            indeg = in_degree.get(node_id, 0)
            outdeg = out_degree.get(node_id, 0)

            if indeg == 0:
                if outdeg == 0:
                    # 孤立节点（非全局）
                    if not is_global:
                        isolated_nodes.append(node_id)
                else:
                    # 开始节点
                    begin_nodes.append(node_id)

        return (global_nodes, isolated_nodes, begin_nodes)

    def _execute_node(self, node_id: str, transmit: bool = True) -> bool:
        """
        执行单个节点

        :param node_id: 节点 ID
        :param transmit: 是否在执行后传递数据到下游
        :return: True 表示执行成功
        """
        node_instance = self._workflow.get_node_by_id(node_id)
        if node_instance is None:
            self._error_messages.append(f"节点 '{node_id}' 不存在")
            return False

        # 执行节点
        try:
            result = node_instance.execute()
            success = bool(result) if result is not None else True
        except Exception as e:
            self._error_messages.append(f"节点 '{node_id}' 执行异常: {e}")
            success = False

        self._executed_count += 1

        # 触发进度回调
        if self._on_progress is not None:
            self._on_progress(self._executed_count, self._total_count)

        # 触发节点完成回调
        if self._on_node_finished is not None:
            self._on_node_finished(node_id, success)

        # 传播数据到下游
        if success and transmit:
            self._propagate_and_transmit(node_id)

        return success

    def _propagate_and_transmit(self, node_id: str):
        """
        传播节点输出数据并检查下游节点是否可以执行

        参考 C++ DAWorkFlowExecuter 的 sendParam + transmit 模式：
        1. 通过 DASignalManager.send_output 发送输出数据
        2. 通过 DASignalManager.process_pending 传递数据到下游节点输入端口
        3. 通过 DASignalManager.is_node_ready 检查下游节点是否满足执行条件

        :param node_id: 节点 ID
        """
        node_instance = self._workflow.get_node_by_id(node_id)
        if node_instance is None:
            return

        # 获取输出端口列表
        output_keys = []
        descriptor = getattr(node_instance, "_node_descriptor", {})
        outputs = descriptor.get("outputs", [])
        for output_info in outputs:
            output_keys.append(output_info.get("name", ""))

        # 发送每个输出端口的数据
        for output_key in output_keys:
            output_data = getattr(node_instance, "_output_data", {}).get(output_key)
            if output_data is not None:
                self._signal_manager.send_output(node_id, output_key, output_data)

        # 处理待传递的信号
        self._signal_manager.process_pending()

        # 检查下游节点是否满足执行条件
        downstream_conns = self._workflow.get_downstream_connections(node_id)
        for conn in downstream_conns:
            if self._signal_manager.is_node_ready(conn.target_node_id):
                if self._check_interrupt():
                    return
                self._execute_node(conn.target_node_id, transmit=True)

    def _check_interrupt(self) -> bool:
        """
        检查是否需要中断执行（终止或暂停）

        :return: True 表示需要中断（终止请求），False 表示可以继续
        """
        # 检查终止请求
        with self._lock:
            if self._terminate_requested:
                return True

        # 检查暂停请求——等待暂停事件
        self._pause_event.wait()

        # 再次检查终止请求（暂停恢复后可能已终止）
        with self._lock:
            if self._terminate_requested:
                return True

        return False

    def __repr__(self) -> str:
        return (
            f"DAWorkflowExecutor(state='{self._state.value}', "
            f"progress={self._executed_count}/{self._total_count}, "
            f"workflow={self._workflow})"
        )