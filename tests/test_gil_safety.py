"""
test_gil_safety — GIL 安全性测试

覆盖：并发执行、多线程安全性验证。
由于 DAWorkFlowPy 是纯 Python 模块，GIL 安全测试验证
多线程场景下工作流操作不崩溃。
"""

import threading
import time
import pytest
from DAWorkbench.DAWorkFlowPy import DAWorkflow, DAConnection, DAWorkflowExecutor, DAExecutorState, NodeDef, Input, Output


# ==================== 辅助节点 ====================

@NodeDef(name="ConcurrentSource", category="Test")
class ConcurrentSourceNode:
    class Outputs:
        data = Output("DataFrame")

    def __init__(self):
        self._output_data = {"data": [1, 2, 3]}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="ConcurrentSink", category="Test")
class ConcurrentSinkNode:
    class Inputs:
        data = Input("DataFrame", required=True)

    def __init__(self):
        self._input_data = {}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        return True


# ==================== 多线程并发执行测试 ====================

class TestGILSafetyConcurrent:
    """GIL 安全性：多线程并发测试"""

    def test_concurrent_workflow_creation(self):
        """多线程同时创建工作流不崩溃"""
        workflows = []
        errors = []

        def create_workflow(idx):
            try:
                wf = DAWorkflow(name=f"wf_{idx}")
                node = ConcurrentSourceNode()
                wf.add_node(node)
                workflows.append(wf)
            except Exception as e:
                errors.append(e)

        threads = [threading.Thread(
            target=create_workflow, args=(i,)) for i in range(10)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=5)

        assert len(errors) == 0
        assert len(workflows) == 10

    def test_concurrent_async_execution(self):
        """多个异步执行不崩溃"""
        wf = DAWorkflow()
        src = ConcurrentSourceNode()
        sink = ConcurrentSinkNode()
        wf.add_node(src)
        wf.add_node(sink)
        wf.add_connection(DAConnection(
            src.node_id, "data", sink.node_id, "data"))

        ex = DAWorkflowExecutor(wf)
        ex.execute_async()
        completed = ex.wait_completion(timeout=10)
        assert completed is True
        assert ex.state == DAExecutorState.Finished

    def test_concurrent_read_workflow_state(self):
        """多线程并发读取工作流状态不崩溃"""
        wf = DAWorkflow()
        src = ConcurrentSourceNode()
        sink = ConcurrentSinkNode()
        wf.add_node(src)
        wf.add_node(sink)
        wf.add_connection(DAConnection(
            src.node_id, "data", sink.node_id, "data"))

        ex = DAWorkflowExecutor(wf)
        ex.execute_async()

        states = []
        errors = []

        def read_state():
            try:
                for _ in range(5):
                    states.append(ex.state)
                    time.sleep(0.01)
            except Exception as e:
                errors.append(e)

        # 在执行过程中并发读取状态
        threads = [threading.Thread(target=read_state) for _ in range(3)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=5)

        ex.wait_completion(timeout=10)
        assert len(errors) == 0
        assert len(states) > 0

    def test_execute_async_then_terminate(self):
        """异步执行后终止不崩溃"""
        wf = DAWorkflow()
        src = ConcurrentSourceNode()
        wf.add_node(src)
        ex = DAWorkflowExecutor(wf)
        ex.execute_async()
        # 终止请求
        ex.terminate()
        ex.wait_completion(timeout=10)
        # 应不崩溃（可能 Finished 也可能被终止）
        assert ex.state in (DAExecutorState.Finished, DAExecutorState.Error)


# ==================== 错误场景测试 ====================

class TestGILSafetyErrorScenarios:
    """GIL 安全性：错误场景测试"""

    def test_error_already_set_safe(self):
        """模拟 error_already_set 场景（Python 异常已设置时不崩溃）"""
        # 在 Python 纯模块中，异常传播是正常的
        # 此测试验证工作流执行器正确捕获节点异常
        from tests.test_nodes.sample_nodes import ErrorNode
        wf = DAWorkflow()
        err = ErrorNode()
        wf.add_node(err)
        ex = DAWorkflowExecutor(wf)
        success = ex.execute()
        assert success is False
        assert ex.state == DAExecutorState.Finished

    def test_safe_object_access_from_multiple_threads(self):
        """多线程并发访问 DAWorkflow 对象属性"""
        wf = DAWorkflow()
        src = ConcurrentSourceNode()
        wf.add_node(src)

        results = []
        errors = []

        def access_workflow():
            try:
                nodes = wf.get_nodes()
                connections = wf.get_connections()
                node_id = src.node_id
                node = wf.get_node_by_id(node_id)
                is_in = node_id in wf
                results.append((len(nodes), len(connections), is_in))
            except Exception as e:
                errors.append(e)

        threads = [threading.Thread(target=access_workflow) for _ in range(5)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=5)

        assert len(errors) == 0
        assert len(results) == 5
