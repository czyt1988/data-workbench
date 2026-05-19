#include "DAPyWorkFlow.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyGILGuard.h"
#include "DAPybind11InQt.h"
#include "DAPyNodeProxy.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>

namespace DA
{

//===================================================
// DAPyWorkFlow
//===================================================

DAPyWorkFlow::DAPyWorkFlow(QObject* parent) : DAPyObjectWrapper(), QObject(parent)
{
    initPyWorkflow();
}

DAPyWorkFlow::DAPyWorkFlow(const pybind11::object& obj, QObject* parent) : DAPyObjectWrapper(obj), QObject(parent)
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
        DAPyModuleWorkflow& pyModule = DAPyModuleWorkflow::getInstance();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlow::initPyWorkflow: cannot import DAWorkbench.DAWorkFlowPy";
                return;
            }
        }
        pybind11::object workflowClass = pyModule.getWorkflowClass();
        if (workflowClass.is_none()) {
            qWarning() << "DAPyWorkFlow::initPyWorkflow: DAWorkflow class is not available";
            return;
        }
        pybind11::object workflowInstance = workflowClass();
        object()                          = workflowInstance;
        qDebug() << "DAPyWorkFlow::initPyWorkflow: Python DAWorkflow instance created";
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
}

/**
 * @brief 设置外部 Python DAWorkflow 实例
 *
 * 接收外部传入的 Python workflow 对象，替代内部 initPyWorkflow() 创建的实例。
 * 直接赋值给基类 _object。
 */
void DAPyWorkFlow::setPyWorkflowObject(const pybind11::object& obj)
{
    object() = obj;
}

bool DAPyWorkFlow::isValid() const
{
    DAPyGILGuard gil;
    try {
        return !isNone();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 添加节点到 Python DAWorkflow
 */
QString DAPyWorkFlow::addNode(DAPyNodeProxy* proxy)
{
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
        Q_EMIT nodeAdded(proxy);
        return nodeIdStr;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
        mLastErrorString = e.what();
        dealException(e);
        return false;
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
        return false;
    }
}

/**
 * @brief 通过代理指针移除节点
 */
bool DAPyWorkFlow::removeNode(DAPyNodeProxy* proxy)
{
    if (!proxy) {
        qWarning() << "DAPyWorkFlow::removeNode(DAPyNodeProxy*): proxy is nullptr";
        return false;
    }
    DAPyGILGuard gil;
    if (isNone()) {
        qWarning() << "DAPyWorkFlow::removeNode: workflow object is invalid";
        return false;
    }
    try {
        attr("remove_node")(proxy->object());
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
        return false;
    }
    Q_EMIT nodeRemoved(proxy);
    return true;
}

/**
 * @brief 连接两个节点的端口，返回连接描述符
 */
DAPyWorkFlowConnection DAPyWorkFlow::connectNode(
    const QString& srcNodeId, const QString& srcChannel, const QString& dstNodeId, const QString& dstChannel
)
{
    DAPyWorkFlowConnection result;
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::connectNode: workflow is not valid";
        return result;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object conn = attr("connect_node")(
            srcNodeId.toStdString(), srcChannel.toStdString(), dstNodeId.toStdString(), dstChannel.toStdString()
        );
        result.connectionId  = QString::fromStdString(pybind11::str(conn.attr("connection_id")));
        result.sourceNodeId  = QString::fromStdString(pybind11::str(conn.attr("source_node_id")));
        result.sourceChannel = QString::fromStdString(pybind11::str(conn.attr("source_output_channel")));
        result.targetNodeId  = QString::fromStdString(pybind11::str(conn.attr("target_node_id")));
        result.targetChannel = QString::fromStdString(pybind11::str(conn.attr("target_input_channel")));
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
        pybind11::object connObj = attr("remove_connection")(connectionId.toStdString());
        QString fromPort = QString::fromStdString(pybind11::str(connObj.attr("source_output_channel")));
        QString toPort   = QString::fromStdString(pybind11::str(connObj.attr("target_input_channel")));
        Q_EMIT nodeDisconnected(nullptr, fromPort, nullptr, toPort);
        return true;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
}

int DAPyWorkFlow::nodeCount()
{
    DAPyGILGuard gil;
    try {
        return static_cast< int >(pybind11::len(object()));
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    return false;
}

/**
 * @brief 通过代理指针连接两个节点
 */
DAPyWorkFlowConnection DAPyWorkFlow::connectNode(
    DAPyNodeProxy* src, const QString& srcChannel, DAPyNodeProxy* dst, const QString& dstChannel
)
{
    if (!src) {
        qWarning() << "DAPyWorkFlow::connectNode(DAPyNodeProxy*, ...): src is nullptr";
        return DAPyWorkFlowConnection();
    }
    if (!dst) {
        qWarning() << "DAPyWorkFlow::connectNode(..., DAPyNodeProxy*, ...): dst is nullptr";
        return DAPyWorkFlowConnection();
    }
    DAPyGILGuard gil;
    QString srcNodeId = src->getNodeId();
    QString dstNodeId = dst->getNodeId();
    if (srcNodeId.isEmpty() || dstNodeId.isEmpty()) {
        qWarning() << "DAPyWorkFlow::connectNode(DAPyNodeProxy*, ...): src/dst has empty nodeId";
        return DAPyWorkFlowConnection();
    }
    DAPyWorkFlowConnection connResult = connectNode(srcNodeId, srcChannel, dstNodeId, dstChannel);
    if (connResult.isValid()) {
        Q_EMIT nodeConnected(src, srcChannel, dst, dstChannel);
    }
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

bool DAPyWorkFlow::hasNode(DAPyNodeProxy* proxy)
{
    if (!proxy) {
        qWarning() << "DAPyWorkFlow::hasNode(DAPyNodeProxy*): proxy is nullptr";
        return false;
    }
    DAPyGILGuard gil;
    QString nodeId = proxy->getNodeId();
    if (nodeId.isEmpty()) {
        qWarning() << "DAPyWorkFlow::hasNode(DAPyNodeProxy*): proxy has empty nodeId";
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
        pybind11::object result = attr("get_node_by_id")(nodeId.toStdString());
        return result;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
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
            std::string nodeIdStr = pybind11::str(item).cast< std::string >();
            result.append(QString::fromStdString(nodeIdStr));
        }
        return result;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    return QStringList();
}

/**
 * @brief 异步执行工作流（无回调）
 */
bool DAPyWorkFlow::executeAsync()
{
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::executeAsync: workflow is not valid";
        return false;
    }
    Q_EMIT executionStarted();
    DAPyGILGuard gil;
    try {
        DAPyModuleWorkflow& pyModule = DAPyModuleWorkflow::getInstance();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlow::executeAsync: cannot import DAWorkbench.DAWorkFlowPy";
                return false;
            }
        }
        pybind11::object executorModule = pybind11::module_::import("DAWorkbench.DAWorkFlowPy.executor");
        pybind11::object executorClass  = executorModule.attr("DAWorkflowExecutor");
        pybind11::object executorObj    = executorClass(object());
        executorObj.attr("execute_async")();
        mPyExecutorObj = DAPyObjectWrapper(executorObj);
        Q_EMIT executionFinished(true);
        return true;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    Q_EMIT executionFinished(false);
    return false;
}

/**
 * @brief 异步执行工作流（带回调）
 */
bool DAPyWorkFlow::executeAsync(pybind11::object onNodeFinished, pybind11::object onStateChange, pybind11::object onProgress)
{
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::executeAsync: workflow is not valid";
        return false;
    }
    Q_EMIT executionStarted();
    DAPyGILGuard gil;
    try {
        DAPyModuleWorkflow& pyModule = DAPyModuleWorkflow::getInstance();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlow::executeAsync: cannot import DAWorkbench.DAWorkFlowPy";
                return false;
            }
        }
        pybind11::object executorModule = pybind11::module_::import("DAWorkbench.DAWorkFlowPy.executor");
        pybind11::object executorClass  = executorModule.attr("DAWorkflowExecutor");
        pybind11::object executorObj    = executorClass(object(), onNodeFinished, onStateChange, onProgress);
        executorObj.attr("execute_async")();
        mPyExecutorObj = DAPyObjectWrapper(executorObj);
        Q_EMIT executionFinished(true);
        return true;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    Q_EMIT executionFinished(false);
    return false;
}

/**
 * @brief 终止执行
 */
void DAPyWorkFlow::terminate()
{
    if (mPyExecutorObj.isNone()) {
        qWarning() << "DAPyWorkFlow::terminate: executor is not created";
        return;
    }
    DAPyGILGuard gil;
    try {
        mPyExecutorObj.object().attr("terminate")();
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
}

/**
 * @brief 暂停执行
 */
bool DAPyWorkFlow::pause()
{
    if (mPyExecutorObj.isNone()) {
        qWarning() << "DAPyWorkFlow::pause: executor is not created";
        return false;
    }
    DAPyGILGuard gil;
    try {
        mPyExecutorObj.object().attr("pause")();
        return true;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    return false;
}

/**
 * @brief 恢复执行
 */
bool DAPyWorkFlow::resume()
{
    if (mPyExecutorObj.isNone()) {
        qWarning() << "DAPyWorkFlow::resume: executor is not created";
        return false;
    }
    DAPyGILGuard gil;
    try {
        mPyExecutorObj.object().attr("resume")();
        return true;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    return false;
}

ExecState DAPyWorkFlow::getExecutorState()
{
    if (mPyExecutorObj.isNone()) {
        return StateIdle;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object stateObj = mPyExecutorObj.object().attr("state");
        std::string stateStr      = pybind11::cast< std::string >(stateObj.attr("value"));
        if (stateStr == "running") {
            return StateRunning;
        }
        if (stateStr == "paused") {
            return StatePaused;
        }
        if (stateStr == "error") {
            return StateError;
        }
        if (stateStr == "finished") {
            return StateFinished;
        }
        return StateIdle;
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    return StateIdle;
}

bool DAPyWorkFlow::getResult()
{
    if (mPyExecutorObj.isNone()) {
        return false;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object resultObj = mPyExecutorObj.object().attr("result");
        if (resultObj.is_none()) {
            return false;
        }
        return pybind11::cast< bool >(resultObj);
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
    return false;
}

bool DAPyWorkFlow::isRunning()
{
    return (getExecutorState() == StateRunning);
}

QString DAPyWorkFlow::getLastError() const
{
    return mLastErrorString;
}

}  // namespace DA