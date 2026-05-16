#ifndef DAPYWORKFLOWLIFECYCLE_H
#define DAPYWORKFLOWLIFECYCLE_H
#include <QObject>
#include <QHash>
#include <QMutex>
#include <QWaitCondition>
#include "DAPyWorkFlowAPI.h"
#include "DAPyWorkFlowTypes.h"

namespace DA
{
class DAPyNodeProxy;
class DAPythonSignalHandler;
class DAPyWorkFlow;

/**
 * @brief 工作流生命周期控制器
 *
 * 薄封装层，将所有执行逻辑委托给 DAPyWorkFlow 封装类，
 * 通过 DAPyWorkFlow* 指针调用 Python DAWorkflow/DAWorkflowExecutor 操作，
 * 消除原始 pybind11 .attr() 调用。
 * C++ 侧仅负责 GIL 管理、状态转换和 Qt 信号发射，
 * 不实现拓扑排序或入度计数等执行细节。
 *
 * 执行流程：
 * 1. startExecute() — 获取 GIL，创建回调 pybind11::cpp_function，
 *    通过 mWorkflow->executeAsync(callbacks) 启动异步执行
 * 2. Python 侧通过回调通知 C++ 节点完成、状态变更和进度
 * 3. C++ 侧通过 DAPyGILRelease 在发射 Qt 信号前临时释放 GIL
 * 4. pause()/resume() — Qt 层协同暂停（QMutex + QWaitCondition）+
 *    mWorkflow->pause()/resume() 通知 Python 侧
 * 5. terminate() — mWorkflow->terminate() 通知 Python 侧
 *
 * @code
 * auto workflow = new DA::DAPyWorkFlow();
 * workflow->initPyWorkflow();
 * auto lifecycle = new DA::DAPyWorkFlowLifecycle(this);
 * lifecycle->setWorkflow(workflow);
 * QThread* thread = new QThread(this);
 * lifecycle->moveToThread(thread);
 * connect(thread, &QThread::started, lifecycle, &DAPyWorkFlowLifecycle::startExecute);
 * connect(lifecycle, &DAPyWorkFlowLifecycle::finished, thread, &QThread::quit);
 * thread->start();
 * @endcode
 *
 * @see DAPyWorkFlow DAPyGILGuard DAPyGILRelease DAPythonSignalHandler DAPyNodeProxy
 */
class DAPYWORKFLOW_API DAPyWorkFlowLifecycle : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyWorkFlowLifecycle)
public:
    // 构造函数
    explicit DAPyWorkFlowLifecycle(QObject* parent = nullptr);
    // 析构函数
    ~DAPyWorkFlowLifecycle();
    // 禁止拷贝
    DAPyWorkFlowLifecycle(const DAPyWorkFlowLifecycle&) = delete;

    // 设置执行期间需要跟踪的节点代理（按 Python 侧 node_id 索引）
    void setNodeProxies(const QList< DAPyNodeProxy* >& proxies);
    // 设置DAPyWorkFlow封装对象（替代原 setWorkflow(pybind11::object)）
    void setWorkflow(DAPyWorkFlow* workflow);
    // 判断是否正在执行（Running或Paused状态）
    bool isExecuting() const;
    // 获取当前执行状态
    ExecState getExecState() const;
    // 获取最后错误信息
    QString getLastErrorString() const;

public Q_SLOTS:
    // 开始执行工作流（在工作线程中调用）
    void startExecute();
    // 暂停执行（设置暂停标记，等待恢复）
    void pause();
    // 恢复执行（清除暂停标记，唤醒等待线程）
    void resume();
    // 终止执行（调用 mWorkflow->terminate()）
    void terminate();

Q_SIGNALS:
    /**
     * @brief 节点执行完成信号
     * @param proxy 执行完成的节点代理（原始指针）
     * @param success 执行是否成功
     */
    void nodeExecuteFinished(DA::DAPyNodeProxy* proxy, bool success);

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
    void execStateChanged(DA::ExecState oldState, DA::ExecState newState);

    /**
     * @brief 节点执行进度信号
     * @param current 当前已完成的节点数
     * @param total 总节点数
     */
    void progressChanged(int current, int total);
};

}  // namespace DA

#endif  // DAPYWORKFLOWLIFECYCLE_H