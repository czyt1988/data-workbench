#include "DAPyWorkFlowLifecycle.h"
#include "DAPyWorkFlow.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
#include "DAPyNodeProxy.h"
#include "DAPythonSignalHandler.h"
#include <QDebug>
#include <QMutexLocker>
#include <QPointer>
#include <QThread>

namespace DA
{

DA_AUTO_REGISTER_META_TYPE(DA::ExecState)

class DAPyWorkFlowLifecycle::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowLifecycle)
public:
    PrivateData(DAPyWorkFlowLifecycle* p);

    // 统一异常处理（在GIL作用域内消费error_already_set）
    void dealException(const std::exception& e);
    // 设置执行状态并发射信号
    void setExecState(ExecState newState);

public:
    DAPyWorkFlow* mWorkflow { nullptr };             ///< DAPyWorkFlow封装对象指针（调用者保证生命周期）
    QPointer< DAPythonSignalHandler > mSignalHandler;  ///< C++侧信号处理器（可选）
    ExecState mExecState { StateIdle };         ///< 当前执行状态
    bool mIsTerminateRequest { false };         ///< 终止请求标记
    bool mIsPauseRequest { false };             ///< 暂停请求标记
    QMutex mMutex;                              ///< 互斥锁保护状态变更
    QWaitCondition mPauseCondition;             ///< 暂停等待条件变量
    QString mLastErrorString;                   ///< 最后错误信息
    QHash< QString, DAPyNodeProxy* > mExecutingProxies;  ///< 执行中节点的 nodeId → proxy 映射
};

//===================================================
// DAPyWorkFlowLifecycle::PrivateData
//===================================================

DAPyWorkFlowLifecycle::PrivateData::PrivateData(DAPyWorkFlowLifecycle* p) : q_ptr(p)
{
}

/**
 * @brief 统一异常处理
 *
 * 将异常信息存储到mLastErrorString中，
 * 对于pybind11::error_already_set异常，在GIL作用域内消费，
 * 避免异常析构时尝试获取GIL导致死锁。
 *
 * @param[in] e 捕获的异常对象
 */
void DAPyWorkFlowLifecycle::PrivateData::dealException(const std::exception& e)
{
    mLastErrorString = e.what();
    qCritical() << "DAPyWorkFlowLifecycle error:" << mLastErrorString;
}

/**
 * @brief 设置执行状态并发射信号
 *
 * @param[in] newState 新的执行状态
 */
void DAPyWorkFlowLifecycle::PrivateData::setExecState(ExecState newState)
{
    DA_Q(DAPyWorkFlowLifecycle, q);
    ExecState oldState = mExecState;
    if (oldState != newState) {
        mExecState = newState;
        emit q->execStateChanged(oldState, newState);
    }
}

//===================================================
// DAPyWorkFlowLifecycle
//===================================================

DAPyWorkFlowLifecycle::DAPyWorkFlowLifecycle(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
}

DAPyWorkFlowLifecycle::~DAPyWorkFlowLifecycle()
{
}

/**
 * @brief 设置DAPyWorkFlow封装对象
 *
 * 替代原 setWorkflow(pybind11::object) 方法，
 * 通过 DAPyWorkFlow* 指针访问所有工作流操作，
 * 消除原始 pybind11 .attr() 调用。
 * 调用者须保证 DAPyWorkFlow 对象在 Lifecycle 执行期间有效。
 *
 * @param[in] workflow DAPyWorkFlow封装对象指针
 */
void DAPyWorkFlowLifecycle::setWorkflow(DAPyWorkFlow* workflow)
{
    DA_D(d);
    d->mWorkflow = workflow;
}

/**
 * @brief 设置执行期间需要追踪的节点代理列表
 *
 * 建立 nodeId → proxy 映射，供 Python 侧回调中获取对应 DAPyNodeProxy。
 * 在 startExecute() 前调用。
 *
 * @param[in] proxies 节点代理列表
 */
void DAPyWorkFlowLifecycle::setNodeProxies(const QList< DAPyNodeProxy* >& proxies)
{
    DA_D(d);
    d->mExecutingProxies.clear();
    for (DAPyNodeProxy* proxy : proxies) {
        if (proxy) {
            QString nodeId = proxy->getNodeId();
            if (!nodeId.isEmpty()) {
                d->mExecutingProxies[nodeId] = proxy;
            }
        }
    }
}

/**
 * @brief 判断是否正在执行
 *
 * 返回true如果当前处于Running或Paused状态。
 *
 * @return true表示正在执行（Running或Paused），false表示空闲/完成/出错
 */
bool DAPyWorkFlowLifecycle::isExecuting() const
{
    DA_DC(d);
    return (d->mExecState == StateRunning || d->mExecState == StatePaused);
}

/**
 * @brief 获取当前执行状态
 *
 * @return 当前ExecState枚举值
 */
ExecState DAPyWorkFlowLifecycle::getExecState() const
{
    DA_DC(d);
    return d->mExecState;
}

/**
 * @brief 获取最后错误信息
 *
 * @return 错误信息字符串，无错误时为空
 */
QString DAPyWorkFlowLifecycle::getLastErrorString() const
{
    DA_DC(d);
    return d->mLastErrorString;
}

/**
 * @brief 开始执行工作流
 *
 * 在工作线程中调用。通过 mWorkflow->executeAsync(callbacks) 创建
 * Python DAWorkflowExecutor 并启动异步执行，
 * 回调通过 pybind11::cpp_function 注册以通知节点完成、状态变更和进度。
 * Python操作委托给 DAPyWorkFlow 封装类处理。
 *
 * 回调注册：
 * - on_node_finished: 节点完成时通过 mWorkflow->getNodeById() 获取节点，
 *   通过DAPythonSignalHandler通知主线程
 * - on_state_change: 状态变更时映射Python状态字符串到C++ ExecState
 * - on_progress: 进度更新时通过DAPythonSignalHandler通知主线程
 *
 * @note 此方法应在工作线程中调用（通过QThread::started信号触发）
 */
void DAPyWorkFlowLifecycle::startExecute()
{
    DA_D(d);

    // 防止重入：已在执行中则直接返回
    if (isExecuting()) {
        qWarning() << "DAPyWorkFlowLifecycle: already executing, ignoring startExecute";
        return;
    }

    // 重置终止和暂停标记
    {
        QMutexLocker locker(&d->mMutex);
        d->mIsTerminateRequest = false;
        d->mIsPauseRequest     = false;
        d->mLastErrorString.clear();
    }

    // 检查工作流封装对象是否已设置且有效
    if (!d->mWorkflow) {
        d->mLastErrorString = "Workflow object is not set";
        qCritical() << d->mLastErrorString;
        d->setExecState(StateError);
        emit finished(false);
        return;
    }

    // 设置状态为Running
    d->setExecState(StateRunning);

    // 在GIL保护下创建回调并启动异步执行
    {
        DA::DAPyGILGuard gil;
        if (!gil.isAcquired()) {
            d->mLastErrorString = "Failed to acquire GIL";
            qCritical() << d->mLastErrorString;
            d->setExecState(StateError);
            {
                DAPyGILRelease release;
                emit finished(false);
            }
            return;
        }

        try {
            // 注册回调函数——通过DAPythonSignalHandler在主线程执行Qt信号发射

            pybind11::object onNodeFinished = pybind11::cpp_function(
                [this, d](const std::string& nodeId, bool success) {
                    if (!d->mWorkflow) {
                        return;
                    }
                    DA::DAPyNodeProxy* proxy = d->mExecutingProxies.take(QString::fromStdString(nodeId));
                    if (!proxy) {
                        qWarning() << "DAPyWorkFlowLifecycle: node finished but proxy not found for nodeId:" << QString::fromStdString(nodeId);
                        return;
                    }
                    {
                        DA::DAPyGILRelease innerRelease;
                        emit nodeExecuteFinished(proxy, success);
                    }
                });

            pybind11::object onStateChange = pybind11::cpp_function(
                [this, d](const std::string& oldStateStr, const std::string& newStateStr) {
                    // 将Python状态字符串映射到C++ ExecState
                    ExecState cppOldState = StateIdle;
                    ExecState cppNewState = StateIdle;
                    if (oldStateStr == "running")  cppOldState = StateRunning;
                    else if (oldStateStr == "paused")   cppOldState = StatePaused;
                    else if (oldStateStr == "error")    cppOldState = StateError;
                    else if (oldStateStr == "finished") cppOldState = StateFinished;

                    if (newStateStr == "running")  cppNewState = StateRunning;
                    else if (newStateStr == "paused")   cppNewState = StatePaused;
                    else if (newStateStr == "error")    cppNewState = StateError;
                    else if (newStateStr == "finished") cppNewState = StateFinished;

                    d->mExecState = cppNewState;
                    {
                        DA::DAPyGILRelease innerRelease;
                        emit execStateChanged(cppOldState, cppNewState);
                    }
                });

            pybind11::object onProgress = pybind11::cpp_function(
                [this](int current, int total) {
                    {
                        DA::DAPyGILRelease innerRelease;
                        emit progressChanged(current, total);
                    }
                });

            // 通过封装类启动异步执行（替代 executorModule.attr + executorClass + execute_async）
            bool ok = d->mWorkflow->executeAsync(onNodeFinished, onStateChange, onProgress);
            if (!ok) {
                d->mLastErrorString = d->mWorkflow->getLastError();
                qCritical() << "DAPyWorkFlowLifecycle: executeAsync failed:" << d->mLastErrorString;
                d->setExecState(StateError);
                {
                    DAPyGILRelease release;
                    emit finished(false);
                }
                return;
            }

        } catch (const pybind11::error_already_set& e) {
            // error_already_set必须在GIL作用域内消费
            d->dealException(e);
            d->setExecState(StateError);
            {
                DAPyGILRelease release;
                emit finished(false);
            }
            return;
        } catch (const std::exception& e) {
            d->dealException(e);
            d->setExecState(StateError);
            {
                DAPyGILRelease release;
                emit finished(false);
            }
            return;
        }
    }  // DAPyGILGuard析构，释放GIL

    // 等待执行完成——在GIL释放后等待
    // 轮询DAPyWorkFlow封装类的执行器状态，同时检查C++侧的暂停/终止请求
    while (true) {
        // 检查C++侧暂停请求
        {
            QMutexLocker locker(&d->mMutex);
            if (d->mIsTerminateRequest) {
                // 终止请求——通过封装类调用 Python terminate()
                d->mWorkflow->terminate();
                d->setExecState(StateFinished);
                emit finished(false);
                return;
            }
            // 协同暂停——等待恢复信号
            while (d->mIsPauseRequest) {
                d->setExecState(StatePaused);
                d->mPauseCondition.wait(&d->mMutex);
            }
            // 恢复后确保状态回到Running
            if (d->mExecState == StatePaused) {
                d->setExecState(StateRunning);
            }
        }

        // 通过封装类检查Python执行器状态（替代 .attr("state") + .attr("value")）
        ExecState execState = d->mWorkflow->getExecutorState();
        if (execState == StateFinished || execState == StateError) {
            bool success = (execState == StateFinished);
            if (success) {
                // 通过封装类获取执行结果（替代 .attr("result")）
                success = d->mWorkflow->getResult();
            }
            d->setExecState(success ? StateFinished : StateError);
            emit finished(success);
            return;
        }

        // 短暂休眠避免密集轮询
        QThread::msleep(50);
    }
}

/**
 * @brief 暂停执行
 *
 * 设置暂停标记，在下次轮询检查时将工作流暂停。
 * 使用QMutex和QWaitCondition实现协同暂停，
 * 工作线程在暂停期间等待resume()唤醒。
 * 同时通过封装类通知Python侧暂停。
 */
void DAPyWorkFlowLifecycle::pause()
{
    DA_D(d);
    QMutexLocker locker(&d->mMutex);
    if (d->mExecState != StateRunning) {
        return;  // 只在Running状态下才能暂停
    }
    d->mIsPauseRequest = true;

    // 通过封装类通知Python侧暂停（替代 .attr("pause")）
    {
        locker.unlock();
        if (d->mWorkflow) {
            d->mWorkflow->pause();
        }
        locker.relock();
    }
}

/**
 * @brief 恢复执行
 *
 * 清除暂停标记并唤醒等待的工作线程。
 * 同时通过封装类通知Python侧恢复执行。
 */
void DAPyWorkFlowLifecycle::resume()
{
    DA_D(d);
    QMutexLocker locker(&d->mMutex);
    if (d->mExecState != StatePaused) {
        return;  // 只在Paused状态下才能恢复
    }
    d->mIsPauseRequest = false;
    d->mPauseCondition.wakeAll();

    // 通过封装类通知Python侧恢复（替代 .attr("resume")）
    {
        locker.unlock();
        if (d->mWorkflow) {
            d->mWorkflow->resume();
        }
        locker.relock();
    }
}

/**
 * @brief 终止执行
 *
 * 设置终止标记并通过封装类调用 mWorkflow->terminate()。
 * 当前正在执行的节点完成后将停止后续执行。
 *
 * @note 封装类内部获取GIL调用Python terminate()方法
 */
void DAPyWorkFlowLifecycle::terminate()
{
    DA_D(d);
    {
        QMutexLocker locker(&d->mMutex);
        d->mIsTerminateRequest = true;
        // 如果处于暂停状态，唤醒以让终止生效
        d->mIsPauseRequest = false;
        d->mPauseCondition.wakeAll();
    }

    // 通过封装类调用Python terminate（替代 .attr("terminate")）
    if (d->mWorkflow) {
        d->mWorkflow->terminate();
    }
}

}  // namespace DA