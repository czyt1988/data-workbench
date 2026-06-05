#include "DAPyWorkFlowManager.h"
#include "DAPyWorkFlow.h"
#include "DAPyNodeFactory.h"
#include "DAPyNode.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPybind11InQt.h"
#include <QDebug>

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
    DAPyWorkFlow* mWorkflow { nullptr };
    // 节点工厂代理实例（shared_ptr统一管理生命周期）
    std::shared_ptr< DAPyNodeFactory > mFactory;
};

//===================================================
// DAPyWorkFlowManager
//===================================================

/**
 * @brief 虚工厂方法，创建默认的工作流实例
 *
 * 子类可覆写此方法以创建自定义的工作流类型（如DADataWorkFlow）。
 *
 * @return 新创建的DAPyWorkFlow实例指针
 */
DAPyWorkFlow* DAPyWorkFlowManager::createWorkflowInstance()
{
    return new DAPyWorkFlow();
}

/**
 * @brief 构造函数
 *
 * 通过虚工厂方法createWorkflowInstance()创建DAPyWorkFlow代理实例。
 * 工厂由外部通过setFactory()注入，构造时不创建默认实例。
 */
DAPyWorkFlowManager::DAPyWorkFlowManager(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mWorkflow = createWorkflowInstance();
}

DAPyWorkFlowManager::~DAPyWorkFlowManager()
{
    DA_D(d);
    delete d->mWorkflow;
}

/**
 * @brief 获取工作流代理
 *
 * @return DAPyWorkFlow指针，始终非nullptr
 */
DAPyWorkFlow* DAPyWorkFlowManager::getWorkflow() const
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
 */
void DAPyWorkFlowManager::setWorkflow(DAPyWorkFlow* wf)
{
    DA_D(d);
    if (d->mWorkflow != wf) {
        delete d->mWorkflow;
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
    return d->mWorkflow && !d->mWorkflow->isNone();
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
    try {
        return d->mWorkflow->addNode(proxy);
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
    try {
        return d->mWorkflow->removeNode(proxy);
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
DAPyWorkFlowConnection DAPyWorkFlowManager::linkNodes(const DAPyNode& srcProxy,
                                                      const QString& srcOutput,
                                                      const DAPyNode& dstProxy,
                                                      const QString& dstInput)
{
    DA_D(d);
    try {
        return d->mWorkflow->connectNode(srcProxy, srcOutput, dstProxy, dstInput);
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::linkNodes:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::linkNodes:" << e.what();
    }
    return DAPyWorkFlowConnection();
}

/**
 * @brief 断开连接（不发射信号）
 *
 * @param[in] connectionId 要断开的连接ID
 * @return 成功返回true，失败返回false
 */
bool DAPyWorkFlowManager::unlinkNode(const QString& connectionId)
{
    DA_D(d);
    try {
        return d->mWorkflow->disconnectNode(connectionId);
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::unlinkNode:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::unlinkNode:" << e.what();
    }
    return false;
}

/**
 * @brief 清空工作流所有节点和连接（不发射信号）
 */
void DAPyWorkFlowManager::clearWorkflow()
{
    DA_D(d);
    try {
        d->mWorkflow->clear();
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
    try {
        if (d->mWorkflow && d->mWorkflow->hasattr("name")) {
            return d->mWorkflow->attr("name").cast< QString >();
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
    try {
        if (d->mWorkflow && d->mWorkflow->hasattr("name")) {
            d->mWorkflow->object().attr("name") = name.toStdString();
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
pybind11::list DAPyWorkFlowManager::workflowNodes()
{
    DA_D(d);
    try {
        return d->mWorkflow->getNodes();
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::workflowNodes:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::workflowNodes:" << e.what();
    }
    return pybind11::list();
}

/**
 * @brief 获取工作流中所有连接
 *
 * @return Python连接列表，失败返回空列表
 */
pybind11::list DAPyWorkFlowManager::workflowConnections()
{
    DA_D(d);
    try {
        return d->mWorkflow->getConnections();
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::workflowConnections:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::workflowConnections:" << e.what();
    }
    return pybind11::list();
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
        QString nodeId = d->mWorkflow->addNode(proxy);
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
    try {
        bool result = d->mWorkflow->removeNode(nodeId);
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
DAPyWorkFlowConnection DAPyWorkFlowManager::connectNode(const QString& srcNodeId,
                                                        const QString& srcChannel,
                                                        const QString& dstNodeId,
                                                        const QString& dstChannel)
{
    DA_D(d);
    DAPyWorkFlowConnection result;
    try {
        result = d->mWorkflow->connectNode(srcNodeId, srcChannel, dstNodeId, dstChannel);
        if (result.isValid()) {
            Q_EMIT connectionAdded(result.connectionId, srcNodeId, srcChannel, dstNodeId, dstChannel);
        } else {
            qCritical() << "DAPyWorkFlowManager::connectNode: connectNode failed";
        }
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::connectNode:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::connectNode:" << e.what();
    }
    return result;
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
    try {
        bool result = d->mWorkflow->disconnectNode(connectionId);
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
 * 调用Python workflow的execute方法，
 * 发射executionStarted信号。
 *
 * @return 成功返回true，失败返回false
 */
bool DAPyWorkFlowManager::executeWorkflow()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        if (d->mWorkflow->isNone()) {
            qCritical() << "DAPyWorkFlowManager::executeWorkflow: workflow is invalid";
            return false;
        }
        d->mWorkflow->attr("execute")();
        Q_EMIT executionStarted();
        return true;
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::executeWorkflow:" << e.what();
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::executeWorkflow:" << e.what();
    }
    return false;
}

}  // namespace DA
