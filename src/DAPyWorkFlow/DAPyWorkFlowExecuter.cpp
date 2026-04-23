#include "DAPyWorkFlowExecuter.h"
#include "DAPyGILGuard.h"
#include "DAPyNodeProxy.h"
#include "DAPyModuleWorkflow.h"
#include "DAPythonSignalHandler.h"
#include <QDebug>
#include <QMutexLocker>
#include <QPointer>
#include <QThread>

namespace DA
{

DA_AUTO_REGISTER_META_TYPE(std::shared_ptr< DA::DAPyNodeProxy >)
DA_AUTO_REGISTER_META_TYPE(DA::DAPyWorkFlowExecuter::ExecState)

class DAPyWorkFlowExecuter::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowExecuter)
public:
    PrivateData(DAPyWorkFlowExecuter* p);
    // 统一异常处理
    void dealException(const std::exception& e);
    // 清空节点分类和入度计数
    void clear();
    // 从Python工作流对象中提取节点信息，创建DAPyNodeProxy
    bool buildNodeProxies();
    // 分类节点：全局节点、孤立节点、开始节点
    void classifyNodes();
    // 获取Python工作流中指定节点的下游连接信息
    pybind11::list getDownstreamConnections(const QString& nodeId);

public:
    DAPySafePyObjectHolder mWorkflowObj;       ///< Python DAWorkflow实例的安全持有者
    DAPySafePyObjectHolder mSignalManagerObj;  ///< Python DASignalManager实例的安全持有者
    QPointer< DAPythonSignalHandler > mSignalHandler;  ///< C++侧信号处理器
    ExecState mExecState { StateIdle };        ///< 当前执行状态
    bool mIsTerminateRequest { false };        ///< 终止请求标记
    bool mIsPauseRequest { false };            ///< 暂停请求标记
    QMutex mMutex;                             ///< 互斥锁保护状态变更
    QWaitCondition mPauseCondition;            ///< 暂停等待条件变量
    QString mLastErrorString;                  ///< 最后错误信息

    // 节点分类（参考DAWorkFlowExecuter::PrivateData模式）
    QMap< std::shared_ptr< DAPyNodeProxy >, int > mNodeIndegreeSetCount;  ///< 入度计数映射
    QList< std::shared_ptr< DAPyNodeProxy > > mGlobalNodes;     ///< 全局节点列表
    QList< std::shared_ptr< DAPyNodeProxy > > mIsolatedNodes;   ///< 孤立节点列表
    QList< std::shared_ptr< DAPyNodeProxy > > mBeginNodes;      ///< 开始节点列表
    QMap< QString, std::shared_ptr< DAPyNodeProxy > > mNodeProxyMap;  ///< nodeId到DAPyNodeProxy的映射
    int mExecutedNodeCount { 0 };                              ///< 已执行节点计数
    int mTotalNodeCount { 0 };                                 ///< 总节点数
};

//===================================================
// DAPyWorkFlowExecuter::PrivateData
//===================================================

DAPyWorkFlowExecuter::PrivateData::PrivateData(DAPyWorkFlowExecuter* p) : q_ptr(p)
{
}

/**
 * @brief 统一异常处理
 *
 * 将异常信息存储到mLastErrorString中，
 * 对于pybind11::error_already_set异常，在GIL作用域内消费，
 * 避免异常析构时尝试获取GIL导致死锁。
 *
 * @param[in] e 捕获的异常对象
 */
void DAPyWorkFlowExecuter::PrivateData::dealException(const std::exception& e)
{
    mLastErrorString = e.what();
    qCritical() << "DAPyWorkFlowExecuter error:" << mLastErrorString;
}

/**
 * @brief 清空节点分类和入度计数
 */
void DAPyWorkFlowExecuter::PrivateData::clear()
{
    mNodeIndegreeSetCount.clear();
    mGlobalNodes.clear();
    mIsolatedNodes.clear();
    mBeginNodes.clear();
    mNodeProxyMap.clear();
    mExecutedNodeCount = 0;
    mTotalNodeCount = 0;
}

/**
 * @brief 从Python工作流对象提取节点信息并创建DAPyNodeProxy
 *
 * 通过DAPyModuleWorkflow获取Python DAWorkflow实例的所有节点，
 * 为每个节点创建DAPyNodeProxy代理并映射node_id。
 *
 * @return true表示成功构建，false表示失败
 */
bool DAPyWorkFlowExecuter::PrivateData::buildNodeProxies()
{
    if (!mWorkflowObj) {
        mLastErrorString = "Workflow object is not set";
        qCritical() << mLastErrorString;
        return false;
    }

    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        mLastErrorString = "Failed to acquire GIL for building node proxies";
        qCritical() << mLastErrorString;
        return false;
    }

    try {
        pybind11::object workflow = mWorkflowObj.object();

        // 获取Python工作流的节点列表
        pybind11::object nodesObj = workflow.attr("get_nodes")();
        pybind11::list nodeList;

        if (pybind11::isinstance< pybind11::list >(nodesObj)) {
            nodeList = pybind11::cast< pybind11::list >(nodesObj);
        } else if (pybind11::isinstance< pybind11::iterable >(nodesObj)) {
            nodeList = pybind11::list(nodesObj);
        } else {
            mLastErrorString = "Workflow get_nodes() did not return a list";
            qCritical() << mLastErrorString;
            return false;
        }

        // 为每个Python节点创建DAPyNodeProxy
        for (auto item : nodeList) {
            pybind11::object pyNode = pybind11::cast< pybind11::object >(item);

            auto proxy = std::make_shared< DAPyNodeProxy >();
            proxy->setPyNodeRef(pyNode);

            // 从Python节点获取node_id作为映射键
            if (pybind11::hasattr(pyNode, "node_id")) {
                QString nodeId = pybind11::cast< QString >(pyNode.attr("node_id"));
                mNodeProxyMap[nodeId] = proxy;
            } else {
                // 如果没有node_id，用qualified_name作为备选键
                QString qname = proxy->getQualifiedName();
                mNodeProxyMap[qname] = proxy;
            }
        }

        mTotalNodeCount = static_cast< int >(mNodeProxyMap.size());
        return true;

    } catch (const pybind11::error_already_set& e) {
        dealException(e);
        return false;
    } catch (const std::exception& e) {
        dealException(e);
        return false;
    }
}

/**
 * @brief 分类节点：全局节点、孤立节点、开始节点
 *
 * 参考DAWorkFlowExecuter::PrivateData::prepareStartExec的实现模式：
 * - 所没有入度的节点都是顶层节点
 * - 入度和出度都没有的节点属于孤立节点
 * - 通过Python DASignalManager获取连接信息来判断入度/出度
 */
void DAPyWorkFlowExecuter::PrivateData::classifyNodes()
{
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qCritical() << "Failed to acquire GIL for node classification";
        return;
    }

    try {
        pybind11::object workflow = mWorkflowObj.object();

        // 获取连接列表，用于计算入度/出度
        pybind11::object connectionsObj = workflow.attr("get_connections")();
        pybind11::list connList;

        if (pybind11::isinstance< pybind11::list >(connectionsObj)) {
            connList = pybind11::cast< pybind11::list >(connectionsObj);
        } else if (pybind11::isinstance< pybind11::iterable >(connectionsObj)) {
            connList = pybind11::list(connectionsObj);
        }

        // 计算每个节点的入度和出度
        QMap< QString, int > inDegreeMap;
        QMap< QString, int > outDegreeMap;

        for (auto item : connList) {
            pybind11::object conn = pybind11::cast< pybind11::object >(item);
            QString sourceNodeId    = pybind11::cast< QString >(conn.attr("source_node_id"));
            QString targetNodeId    = pybind11::cast< QString >(conn.attr("target_node_id"));

            outDegreeMap[sourceNodeId]++;
            inDegreeMap[targetNodeId]++;
        }

        // 分类节点
        for (auto it = mNodeProxyMap.begin(); it != mNodeProxyMap.end(); ++it) {
            QString nodeId       = it.key();
            auto proxy           = it.value();
            int inDegree         = inDegreeMap.value(nodeId, 0);
            int outDegree        = outDegreeMap.value(nodeId, 0);

            // 检查是否是全局节点（Python节点如果有is_global属性）
            bool isGlobal = false;
            pybind11::object pyNode = proxy->getPyNodeRef();
            if (pybind11::hasattr(pyNode, "is_global")) {
                pybind11::object isGlobalObj = pyNode.attr("is_global");
                if (pybind11::isinstance< pybind11::bool_ >(isGlobalObj)) {
                    isGlobal = pybind11::cast< bool >(isGlobalObj);
                }
            }

            if (isGlobal) {
                mGlobalNodes.append(proxy);
            }

            if (0 == inDegree) {
                if (0 == outDegree) {
                    // 孤立节点（如果不是全局节点）
                    if (!isGlobal) {
                        mIsolatedNodes.append(proxy);
                    }
                } else {
                    // 开始节点（入度为0但有出度）
                    mBeginNodes.append(proxy);
                }
            }
        }

    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 获取Python工作流中指定节点的下游连接信息
 *
 * @param[in] nodeId 节点ID
 * @return 下游连接的pybind11::list
 */
pybind11::list DAPyWorkFlowExecuter::PrivateData::getDownstreamConnections(const QString& nodeId)
{
    pybind11::object workflow = mWorkflowObj.object();
    pybind11::object result   = workflow.attr("get_downstream_connections")(pybind11::cast(nodeId));

    if (pybind11::isinstance< pybind11::list >(result)) {
        return pybind11::cast< pybind11::list >(result);
    }
    return pybind11::list();
}

//===================================================
// DAPyWorkFlowExecuter
//===================================================

DAPyWorkFlowExecuter::DAPyWorkFlowExecuter(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
}

DAPyWorkFlowExecuter::~DAPyWorkFlowExecuter()
{
}

/**
 * @brief 设置Python工作流对象
 *
 * 关联一个Python DAWorkflow实例到此C++执行器。
 * 设置后会在GIL保护下提取工作流中的节点信息并创建DAPyNodeProxy代理。
 *
 * @param[in] workflowObj Python DAWorkflow实例对象
 */
void DAPyWorkFlowExecuter::setWorkflow(const pybind11::object& workflowObj)
{
    DA_D(d);
    d->mWorkflowObj = DAPySafePyObjectHolder(workflowObj);

    // 同时获取或创建对应的DASignalManager
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qWarning() << "DAPyWorkFlowExecuter::setWorkflow: Failed to acquire GIL";
        return;
    }

    try {
        // 从DAPyModuleWorkflow获取DASignalManager类
        pybind11::object signalManagerClass = DAPyModuleWorkflow::getInstance().attr("DASignalManager");
        pybind11::object signalManager      = signalManagerClass(workflowObj);
        d->mSignalManagerObj = DAPySafePyObjectHolder(signalManager);
    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
}

/**
 * @brief 获取Python工作流对象
 *
 * @return Python DAWorkflow实例的pybind11::object引用
 */
pybind11::object DAPyWorkFlowExecuter::getWorkflow() const
{
    DA_DC(d);
    return d->mWorkflowObj.object();
}

/**
 * @brief 设置DAPythonSignalHandler
 *
 * @param[in] handler C++侧信号处理器，用于Python线程到Qt主线程的回调
 */
void DAPyWorkFlowExecuter::setSignalHandler(DAPythonSignalHandler* handler)
{
    DA_D(d);
    d->mSignalHandler = handler;
}

/**
 * @brief 获取当前执行状态
 *
 * @return 当前执行状态枚举值
 */
DAPyWorkFlowExecuter::ExecState DAPyWorkFlowExecuter::getExecState() const
{
    DA_DC(d);
    return d->mExecState;
}

/**
 * @brief 判断是否请求终止
 *
 * @return true表示已请求终止，false表示未请求
 */
bool DAPyWorkFlowExecuter::isTerminateRequest() const
{
    DA_DC(d);
    return d->mIsTerminateRequest;
}

/**
 * @brief 获取全局节点列表
 *
 * @return 全局节点DAPyNodeProxy指针列表
 */
QList< std::shared_ptr< DAPyNodeProxy > > DAPyWorkFlowExecuter::getGlobalNodes() const
{
    DA_DC(d);
    return d->mGlobalNodes;
}

/**
 * @brief 获取孤立节点列表
 *
 * @return 孤立节点DAPyNodeProxy指针列表
 */
QList< std::shared_ptr< DAPyNodeProxy > > DAPyWorkFlowExecuter::getIsolatedNodes() const
{
    DA_DC(d);
    return d->mIsolatedNodes;
}

/**
 * @brief 获取最后错误信息
 *
 * @return 最后一次错误的描述字符串
 */
QString DAPyWorkFlowExecuter::getLastErrorString() const
{
    DA_DC(d);
    return d->mLastErrorString;
}

/**
 * @brief 开始执行工作流
 *
 * 执行流程：
 * 1. prepareStartExec() — 构建节点代理并分类节点
 * 2. 执行全局节点（执行但不传递数据）
 * 3. 执行孤立节点（执行并传递数据）
 * 4. 执行开始节点（执行并传递数据到下游）
 * 5. 下游节点通过入度计数机制触发执行
 *
 * 每个节点执行前检查终止请求和暂停请求，
 * 终止请求会立即停止执行并发射finished(false)信号。
 * 暂停请求会等待resume()唤醒后继续执行。
 *
 * @note 此方法应在工作线程中调用（通过QThread::started信号触发）
 */
void DAPyWorkFlowExecuter::startExecute()
{
    DA_D(d);

    // 重置终止和暂停标记
    {
        QMutexLocker locker(&d->mMutex);
        d->mIsTerminateRequest = false;
        d->mIsPauseRequest     = false;
    }

    setExecState(StateRunning);

    // 准备执行——构建节点代理和分类
    if (!prepareStartExec()) {
        setExecState(StateError);
        emit finished(false);
        return;
    }

    // 启动Python侧的DASignalManager
    {
        DAPyGILGuard gilGuard;
        if (gilGuard.isAcquired() && d->mSignalManagerObj) {
            try {
                d->mSignalManagerObj.object().attr("start")();
            } catch (const std::exception& e) {
                d->dealException(e);
            }
        }
    }

    // 执行全局节点——执行但不传递数据
    for (const auto& proxy : std::as_const(d->mGlobalNodes)) {
        {
            QMutexLocker locker(&d->mMutex);
            if (d->mIsTerminateRequest) {
                setExecState(StateFinished);
                emit finished(false);
                return;
            }
            // 检查暂停请求
            while (d->mIsPauseRequest) {
                d->mPauseCondition.wait(&d->mMutex);
            }
        }
        executeNodeNotTransmit(proxy);
    }

    // 执行孤立节点——执行并传递数据
    for (const auto& proxy : std::as_const(d->mIsolatedNodes)) {
        {
            QMutexLocker locker(&d->mMutex);
            if (d->mIsTerminateRequest) {
                setExecState(StateFinished);
                emit finished(false);
                return;
            }
            while (d->mIsPauseRequest) {
                d->mPauseCondition.wait(&d->mMutex);
            }
        }
        executeNode(proxy);
    }

    // 执行开始节点——执行并传递数据到下游
    for (const auto& proxy : std::as_const(d->mBeginNodes)) {
        {
            QMutexLocker locker(&d->mMutex);
            if (d->mIsTerminateRequest) {
                setExecState(StateFinished);
                emit finished(false);
                return;
            }
            while (d->mIsPauseRequest) {
                d->mPauseCondition.wait(&d->mMutex);
            }
        }

        // 如果开始节点也是全局节点，全局节点已经执行了，只需传递
        if (d->mGlobalNodes.contains(proxy)) {
            transmitDownstream(proxy);
        } else {
            executeNode(proxy);
        }
    }

    // 停止Python侧的DASignalManager
    {
        DAPyGILGuard gilGuard;
        if (gilGuard.isAcquired() && d->mSignalManagerObj) {
            try {
                d->mSignalManagerObj.object().attr("stop")();
            } catch (const std::exception& e) {
                d->dealException(e);
            }
        }
    }

    setExecState(StateFinished);
    emit finished(true);
}

/**
 * @brief 请求终止执行
 *
 * 设置终止标记，当前正在执行的节点完成后将停止后续节点执行。
 * 不会中断正在执行中的节点，而是等待其完成后再停止。
 */
void DAPyWorkFlowExecuter::terminateRequest()
{
    DA_D(d);
    QMutexLocker locker(&d->mMutex);
    d->mIsTerminateRequest = true;
    // 如果处于暂停状态，唤醒以让终止生效
    d->mIsPauseRequest = false;
    d->mPauseCondition.wakeAll();
}

/**
 * @brief 暂停执行
 *
 * 设置暂停标记，在当前节点完成后暂停后续节点执行。
 * 暂停后工作流处于StatePaused状态，
 * 需调用resume()恢复执行。
 */
void DAPyWorkFlowExecuter::pause()
{
    DA_D(d);
    {
        QMutexLocker locker(&d->mMutex);
        d->mIsPauseRequest = true;
    }
    setExecState(StatePaused);
}

/**
 * @brief 恢复执行
 *
 * 清除暂停标记并唤醒等待的线程，
 * 工作流恢复到StateRunning状态继续执行后续节点。
 */
void DAPyWorkFlowExecuter::resume()
{
    DA_D(d);
    {
        QMutexLocker locker(&d->mMutex);
        d->mIsPauseRequest = false;
        d->mPauseCondition.wakeAll();
    }
    setExecState(StateRunning);
}

/**
 * @brief 执行单个节点并传递数据到下游
 *
 * 核心执行流程：
 * 1. DAPyGILGuard获取GIL
 * 2. 调用DAPyNodeProxy::exec()执行Python节点
 * 3. DAPyGILRelease释放GIL
 * 4. 释放GIL后发射nodeExecuteFinished信号（避免死锁）
 * 5. 传播节点输出数据到下游（propagateOutput）
 * 6. 检查下游节点入度是否满足（transmitDownstream）
 *
 * @param[in] proxy 要执行的节点代理
 * @note 信号在GIL释放后发射，确保不阻塞Python线程
 */
void DAPyWorkFlowExecuter::executeNode(std::shared_ptr< DAPyNodeProxy > proxy)
{
    DA_D(d);
    bool execSuccess = false;

    {
        // 获取GIL执行Python节点
        DAPyGILGuard gilGuard;
        if (!gilGuard.isAcquired()) {
            d->mLastErrorString = "Failed to acquire GIL for node execution";
            proxy->setNodeState(DAPyNodeState::Error);
            // 释放GIL后发射信号
            {
                DAPyGILRelease gilRelease;
                emit nodeExecuteFinished(proxy, false);
            }
            return;
        }

        // 执行节点
        execSuccess = proxy->exec();

        // 传播输出数据到下游（在GIL保护下调用Python DASignalManager）
        if (execSuccess) {
            propagateOutput(proxy);
        }

        // 释放GIL后发射Qt信号——关键：避免在持有GIL时发射Qt信号
        {
            DAPyGILRelease gilRelease;
            emit nodeExecuteFinished(proxy, execSuccess);
        }
    }

    // 更新进度
    d->mExecutedNodeCount++;
    emit progressChanged(d->mExecutedNodeCount, d->mTotalNodeCount);

    // 如果执行失败，跳过下游节点
    if (!execSuccess) {
        qWarning() << "Node execution failed, skipping downstream nodes:"
                   << proxy->getNodeName();
        return;
    }

    // 检查并触发下游节点执行
    transmitDownstream(proxy);
}

/**
 * @brief 执行节点但不传递数据到下游
 *
 * 用于执行全局节点。全局节点只执行，其输出数据通过DASignalManager传播，
 * 但不会立即触发下游节点的执行（下游节点在开始节点执行流程中触发）。
 *
 * @param[in] proxy 要执行的节点代理
 */
void DAPyWorkFlowExecuter::executeNodeNotTransmit(std::shared_ptr< DAPyNodeProxy > proxy)
{
    DA_D(d);
    bool execSuccess = false;

    {
        DAPyGILGuard gilGuard;
        if (!gilGuard.isAcquired()) {
            d->mLastErrorString = "Failed to acquire GIL for node execution";
            proxy->setNodeState(DAPyNodeState::Error);
            {
                DAPyGILRelease gilRelease;
                emit nodeExecuteFinished(proxy, false);
            }
            return;
        }

        execSuccess = proxy->exec();

        // 全局节点：传播输出但不触发下游执行
        if (execSuccess) {
            propagateOutput(proxy);
        }

        // 释放GIL后发射Qt信号
        {
            DAPyGILRelease gilRelease;
            emit nodeExecuteFinished(proxy, execSuccess);
        }
    }

    d->mExecutedNodeCount++;
    emit progressChanged(d->mExecutedNodeCount, d->mTotalNodeCount);
}

/**
 * @brief 准备执行——构建节点代理并分类节点
 *
 * 执行前准备工作：
 * 1. 清空之前的分类数据
 * 2. 从Python工作流构建DAPyNodeProxy代理
 * 3. 分类节点为全局、孤立、开始节点
 *
 * @return true表示准备成功，false表示失败
 */
bool DAPyWorkFlowExecuter::prepareStartExec()
{
    DA_D(d);
    d->clear();

    // 构建节点代理
    if (!d->buildNodeProxies()) {
        return false;
    }

    // 分类节点
    d->classifyNodes();

    return true;
}

/**
 * @brief 通过DASignalManager传播节点输出数据到下游
 *
 * 在GIL保护下调用Python DASignalManager的send_output方法，
 * 将节点输出数据传递到信号队列。
 * 后续通过process_pending将数据实际传递到下游节点输入端口。
 *
 * @param[in] proxy 执行完成的节点代理
 */
void DAPyWorkFlowExecuter::propagateOutput(std::shared_ptr< DAPyNodeProxy > proxy)
{
    DA_D(d);
    if (!d->mSignalManagerObj) {
        qWarning() << "DAPyWorkFlowExecuter::propagateOutput: No signal manager";
        return;
    }

    try {
        pybind11::object signalManager = d->mSignalManagerObj.object();
        pybind11::object pyNode        = proxy->getPyNodeRef();
        QString nodeId                 = proxy->getQualifiedName();

        // 从Python节点获取node_id
        if (pybind11::hasattr(pyNode, "node_id")) {
            nodeId = pybind11::cast< QString >(pyNode.attr("node_id"));
        }

        // 获取节点的输出端口key列表
        QList< QString > outputKeys = proxy->getOutputKeys();

        for (const QString& outputKey : outputKeys) {
            // 获取输出数据
            pybind11::object outputData = proxy->getPyOutputData(outputKey);
            if (!outputData.is_none()) {
                // 通过DASignalManager发送输出
                signalManager.attr("send_output")(
                    pybind11::cast(nodeId),
                    pybind11::cast(outputKey),
                    outputData
                );
            }
        }

        // 处理待传递信号
        signalManager.attr("process_pending")();

    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
}

/**
 * @brief 检查下游节点是否满足执行条件并触发执行
 *
 * 参考DAWorkFlowExecuter::PrivateData::transmit的实现模式：
 * 查找当前节点的所有下游连接，检查下游节点的入度计数是否等于
 * 其总入度数（上游连接数）。如果满足条件，则触发该下游节点的执行。
 *
 * 入度计数通过DASignalManager.is_node_ready()方法判断，
 * 或通过mNodeIndegreeSetCount映射进行C++侧追踪。
 *
 * @param[in] proxy 刚执行完成的节点代理
 */
void DAPyWorkFlowExecuter::transmitDownstream(std::shared_ptr< DAPyNodeProxy > proxy)
{
    DA_D(d);

    {
        QMutexLocker locker(&d->mMutex);
        if (d->mIsTerminateRequest) {
            return;
        }
    }

    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qWarning() << "DAPyWorkFlowExecuter::transmitDownstream: Failed to acquire GIL";
        return;
    }

    try {
        // 获取当前节点的node_id
        pybind11::object pyNode = proxy->getPyNodeRef();
        QString nodeId;

        if (pybind11::hasattr(pyNode, "node_id")) {
            nodeId = pybind11::cast< QString >(pyNode.attr("node_id"));
        } else {
            nodeId = proxy->getQualifiedName();
        }

        // 获取Python工作流对象
        pybind11::object workflow = d->mWorkflowObj.object();

        // 获取所有下游连接
        pybind11::list downstreamConns = d->getDownstreamConnections(nodeId);

        // 递增下游节点的入度计数
        for (auto item : downstreamConns) {
            pybind11::object conn = pybind11::cast< pybind11::object >(item);
            QString targetNodeId  = pybind11::cast< QString >(conn.attr("target_node_id"));

            auto it = d->mNodeProxyMap.find(targetNodeId);
            if (it == d->mNodeProxyMap.end()) {
                continue;
            }

            auto downstreamProxy = it.value();

            // 递增入度计数
            auto indegIte = d->mNodeIndegreeSetCount.find(downstreamProxy);
            if (indegIte == d->mNodeIndegreeSetCount.end()) {
                indegIte = d->mNodeIndegreeSetCount.insert(downstreamProxy, 1);
            } else {
                ++(indegIte.value());
            }

            // 检查是否满足执行条件——入度计数等于总入度数
            // 使用DASignalManager的is_node_ready()方法判断
            if (d->mSignalManagerObj) {
                pybind11::object signalManager = d->mSignalManagerObj.object();
                pybind11::object isReady       = signalManager.attr("is_node_ready")(pybind11::cast(targetNodeId));

                if (pybind11::isinstance< pybind11::bool_ >(isReady)) {
                    bool ready = pybind11::cast< bool >(isReady);
                    if (ready) {
                        // 入度满足，触发下游节点执行
                        // 释放GIL后执行Qt信号相关的操作
                        {
                            DAPyGILRelease gilRelease;
                            executeNode(downstreamProxy);
                        }
                    }
                }
            }
        }

    } catch (const pybind11::error_already_set& e) {
        d->dealException(e);
    } catch (const std::exception& e) {
        d->dealException(e);
    }
}

/**
 * @brief 设置执行状态并发射状态变更信号
 *
 * @param[in] newState 新的执行状态
 */
void DAPyWorkFlowExecuter::setExecState(ExecState newState)
{
    DA_D(d);
    ExecState oldState = d->mExecState;
    if (oldState != newState) {
        d->mExecState = newState;
        emit execStateChanged(oldState, newState);
    }
}

}  // namespace DA