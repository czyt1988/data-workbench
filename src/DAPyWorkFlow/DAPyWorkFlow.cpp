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
/**
 * @brief 注意，这里构造不会构造一个None，而是会实例化一个DAWorkflow的python对象
 */
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
QString DAPyWorkFlow::addNode(const DAPyNode& proxy)
{
    if (proxy.isNone()) {
        qWarning() << "DAPyWorkFlow::addNode: proxy is none";
        return QString();
    }
    DAPyGILGuard gil;
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::addNode: workflow object is invalid";
            return QString();
        }
        pybind11::object pyNodeRef = proxy.object();
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
        pybind11::object pyNodeRef = attr("remove_node")(pybind11::arg("node_id") = nodeId);
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
 * @brief 通过代理引用移除节点
 */
bool DAPyWorkFlow::removeNode(const DAPyNode& proxy)
{
    if (proxy.isNone()) {
        qWarning() << "DAPyWorkFlow::removeNode(const DAPyNode&): proxy is none";
        return false;
    }
    DAPyGILGuard gil;
    if (isNone()) {
        qWarning() << "DAPyWorkFlow::removeNode: workflow object is invalid";
        return false;
    }
    try {
        pybind11::object pyNodeRef = proxy.object();
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
DAPyNodeConnection DAPyWorkFlow::connectNode(const QString& srcNodeId,
                                             const QString& srcChannel,
                                             const QString& dstNodeId,
                                             const QString& dstChannel)
{
    DAPyNodeConnection result;
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::connectNode: workflow is not valid";
        return result;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object conn = attr("connect_node")(srcNodeId, srcChannel, dstNodeId, dstChannel);
        result                = conn;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return result;
}

/**
 * @brief 通过代理引用连接两个节点
 */
DAPyNodeConnection
DAPyWorkFlow::connectNode(const DAPyNode& src, const QString& srcChannel, const DAPyNode& dst, const QString& dstChannel)
{
    if (src.isNone()) {
        qWarning() << "DAPyWorkFlow::connectNode(const DAPyNode&, ...): src is none";
        return DAPyNodeConnection();
    }
    if (dst.isNone()) {
        qWarning() << "DAPyWorkFlow::connectNode(..., const DAPyNode&, ...): dst is none";
        return DAPyNodeConnection();
    }
    QString srcNodeId = src.getNodeId();
    QString dstNodeId = dst.getNodeId();
    if (srcNodeId.isEmpty() || dstNodeId.isEmpty()) {
        qWarning() << "DAPyWorkFlow::connectNode(const DAPyNode&, ...): src/dst has empty nodeId";
        return DAPyNodeConnection();
    }
    DAPyNodeConnection connResult = connectNode(srcNodeId, srcChannel, dstNodeId, dstChannel);
    return connResult;
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

bool DAPyWorkFlow::hasNode(const DAPyNode& proxy)
{
    if (proxy.isNone()) {
        qWarning() << "DAPyWorkFlow::hasNode(const DAPyNode&): proxy is none";
        return false;
    }
    DAPyGILGuard gil;
    QString nodeId = proxy.getNodeId();
    if (nodeId.isEmpty()) {
        qWarning() << "DAPyWorkFlow::hasNode(const DAPyNode&): proxy has empty nodeId";
        return false;
    }
    return hasNode(nodeId);
}

/**
 * @brief 通过 node_id 获取 Python 节点对象
 */
DAPyNode DAPyWorkFlow::getNodeById(const QString& nodeId)
{
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::getNodeById: workflow object is invalid";
            return DAPyNode();
        }
        pybind11::object result = attr("get_node_by_id")(nodeId);
        DAPyNode node(result);
        return node;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyNode();
}

/**
 * @brief 获取所有节点列表
 */
QList< DAPyNode > DAPyWorkFlow::getNodes()
{
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::getNodes: workflow object is invalid";
            return QList< DAPyNode >();
        }
        pybind11::list result = attr("get_nodes")();
        QList< DAPyNode > ns;
        for (std::size_t i = 0; i < result.size(); ++i) {
            DAPyNode n(result[ i ]);
            if (!n.isNone()) {
                ns.append(n);
            }
        }
        return ns;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QList< DAPyNode >();
}

/**
 * @brief 获取所有连接列表
 */
QList< DAPyNodeConnection > DAPyWorkFlow::getConnections()
{
    try {
        if (isNone()) {
            qWarning() << "DAPyWorkFlow::getConnections: workflow object is invalid";
            return QList< DAPyNodeConnection >();
        }
        pybind11::list result = attr("get_connections")();
        QList< DAPyNodeConnection > connections;
        for (std::size_t i = 0; i < result.size(); ++i) {
            DAPyNodeConnection conn(result[ i ]);
            if (!conn.isNone()) {
                connections.append(conn);
            }
        }
        return connections;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QList< DAPyNodeConnection >();
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
