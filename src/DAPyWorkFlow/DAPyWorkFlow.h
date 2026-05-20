#ifndef DAPYWORKFLOW_H
#define DAPYWORKFLOW_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyWorkFlowTypes.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11InQt.h"
#include <QObject>
#include <QString>
#include <QStringList>

namespace DA
{
class DAPyNodeProxy;
class DAPyLinkGraphicsItem;

/**
 * @brief Python DAWorkflow 对象的 C++ 纯代理
 *
 * 继承 DAPyObjectWrapper + QObject，像 DAPyDataFrame 一样通过 attr() 与 Python 对象交互。
 * DAWorkflow Python 实例存储在基类 DAPyObjectWrapper 的 _object 中，
 * 所有方法直接通过 attr()/object() 与 Python DAWorkflow 对象交互，不做本地缓存。
 *
 * mPyExecutorObj 作为独立成员变量持有 DAWorkflowExecutor 实例，
 * executor 是独立对象，不属于 workflow 对象的一部分。
 *
 * @code
 * DAPyWorkFlow workflow;
 * if (!workflow.isNone()) {
 *     QString nodeId = workflow.addNode(proxy);
 * }
 * @endcode
 *
 * @see DAPyNodeProxy DAPyObjectWrapper DAPyModuleWorkflow
 */
class DAPYWORKFLOW_API DAPyWorkFlow : public QObject, public DAPyObjectWrapper
{
    Q_OBJECT
public:
    // 构造/析构
    DAPyWorkFlow(QObject* parent = nullptr);
    DAPyWorkFlow(const pybind11::object& obj, QObject* parent = nullptr);
    ~DAPyWorkFlow();
#if 0  // 暂时屏蔽信号，这里信号槽的方案还不明确，由于目前其它程序还未依赖此信号，因此全部屏蔽
    // --- 信号 ---
Q_SIGNALS:
    // 节点添加信号
    void nodeAdded(DAPyNodeProxy* node);
    // 节点移除信号
    void nodeRemoved(DAPyNodeProxy* node);
    // 节点连接信号
    void nodeConnected(DAPyNodeProxy* fromNode, const QString& fromPort, DAPyNodeProxy* toNode, const QString& toPort);
    // 节点断开信号
    void nodeDisconnected(DAPyNodeProxy* fromNode, const QString& fromPort, DAPyNodeProxy* toNode, const QString& toPort);
    // 执行开始信号
    void executionStarted();
    // 执行完成信号
    void executionFinished(bool success);
#endif
public:
    // 检查 Python DAWorkflow 实例是否有效
    bool isValid() const;

    // --- DAG 操作方法 ---
    // 添加节点到workflow，返回Python分配的node_id
    QString addNode(DAPyNodeProxy* proxy);
    // 从workflow移除节点
    bool removeNode(const QString& nodeId);
    // 连接两个节点，返回连接描述符
    DAPyWorkFlowConnection
    connectNode(const QString& srcNodeId, const QString& srcChannel, const QString& dstNodeId, const QString& dstChannel);
    // 断开连接（按 connectionId）
    bool disconnectNode(const QString& connectionId);
    // 移除连接（disconnectNode 的别名）
    bool removeConnection(const QString& connectionId);
    // 清空所有节点和连接
    void clear();
    // 获取节点数量
    int nodeCount();
    // 检查节点是否存在
    bool hasNode(const QString& nodeId);
    // 通过代理指针移除节点；内部调用 bool removeNode(nodeId)
    bool removeNode(DAPyNodeProxy* proxy);
    // 通过代理指针连接两个节点
    DAPyWorkFlowConnection
    connectNode(DAPyNodeProxy* src, const QString& srcChannel, DAPyNodeProxy* dst, const QString& dstChannel);
    // 通过连接图形项断开连接
    bool disconnectNode(DAPyLinkGraphicsItem* link);
    // 通过代理指针检查节点是否存在
    bool hasNode(DAPyNodeProxy* proxy);

    // --- 数据查询方法 ---
    // 通过 node_id 获取 Python 节点对象
    pybind11::object getNodeById(const QString& nodeId);
    // 获取所有节点列表
    pybind11::list getNodes();
    // 获取所有连接列表
    pybind11::list getConnections();
    // 验证 DAG 是否有效（无环）
    bool isValidDag();
    // 获取拓扑排序结果
    QStringList topologicalSort();

#if 0  // 工作流的操作暂时屏蔽，后续工作流的操作应该有个独立的执行器，传入工作量对象，执行器进行执行，而不是在DAPyWorkFlow中，DAPyWorkFlow仅仅只是一个图数据结构的描述
    // --- Executor 操作方法 ---
    // 异步执行工作流（无回调）
    bool executeAsync();
    // 异步执行工作流（带回调）
    bool executeAsync(pybind11::object onNodeFinished, pybind11::object onStateChange, pybind11::object onProgress);
    // 终止执行
    void terminate();
    // 暂停执行
    bool pause();
    // 恢复执行
    bool resume();

    // --- Executor 状态查询方法 ---
    // 获取执行器状态
    ExecState getExecutorState();
    // 获取执行结果
    bool getResult();
    // 检查是否正在运行
    bool isRunning();
#endif
private:
    // 初始化 Python DAWorkflow 实例
    void initPyWorkflow();
};

}  // namespace DA

#endif  // DAPYWORKFLOW_H
