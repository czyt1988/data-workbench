#include "DAPyWorkFlow.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyGILGuard.h"
#include "DAPybind11InQt.h"
#include "DAPyNode.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>

namespace DA
{

//===================================================
// DAPyWorkFlow
//===================================================

DAPyWorkFlow::DAPyWorkFlow() : DAPyObjectWrapper()
{
    initPyWorkflow();
}

DAPyWorkFlow::DAPyWorkFlow(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
}

DAPyWorkFlow::~DAPyWorkFlow()
{
}

/**
 * @brief 初始化 Python DAWorkflow 实例
 *
 * 导入 DAWorkbench.DAWorkFlowPy 模块，获取 DAWorkflow 类引用，
 * 创建 Python DAWorkflow 实例并存储到基类 _object。
 */
void DAPyWorkFlow::initPyWorkflow()
{
    if (!isNone()) {
        return;
    }
    DAPyGILGuard gil;
    try {
        DAPyModuleWorkflow pyModule = DAPyModuleWorkflow();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlow::initPyWorkflow: cannot import DAWorkbench.DAWorkFlowPy";
                return;
            }
        }
        pybind11::object workflowClass = pyModule.getWorkflowObject();
        if (workflowClass.is_none()) {
            qWarning() << "DAPyWorkFlow::initPyWorkflow: DAWorkflow class is not available";
            return;
        }
        pybind11::object workflowInstance = workflowClass();
        object()                          = workflowInstance;
        qDebug() << "DAPyWorkFlow::initPyWorkflow: Python DAWorkflow instance created";
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

bool DAPyWorkFlow::isValid() const
{
    try {
        return !isNone();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 添加节点到 Python DAWorkflow
 *
 * 返回节点id
 */
QString DAPyWorkFlow::addNode(DAPyNode* proxy)
{
    // TODO:DAPyNode是pybind11的代理，不需要用指针，直接对象传递
    if (!proxy) {
        qWarning() << "DAPyWorkFlow::addNode: proxy is nullptr";
        return QString();
    }
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::addNode: workflow object is invalid";
            return QString();
        }
        pybind11::object pyNodeRef = proxy->object();
        if (!pyNodeRef) {
            qWarning() << "DAPyWorkFlow::addNode: proxy has no valid Python node reference";
            return QString();
        }
        pybind11::object result = attr("add_node")(pyNodeRef);
        QString nodeIdStr       = result.cast< QString >();
        return nodeIdStr;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 移除节点从 Python DAWorkflow（按 nodeId）
 */
bool DAPyWorkFlow::removeNode(const QString& nodeId)
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::removeNode: workflow object is invalid";
            return false;
        }
        pybind11::object pyNodeRef = attr("removeNode")(pybind11::arg("node_id") = nodeId);
        if (pyNodeRef.is_none()) {
            qWarning() << "DAPyWorkFlow::removeNode: node not found for id:" << nodeId;
            return false;
        }
        return true;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 通过代理指针移除节点
 */
bool DAPyWorkFlow::removeNode(DAPyNode* proxy)
{
    if (!proxy) {
        qWarning() << "DAPyWorkFlow::removeNode(DAPyNode*): proxy is nullptr";
        return false;
    }
    DAPyGILGuard gil;
    if (isNone()) {
        qWarning() << "DAPyWorkFlow::removeNode: workflow object is invalid";
        return false;
    }
    try {
        pybind11::object pyNodeRef = proxy->object();
        attr("remove_node")(pybind11::arg("node_instance") = pyNodeRef);
    } catch (const std::exception& e) {
        dealException(e);
        return false;
    }
    return true;
}

/**
 * @brief 连接两个节点的端口，返回连接描述符
 */
DAPyWorkFlowConnection DAPyWorkFlow::connectNode(const QString& srcNodeId,
                                                 const QString& srcChannel,
                                                 const QString& dstNodeId,
                                                 const QString& dstChannel)
{
    DAPyWorkFlowConnection result;
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::connectNode: workflow is not valid";
        return result;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object conn = attr("connect_node")(srcNodeId, srcChannel, dstNodeId, dstChannel);
        result.connectionId   = conn.attr("connection_id").cast< QString >();
        result.sourceNodeId   = conn.attr("source_node_id").cast< QString >();
        result.sourceChannel  = conn.attr("source_output_channel").cast< QString >();
        result.targetNodeId   = conn.attr("target_node_id").cast< QString >();
        result.targetChannel  = conn.attr("target_input_channel").cast< QString >();
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return result;
}

/**
 * @brief 断开指定连接
 */
bool DAPyWorkFlow::disconnectNode(const QString& connectionId)
{
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::disconnectNode: workflow is not valid";
        return false;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object connObj = attr("remove_connection")(connectionId);
        QString fromPort         = connObj.attr("source_output_channel").cast< QString >();
        QString toPort           = connObj.attr("target_input_channel").cast< QString >();
        return true;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

bool DAPyWorkFlow::removeConnection(const QString& connectionId)
{
    return disconnectNode(connectionId);
}

void DAPyWorkFlow::clear()
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::clear: workflow object is invalid";
            return;
        }
        attr("clear")();
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

int DAPyWorkFlow::nodeCount()
{
    DAPyGILGuard gil;
    try {
        return static_cast< int >(pybind11::len(object()));
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return 0;
}

bool DAPyWorkFlow::hasNode(const QString& nodeId)
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::hasNode: workflow object is invalid";
            return false;
        }
        pybind11::object result = attr("__contains__")(nodeId);
        return result.cast< bool >();
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 通过代理指针连接两个节点
 */
DAPyWorkFlowConnection DAPyWorkFlow::connectNode(DAPyNode* src, const QString& srcChannel, DAPyNode* dst, const QString& dstChannel)
{
    if (!src) {
        qWarning() << "DAPyWorkFlow::connectNode(DAPyNode*, ...): src is nullptr";
        return DAPyWorkFlowConnection();
    }
    if (!dst) {
        qWarning() << "DAPyWorkFlow::connectNode(..., DAPyNode*, ...): dst is nullptr";
        return DAPyWorkFlowConnection();
    }
    DAPyGILGuard gil;
    QString srcNodeId = src->getNodeId();
    QString dstNodeId = dst->getNodeId();
    if (srcNodeId.isEmpty() || dstNodeId.isEmpty()) {
        qWarning() << "DAPyWorkFlow::connectNode(DAPyNode*, ...): src/dst has empty nodeId";
        return DAPyWorkFlowConnection();
    }
    DAPyWorkFlowConnection connResult = connectNode(srcNodeId, srcChannel, dstNodeId, dstChannel);
    return connResult;
}

bool DAPyWorkFlow::disconnectNode(DAPyLinkGraphicsItem* link)
{
    if (!link) {
        qWarning() << "DAPyWorkFlow::disconnectNode(DAPyLinkGraphicsItem*): link is nullptr";
        return false;
    }
    qWarning() << "DAPyWorkFlow::disconnectNode(DAPyLinkGraphicsItem*): DAPyLinkGraphicsItem lacks getConnectionId(), "
                  "unimplemented";
    return false;
}

bool DAPyWorkFlow::hasNode(DAPyNode* proxy)
{
    if (!proxy) {
        qWarning() << "DAPyWorkFlow::hasNode(DAPyNode*): proxy is nullptr";
        return false;
    }
    DAPyGILGuard gil;
    QString nodeId = proxy->getNodeId();
    if (nodeId.isEmpty()) {
        qWarning() << "DAPyWorkFlow::hasNode(DAPyNode*): proxy has empty nodeId";
        return false;
    }
    return hasNode(nodeId);
}

/**
 * @brief 通过 node_id 获取 Python 节点对象
 */
pybind11::object DAPyWorkFlow::getNodeById(const QString& nodeId)
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::getNodeById: workflow object is invalid";
            return pybind11::none();
        }
        pybind11::object result = attr("get_node_by_id")(nodeId);
        return result;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return pybind11::none();
}

/**
 * @brief 获取所有节点列表
 */
pybind11::list DAPyWorkFlow::getNodes()
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::getNodes: workflow object is invalid";
            return pybind11::list();
        }
        pybind11::object result = attr("get_nodes")();
        return pybind11::list(result);
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return pybind11::list();
}

/**
 * @brief 获取所有连接列表
 */
pybind11::list DAPyWorkFlow::getConnections()
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::getConnections: workflow object is invalid";
            return pybind11::list();
        }
        pybind11::object result = attr("get_connections")();
        return pybind11::list(result);
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return pybind11::list();
}

/**
 * @brief 验证工作流是否为有效 DAG
 */
bool DAPyWorkFlow::isValidDag()
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::isValidDag: workflow object is invalid";
            return false;
        }
        return attr("is_valid_dag")().cast< bool >();
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 获取拓扑排序结果
 */
QStringList DAPyWorkFlow::topologicalSort()
{
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::topologicalSort: workflow object is invalid";
            return QStringList();
        }
        pybind11::list pyResult = attr("topological_sort")();
        QStringList result;
        for (auto item : pyResult) {
            result.append(item.cast< QString >());
        }
        return result;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QStringList();
}

}  // namespace DA
