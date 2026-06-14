#include "DAPyWorkFlowExecutor.h"
#include "DAPyWorkFlow.h"
#include "DAPyModuleWorkflow.h"
#include "DAPybind11InQt.h"
#include "DAPyWorkFlowEnumStringUtils.h"
#include <QDebug>
#include <pybind11/functional.h>

namespace DA
{

//===================================================
// DAPyWorkFlowExecutor
//===================================================

/**
 * @brief 默认构造函数，创建空的执行器代理
 */
DAPyWorkFlowExecutor::DAPyWorkFlowExecutor() : DAPyObjectWrapper()
{
}

DAPyWorkFlowExecutor::DAPyWorkFlowExecutor(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
}

DAPyWorkFlowExecutor::DAPyWorkFlowExecutor(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
}

DAPyWorkFlowExecutor::DAPyWorkFlowExecutor(const DAPyObjectWrapper& obj) : DAPyObjectWrapper(obj)
{
}

/**
 * @brief 通过 DAPyWorkFlow 构造执行器
 *
 * 创建 Python DAWorkflowExecutor 实例并绑定到传入的工作流。
 * Python 侧的回调函数在 initExecutor 中注册，
 * 内部转发给 C++ 的 std::function 回调。
 *
 * @param[in] workflow 工作流代理
 */
DAPyWorkFlowExecutor::DAPyWorkFlowExecutor(const DAPyWorkFlow& workflow) : DAPyObjectWrapper()
{
    initExecutor(workflow);
}

DAPyWorkFlowExecutor::~DAPyWorkFlowExecutor()
{
}

void DAPyWorkFlowExecutor::setOnStateChange(StateChangeCallback callback)
{
    mOnStateChange = std::move(callback);
}

void DAPyWorkFlowExecutor::setOnNodeFinished(NodeFinishedCallback callback)
{
    mOnNodeFinished = std::move(callback);
}

void DAPyWorkFlowExecutor::setOnProgress(ProgressCallback callback)
{
    mOnProgress = std::move(callback);
}

/**
 * @brief 初始化 Python DAWorkflowExecutor 实例
 *
 * 导入 DAWorkbench.DAWorkFlowPy 模块，获取 DAWorkflowExecutor 类，
 * 将 C++ 回调 lambda 包装为 pybind11::cpp_function 传入构造函数。
 */
void DAPyWorkFlowExecutor::initExecutor(const DAPyWorkFlow& workflow)
{
    if (workflow.isNone()) {
        qWarning() << "DAPyWorkFlowExecutor::initExecutor: workflow is none";
        return;
    }
    try {
        DAPyModuleWorkflow pyModule = DAPyModuleWorkflow();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlowExecutor::initExecutor: cannot import DAWorkbench.DAWorkFlowPy";
                return;
            }
        }
        pybind11::object executorClass = pyModule.getWorkflowExecutorObject();
        if (executorClass.is_none()) {
            qWarning() << "DAPyWorkFlowExecutor::initExecutor: DAWorkflowExecutor class not available";
            return;
        }

        // 创建 C++ 回调 lambda，通过 pybind11::cpp_function 包装传给 Python
        auto stateCallback = pybind11::cpp_function([ this ](const std::string& oldState, const std::string& newState) {
            onPyStateChange(oldState, newState);
        });
        auto nodeFinishedCallback = pybind11::cpp_function([ this ](const std::string& nodeId, bool success) {
            onPyNodeFinished(nodeId, success);
        });
        auto progressCallback = pybind11::cpp_function([ this ](int executedCount, int totalCount) {
            onPyProgress(executedCount, totalCount);
        });

        // 创建 Python DAWorkflowExecutor 实例
        object() = executorClass(workflow.object(),
                                 pybind11::arg("on_state_change")  = stateCallback,
                                 pybind11::arg("on_node_finished") = nodeFinishedCallback,
                                 pybind11::arg("on_progress")      = progressCallback);
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 同步执行工作流
 */
bool DAPyWorkFlowExecutor::execute()
{
    if (isNone()) {
        qWarning() << "DAPyWorkFlowExecutor::execute: executor is not initialized";
        return false;
    }

    try {
        return attr("execute")().cast< bool >();
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowExecutor::execute: Python exception -" << e.what();
        dealException(e);
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowExecutor::execute: Standard exception -" << e.what();
        dealException(e);
    }
    return false;
}

/**
 * @brief 异步执行工作流（在后台线程中）
 */
void DAPyWorkFlowExecutor::executeAsync()
{
    if (isNone()) {
        qWarning() << "DAPyWorkFlowExecutor::executeAsync: executor is not initialized";
        return;
    }

    try {
        attr("execute_async")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 等待异步执行完成
 *
 * @param[in] timeoutSec 最大等待时间（秒），负数表示无限等待
 * @return True 表示执行完成，False 表示超时
 */
bool DAPyWorkFlowExecutor::waitCompletion(double timeoutSec)
{
    if (isNone()) {
        return true;
    }

    try {
        if (timeoutSec < 0) {
            return attr("wait_completion")().cast< bool >();
        }
        return attr("wait_completion")(pybind11::arg("timeout") = timeoutSec).cast< bool >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

void DAPyWorkFlowExecutor::terminate()
{
    if (isNone()) {
        return;
    }

    try {
        attr("terminate")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

void DAPyWorkFlowExecutor::pause()
{
    if (isNone()) {
        return;
    }

    try {
        attr("pause")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

void DAPyWorkFlowExecutor::resume()
{
    if (isNone()) {
        return;
    }

    try {
        attr("resume")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 获取当前执行器状态
 *
 * 从 Python 对象的 state 属性读取 DAExecutorState 枚举值，
 * 通过字符串映射转换为 C++ DAPyExecutorState 枚举。
 */
DAPyExecutorState DAPyWorkFlowExecutor::getState() const
{
    if (isNone()) {
        return ExecutorIdle;
    }
    try {
        pybind11::object stateObj = attr("state");
        pybind11::object valueObj = stateObj.attr("value");
        QString stateStr          = valueObj.cast< QString >();
        return stringToEnum< DAPyExecutorState >(stateStr);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return ExecutorIdle;
}

/**
 * @brief 获取执行进度
 *
 * @return (已执行节点数, 总节点数) 的 QPair
 */
QPair< int, int > DAPyWorkFlowExecutor::getProgress() const
{
    if (isNone()) {
        return { 0, 0 };
    }
    try {
        pybind11::tuple result = attr("get_progress")().cast< pybind11::tuple >();
        return { result[ 0 ].cast< int >(), result[ 1 ].cast< int >() };
    } catch (const std::exception& e) {
        dealException(e);
    }
    return { 0, 0 };
}

bool DAPyWorkFlowExecutor::getResult() const
{
    if (isNone()) {
        return false;
    }
    try {
        pybind11::object result = attr("result");
        if (result.is_none()) {
            return false;
        }
        return result.cast< bool >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

bool DAPyWorkFlowExecutor::hasResult() const
{
    if (isNone()) {
        return false;
    }
    try {
        pybind11::object result = attr("result");
        return !result.is_none();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 获取执行过程中的错误信息列表
 */
QStringList DAPyWorkFlowExecutor::getErrorMessages() const
{
    QStringList msgs;
    if (isNone()) {
        return msgs;
    }
    try {
        pybind11::list pyMsgs = attr("error_messages").cast< pybind11::list >();
        for (auto item : pyMsgs) {
            msgs.append(item.cast< QString >());
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return msgs;
}

QString DAPyWorkFlowExecutor::getCurrentNodeId() const
{
    if (isNone()) {
        return QString();
    }
    try {
        pybind11::object nodeId = attr("current_node_id");
        if (nodeId.is_none()) {
            return QString();
        }
        return nodeId.cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取关联的信号管理器
 */
DAPySignalManager DAPyWorkFlowExecutor::getSignalManager() const
{
    if (isNone()) {
        return DAPySignalManager();
    }
    try {
        return DAPySignalManager(attr("signal_manager"));
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPySignalManager();
}

// ==================== Python 回调桥接 ====================

void DAPyWorkFlowExecutor::onPyStateChange(const std::string& oldState, const std::string& newState)
{
    if (mOnStateChange) {
        mOnStateChange(QString::fromStdString(oldState), QString::fromStdString(newState));
    }
}

void DAPyWorkFlowExecutor::onPyNodeFinished(const std::string& nodeId, bool success)
{
    if (mOnNodeFinished) {
        mOnNodeFinished(QString::fromStdString(nodeId), success);
    }
}

void DAPyWorkFlowExecutor::onPyProgress(int executedCount, int totalCount)
{
    if (mOnProgress) {
        mOnProgress(executedCount, totalCount);
    }
}

}  // namespace DA
