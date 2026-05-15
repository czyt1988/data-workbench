#include "DAPyWorkFlow.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyGILGuard.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyNodeProxy.h"
#include <QDebug>

namespace DA
{

//===================================================
// DAPyWorkFlow::PrivateData
//===================================================
class DAPyWorkFlow::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlow)
public:
    PrivateData(DAPyWorkFlow* p);
    // 统一异常处理 — pybind11 专用，提取 Python traceback
    void dealException(const pybind11::error_already_set& e);
    // 统一异常处理 — std::exception 通用
    void dealException(const std::exception& e);

public:
    DA::PY::safe_pyobject mPyWorkflowObj;      ///< Python DAWorkflow 实例的安全持有者
    DA::PY::safe_pyobject mPyExecutorObj;      ///< Python DAWorkflowExecutor 实例的安全持有者
    QString mLastErrorString;                   ///< 最后一次错误信息
};

//===================================================
// DAPyWorkFlow::PrivateData 实现
//===================================================

DAPyWorkFlow::PrivateData::PrivateData(DAPyWorkFlow* p) : q_ptr(p)
{
}

/**
 * @brief 统一异常处理 — pybind11 专用
 *
 * 提取 Python traceback 信息（e.what() 包含完整 traceback），
 * 转换为 QString 存储到 mLastErrorString，通过 qWarning 输出。
 * 在 GIL 作用域内调用此方法，确保 error_already_set 在 GIL 持有期间被消费，
 * 避免异常析构时尝试获取 GIL 导致死锁。
 *
 * @param[in] e 捕获的 pybind11 异常对象
 */
void DAPyWorkFlow::PrivateData::dealException(const pybind11::error_already_set& e)
{
    mLastErrorString = QString::fromUtf8(e.what());
    qWarning().noquote() << mLastErrorString;
}

/**
 * @brief 统一异常处理 — std::exception 通用
 *
 * 将异常信息存储到 mLastErrorString 中。
 *
 * @param[in] e 捕获的异常对象
 */
void DAPyWorkFlow::PrivateData::dealException(const std::exception& e)
{
    mLastErrorString = QString::fromUtf8(e.what());
    qWarning().noquote() << mLastErrorString;
}

//===================================================
// DAPyWorkFlow
//===================================================

DAPyWorkFlow::DAPyWorkFlow() : DA_PIMPL_CONSTRUCT
{
}

DAPyWorkFlow::~DAPyWorkFlow()
{
}

/**
 * @brief 初始化 Python DAWorkflow 实例
 *
 * 导入 DAWorkbench.DAWorkFlowPy 模块，获取 DAWorkflow 类引用，
 * 创建 Python DAWorkflow 实例并存储到 safe_pyobject。
 * 如果已初始化则跳过重复调用。
 *
 * @note 需在 Python 解释器初始化后调用此方法
 */
void DAPyWorkFlow::initPyWorkflow()
{
    DA_D(d);
    if (!d->mPyWorkflowObj.is_none()) {
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
        d->mPyWorkflowObj               = DA::PY::safe_pyobject(std::move(workflowInstance));
        qDebug() << "DAPyWorkFlow::initPyWorkflow: Python DAWorkflow instance created";
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
}

/**
 * @brief 设置外部 Python DAWorkflow 实例
 *
 * 接收外部传入的 Python workflow 对象，替代内部 initPyWorkflow() 创建的实例。
 * 用于 DAPyWorkFlowScene::setPyWorkflow() 透传场景级 workflow 设置。
 *
 * @param[in] obj Python DAWorkflow 实例的 pybind11::object
 */
void DAPyWorkFlow::setPyWorkflowObject(const pybind11::object& obj)
{
    DA_D(d);
    d->mPyWorkflowObj = DA::PY::safe_pyobject(pybind11::object(obj));
}

/**
 * @brief 获取内部 Python DAWorkflow 实例对象
 *
 * 返回内部持有的 Python workflow 对象引用，用于 DAPyWorkFlowScene::getPyWorkflow() 透传。
 *
 * @return Python DAWorkflow 实例的 pybind11::object，未初始化时返回空对象
 */
pybind11::object DAPyWorkFlow::getPyWorkflowObject() const
{
    DA_DC(d);
    return d->mPyWorkflowObj.object();
}

bool DAPyWorkFlow::isValid() const
{
    DA_DC(d);
    DAPyGILGuard gil;
    try {
        return !d->mPyWorkflowObj.is_none();
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

//===================================================
// Wave 2 方法 stub — 后续 Wave 2 实现
//===================================================

/**
 * @brief 添加节点到 Python DAWorkflow
 *
 * 通过代理获取 Python 节点引用，调用 `DAWorkflow.add_node()` 注册节点，
 * 返回 Python 分配的 node_id 字符串。
 *
 * @param[in] proxy 节点代理指针，必须持有有效的 Python 节点引用
 * @return Python 分配的 node_id；空字符串表示失败（proxy无效、workflow未初始化、Python异常）
 * @see removeNode DAPyNodeProxy::getPyNodeRef
 */
QString DAPyWorkFlow::addNode(DAPyNodeProxy* proxy)
{
    if (!proxy) {
        qWarning() << "DAPyWorkFlow::addNode: proxy is nullptr";
        return QString();
    }
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        if (!workflowObj) {
            qWarning() << "DAPyWorkFlow::addNode: workflow object is invalid";
            return QString();
        }
        pybind11::object pyNodeRef = proxy->getPyNodeRef();
        if (!pyNodeRef) {
            qWarning() << "DAPyWorkFlow::addNode: proxy has no valid Python node reference";
            return QString();
        }
        pybind11::object result = workflowObj.attr("add_node")(pyNodeRef);
        std::string nodeIdStr    = result.attr("node_id").cast<std::string>();
        return QString::fromStdString(nodeIdStr);
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
        return QString();
    } catch (const std::exception& e) {
        d->dealException(e);
        return QString();
    }
}

/**
 * @brief 移除节点从 Python DAWorkflow
 *
 * 通过 nodeId 查找 Python 节点引用，调用 `DAWorkflow.remove_node()` 移除节点。
 *
 * @param[in] nodeId 要移除的节点 ID
 * @return true 移除成功；false 表示节点不存在、workflow未初始化或Python异常
 * @see addNode getNodeById
 */
bool DAPyWorkFlow::removeNode(const QString& nodeId)
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        if (!workflowObj) {
            qWarning() << "DAPyWorkFlow::removeNode: workflow object is invalid";
            return false;
        }
        pybind11::object pyNodeRef = getNodeById(nodeId);
        if (pyNodeRef.is_none()) {
            qWarning() << "DAPyWorkFlow::removeNode: node not found for id:" << nodeId;
            return false;
        }
        workflowObj.attr("remove_node")(pyNodeRef);
        return true;
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
        return false;
    } catch (const std::exception& e) {
        d->dealException(e);
        return false;
    }
}

/**
 * @brief 连接两个节点的端口，返回连接描述符
 *
 * 调用 Python DAWorkflow.connect_node() 创建连接，
 * 从返回的 DAConnection 对象中提取各字段构建 DAPyWorkFlowConnection。
 * 如果 Python 调用失败或 workflow 未初始化，返回空连接（connectionId 为空）。
 *
 * @param[in] srcNodeId 源节点 ID
 * @param[in] srcChannel 源节点输出端口名称
 * @param[in] dstNodeId 目标节点 ID
 * @param[in] dstChannel 目标节点输入端口名称
 * @return DAPyWorkFlowConnection 连接描述符，失败时 isValid() 为 false
 */
DAPyWorkFlowConnection DAPyWorkFlow::connectNode(const QString& srcNodeId,
                                                   const QString& srcChannel,
                                                   const QString& dstNodeId,
                                                   const QString& dstChannel)
{
    DA_D(d);
    DAPyWorkFlowConnection result;
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::connectNode: workflow is not valid";
        return result;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        pybind11::object conn = workflowObj.attr("connect_node")(srcNodeId.toStdString(),
                                                                  srcChannel.toStdString(),
                                                                  dstNodeId.toStdString(),
                                                                  dstChannel.toStdString());
        result.connectionId  = QString::fromStdString(pybind11::str(conn.attr("connection_id")));
        result.sourceNodeId  = QString::fromStdString(pybind11::str(conn.attr("source_node_id")));
        result.sourceChannel = QString::fromStdString(pybind11::str(conn.attr("source_output_channel")));
        result.targetNodeId  = QString::fromStdString(pybind11::str(conn.attr("target_node_id")));
        result.targetChannel = QString::fromStdString(pybind11::str(conn.attr("target_input_channel")));
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return result;
}

/**
 * @brief 断开指定连接
 *
 * 调用 Python DAWorkflow.remove_connection() 按 connectionId 移除连接。
 * 如果 Python 调用成功返回 True，否则返回 False。
 *
 * @param[in] connectionId 要断开的连接 ID
 * @return true 成功断开，false 失败或 workflow 未初始化
 */
bool DAPyWorkFlow::disconnectNode(const QString& connectionId)
{
    DA_D(d);
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::disconnectNode: workflow is not valid";
        return false;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        bool res = workflowObj.attr("remove_connection")(connectionId.toStdString()).cast<bool>();
        return res;
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

/**
 * @brief 移除连接（disconnectNode 的别名）
 *
 * @param[in] connectionId 要移除的连接 ID
 * @return true 成功移除，false 失败
 * @see disconnectNode
 */
bool DAPyWorkFlow::removeConnection(const QString& connectionId)
{
    return disconnectNode(connectionId);
}

void DAPyWorkFlow::clear()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        d->mPyWorkflowObj.object().attr("clear")();
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
}

int DAPyWorkFlow::nodeCount()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        return static_cast< int >(pybind11::len(d->mPyWorkflowObj.object()));
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return 0;
}

bool DAPyWorkFlow::hasNode(const QString& nodeId)
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object result = d->mPyWorkflowObj.object().attr("__contains__")(nodeId);
        return result.cast< bool >();
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

/**
 * @brief 通过 node_id 获取 Python 节点对象
 *
 * 调用 Python DAWorkflow.get_node_by_id() 获取节点实例。
 * 如果节点不存在或 workflow 未初始化，返回 pybind11::none()。
 *
 * @param[in] nodeId 节点唯一 ID
 * @return Python 节点对象，不存在时返回 py::none()
 * @see addNode hasNode getNodes
 */
pybind11::object DAPyWorkFlow::getNodeById(const QString& nodeId)
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        if (!workflowObj) {
            qWarning() << "DAPyWorkFlow::getNodeById: workflow object is invalid";
            return pybind11::none();
        }
        pybind11::object result = workflowObj.attr("get_node_by_id")(nodeId.toStdString());
        return result;
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return pybind11::none();
}

/**
 * @brief 获取所有节点列表
 *
 * 调用 Python DAWorkflow.get_nodes() 返回节点实例列表。
 * 如果 workflow 未初始化或 Python 异常，返回空列表。
 *
 * @return pybind11::list 节点实例列表
 * @see getNodeById nodeCount
 */
pybind11::list DAPyWorkFlow::getNodes()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        if (!workflowObj) {
            qWarning() << "DAPyWorkFlow::getNodes: workflow object is invalid";
            return pybind11::list();
        }
        pybind11::object result = workflowObj.attr("get_nodes")();
        return pybind11::list(result);
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return pybind11::list();
}

/**
 * @brief 获取所有连接列表
 *
 * 调用 Python DAWorkflow.get_connections() 返回 DAConnection 实例列表。
 * 如果 workflow 未初始化或 Python 异常，返回空列表。
 *
 * @return pybind11::list DAConnection 实例列表
 * @see connectNode disconnectNode
 */
pybind11::list DAPyWorkFlow::getConnections()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        if (!workflowObj) {
            qWarning() << "DAPyWorkFlow::getConnections: workflow object is invalid";
            return pybind11::list();
        }
        pybind11::object result = workflowObj.attr("get_connections")();
        return pybind11::list(result);
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return pybind11::list();
}

/**
 * @brief 验证工作流是否为有效 DAG（无环有向图）
 *
 * 调用 Python DAWorkflow.is_valid_dag() 方法，使用 Kahn 算法检测是否存在环。
 * 如果 Python 调用失败或 workflow 未初始化，返回 false。
 *
 * @return true 工作流是有效的 DAG（无环），false 存在环或调用失败
 * @see topologicalSort
 */
bool DAPyWorkFlow::isValidDag()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        if (!workflowObj) {
            qWarning() << "DAPyWorkFlow::isValidDag: workflow object is invalid";
            return false;
        }
        return workflowObj.attr("is_valid_dag")().cast<bool>();
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

/**
 * @brief 获取工作流节点的拓扑排序结果
 *
 * 调用 Python DAWorkflow.topological_sort() 方法，返回从源节点到终端节点的有序列表。
 * Python 方法在存在环时抛出 ValueError，此方法捕获该异常并返回空 QStringList。
 *
 * @return 拓扑排序后的 node_id 列表；空列表表示存在环或调用失败
 * @see isValidDag
 */
QStringList DAPyWorkFlow::topologicalSort()
{
    DA_D(d);
    DAPyGILGuard gil;
    try {
        pybind11::object workflowObj = d->mPyWorkflowObj.object();
        if (!workflowObj) {
            qWarning() << "DAPyWorkFlow::topologicalSort: workflow object is invalid";
            return QStringList();
        }
        pybind11::list pyResult = workflowObj.attr("topological_sort")();
        QStringList result;
        for (auto item : pyResult) {
            std::string nodeIdStr = pybind11::str(item).cast<std::string>();
            result.append(QString::fromStdString(nodeIdStr));
        }
        return result;
    } catch (const pybind11::error_already_set& e) {
        // Python ValueError (cycle) or other error — return empty list
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return QStringList();
}

/**
 * @brief 异步执行工作流（无回调版本）
 *
 * 导入 DAWorkflowExecutor 类，创建执行器实例并调用 execute_async() 启动异步执行。
 * 执行器实例存储在 mPyExecutorObj 中，供后续 terminate/pause/resume 调用使用。
 * 不设置 Python 回调（由 DAPyWorkFlowLifecycle 通过带回调版本注册）。
 *
 * @return true 成功启动异步执行；false workflow 未初始化、模块导入失败或 Python 异常
 * @see terminate pause resume DAPyWorkFlowLifecycle
 */
bool DAPyWorkFlow::executeAsync()
{
    DA_D(d);
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::executeAsync: workflow is not valid";
        return false;
    }
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
        pybind11::object workflowObj    = d->mPyWorkflowObj.object();
        pybind11::object executorObj    = executorClass(workflowObj);
        executorObj.attr("execute_async")();
        d->mPyExecutorObj = DA::PY::safe_pyobject(std::move(executorObj));
        return true;
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

/**
 * @brief 异步执行工作流（带回调版本）
 *
 * 导入 DAWorkflowExecutor 类，创建执行器实例时传入三个回调函数，
 * 调用 execute_async() 启动异步执行。
 * 执行器实例存储在 mPyExecutorObj 中，供后续 terminate/pause/resume 调用使用。
 *
 * @param[in] onNodeFinished 节点完成回调（pybind11::cpp_function）
 * @param[in] onStateChange 状态变更回调（pybind11::cpp_function）
 * @param[in] onProgress 进度回调（pybind11::cpp_function）
 * @return true 成功启动异步执行；false workflow 未初始化、模块导入失败或 Python 异常
 * @see terminate pause resume DAPyWorkFlowLifecycle
 */
bool DAPyWorkFlow::executeAsync(pybind11::object onNodeFinished,
                                  pybind11::object onStateChange,
                                  pybind11::object onProgress)
{
    DA_D(d);
    if (!isValid()) {
        qWarning() << "DAPyWorkFlow::executeAsync: workflow is not valid";
        return false;
    }
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
        pybind11::object workflowObj    = d->mPyWorkflowObj.object();
        pybind11::object executorObj    = executorClass(workflowObj, onNodeFinished, onStateChange, onProgress);
        executorObj.attr("execute_async")();
        d->mPyExecutorObj = DA::PY::safe_pyobject(std::move(executorObj));
        return true;
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

/**
 * @brief 终止正在执行的工作流
 *
 * 通过 mPyExecutorObj 调用 Python DAWorkflowExecutor.terminate() 方法终止执行。
 * 如果执行器未创建（mPyExecutorObj 为空），则跳过调用。
 *
 * @see executeAsync pause resume
 */
void DAPyWorkFlow::terminate()
{
    DA_D(d);
    if (d->mPyExecutorObj.is_none()) {
        qWarning() << "DAPyWorkFlow::terminate: executor is not created";
        return;
    }
    DAPyGILGuard gil;
    try {
        d->mPyExecutorObj.object().attr("terminate")();
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
}

/**
 * @brief 暂停正在执行的工作流
 *
 * 通过 mPyExecutorObj 谬用 Python DAWorkflowExecutor.pause() 方法暂停执行。
 * 如果执行器未创建（mPyExecutorObj 为空），返回 false。
 *
 * @return true 成功暂停；false 执行器未创建或 Python 异常
 * @see resume executeAsync
 */
bool DAPyWorkFlow::pause()
{
    DA_D(d);
    if (d->mPyExecutorObj.is_none()) {
        qWarning() << "DAPyWorkFlow::pause: executor is not created";
        return false;
    }
    DAPyGILGuard gil;
    try {
        d->mPyExecutorObj.object().attr("pause")();
        return true;
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

/**
 * @brief 恢复暂停的工作流
 *
 * 通过 mPyExecutorObj 谬用 Python DAWorkflowExecutor.resume() 方法恢复执行。
 * 如果执行器未创建（mPyExecutorObj 为空），返回 false。
 *
 * @return true 成功恢复；false 执行器未创建或 Python 异常
 * @see pause executeAsync
 */
bool DAPyWorkFlow::resume()
{
    DA_D(d);
    if (d->mPyExecutorObj.is_none()) {
        qWarning() << "DAPyWorkFlow::resume: executor is not created";
        return false;
    }
    DAPyGILGuard gil;
    try {
        d->mPyExecutorObj.object().attr("resume")();
        return true;
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

ExecState DAPyWorkFlow::getExecutorState()
{
    DA_D(d);
    if (d->mPyExecutorObj.is_none()) {
        return StateIdle;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object stateObj = d->mPyExecutorObj.object().attr("state");
        // state is a Python DAExecutorState enum; .value returns the string value
        std::string stateStr = pybind11::cast< std::string >(stateObj.attr("value"));
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
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return StateIdle;
}

bool DAPyWorkFlow::getResult()
{
    DA_D(d);
    if (d->mPyExecutorObj.is_none()) {
        return false;
    }
    DAPyGILGuard gil;
    try {
        pybind11::object resultObj = d->mPyExecutorObj.object().attr("result");
        // result may be None (not executed yet) or bool
        if (resultObj.is_none()) {
            return false;
        }
        return pybind11::cast< bool >(resultObj);
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
    return false;
}

bool DAPyWorkFlow::isRunning()
{
    return (getExecutorState() == StateRunning);
}

/**
 * @brief 获取最后一次错误信息
 *
 * 返回最近一次 dealException() 存储的错误字符串。
 * 如果没有发生过错误，返回空 QString。
 *
 * @return 最后一次错误信息
 */
QString DAPyWorkFlow::getLastError() const
{
    DA_DC(d);
    return d->mLastErrorString;
}

}  // namespace DA