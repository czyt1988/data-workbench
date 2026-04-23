#ifndef DAPYWORKFLOWEXECUTER_H
#define DAPYWORKFLOWEXECUTER_H
#include <QObject>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include "DAPyWorkFlowAPI.h"
#include "DAPybind11InQt.h"
#include "DAPyNodeState.h"
#include "DAPyNodeProxy.h"

namespace DA
{

class DAPythonSignalHandler;

/**
 * @brief Python工作流执行引擎
 *
 * 基于拓扑排序和入度计数的Python工作流执行器，在独立QThread中运行，
 * 通过DAPyGILGuard确保Python GIL安全，通过Qt信号与主线程通信。
 *
 * 执行流程：
 * 1. prepareStartExec() — 分类全局节点、孤立节点、开始节点
 * 2. 按序执行全局节点（执行但不传递）
 * 3. 按序执行孤立节点和开始节点（执行并传递数据）
 * 4. 每个节点执行完成后，通过DASignalManager传播数据到下游
 * 5. 下游节点入度满足时触发执行
 *
 * @code
 * auto executer = new DA::DAPyWorkFlowExecuter();
 * executer->setWorkflow(pyWorkflowObj);
 * QThread* thread = new QThread();
 * executer->moveToThread(thread);
 * connect(thread, &QThread::started, executer, &DAPyWorkFlowExecuter::startExecute);
 * thread->start();
 * @endcode
 *
 * @see DAPyGILGuard DAPyNodeProxy DAPythonSignalHandler DASignalManager
 */
class DAPYWORKFLOW_API DAPyWorkFlowExecuter : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyWorkFlowExecuter)
public:
    /**
     * @brief 工作流执行状态枚举
     */
    enum ExecState
    {
        StateIdle = 0,      ///< 空闲，未开始执行
        StateRunning = 1,   ///< 运行中
        StatePaused = 2,    ///< 已暂停
        StateError = 3,     ///< 执行出错
        StateFinished = 4   ///< 执行完成
    };

    DAPyWorkFlowExecuter(QObject* parent = nullptr);
    ~DAPyWorkFlowExecuter();

    // 设置Python工作流对象（DAWorkflow实例）
    void setWorkflow(const pybind11::object& workflowObj);
    // 获取Python工作流对象
    pybind11::object getWorkflow() const;

    // 设置DAPythonSignalHandler用于主线程回调
    void setSignalHandler(DAPythonSignalHandler* handler);
    // 获取当前执行状态
    ExecState getExecState() const;
    // 判断是否请求终止
    bool isTerminateRequest() const;
    // 获取全局节点列表
    QList< std::shared_ptr< DAPyNodeProxy > > getGlobalNodes() const;
    // 获取孤立节点列表
    QList< std::shared_ptr< DAPyNodeProxy > > getIsolatedNodes() const;
    // 获取最后错误信息
    QString getLastErrorString() const;

public Q_SLOTS:
    // 开始执行工作流（在工作线程中调用）
    void startExecute();
    // 请求终止执行
    void terminateRequest();
    // 暂停执行（在节点间暂停）
    void pause();
    // 恢复执行
    void resume();

Q_SIGNALS:
    /**
     * @brief 节点执行完成信号
     * @param nodeProxy 执行完成的节点代理
     * @param success 执行是否成功
     */
    void nodeExecuteFinished(std::shared_ptr< DA::DAPyNodeProxy > nodeProxy, bool success);

    /**
     * @brief 工作流执行完成信号
     * @param success 整体执行是否成功
     */
    void finished(bool success);

    /**
     * @brief 执行状态变更信号
     * @param oldState 原状态
     * @param newState 新状态
     */
    void execStateChanged(DA::DAPyWorkFlowExecuter::ExecState oldState, DA::DAPyWorkFlowExecuter::ExecState newState);

    /**
     * @brief 节点执行进度信号
     * @param current 当前已完成的节点数
     * @param total 总节点数
     */
    void progressChanged(int current, int total);

private:
    // 执行单个节点并传递数据
    void executeNode(std::shared_ptr< DAPyNodeProxy > proxy);
    // 执行节点但不传递数据（用于全局节点）
    void executeNodeNotTransmit(std::shared_ptr< DAPyNodeProxy > proxy);
    // 准备执行——分类节点并构建入度计数
    bool prepareStartExec();
    // 通过DASignalManager传递节点输出数据到下游
    void propagateOutput(std::shared_ptr< DAPyNodeProxy > proxy);
    // 检查下游节点是否满足执行条件并触发执行
    void transmitDownstream(std::shared_ptr< DAPyNodeProxy > proxy);
    // 设置执行状态并发射信号
    void setExecState(ExecState newState);
};

}  // namespace DA

Q_DECLARE_METATYPE(std::shared_ptr< DA::DAPyNodeProxy >)
Q_DECLARE_METATYPE(DA::DAPyWorkFlowExecuter::ExecState)

#endif  // DAPYWORKFLOWEXECUTER_H