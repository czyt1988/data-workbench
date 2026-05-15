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
    // 检查 Python 实例是否有效
    bool isValid() const;

    // --- Wave 2: DAG 操作方法 (TODO: 实现于 Wave 2) ---
    // 添加节点，返回 Python 分配的 node_id
    QString addNode(DAPyNodeProxy* proxy);  // TODO: Wave 2 Task 8
    // 移除节点
    bool removeNode(const QString& nodeId);  // TODO: Wave 2 Task 8
    // 连接两个节点，返回连接描述符
    DAPyWorkFlowConnection connectNode(const QString& srcNodeId,
                                       const QString& srcChannel,
                                       const QString& dstNodeId,
                                       const QString& dstChannel);  // TODO: Wave 2 Task 9
    // 断开连接（按 connectionId）
    bool disconnectNode(const QString& connectionId);  // TODO: Wave 2 Task 9
    // 移除连接（按 connectionId）
    bool removeConnection(const QString& connectionId);  // TODO: Wave 2 Task 9
    // 清空所有节点和连接
    void clear();  // TODO: Wave 2 Task 10
    // 获取节点数量
    int nodeCount();  // TODO: Wave 2 Task 10
    // 检查节点是否存在
    bool hasNode(const QString& nodeId);  // TODO: Wave 2 Task 10

    // --- Wave 2: 数据查询方法 (TODO: 实现于 Wave 2) ---
    // 通过 node_id 获取 Python 节点对象
    pybind11::object getNodeById(const QString& nodeId);  // TODO: Wave 2 Task 11
    // 获取所有节点列表
    pybind11::list getNodes();  // TODO: Wave 2 Task 11
    // 获取所有连接列表
    pybind11::list getConnections();  // TODO: Wave 2 Task 11
    // 验证 DAG 是否有效（无环）
    bool isValidDag();  // TODO: Wave 2 Task 12
    // 获取拓扑排序结果
    QStringList topologicalSort();  // TODO: Wave 2 Task 12

    // --- Wave 2: Executor 操作方法 (TODO: 实现于 Wave 2) ---
    // 异步执行工作流
    bool executeAsync();  // TODO: Wave 2 Task 13
    // 终止执行
    void terminate();  // TODO: Wave 2 Task 13
    // 暂停执行
    bool pause();  // TODO: Wave 2 Task 13
    // 恢复执行
    bool resume();  // TODO: Wave 2 Task 13

    // --- Wave 2: Executor 状态查询方法 (TODO: 实现于 Wave 2) ---
    // 获取执行器状态
    ExecState getExecutorState();  // TODO: Wave 2 Task 14
    // 获取执行结果
    bool getResult();  // TODO: Wave 2 Task 14
    // 检查是否正在运行
    bool isRunning();  // TODO: Wave 2 Task 14

    // --- Wave 2: 错误处理 (TODO: 实现于 Wave 2) ---
    // 获取最后一次错误信息
    QString getLastError() const;  // TODO: Wave 2 Task 15
};

}  // namespace DA

#endif  // DAPYWORKFLOW_H