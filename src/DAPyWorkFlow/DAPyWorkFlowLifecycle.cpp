#include "DAPyWorkFlowLifecycle.h"
#include "DAPyGILGuard.h"
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
    DAPySafePyObjectHolder mWorkflowObj;        ///< Python DAWorkflow实例的安全持有者
    DAPySafePyObjectHolder mPyExecutorObj;      ///< Python DAWorkflowExecutor实例的安全持有者
    QPointer< DAPythonSignalHandler > mSignalHandler;  ///< C++侧信号处理器（可选）
    ExecState mExecState { StateIdle };         ///< 当前执行状态
    bool mIsTerminateRequest { false };         ///< 终止请求标记
    bool mIsPauseRequest { false };             ///< 暂停请求标记
    QMutex mMutex;                              ///< 互斥锁保护状态变更
    QWaitCondition mPauseCondition;             ///< 暂停等待条件变量
    QString mLastErrorString;                   ///< 最后错误信息
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
 * @brief 设置Python工作流对象
 *
 * 将Python DAWorkflow实例存储在DAPySafePyObjectHolder中，
 * 确保在析构时安全释放Python对象引用。
 *
 * @param[in] workflowObj Python DAWorkflow实例的pybind11::object
 */
void DAPyWorkFlowLifecycle::setWorkflow(const pybind11::object& workflowObj)
{
    DA_D(d);
    d->mWorkflowObj = workflowObj;
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
 * 在工作线程中调用。创建Python DAWorkflowExecutor，
 * 注册回调函数，调用execute_async()启动异步执行。
 * 所有Python操作在单个DAPyGILGuard作用域内完成，
 * 发射Qt信号前通过DAPyGILRelease临时释放GIL。
 *
 * 回调注册：
 * - on_node_finished: 节点完成时通过DAPythonSignalHandler通知主线程
 * - on_state_change: 状态变更时通过DAPythonSignalHandler通知主线程
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

    // 检查工作流对象是否已设置
    if (!d->mWorkflowObj) {
        d->mLastErrorString = "Workflow object is not set";
        qCritical() << d->mLastErrorString;
        d->setExecState(StateError);
        {
            DAPyGILRelease release;  // 释放GIL后发射信号
            emit finished(false);
        }
        return;
    }

    // 设置状态为Running
    d->setExecState(StateRunning);

    // 在GIL保护下创建Python执行器并启动异步执行
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
            pybind11::object pyWorkflow = d->mWorkflowObj.object();

            // 创建DAWorkflowExecutor实例
            pybind11::object executorModule = pybind11::module_::import("DAWorkbench.DAWorkFlowPy.executor");
            pybind11::object executorClass  = executorModule.attr("DAWorkflowExecutor");

            // 注册回调函数——通过DAPythonSignalHandler在主线程执行Qt信号发射

            pybind11::object onNodeFinished = pybind11::cpp_function(
                [this, d](const std::string& nodeId, bool success) {
                    // 获取节点代理指针
                    DA::DAPyGILGuard innerGil;
                    try {
                        pybind11::object workflow = d->mWorkflowObj.object();
                        pybind11::object pyNode   = workflow.attr("get_node_by_id")(nodeId);
                        // 创建临时DAPyNodeProxy获取指针——不持有所有权
                        DA::DAPyNodeProxy* proxy = nullptr;
                        // 通过node_id查找已有代理或创建临时代理
                        // 由于Lifecycle不持有节点列表，这里传递nullptr作为占位
                        // 上层消费者（如DAPyWorkFlowEditWidget）通过其他途径获取节点指针
                        {
                            DA::DAPyGILRelease innerRelease;
                            emit nodeExecuteFinished(proxy, success);
                        }
                    } catch (const std::exception& e) {
                        d->dealException(e);
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

            // 创建执行器实例，传入回调
            pybind11::object pyExecutor = executorClass(pyWorkflow, onNodeFinished, onStateChange, onProgress);
            d->mPyExecutorObj = pyExecutor;

            // 启动异步执行
            pyExecutor.attr("execute_async")();

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
    // 轮询Python执行器状态，同时检查C++侧的暂停/终止请求
    while (true) {
        // 检查C++侧暂停请求
        {
            QMutexLocker locker(&d->mMutex);
            if (d->mIsTerminateRequest) {
                // 终止请求——获取GIL调用Python terminate()
                {
                    DA::DAPyGILGuard gil;
                    try {
                        if (d->mPyExecutorObj) {
                            d->mPyExecutorObj.object().attr("terminate")();
                        }
                    } catch (const std::exception& e) {
                        d->dealException(e);
                    }
                }
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

        // 检查Python侧执行是否完成
        {
            DA::DAPyGILGuard gil;
            try {
                if (d->mPyExecutorObj) {
                    pybind11::object stateObj = d->mPyExecutorObj.object().attr("state");
                    std::string stateStr      = pybind11::cast< std::string >(stateObj.attr("value"));
                    if (stateStr == "finished" || stateStr == "error") {
                        bool success = (stateStr == "finished");
                        // 获取Python侧结果
                        if (success && d->mPyExecutorObj) {
                            success = pybind11::cast< bool >(d->mPyExecutorObj.object().attr("result"));
                        }
                        {
                            DAPyGILRelease release;
                            d->setExecState(success ? StateFinished : StateError);
                            emit finished(success);
                        }
                        return;
                    }
                }
            } catch (const pybind11::error_already_set& e) {
                d->dealException(e);
                {
                    DAPyGILRelease release;
                    d->setExecState(StateError);
                    emit finished(false);
                }
                return;
            } catch (const std::exception& e) {
                d->dealException(e);
                {
                    DAPyGILRelease release;
                    d->setExecState(StateError);
                    emit finished(false);
                }
                return;
            }
        }  // GIL释放

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
 */
void DAPyWorkFlowLifecycle::pause()
{
    DA_D(d);
    QMutexLocker locker(&d->mMutex);
    if (d->mExecState != StateRunning) {
        return;  // 只在Running状态下才能暂停
    }
    d->mIsPauseRequest = true;

    // 同时通知Python侧暂停
    {
        // 释放mutex以允许Python操作
        locker.unlock();
        DA::DAPyGILGuard gil;
        try {
            if (d->mPyExecutorObj) {
                d->mPyExecutorObj.object().attr("pause")();
            }
        } catch (const std::exception& e) {
            d->dealException(e);
        }
        locker.relock();
    }
}

/**
 * @brief 恢复执行
 *
 * 清除暂停标记并唤醒等待的工作线程。
 * 同时通知Python侧恢复执行。
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

    // 同时通知Python侧恢复
    {
        locker.unlock();
        DA::DAPyGILGuard gil;
        try {
            if (d->mPyExecutorObj) {
                d->mPyExecutorObj.object().attr("resume")();
            }
        } catch (const std::exception& e) {
            d->dealException(e);
        }
        locker.relock();
    }
}

/**
 * @brief 终止执行
 *
 * 设置终止标记并调用Python executor.terminate()。
 * 当前正在执行的节点完成后将停止后续执行。
 *
 * @note 获取GIL调用Python terminate()方法
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

    // 获取GIL调用Python terminate
    {
        DA::DAPyGILGuard gil;
        try {
            if (d->mPyExecutorObj) {
                d->mPyExecutorObj.object().attr("terminate")();
            }
        } catch (const pybind11::error_already_set& e) {
            // error_already_set必须在GIL作用域内消费
            d->dealException(e);
        } catch (const std::exception& e) {
            d->dealException(e);
        }
    }
}

}  // namespace DA