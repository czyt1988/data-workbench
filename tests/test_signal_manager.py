"""
test_signal_manager — DASignalManager 状态转换、信号队列、数据传播测试

覆盖：Stopped/Paused/Running 状态转换、信号队列、数据传播、
入度计数、error node 处理。
"""

import pytest
from DAWorkFlowPy import DAWorkflow, DAConnection, DASignalManager, DAWorkflowState


# ==================== 辅助节点 ====================

def _make_simple_workflow():
    """创建一个简单的两节点工作流（source → sink）"""
    from DAWorkFlowPy import NodeDef, Output, Input

    @NodeDef(name="SignalSource", category="Test")
    class SignalSource:
        class Outputs:
            data = Output("DataFrame")
        def __init__(self):
            self._output_data = {"data": [1, 2, 3]}
        def execute(self, inputs=None, params=None):
            return True

    @NodeDef(name="SignalSink", category="Test")
    class SignalSink:
        class Inputs:
            data = Input("DataFrame", required=True)
        def __init__(self):
            self._input_data = {}
        def set_input_data(self, channel, data):
            self._input_data[channel] = data
        def execute(self, inputs=None, params=None):
            return True

    wf = DAWorkflow()
    src = SignalSource()
    sink = SignalSink()
    wf.add_node(src)
    wf.add_node(sink)
    wf.add_connection(DAConnection(src.node_id, "data", sink.node_id, "data"))
    return wf, src, sink


# ==================== 状态转换测试 ====================

class TestDASignalManagerState:
    """DASignalManager 状态机测试"""

    def test_initial_state_is_stopped(self):
        """初始状态为 Stopped"""
        wf = DAWorkflow()
        mgr = DASignalManager(wf)
        assert mgr.state == DAWorkflowState.Stopped

    def test_start_transitions_to_running(self):
        """start() 切换到 Running"""
        wf = DAWorkflow()
        mgr = DASignalManager(wf)
        mgr.start()
        assert mgr.state == DAWorkflowState.Running

    def test_stop_transitions_to_stopped(self):
        """stop() 切换到 Stopped"""
        wf = DAWorkflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.stop()
        assert mgr.state == DAWorkflowState.Stopped

    def test_pause_from_running(self):
        """pause() 从 Running 切换到 Paused"""
        wf = DAWorkflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.pause()
        assert mgr.state == DAWorkflowState.Paused

    def test_pause_from_stopped_no_effect(self):
        """从 Stopped 暂停无效"""
        wf = DAWorkflow()
        mgr = DASignalManager(wf)
        mgr.pause()
        assert mgr.state == DAWorkflowState.Stopped

    def test_resume_from_paused(self):
        """resume() 从 Paused 切换到 Running"""
        wf = DAWorkflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.pause()
        mgr.resume()
        assert mgr.state == DAWorkflowState.Running

    def test_resume_from_running_no_effect(self):
        """从 Running 恢复无效"""
        wf = DAWorkflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.resume()  # 已经是 Running，无效
        assert mgr.state == DAWorkflowState.Running

    def test_stop_clears_pending_signals(self):
        """stop() 清空信号队列"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(src.node_id, "data", [1, 2, 3])
        assert mgr.get_pending_count() > 0
        mgr.stop()
        assert mgr.get_pending_count() == 0

    def test_stop_clears_indegree_received(self):
        """stop() 清空入度计数"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(src.node_id, "data", [1, 2, 3])
        mgr.process_pending()
        assert mgr._indegree_received.get(sink.node_id, 0) > 0
        mgr.stop()
        assert len(mgr._indegree_received) == 0


# ==================== 信号队列与数据传播测试 ====================

class TestDASignalManagerPropagation:
    """DASignalManager 数据传播测试"""

    def test_send_output_enqueues_signal(self):
        """send_output 将信号加入队列"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(src.node_id, "data", [1, 2, 3])
        assert mgr.get_pending_count() == 1

    def test_send_output_when_stopped_raises(self):
        """Stopped 状态 send_output 抛 ValueError"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        with pytest.raises(ValueError, match="工作流未运行"):
            mgr.send_output(src.node_id, "data", [1, 2, 3])

    def test_process_pending_delivers_data(self):
        """process_pending 将数据传递到下游节点"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(src.node_id, "data", [1, 2, 3])
        processed = mgr.process_pending()
        assert processed == 1
        # sink 应收到数据
        assert sink._input_data.get("data") == [1, 2, 3]

    def test_process_pending_when_paused_returns_zero(self):
        """Paused 状态 process_pending 返回 0"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(src.node_id, "data", [1, 2, 3])
        mgr.pause()
        processed = mgr.process_pending()
        assert processed == 0
        assert mgr.get_pending_count() > 0  # 队列不清空

    def test_process_pending_when_stopped_returns_zero(self):
        """Stopped 状态 process_pending 返回 0"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        processed = mgr.process_pending()
        assert processed == 0

    def test_send_output_no_downstream_skips_enqueue(self):
        """send_output 无下游连接时不入队"""
        wf = DAWorkflow()
        from DAWorkFlowPy import NodeDef, Output
        @NodeDef(name="NoDS", category="Test")
        class NoDownstream:
            class Outputs:
                data = Output("DataFrame")
            def __init__(self):
                self._output_data = {"data": 42}
            def execute(self, inputs=None, params=None):
                return True
        wf.add_node(NoDownstream())
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(wf.get_nodes()[0].node_id, "data", 42)
        assert mgr.get_pending_count() == 0


# ==================== 入度计数与节点就绪测试 ====================

class TestDASignalManagerIndegree:
    """DASignalManager 入度计数测试"""

    def test_is_node_ready_after_all_inputs(self):
        """收到全部输入数据后 is_node_ready 返回 True"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(src.node_id, "data", [1, 2, 3])
        mgr.process_pending()
        assert mgr.is_node_ready(sink.node_id) is True

    def test_is_node_ready_before_all_inputs(self):
        """未收到全部输入数据时 is_node_ready 返回 False"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        # 未发送任何数据
        assert mgr.is_node_ready(sink.node_id) is False

    def test_is_node_ready_no_upstream(self):
        """无上游连接的节点 is_node_ready 返回 True（入度=0, 收到=0）"""
        wf, src, sink = _make_simple_workflow()
        mgr = DASignalManager(wf)
        mgr.start()
        # source 无上游，入度=0，收到=0，0==0 → True
        assert mgr.is_node_ready(src.node_id) is True


# ==================== 状态变更回调测试 ====================

class TestDASignalManagerCallback:
    """DASignalManager 状态变更回调测试"""

    def test_state_change_callback_on_start(self):
        """start() 触发状态变更回调"""
        transitions = []
        wf = DAWorkflow()
        mgr = DASignalManager(wf, on_state_change=lambda o, n: transitions.append((o, n)))
        mgr.start()
        assert len(transitions) == 1
        assert transitions[0] == ("stopped", "running")

    def test_state_change_callback_on_stop(self):
        """stop() 触发状态变更回调"""
        transitions = []
        wf = DAWorkflow()
        mgr = DASignalManager(wf, on_state_change=lambda o, n: transitions.append((o, n)))
        mgr.start()
        mgr.stop()
        # 应有两次回调：start 和 stop
        assert len(transitions) == 2
        assert transitions[1] == ("running", "stopped")

    def test_state_change_callback_on_pause_resume(self):
        """pause/resume 触发回调"""
        transitions = []
        wf = DAWorkflow()
        mgr = DASignalManager(wf, on_state_change=lambda o, n: transitions.append((o, n)))
        mgr.start()
        mgr.pause()
        mgr.resume()
        assert len(transitions) == 3
        assert transitions[1] == ("running", "paused")
        assert transitions[2] == ("paused", "running")


# ==================== 数据设置 fallback 测试 ====================

class TestDASignalManagerFallback:
    """DASignalManager 数据传递 fallback 测试"""

    def test_deliver_to_node_without_set_input_data(self):
        """目标节点无 set_input_data 方法时使用 _input_data 字典"""
        from DAWorkFlowPy import NodeDef, Input, Output

        @NodeDef(name="SimpleSrc", category="Test")
        class SimpleSrc:
            class Outputs:
                data = Output("int")
            def __init__(self):
                self._output_data = {"data": 42}
            def execute(self, inputs=None, params=None):
                return True

        @NodeDef(name="NoSetInput", category="Test")
        class NoSetInput:
            class Inputs:
                data = Input("int", required=True)
            def execute(self, inputs=None, params=None):
                return True

        wf = DAWorkflow()
        src = SimpleSrc()
        sink = NoSetInput()
        wf.add_node(src)
        wf.add_node(sink)
        wf.add_connection(DAConnection(src.node_id, "data", sink.node_id, "data"))

        mgr = DASignalManager(wf)
        mgr.start()
        mgr.send_output(src.node_id, "data", 42)
        mgr.process_pending()
        # sink 应有 _input_data 字典
        assert hasattr(sink, "_input_data")
        assert sink._input_data.get("data") == 42