#include "DAPyWorkFlowManager.h"
#include "DAPyWorkFlow.h"
#include "DAPyNodeFactory.h"
#include "DAPyNodeProxy.h"
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
    PrivateData(DAPyWorkFlowManager* p) : q_ptr(p) {}

    // 工作流代理实例
    DAPyWorkFlow* mWorkflow { nullptr };
    // 节点工厂代理实例
    DAPyNodeFactory* mFactory { nullptr };
};

//===================================================
// DAPyWorkFlowManager
//===================================================

/**
 * @brief 构造函数
 *
 * 创建DAPyWorkFlow和DAPyNodeFactory代理实例作为成员。
 */
DAPyWorkFlowManager::DAPyWorkFlowManager(QObject* parent)
    : QObject(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mWorkflow = new DAPyWorkFlow();
    d->mFactory  = new DAPyNodeFactory();
}

DAPyWorkFlowManager::~DAPyWorkFlowManager()
{
    DA_D(d);
    delete d->mWorkflow;
    delete d->mFactory;
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
    return d->mFactory;
}

/**
 * @brief 通过工厂创建节点并添加到workflow
 *
 * 调用DAPyNodeFactory::createNodeProxy创建节点代理，
 * 然后调用DAPyWorkFlow::addNode将节点添加到Python workflow。
 * 成功后发射nodeAdded信号。
 *
 * @param[in] qualifiedName Python节点的限定名
 * @return 成功返回DAPyNodeProxy指针，失败返回nullptr
 */
DAPyNodeProxy* DAPyWorkFlowManager::addNode(const QString& qualifiedName)
{
    DA_D(d);
    DAPyNodeProxy* proxy = nullptr;
    try {
        proxy = d->mFactory->createNodeProxy(qualifiedName);
        if (!proxy) {
            qCritical() << "DAPyWorkFlowManager::addNode: createNodeProxy failed for" << qualifiedName;
            return nullptr;
        }
        QString nodeId = d->mWorkflow->addNode(proxy);
        if (nodeId.isEmpty()) {
            qCritical() << "DAPyWorkFlowManager::addNode: workflow addNode returned empty nodeId";
            delete proxy;
            return nullptr;
        }
        Q_EMIT nodeAdded(nodeId, proxy);
    } catch (const pybind11::error_already_set& e) {
        qCritical() << "DAPyWorkFlowManager::addNode:" << e.what();
        delete proxy;
        return nullptr;
    } catch (const std::exception& e) {
        qCritical() << "DAPyWorkFlowManager::addNode:" << e.what();
        delete proxy;
        return nullptr;
    }
    return proxy;
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