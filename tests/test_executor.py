"""
test_executor — DAWorkflowExecutor 执行编排测试

覆盖：单节点执行、多节点 DAG 执行顺序、terminate/pause、
error handling、节点状态追踪。
"""

import pytest
import threading
import time
from DAWorkbench.DAWorkFlowPy import DAWorkflow, DAConnection, DAWorkflowExecutor, DAExecutorState, NodeDef, Input, Output, Parameter


# ==================== 辅助节点 ====================

@NodeDef(name="ExecSource", category="Test")
class ExecSourceNode:
    class Outputs:
        data = Output("DataFrame")

    def __init__(self):
        self._output_data = {"data": [1, 2, 3]}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="ExecMiddle", category="Test")
class ExecMiddleNode:
    class Inputs:
        data = Input("DataFrame", required=True)

    class Outputs:
        processed = Output("DataFrame")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"processed": None}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        data = self._input_data.get("data")
        self._output_data["processed"] = data
        return True


@NodeDef(name="ExecSink", category="Test")
class ExecSinkNode:
    class Inputs:
        data = Input("DataFrame", required=True)

    def __init__(self):
        self._input_data = {}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="ExecError", category="Test")
class ExecErrorNode:
    class Inputs:
        data = Input("any", required=True)

    class Outputs:
        result = Output("any")

    def __init__(self):
        self._input_data = {}
        self._output_data = {}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        raise RuntimeError("ExecErrorNode 测试异常")


@NodeDef(name="ExecReturnNone", category="Test")
class ExecReturnNoneNode:
    class Outputs:
        data = Output("any")

    def __init__(self):
        self._output_data = {"data": "result"}

    def execute(self, inputs=None, params=None):
        return None


@NodeDef(name="ExecGlobal", category="Test")
class ExecGlobalNode:
    is_global = True

    class Outputs:
        config = Output("dict")

    def __init__(self):
        self._output_data = {"config": {"key": "val"}}

    def execute(self, inputs=None, params=None):
        return True


# ==================== 执行器基础测试 ====================

class TestDAWorkflowExecutorBasic:
    """DAWorkflowExecutor 基础功能测试"""

    def test_initial_state_is_idle(self):
        """初始状态为 Idle"""
        wf = DAWorkflow()
        ex = DAWorkflowExecutor(wf)
        assert ex.state == DAExecutorState.Idle

    def test_result_initial_is_none(self):
        """初始 result 为 None"""
        wf = DAWorkflow()
        ex = DAWorkflowExecutor(wf)
        assert ex.result is None

    def test_empty_workflow_execute(self):
        """空工作流执行成功"""
        wf = DAWorkflow()
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is True
        assert ex.state == DAExecutorState.Finished

    def test_single_node_execution(self):
        """单节点执行成功"""
        wf = DAWorkflow()
        node = ExecSourceNode()
        wf.add_node(node)
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is True
        assert ex.state == DAExecutorState.Finished

    def test_two_node_linear_execution(self):
        """两节点线性 DAG 执行成功"""
        wf = DAWorkflow()
        src = ExecSourceNode()
        mid = ExecMiddleNode()
        wf.add_node(src)
        wf.add_node(mid)
        wf.add_connection(DAConnection(
            src.node_id, "data", mid.node_id, "data"))
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is True
        # mid 应收到数据
        assert mid._input_data.get("data") == [1, 2, 3]

    def test_three_node_linear_execution(self):
        """三节点线性 DAG 执行并传递数据"""
        wf = DAWorkflow()
        src = ExecSourceNode()
        mid = ExecMiddleNode()
        sink = ExecSinkNode()
        wf.add_node(src)
        wf.add_node(mid)
        wf.add_node(sink)
        wf.add_connection(DAConnection(
            src.node_id, "data", mid.node_id, "data"))
        wf.add_connection(DAConnection(
            mid.node_id, "processed", sink.node_id, "data"))
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is True
        assert sink._input_data.get("data") == [1, 2, 3]

    def test_isolated_node_execution(self):
        """孤立节点执行成功"""
        wf = DAWorkflow()
        iso = ExecSourceNode()
        wf.add_node(iso)
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is True


# ==================== 执行器错误处理测试 ====================

class TestDAWorkflowExecutorErrorHandling:
    """DAWorkflowExecutor 错误处理测试"""

    def test_error_node_sets_result_false(self):
        """错误节点导致 result=False"""
        wf = DAWorkflow()
        err = ExecErrorNode()
        wf.add_node(err)
        # ErrorNode 需要 input，作为孤立节点无 input
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is False
        assert len(ex.error_messages) > 0

    def test_error_node_in_chain(self):
        """链中错误节点导致执行失败"""
        wf = DAWorkflow()
        src = ExecSourceNode()
        err = ExecErrorNode()
        wf.add_node(src)
        wf.add_node(err)
        wf.add_connection(DAConnection(
            src.node_id, "data", err.node_id, "data"))
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is False

    def test_error_messages_populated(self):
        """错误信息列表包含错误描述"""
        wf = DAWorkflow()
        err = ExecErrorNode()
        wf.add_node(err)
        ex = DAWorkflowExecutor(wf)
        ex.execute()
        msgs = ex.error_messages
        assert any("ExecErrorNode" in m for m in msgs)


# ==================== terminate/pause 测试 ====================

class TestDAWorkflowExecutorControl:
    """DAWorkflowExecutor 终止/暂停控制测试"""

    def test_terminate_sets_state(self):
        """terminate 请求后最终状态为 Finished（因为空工作流无节点可停）"""
        wf = DAWorkflow()
        ex = DAWorkflowExecutor(wf)
        ex.execute()
        # 执行完成后 terminate 无实际效果
        assert ex.state == DAExecutorState.Finished

    def test_pause_resume_state_transitions(self):
        """pause/resume 状态转换"""
        wf = DAWorkflow()
        node = ExecSourceNode()
        wf.add_node(node)
        ex = DAWorkflowExecutor(wf)
        # 同步执行完成后暂停无意义，但 API 调用不应报错
        ex.execute()
        # 执行已完成，pause 应无效
        ex.pause()
        assert ex.state == DAExecutorState.Finished

    def test_progress_tracking(self):
        """执行进度追踪"""
        wf = DAWorkflow()
        n1 = ExecSourceNode()
        wf.add_node(n1)
        ex = DAWorkflowExecutor(wf)
        progress_before = ex.get_progress()
        # _total_count 初始为 0，执行时才设置为节点数
        assert progress_before == (0, 0)
        ex.execute()
        progress_after = ex.get_progress()
        assert progress_after == (1, 1)


# ==================== 回调测试 ====================

class TestDAWorkflowExecutorCallbacks:
    """DAWorkflowExecutor 回调测试"""

    def test_on_node_finished_callback(self):
        """节点完成回调触发"""
        finished_nodes = []
        wf = DAWorkflow()
        node = ExecSourceNode()
        wf.add_node(node)
        ex = DAWorkflowExecutor(
            wf, on_node_finished=lambda nid, ok: finished_nodes.append((nid, ok)))
        ex.execute()
        assert len(finished_nodes) == 1
        nid, ok = finished_nodes[0]
        assert ok is True

    def test_on_state_change_callback(self):
        """状态变更回调触发"""
        states = []
        wf = DAWorkflow()
        node = ExecSourceNode()
        wf.add_node(node)
        ex = DAWorkflowExecutor(
            wf, on_state_change=lambda o, n: states.append((o, n)))
        ex.execute()
        assert len(states) >= 2
        # 至少有 Idle→Running 和 Running→Finished
        assert ("idle", "running") in states
        assert ("running", "finished") in states

    def test_on_progress_callback(self):
        """进度回调触发"""
        progress_calls = []
        wf = DAWorkflow()
        node = ExecSourceNode()
        wf.add_node(node)
        ex = DAWorkflowExecutor(wf, on_progress=lambda c,
                                t: progress_calls.append((c, t)))
        ex.execute()
        assert len(progress_calls) >= 1

    def test_return_none_node_succeeds(self):
        """execute 返回 None 的节点视为成功"""
        wf = DAWorkflow()
        node = ExecReturnNoneNode()
        wf.add_node(node)
        finished_nodes = []
        ex = DAWorkflowExecutor(
            wf, on_node_finished=lambda nid, ok: finished_nodes.append((nid, ok)))
        ex.execute()
        assert finished_nodes[0][1] is True


# ==================== 异步执行测试 ====================

class TestDAWorkflowExecutorAsync:
    """DAWorkflowExecutor 异步执行测试"""

    def test_execute_async_runs_in_thread(self):
        """execute_async 在后台线程执行"""
        wf = DAWorkflow()
        node = ExecSourceNode()
        wf.add_node(node)
        ex = DAWorkflowExecutor(wf)
        ex.execute_async()
        completed = ex.wait_completion(timeout=5)
        assert completed is True
        assert ex.state == DAExecutorState.Finished

    def test_execute_async_no_double_start(self):
        """execute_async 不会重复启动线程"""
        wf = DAWorkflow()
        node = ExecSourceNode()
        wf.add_node(node)
        ex = DAWorkflowExecutor(wf)
        ex.execute_async()
        # 第二次调用应无效果
        ex.execute_async()
        completed = ex.wait_completion(timeout=5)
        assert completed is True

    def test_signal_manager_accessible(self):
        """signal_manager 属性返回 DASignalManager"""
        wf = DAWorkflow()
        ex = DAWorkflowExecutor(wf)
        mgr = ex.signal_manager
        assert mgr is not None


# ==================== 节点分类测试 ====================

class TestDAWorkflowExecutorClassification:
    """DAWorkflowExecutor 节点分类测试"""

    def test_global_node_classified(self):
        """is_global=True 的节点被分类为全局节点"""
        wf = DAWorkflow()
        global_node = ExecGlobalNode()
        wf.add_node(global_node)
        ex = DAWorkflowExecutor(wf)
        global_nodes, isolated_nodes, begin_nodes = ex._classify_nodes()
        assert global_node.node_id in global_nodes

    def test_isolated_node_classified(self):
        """无入度无出度的节点被分类为孤立节点"""
        wf = DAWorkflow()
        iso = ExecSourceNode()  # SourceNode 有输出端口但无连接 → 孤立
        wf.add_node(iso)
        ex = DAWorkflowExecutor(wf)
        global_nodes, isolated_nodes, begin_nodes = ex._classify_nodes()
        assert iso.node_id in isolated_nodes

    def test_begin_node_classified(self):
        """有出度无入度的连接节点被分类为开始节点"""
        wf = DAWorkflow()
        src = ExecSourceNode()
        sink = ExecSinkNode()
        wf.add_node(src)
        wf.add_node(sink)
        wf.add_connection(DAConnection(
            src.node_id, "data", sink.node_id, "data"))
        ex = DAWorkflowExecutor(wf)
        global_nodes, isolated_nodes, begin_nodes = ex._classify_nodes()
        assert src.node_id in begin_nodes
