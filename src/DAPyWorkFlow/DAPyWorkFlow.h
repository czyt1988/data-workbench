#ifndef DAPYWORKFLOW_H
#define DAPYWORKFLOW_H
#include "DAPyWorkFlowAPI.h"
#include "DAGlobals.h"
#include "DAPyWorkFlowTypes.h"
#include "DAPybind11InQt.h"
#include <QString>
#include <QStringList>

namespace DA
{
class DAPyNodeProxy;
class DAPyLinkGraphicsItem;

/**
 * @brief Python DAWorkflow 类的 C++ 封装
 *
 * 通过 PIMPL + safe_pyobject 模式封装 Python DAWorkflow 实例，
 * 提供类型安全的 C++ API 操作 Python workflow，消除原始 pybind11::object 和 .attr() 调用。
 * 遵循非 QObject + PIMPL + safe_pyobject 模式（参考 DAPyNodeProxy）。
 *
 * @code
 * DAPyWorkFlow workflow;
 * workflow.initPyWorkflow();
 * if (workflow.isValid()) {
 *     QString nodeId = workflow.addNode(proxy);
 *     qDebug() << "Node added:" << nodeId;
 * }
 * @endcode
 *
 * @see DAPyNodeProxy DAPyModuleWorkflow DAPyGILGuard
 */
class DAPYWORKFLOW_API DAPyWorkFlow
{
    DA_DECLARE_PRIVATE(DAPyWorkFlow)
public:
    // 构造/析构
    DAPyWorkFlow();
    ~DAPyWorkFlow();

    // 初始化 Python DAWorkflow 实例
    void initPyWorkflow();
    // 设置外部 Python DAWorkflow 实例（用于 setPyWorkflow 透传）
    void setPyWorkflowObject(const pybind11::object& obj);
    // 获取内部 Python DAWorkflow 实例对象（用于 getPyWorkflow 透传）
    pybind11::object getPyWorkflowObject() const;
    // 检查 Python 实例是否有效
    bool isValid() const;

    // --- Wave 2: DAG 操作方法 ---
    // 添加节点到workflow，返回Python分配的node_id
    QString addNode(DAPyNodeProxy* proxy);
    // 从workflow移除节点
    bool removeNode(const QString& nodeId);
    // 连接两个节点，返回连接描述符
    DAPyWorkFlowConnection connectNode(const QString& srcNodeId,
                                       const QString& srcChannel,
                                       const QString& dstNodeId,
                                       const QString& dstChannel);
    // 断开连接（按 connectionId）
    bool disconnectNode(const QString& connectionId);
    // 移除连接（disconnectNode 的别名）
    bool removeConnection(const QString& connectionId);
    // 清空所有节点和连接
    void clear();  // TODO: Wave 2 Task 10
    // 获取节点数量
    int nodeCount();  // TODO: Wave 2 Task 10
    // 检查节点是否存在
    bool hasNode(const QString& nodeId);  // TODO: Wave 2 Task 10

    // --- 指针便捷重载（内部委托至字符串ID方法） ---
    // 添加节点，返回传入的代理指针（失败时返回nullptr）；内部调用 QString addNode(proxy)
    DAPyNodeProxy* addNodeProxy(DAPyNodeProxy* proxy);
    // 通过代理指针移除节点；内部调用 bool removeNode(nodeId)
    bool removeNode(DAPyNodeProxy* proxy);
    // 通过代理指针连接两个节点；内部调用 DAPyWorkFlowConnection connectNode(srcId, srcChannel, dstId, dstChannel)
    DAPyWorkFlowConnection connectNode(DAPyNodeProxy* src, const QString& srcChannel,
                                       DAPyNodeProxy* dst, const QString& dstChannel);
    // 通过连接图形项断开连接；内部调用 bool disconnectNode(connectionId)
    // ⚠️ 注意：DAPyLinkGraphicsItem 当前缺少 getConnectionId() 方法，此重载暂返回 false
    bool disconnectNode(DAPyLinkGraphicsItem* link);
    // 通过代理指针检查节点是否存在；内部调用 bool hasNode(nodeId)
    bool hasNode(DAPyNodeProxy* proxy);

    // --- Wave 2: 数据查询方法 ---
    // 通过 node_id 获取 Python 节点对象，不存在时返回 py::none()
    pybind11::object getNodeById(const QString& nodeId);
    // 获取所有节点列表
    pybind11::list getNodes();
    // 获取所有连接列表
    pybind11::list getConnections();
    // 验证 DAG 是否有效（无环）
    bool isValidDag();
    // 获取拓扑排序结果
    QStringList topologicalSort();

    // --- Wave 2: Executor 操作方法 ---
    // 异步执行工作流（无回调）
    bool executeAsync();
    // 异步执行工作流（带回调）— 回调由 DAPyWorkFlowLifecycle 注册
    bool executeAsync(pybind11::object onNodeFinished,
                       pybind11::object onStateChange,
                       pybind11::object onProgress);
    // 终止执行
    void terminate();
    // 暂停执行
    bool pause();
    // 恢复执行
    bool resume();

    // --- Wave 2: Executor 状态查询方法 (TODO: 实现于 Wave 2) ---
    // 获取执行器状态
    ExecState getExecutorState();  // TODO: Wave 2 Task 14
    // 获取执行结果
    bool getResult();  // TODO: Wave 2 Task 14
    // 检查是否正在运行
    bool isRunning();  // TODO: Wave 2 Task 14

    // --- Wave 2: 错误处理 (TODO: 实现于 Wave 2) ---
    // 获取最后一次错误信息
    QString getLastError() const;
};

}  // namespace DA

#endif  // DAPYWORKFLOW_H