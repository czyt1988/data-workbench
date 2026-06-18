#ifndef DAPYWORKFLOWEXECUTOR_H
#define DAPYWORKFLOWEXECUTOR_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyObjectWrapper.h"
#include "DAPyExecutorState.h"
#include "DAPySignalManager.h"
#include <QString>
#include <QStringList>
#include <QPair>
#include <functional>

namespace DA
{
class DAPyWorkFlow;

/**
 * @brief Python DAWorkflowExecutor 的 C++ 纯代理类
 *
 * 代理 Python DAWorkflowExecutor 定义的工作流执行引擎，继承 DAPyObjectWrapper，
 * 通过 attr() 与 Python 对象交互，不缓存任何 Python 数据到 C++ 成员变量。
 *
 * 支持同步/异步执行、暂停/恢复/终止、进度和状态回调。
 * Python 侧的回调通过 std::function 传递给上层，
 * 由上层（如 DAPyWorkFlowManager）桥接到 Qt 信号。
 *
 * @code
 * DAPyWorkFlowExecutor executor(workflow);
 * executor.setOnStateChange([](const QString& old, const QString& newState) {
 *     qDebug() << "state:" << old << "->" << newState;
 * });
 * bool success = executor.execute();
 * @endcode
 *
 * @see DAPyObjectWrapper DAPyWorkFlow DAPySignalManager DAPyExecutorState
 */
class DAPYWORKFLOW_API DAPyWorkFlowExecutor : public DAPyObjectWrapper
{
public:
    // 回调类型定义
    using StateChangeCallback = std::function< void(const QString& oldState, const QString& newState) >;
    using NodeFinishedCallback = std::function< void(const QString& nodeId, bool success) >;
    using ProgressCallback = std::function< void(int executedCount, int totalCount) >;

    DAPyWorkFlowExecutor();
    DAPyWorkFlowExecutor(const pybind11::object& obj);
    DAPyWorkFlowExecutor(pybind11::object&& obj);
    DAPyWorkFlowExecutor(const DAPyObjectWrapper& obj);
    DAPyWorkFlowExecutor(const DAPyWorkFlow& workflow);
    ~DAPyWorkFlowExecutor();

    // 设置回调（在 execute 之前调用）
    void setOnStateChange(StateChangeCallback callback);
    void setOnNodeFinished(NodeFinishedCallback callback);
    void setOnProgress(ProgressCallback callback);

    // 同步执行工作流，返回是否成功
    bool execute();
    // 异步执行工作流（在后台线程中）
    void executeAsync();
    // 等待异步执行完成，返回是否完成（超时返回false）
    bool waitCompletion(double timeoutSec = -1.0);
    // 请求终止执行
    void terminate();
    // 暂停执行
    void pause();
    // 恢复执行
    void resume();

    // 获取当前执行器状态
    DAPyExecutorState getState() const;
    // 获取执行进度 (已执行节点数, 总节点数)
    QPair< int, int > getProgress() const;
    // 获取执行结果 (true=成功, false=失败)
    bool getResult() const;
    // 执行结果是否有效（是否已执行过）
    bool hasResult() const;
    // 获取执行过程中的错误信息列表
    QStringList getErrorMessages() const;
    // 获取当前正在执行的节点 ID
    QString getCurrentNodeId() const;

    // 获取关联的信号管理器
    DAPySignalManager getSignalManager() const;

private:
    // 初始化 Python DAWorkflowExecutor 实例，将 C++ 回调注册到 Python
    void initExecutor(const DAPyWorkFlow& workflow);

    // Python 回调桥接（内部使用，转发给 std::function）
    void onPyStateChange(const std::string& oldState, const std::string& newState);
    void onPyNodeFinished(const std::string& nodeId, bool success);
    void onPyProgress(int executedCount, int totalCount);

    // C++ 回调
    StateChangeCallback mOnStateChange;
    NodeFinishedCallback mOnNodeFinished;
    ProgressCallback mOnProgress;
};

}  // namespace DA

#endif  // DAPYWORKFLOWEXECUTOR_H
