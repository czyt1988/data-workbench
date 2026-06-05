#ifndef DAPYWORKFLOWMANAGER_H
#define DAPYWORKFLOWMANAGER_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyWorkFlowTypes.h"
#include "DAPyNode.h"
#include "DAPyNodeMetaData.h"
#include <QObject>
#include <QString>
#include <memory>

namespace DA
{
class DAPyWorkFlow;
class DAPyNodeFactory;

/**
 * @brief 工作流管理器，桥接Python代理层与Qt UI层
 *
 * 持有 DAPyWorkFlow（图数据代理）和 DAPyNodeFactory（节点工厂代理），
 * 封装所有工作流操作方法，在成功执行后发射Qt信号通知UI层。
 * 错误处理不向Qt层传播Python异常，返回安全默认值并通过qCritical()记录。
 *
 * @see DAPyWorkFlow DAPyNodeFactory DAPyNode
 */
class DAPYWORKFLOW_API DAPyWorkFlowManager : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyWorkFlowManager)
public:
    DAPyWorkFlowManager(QObject* parent = nullptr);
    ~DAPyWorkFlowManager();

    // 获取工作流代理
    DAPyWorkFlow* getWorkflow() const;
    // 获取节点工厂代理
    DAPyNodeFactory* getFactory() const;

    // --- 工作流注入（支持子类如DADataWorkFlow）---
    // 替换内部 workflow（Manager 取得所有权）
    void setWorkflow(DAPyWorkFlow* wf);
    // 替换内部 factory（Manager 取得所有权）
    void setFactory(DAPyNodeFactory* factory);
    // 替换内部 factory（shared_ptr 版本，与外部共享所有权）
    void setFactory(std::shared_ptr< DAPyNodeFactory > factory);

    // --- Scene 内部同步用（不发射信号）---
    // 检查工作流是否有效
    bool isWorkflowValid() const;
    // 注册节点到工作流，返回 Python 分配的 nodeId
    QString registerNode(const DAPyNode& proxy);
    // 从工作流移除节点（按代理引用）
    bool unregisterNode(const DAPyNode& proxy);
    // 连接两个节点端口（按代理引用），返回连接描述符
    DAPyWorkFlowConnection linkNodes(const DAPyNode& srcProxy, const QString& srcOutput,
                                      const DAPyNode& dstProxy, const QString& dstInput);
    // 断开连接（按 connectionId）
    bool unlinkNode(const QString& connectionId);
    // 清空工作流所有节点和连接
    void clearWorkflow();
    // 通过元数据创建节点代理（委托给 factory）
    DAPyNode createNodeProxy(const DAPyNodeMetaData& metaData);
    // 获取工作流名称
    QString workflowName() const;
    // 设置工作流名称
    void setWorkflowName(const QString& name);
    // 获取所有节点（pybind11::list，仅供同模块内序列化使用）
    pybind11::list workflowNodes();
    // 获取所有连接（pybind11::list，仅供同模块内序列化使用）
    pybind11::list workflowConnections();

    // --- 节点操作 ---
    // 通过工厂创建节点并添加到workflow，成功后发射nodeAdded信号
    DAPyNode addNode(const QString& qualifiedName);
    // 从workflow移除节点，成功后发射nodeRemoved信号
    bool removeNode(const QString& nodeId);

    // --- 连接操作 ---
    // 连接两个节点端口，成功后发射connectionAdded信号
    DAPyWorkFlowConnection
    connectNode(const QString& srcNodeId, const QString& srcChannel, const QString& dstNodeId, const QString& dstChannel);
    // 断开连接，成功后发射connectionRemoved信号
    bool disconnectNode(const QString& connectionId);

    // --- 执行操作 ---
    // 开始执行工作流，发射executionStarted信号
    bool executeWorkflow();
    // 执行完成时发射executionFinished信号（由回调触发）
    // 执行器状态变更时发射executorStateChanged信号

Q_SIGNALS:
    // 节点添加信号
    void nodeAdded(QString nodeId, const DA::DAPyNode& proxy);
    // 节点移除信号
    void nodeRemoved(QString nodeId);
    // 连接添加信号
    void connectionAdded(QString connId, QString srcNodeId, QString srcChannel, QString dstNodeId, QString dstChannel);
    // 连接移除信号
    void connectionRemoved(QString connId);
    // 执行开始信号
    void executionStarted();
    // 执行完成信号
    void executionFinished(bool success);
    // 节点执行完成信号
    void nodeExecuted(QString nodeId, bool success);
    // 执行器状态变更信号
    void executorStateChanged(QString oldState, QString newState);

protected:
    // 虚工厂方法，子类可覆写以创建自定义 workflow 类型（如 DADataWorkFlow）
    virtual DAPyWorkFlow* createWorkflowInstance();
};

}  // namespace DA

#endif  // DAPYWORKFLOWMANAGER_H
