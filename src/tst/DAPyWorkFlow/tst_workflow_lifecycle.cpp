#include "tst_workflow_lifecycle.h"
#include "DAPyWorkFlowLifecycle.h"
#include "DAPyWorkFlowTypes.h"
#include "DAPyGILGuard.h"
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QThread>
#include <QTimer>
#include "DAPybind11InQt.h"

namespace py = pybind11;

namespace DA
{

// ============================================================
// Helper: 初始化 Python 并创建简单 2 节点工作流
// ============================================================

/**
 * @brief 在 Python 中创建包含两个成功节点的 DAWorkflow
 *
 * 创建 SourceNode (无输入，一个输出 "out") → FilterNode (一个输入 "in"，一个输出 "filtered")，
 * 两者通过 DAConnection 连接。两个节点的 execute() 均返回 True。
 *
 * @return pybind11::object Python DAWorkflow 实例
 */
static py::object createSimpleTwoNodeWorkflow()
{
    // 创建简单的节点类 — 无 NodeDef 装饰器（仅用于测试生命周期信号）
    py::object sourceNodeClass = py::class_< py::object >(py::module_::import("builtins"), "_TestSourceNode");
    // 用 Python 代码创建类更简洁
    py::object builtins = py::module_::import("builtins");

    // 动态创建 SourceNode 类
    py::object sourceType = py::eval(R"(
class _TestSourceNode:
    qualified_name = "test._TestSourceNode"
    node_id = "test._TestSourceNode_1"
    _node_descriptor = {
        "name": "TestSource",
        "qualified_name": "test._TestSourceNode",
        "inputs": [],
        "outputs": [{"name": "out", "data_type": "DataFrame"}],
    }
    _output_data = {"out": 42}
    _input_data = {}
    def execute(self):
        return True
)");

    // 动态创建 FilterNode 类
    py::object filterType = py::eval(R"(
class _TestFilterNode:
    qualified_name = "test._TestFilterNode"
    node_id = "test._TestFilterNode_1"
    _node_descriptor = {
        "name": "TestFilter",
        "qualified_name": "test._TestFilterNode",
        "inputs": [{"name": "in", "data_type": "DataFrame"}],
        "outputs": [{"name": "filtered", "data_type": "DataFrame"}],
    }
    _output_data = {"filtered": 99}
    _input_data = {}
    def execute(self):
        return True
)");

    // 创建 DAWorkflow 并添加节点和连接
    py::object wfModule   = py::module_::import("DAWorkbench.DAWorkFlowPy.workflow");
    py::object wfClass    = wfModule.attr("DAWorkflow");
    py::object connModule = py::module_::import("DAWorkbench.DAWorkFlowPy.connection");
    py::object connClass  = connModule.attr("DAConnection");

    py::object workflow = wfClass("test_simple");

    // 创建节点实例并设置 node_id
    py::object sourceNode = sourceType();
    py::object filterNode = filterType();

    // 设置 node_id（add_node 会自动分配，但我们需要固定 ID 以便创建连接）
    sourceNode.attr("node_id") = "test._TestSourceNode_1";
    filterNode.attr("node_id") = "test._TestFilterNode_1";

    // 添加节点到工作流
    workflow.attr("add_node")(sourceNode);
    workflow.attr("add_node")(filterNode);

    // 创建连接：SourceNode.out → FilterNode.in
    py::object conn = connClass("test._TestSourceNode_1", "out", "test._TestFilterNode_1", "in");
    workflow.attr("add_connection")(conn);

    return workflow;
}

/**
 * @brief 在 Python 中创建包含失败节点的工作流
 *
 * 创建 FailingSourceNode（execute() 返回 False）和下游 FilterNode，
 * 通过连接构成工作流。FailingSourceNode 执行失败后不会传播数据到下游。
 *
 * @return pybind11::object Python DAWorkflow 实例
 */
static py::object createFailingWorkflow()
{
    py::object failingType = py::eval(R"(
class _TestFailingNode:
    qualified_name = "test._TestFailingNode"
    node_id = "test._TestFailingNode_1"
    _node_descriptor = {
        "name": "TestFailing",
        "qualified_name": "test._TestFailingNode",
        "inputs": [],
        "outputs": [{"name": "out", "data_type": "DataFrame"}],
    }
    _output_data = {"out": None}
    _input_data = {}
    def execute(self):
        raise RuntimeError("intentional failure for testing")
)");

    py::object downstreamType = py::eval(R"(
class _TestDownstreamNode:
    qualified_name = "test._TestDownstreamNode"
    node_id = "test._TestDownstreamNode_1"
    _node_descriptor = {
        "name": "TestDownstream",
        "qualified_name": "test._TestDownstreamNode",
        "inputs": [{"name": "in", "data_type": "DataFrame"}],
        "outputs": [{"name": "result", "data_type": "DataFrame"}],
    }
    _output_data = {"result": 0}
    _input_data = {}
    def execute(self):
        return True
)");

    py::object wfModule   = py::module_::import("DAWorkbench.DAWorkFlowPy.workflow");
    py::object wfClass    = wfModule.attr("DAWorkflow");
    py::object connModule = py::module_::import("DAWorkbench.DAWorkFlowPy.connection");
    py::object connClass  = connModule.attr("DAConnection");

    py::object workflow = wfClass("test_failing");

    py::object failingNode    = failingType();
    py::object downstreamNode = downstreamType();

    failingNode.attr("node_id")    = "test._TestFailingNode_1";
    downstreamNode.attr("node_id") = "test._TestDownstreamNode_1";

    workflow.attr("add_node")(failingNode);
    workflow.attr("add_node")(downstreamNode);

    py::object conn = connClass("test._TestFailingNode_1", "out", "test._TestDownstreamNode_1", "in");
    workflow.attr("add_connection")(conn);

    return workflow;
}

/**
 * @brief 在 Python 中创建包含慢速节点的工作流（用于暂停/恢复测试）
 *
 * 创建 SlowSourceNode（execute() 中带 time.sleep 延迟），
 * 使工作流执行时间足够长以允许暂停操作。
 *
 * @return pybind11::object Python DAWorkflow 实例
 */
static py::object createSlowWorkflow()
{
    py::object slowType = py::eval(R"(
import time
class _TestSlowNode:
    qualified_name = "test._TestSlowNode"
    node_id = "test._TestSlowNode_1"
    _node_descriptor = {
        "name": "TestSlow",
        "qualified_name": "test._TestSlowNode",
        "inputs": [],
        "outputs": [{"name": "out", "data_type": "DataFrame"}],
    }
    _output_data = {"out": 42}
    _input_data = {}
    def execute(self):
        time.sleep(0.5)  # 模拟慢速执行
        return True
)");

    py::object wfModule = py::module_::import("DAWorkbench.DAWorkFlowPy.workflow");
    py::object wfClass  = wfModule.attr("DAWorkflow");

    py::object workflow = wfClass("test_slow");

    py::object slowNode      = slowType();
    slowNode.attr("node_id") = "test._TestSlowNode_1";

    workflow.attr("add_node")(slowNode);

    return workflow;
}

// ============================================================
// testSimpleWorkflow — 简单2节点工作流执行验证
// ============================================================

/**
 * @brief 验证 DAPyWorkFlowLifecycle 在简单工作流上的执行信号
 *
 * 创建包含两个成功节点的 Python DAWorkflow，
 * 通过 DAPyWorkFlowLifecycle 执行，验证：
 * - finished(bool) 信号发射，参数为 true
 * - execStateChanged 信号包含 Running 和 Finished 状态
 * - getExecState() 最终为 StateFinished
 *
 * 若 Python 解释器不可用则 QSKIP。
 */
void TestWorkflowLifecycle::testSimpleWorkflow()
{
    // 检查 Python 可用性
    if (!Py_IsInitialized()) {
        QSKIP("Python interpreter not available");
    }

    // 在 GIL 保护下创建工作流
    py::object workflow;
    try {
        DA::DAPyGILGuard gil;
        if (!gil.isAcquired()) {
            QSKIP("Failed to acquire GIL");
        }
        workflow = createSimpleTwoNodeWorkflow();
    } catch (const py::error_already_set& e) {
        QSKIP(QString("Python error creating workflow: %1").arg(e.what()).toLocal8Bit().constData());
    } catch (const std::exception& e) {
        QSKIP(QString("Error creating workflow: %1").arg(e.what()).toLocal8Bit().constData());
    }

    // 创建生命周期控制器
    DAPyWorkFlowLifecycle lifecycle;

    // 设置工作流（需要 GIL）
    {
        DA::DAPyGILGuard gil;
        lifecycle.setWorkflow(workflow);
    }

    // 设置信号监听
    QSignalSpy finishedSpy(&lifecycle, &DAPyWorkFlowLifecycle::finished);
    QSignalSpy stateChangedSpy(&lifecycle, &DAPyWorkFlowLifecycle::execStateChanged);

    // 在工作线程中执行生命周期
    QThread workerThread;
    lifecycle.moveToThread(&workerThread);

    // 连接：线程启动时开始执行，执行完成时退出线程
    QObject::connect(&workerThread, &QThread::started, &lifecycle, &DAPyWorkFlowLifecycle::startExecute);
    QObject::connect(&lifecycle, &DAPyWorkFlowLifecycle::finished, &workerThread, &QThread::quit);

    workerThread.start();

    // 等待 finished 信号（最多 10 秒）
    QVERIFY(finishedSpy.wait(10000));

    // 等待线程完全退出
    workerThread.wait(5000);

    // 验证 finished(true)
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(finishedSpy.at(0).at(0).toBool());

    // 验证状态转换包含 Running → Finished
    bool hasRunningToFinished = false;
    for (int i = 0; i < stateChangedSpy.count(); ++i) {
        ExecState oldState = static_cast< ExecState >(stateChangedSpy.at(i).at(0).toInt());
        ExecState newState = static_cast< ExecState >(stateChangedSpy.at(i).at(1).toInt());
        if (oldState == StateRunning && newState == StateFinished) {
            hasRunningToFinished = true;
        }
    }
    QVERIFY(hasRunningToFinished);

    // 验证最终状态为 Finished
    QCOMPARE(lifecycle.getExecState(), StateFinished);
}

// ============================================================
// testFailingWorkflow — 失败节点工作流执行验证
// ============================================================

/**
 * @brief 验证 DAPyWorkFlowLifecycle 在包含失败节点的工作流上的信号行为
 *
 * 创建包含一个会抛出异常的节点的工作流，
 * 通过 DAPyWorkFlowLifecycle 执行，验证：
 * - finished(bool) 信号发射，参数为 false
 * - nodeExecuteFinished 信号至少发射一次，其中包含 success=false
 * - getExecState() 最终为 StateError
 *
 * 若 Python 解释器不可用则 QSKIP。
 */
void TestWorkflowLifecycle::testFailingWorkflow()
{
    // 检查 Python 可用性
    if (!Py_IsInitialized()) {
        QSKIP("Python interpreter not available");
    }

    // 在 GIL 保护下创建失败工作流
    py::object workflow;
    try {
        DA::DAPyGILGuard gil;
        if (!gil.isAcquired()) {
            QSKIP("Failed to acquire GIL");
        }
        workflow = createFailingWorkflow();
    } catch (const py::error_already_set& e) {
        QSKIP(QString("Python error creating workflow: %1").arg(e.what()).toLocal8Bit().constData());
    } catch (const std::exception& e) {
        QSKIP(QString("Error creating workflow: %1").arg(e.what()).toLocal8Bit().constData());
    }

    // 创建生命周期控制器
    DAPyWorkFlowLifecycle lifecycle;

    {
        DA::DAPyGILGuard gil;
        lifecycle.setWorkflow(workflow);
    }

    QSignalSpy finishedSpy(&lifecycle, &DAPyWorkFlowLifecycle::finished);
    QSignalSpy nodeFinishedSpy(&lifecycle, &DAPyWorkFlowLifecycle::nodeExecuteFinished);

    QThread workerThread;
    lifecycle.moveToThread(&workerThread);

    QObject::connect(&workerThread, &QThread::started, &lifecycle, &DAPyWorkFlowLifecycle::startExecute);
    QObject::connect(&lifecycle, &DAPyWorkFlowLifecycle::finished, &workerThread, &QThread::quit);

    workerThread.start();

    QVERIFY(finishedSpy.wait(10000));
    workerThread.wait(5000);

    // 验证 finished(false) — 工作流因节点失败而失败
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(!finishedSpy.at(0).at(0).toBool());

    // 验证至少有一个 nodeExecuteFinished 信号，且包含 success=false
    bool hasFailedNode = false;
    for (int i = 0; i < nodeFinishedSpy.count(); ++i) {
        bool success = nodeFinishedSpy.at(i).at(1).toBool();
        if (!success) {
            hasFailedNode = true;
        }
    }
    QVERIFY(hasFailedNode);

    // 验证最终状态为 Error
    QCOMPARE(lifecycle.getExecState(), StateError);
}

// ============================================================
// testPauseResume — 暂停/恢复状态转换验证
// ============================================================

/**
 * @brief 验证 DAPyWorkFlowLifecycle 暂停/恢复操作的状态转换序列
 *
 * 创建包含慢速节点的工作流，在执行过程中调用 pause() 暂停，
 * 随后调用 resume() 恢复，验证：
 * - execStateChanged 信号序列包含：
 *   Idle→Running（开始执行）
 *   Running→Paused（暂停）
 *   Paused→Running（恢复）
 *   Running→Finished/Error（完成）
 * - 最终 finished(bool) 信号发射
 *
 * 若 Python 解释器不可用则 QSKIP。
 */
void TestWorkflowLifecycle::testPauseResume()
{
    // 检查 Python 可用性
    if (!Py_IsInitialized()) {
        QSKIP("Python interpreter not available");
    }

    // 在 GIL 保护下创建慢速工作流
    py::object workflow;
    try {
        DA::DAPyGILGuard gil;
        if (!gil.isAcquired()) {
            QSKIP("Failed to acquire GIL");
        }
        workflow = createSlowWorkflow();
    } catch (const py::error_already_set& e) {
        QSKIP(QString("Python error creating workflow: %1").arg(e.what()).toLocal8Bit().constData());
    } catch (const std::exception& e) {
        QSKIP(QString("Error creating workflow: %1").arg(e.what()).toLocal8Bit().constData());
    }

    // 创建生命周期控制器
    DAPyWorkFlowLifecycle lifecycle;

    {
        DA::DAPyGILGuard gil;
        lifecycle.setWorkflow(workflow);
    }

    QSignalSpy finishedSpy(&lifecycle, &DAPyWorkFlowLifecycle::finished);
    QSignalSpy stateChangedSpy(&lifecycle, &DAPyWorkFlowLifecycle::execStateChanged);

    QThread workerThread;
    lifecycle.moveToThread(&workerThread);

    QObject::connect(&workerThread, &QThread::started, &lifecycle, &DAPyWorkFlowLifecycle::startExecute);
    QObject::connect(&lifecycle, &DAPyWorkFlowLifecycle::finished, &workerThread, &QThread::quit);

    // 等待 Running 状态后再暂停
    // 使用定时器在工作流开始执行后 200ms 调用 pause()
    QTimer::singleShot(200, [ &lifecycle ]() {
        // pause() 是线程安全的，可以从任何线程调用
        lifecycle.pause();
    });

    // 暂停 500ms 后恢复
    QTimer::singleShot(700, [ &lifecycle ]() { lifecycle.resume(); });

    workerThread.start();

    QVERIFY(finishedSpy.wait(15000));
    workerThread.wait(5000);

    // 验证 finished 信号发射
    QCOMPARE(finishedSpy.count(), 1);

    // 验证状态转换序列
    // 期望序列: Idle→Running, Running→Paused, Paused→Running, Running→Finished
    bool hasIdleToRunning   = false;
    bool hasRunningToPaused = false;
    bool hasPausedToRunning = false;
    bool hasRunningToFinish = false;

    for (int i = 0; i < stateChangedSpy.count(); ++i) {
        ExecState oldState = static_cast< ExecState >(stateChangedSpy.at(i).at(0).toInt());
        ExecState newState = static_cast< ExecState >(stateChangedSpy.at(i).at(1).toInt());

        if (oldState == StateIdle && newState == StateRunning) {
            hasIdleToRunning = true;
        }
        if (oldState == StateRunning && newState == StatePaused) {
            hasRunningToPaused = true;
        }
        if (oldState == StatePaused && newState == StateRunning) {
            hasPausedToRunning = true;
        }
        if (oldState == StateRunning && newState == StateFinished) {
            hasRunningToFinish = true;
        }
    }

    QVERIFY(hasIdleToRunning);
    QVERIFY(hasRunningToPaused);
    QVERIFY(hasPausedToRunning);
    QVERIFY(hasRunningToFinish);
}

}  // namespace DA
