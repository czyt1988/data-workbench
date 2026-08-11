#include "DAPyWorkFlowManager.h"
#include "DAPyWorkFlow.h"
#include "DAPyNodeFactory.h"
#include "DAPyNode.h"
#include "DAPyWorkFlowExecutor.h"
#include "DAPyWorkFlowAPI.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>
#include "DALogCategory.h"

namespace DA
{

//===================================================
// DAPyWorkFlowManager::PrivateData
//===================================================

class DAPyWorkFlowManager::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowManager)
public:
    PrivateData(DAPyWorkFlowManager* p) : q_ptr(p)
    {
    }

    // 工作流代理实例
    DAPyWorkFlow mWorkflow;
    // 节点工厂代理实例（shared_ptr统一管理生命周期）
    std::shared_ptr< DAPyNodeFactory > mFactory;
};

//===================================================
// DAPyWorkFlowManager
//===================================================

/**
 * @brief 构造函数
 *
 * 通过虚工厂方法createWorkflowInstance()创建DAPyWorkFlow代理实例。
 * 工厂由外部通过setFactory()注入，构造时不创建默认实例。
 */
DAPyWorkFlowManager::DAPyWorkFlowManager(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mWorkflow = DAPyWorkFlow();
}

DAPyWorkFlowManager::~DAPyWorkFlowManager()
{
}

/**
 * @brief 获取工作流代理
 *
 * @return DAPyWorkFlow指针，始终非nullptr
 */
DAPyWorkFlow DAPyWorkFlowManager::getWorkflow() const
{
    DA_DC(d);
    return d->mWorkflow;
}

/**
 * @brief 获取节点工厂代理
 *
 * @return DAPyNodeFactory指针，始终非nullptr
 */
DAPyNodeFactory* DAPyWorkFlowManager::getFactory() const
{
    DA_DC(d);
    return d->mFactory.get();
}

/**
 * @brief 替换内部workflow实例
 *
 * Manager取得传入workflow的所有权，旧实例会被delete。
 * 如果传入的指针与当前持有的相同，则不做任何操作。
 *
 * @param[in] wf 新的DAPyWorkFlow实例指针（Manager取得所有权）
 *
 * @note DAPyWorkFlowManager会取得DAPyWorkFlow的所有权，在DAPyWorkFlowManager析构时会删除
 */
void DAPyWorkFlowManager::setWorkflow(const DAPyWorkFlow& wf)
{
    DA_D(d);
    if (d->mWorkflow != wf) {
        d->mWorkflow = wf;
    }
}

/**
 * @brief 替换内部factory实例（shared_ptr版本）
 *
 * Manager与外部共享factory的所有权，避免重复创建。
 * 传入shared_ptr后Manager不再用delete管理factory生命周期。
 *
 * @param[in] factory Python节点工厂的共享指针
 */
void DAPyWorkFlowManager::setFactory(std::shared_ptr< DAPyNodeFactory > factory)
{
    DA_D(d);
    if (d->mFactory != factory) {
        d->mFactory = std::move(factory);
    }
}

/**
 * @brief 检查工作流代理是否有效
 *
 * @return 如果workflow指针非空且Python对象有效返回true
 */
bool DAPyWorkFlowManager::isWorkflowValid() const
{
    DA_DC(d);
    return d->mWorkflow && !d->mWorkflow.isNone();
}

/**
 * @brief 注册节点到工作流（不发射信号）
 *
 * 将节点代理添加到Python workflow，返回Python分配的nodeId。
 * 捕获所有Python异常并返回空字符串表示失败。
 *
 * @param[in] proxy 节点代理
 * @return Python分配的nodeId，失败返回空字符串
 */
QString DAPyWorkFlowManager::registerNode(const DAPyNode& proxy)
{
    DA_D(d);
    DAPyGILGuard gil;
    DA_WF_DBG("[C++] Manager::registerNode: start registering node");
    try {
        QString nodeId = d->mWorkflow.addNode(proxy);
        DA_WF_DBG("[C++] Manager::registerNode: node registered successfully, nodeId=%s", nodeId.toUtf8().constData());
        return nodeId;
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::registerNode:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::registerNode:" << e.what();
    }
    return QString();
}

/**
 * @brief 从工作流移除节点（不发射信号）
 *
 * @param[in] proxy 要移除的节点代理
 * @return 成功返回true，失败返回false
 */
bool DAPyWorkFlowManager::unregisterNode(const DAPyNode& proxy)
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        return d->mWorkflow.removeNode(proxy);
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::unregisterNode:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::unregisterNode:" << e.what();
    }
    return false;
}

/**
 * @brief 连接两个节点端口（不发射信号）
 *
 * @param[in] srcProxy 源节点代理
 * @param[in] srcOutput 源节点输出端口名
 * @param[in] dstProxy 目标节点代理
 * @param[in] dstInput 目标节点输入端口名
 * @return 连接描述符，失败返回无效描述符
 */
DAPyNodeConnection DAPyWorkFlowManager::connectNode(const DAPyNode& srcProxy,
                                                    const QString& srcOutput,
                                                    const DAPyNode& dstProxy,
                                                    const QString& dstInput)
{
    DA_D(d);
    DAPyGILGuard gil;
    DA_WF_DBG("[C++] Manager::connectNode: %s.%s -> %s.%s",
              srcProxy.getNodeId().toUtf8().constData(),
              srcOutput.toUtf8().constData(),
              dstProxy.getNodeId().toUtf8().constData(),
              dstInput.toUtf8().constData());
    try {
        DAPyNodeConnection conn = d->mWorkflow.connectNode(srcProxy, srcOutput, dstProxy, dstInput);
        if (conn) {
            QString connId = conn.getConnectionId();
            DA_WF_DBG("[C++] Manager::connectNode: connection succeeded, connId=%s", connId.toUtf8().constData());
            Q_EMIT connectionAdded(connId, srcProxy.getNodeId(), srcOutput, dstProxy.getNodeId(), dstInput);
        }
        return conn;
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::linkNodes:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::linkNodes:" << e.what();
    }
    return DAPyNodeConnection();
}

/**
 * @brief 清空工作流所有节点和连接（不发射信号）
 */
void DAPyWorkFlowManager::clearWorkflow()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        d->mWorkflow.clear();
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::clearWorkflow:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::clearWorkflow:" << e.what();
    }
}

/**
 * @brief 通过元数据创建节点代理（委托给factory）
 *
 * @param[in] metaData 节点元数据
 * @return 创建的节点代理，失败返回isNone()为true的默认代理
 */
DAPyNode DAPyWorkFlowManager::createNodeProxy(const DAPyNodeMetaData& metaData)
{
    DA_D(d);
    DAPyGILGuard gil;
    if (!d->mFactory) {
        qCritical() << "DAPyWorkFlowManager::createNodeProxy: factory not set";
        return DAPyNode();
    }
    try {
        return d->mFactory->createNode(metaData);
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::createNodeProxy:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::createNodeProxy:" << e.what();
    }
    return DAPyNode();
}

/**
 * @brief 获取工作流名称
 *
 * @return 工作流名称，获取失败返回空字符串
 */
QString DAPyWorkFlowManager::workflowName() const
{
    DA_DC(d);
    DAPyGILGuard gil;
    try {
        if (d->mWorkflow && d->mWorkflow.hasattr("name")) {
            return d->mWorkflow.attr("name").cast< QString >();
        }
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::workflowName:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::workflowName:" << e.what();
    }
    return QString();
}

/**
 * @brief 设置工作流名称
 *
 * @param[in] name 新的工作流名称
 */
void DAPyWorkFlowManager::setWorkflowName(const QString& name)
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        if (d->mWorkflow && d->mWorkflow.hasattr("name")) {
            d->mWorkflow.object().attr("name") = pybind11::cast(name);
        }
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::setWorkflowName:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::setWorkflowName:" << e.what();
    }
}

/**
 * @brief 获取工作流中所有节点
 *
 * @return Python节点列表，失败返回空列表
 */
QList< DAPyNode > DAPyWorkFlowManager::workflowNodes()
{
    DA_D(d);
    DAPyGILGuard gil;
    return d->mWorkflow.getNodes();
}

/**
 * @brief 获取工作流中所有连接
 *
 * @return Python连接列表，失败返回空列表
 */
QList< DAPyNodeConnection > DAPyWorkFlowManager::workflowConnections()
{
    DA_D(d);
    DAPyGILGuard gil;
    return d->mWorkflow.getConnections();
}

/**
 * @brief 通过工厂创建节点并添加到workflow
 *
 * 调用DAPyNodeFactory::createNode创建节点代理，
 * 然后调用DAPyWorkFlow::addNode将节点添加到Python workflow。
 * 成功后发射nodeAdded信号。
 *
 * @param[in] qualifiedName Python节点的限定名
 * @return 成功返回DAPyNode值，失败返回isNone()为true的默认DAPyNode
 */
DAPyNode DAPyWorkFlowManager::addNode(const QString& qualifiedName)
{
    DA_D(d);
    DAPyGILGuard gil;
    if (!d->mFactory) {
        qCritical() << "DAPyWorkFlowManager::addNode: factory not set";
        return DAPyNode();
    }
    try {
        DAPyNode proxy = d->mFactory->createNode(qualifiedName);
        if (proxy.isNone()) {
            qCritical() << "DAPyWorkFlowManager::addNode: createNodeProxy failed for" << qualifiedName;
            return DAPyNode();
        }
        QString nodeId = d->mWorkflow.addNode(proxy);
        if (nodeId.isEmpty()) {
            qCritical() << "DAPyWorkFlowManager::addNode: workflow addNode returned empty nodeId";
            return DAPyNode();
        }
        Q_EMIT nodeAdded(nodeId, proxy);
        return proxy;
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::addNode:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::addNode:" << e.what();
    }
    return DAPyNode();
}

/**
 * @brief 从workflow移除节点
 *
 * 调用DAPyWorkFlow::removeNode移除节点，
 * 成功后发射nodeRemoved信号。
 *
 * @param[in] nodeId 要移除的节点ID
 * @return 成功返回true，失败返回false
 */
bool DAPyWorkFlowManager::removeNode(const QString& nodeId)
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        bool result = d->mWorkflow.removeNode(nodeId);
        if (result) {
            Q_EMIT nodeRemoved(nodeId);
        } else {
            qCritical() << "DAPyWorkFlowManager::removeNode: removeNode failed for" << nodeId;
        }
        return result;
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::removeNode:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::removeNode:" << e.what();
    }
    return false;
}

/**
 * @brief 连接两个节点端口
 *
 * 调用DAPyWorkFlow::connectNode建立连接，
 * 成功后发射connectionAdded信号。
 *
 * @param[in] srcNodeId 源节点ID
 * @param[in] srcChannel 源节点输出端口名
 * @param[in] dstNodeId 目标节点ID
 * @param[in] dstChannel 目标节点输入端口名
 * @return 连接描述符，失败时connectionId为空
 */
DAPyNodeConnection DAPyWorkFlowManager::connectNode(const QString& srcNodeId,
                                                    const QString& srcChannel,
                                                    const QString& dstNodeId,
                                                    const QString& dstChannel)
{
    DA_D(d);
    DAPyGILGuard gil;
    DAPyNodeConnection conn = d->mWorkflow.connectNode(srcNodeId, srcChannel, dstNodeId, dstChannel);
    if (conn) {
        Q_EMIT connectionAdded(conn.getConnectionId(), srcNodeId, srcChannel, dstNodeId, dstChannel);
    }
    return conn;
}

/**
 * @brief 断开连接
 *
 * 调用DAPyWorkFlow::disconnectNode断开连接，
 * 成功后发射connectionRemoved信号。
 *
 * @param[in] connectionId 要断开的连接ID
 * @return 成功返回true，失败返回false
 */
bool DAPyWorkFlowManager::disconnectNode(const QString& connectionId)
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        bool result = d->mWorkflow.disconnectNode(connectionId);
        if (result) {
            Q_EMIT connectionRemoved(connectionId);
        } else {
            qCritical() << "DAPyWorkFlowManager::disconnectNode: disconnectNode failed for" << connectionId;
        }
        return result;
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::disconnectNode:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::disconnectNode:" << e.what();
    }
    return false;
}

/**
 * @brief 开始执行工作流
 *
 * 通过 DAPyWorkFlowExecutor 创建 Python DAWorkflowExecutor 实例，
 * 将 Python 回调桥接到 Manager 的 Qt 信号，然后同步执行工作流。
 * 发射 executionStarted 和 executionFinished 信号。
 *
 * @return 成功返回true，失败返回false
 */
bool DAPyWorkFlowManager::executeWorkflow()
{
    DA_D(d);
    DAPyGILGuard gil;
    if (d->mWorkflow.isNone()) {
        qCritical() << "DAPyWorkFlowManager::executeWorkflow: workflow is invalid";
        return false;
    }
    // 创建执行器代理（纯 DAPyObjectWrapper 子类）
    DAPyWorkFlowExecutor executor(d->mWorkflow);
    if (executor.isNone()) {
        qCritical() << "DAPyWorkFlowManager::executeWorkflow: failed to create executor";
        return false;
    }
    // 将 Python 会调桥接到 Manager 的 Qt 信号
    executor.setOnStateChange([ this ](const QString& oldState, const QString& newState) {
        DA_WF_DBG("[C++] Manager: State Change %s -> %s", oldState.toUtf8().constData(), newState.toUtf8().constData());
        Q_EMIT executorStateChanged(oldState, newState);
    });
    executor.setOnNodeFinished([ this ](const QString& nodeId, bool success) {
        DA_WF_DBG("[C++] Manager: Node Finished nodeId=%s, success=%s", nodeId.toUtf8().constData(), success ? "true" : "false");
        Q_EMIT nodeExecuted(nodeId, success);
    });

    Q_EMIT executionStarted();
    bool success = executor.execute();
    DA_WF_DBG("[C++] Manager::executeWorkflow: execute finished, success=%s", success ? "true" : "false");
    if (!success) {
        QStringList errors = executor.getErrorMessages();
        DA_WF_DBG("[C++] Manager::executeWorkflow: error count=%d", errors.size());
        for (const QString& err : std::as_const(errors)) {
            qCritical() << "Workflow error:" << err;
        }
    }
    Q_EMIT executionFinished(success);
    return success;
}

}  // namespace DA
