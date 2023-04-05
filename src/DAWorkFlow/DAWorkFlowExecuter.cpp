#include "DAWorkFlowExecuter.h"
#include <QMap>
#include <QDebug>
#include <QPointer>
#include <QSet>
#include "DAWorkFlow.h"
#include "DAAbstractNodeFactory.h"
namespace DA
{
class DAWorkFlowExecuterPrivate
{
    DA_IMPL_PUBLIC(DAWorkFlowExecuter)
public:
    DAWorkFlowExecuterPrivate(DAWorkFlowExecuter* p);
    //查找开始节点，根据网结构， 查找到最终的顶层节点_isolatedNodes和_beginNodes通过此函数查找
    void prepareStartExec();
    //清空内容
    void clear();
    //
    void sendParam(DAAbstractNode::SharedPointer& n, const QList< DAAbstractNode::LinkInfo >& outInfo);
    void transmit(const QList< DAAbstractNode::LinkInfo >& outInfo);

public:
    QPointer< DAWorkFlow > _workflow;
    DAAbstractNode::SharedPointer _startNode;
    QList< DAWorkFlow::CallbackPrepareStartExecute > _callbackStart;
    QList< DAWorkFlow::CallbackPrepareEndExecute > _callbackEnd;
    /**
     * @brief 记录所有节点执行的入度数量
     *
     * 只有入度数量满足,才能触发节点的出度进行后续的传递，此变量用于解决多从入度和出度下的执行顺序问题
     */
    QMap< DAAbstractNode::SharedPointer, int > _nodeIndegreeSetCount;
    QList< DAAbstractNode::SharedPointer > _globalNodes;       ///< 全局节点
    QList< DAAbstractNode::SharedPointer > _isolatedNodes;     ///< 孤立节点
    QList< DAAbstractNode::SharedPointer > _beginNodes;        ///< 开始节点
    QList< QPointer< DAAbstractNodeFactory > > _nodeFactorys;  ///< 记录本次工作流涉及到的工厂
};

//===================================================
// DAWorkFlowExecuterPrivate
//===================================================
DAWorkFlowExecuterPrivate::DAWorkFlowExecuterPrivate(DAWorkFlowExecuter* p) : q_ptr(p)
{
}

/**
 * @brief 通过网结构查找顶层
 *
 * - 所有没有入度的节点都认为是顶层
 * - 入度和出度都没有的节点属于孤立节点，孤立节点先执行
 *
 */
void DAWorkFlowExecuterPrivate::prepareStartExec()
{
    //清空记录
    clear();
    //查找孤立节点和0入度节点
    QList< DAAbstractNode::SharedPointer > nodes = _workflow->nodes();
    QSet< DAAbstractNodeFactory* > factorys;
    for (const DAAbstractNode::SharedPointer& n : qAsConst(nodes)) {
        factorys.insert(n->factory());
        if (DAAbstractNode::GlobalNode == n->nodeType()) {
            _globalNodes.append(n);
        }
        //全局节点的判断和隐式节点类型的判断不冲突，这里是if不是else if
        if (0 == n->getInputNodesCount()) {
            if (0 == n->getOutputNodesCount()) {
                //孤立节点
                if (!_globalNodes.contains(n)) {
                    //如果孤立节点是全局节点，那么不作为孤立节点
                    _isolatedNodes.append(n);
                }
            } else {
                //否则为开始节点
                _beginNodes.append(n);
            }
        }
    }
    //记录工厂
    for (DAAbstractNodeFactory* f : factorys) {
        _nodeFactorys.append(f);
    }
}

/**
 * @brief 清空
 */
void DAWorkFlowExecuterPrivate::clear()
{
    _nodeIndegreeSetCount.clear();
    _globalNodes.clear();
    _isolatedNodes.clear();
    _beginNodes.clear();
    _nodeFactorys.clear();
}

/**
 * @brief 传递参数
 *
 * 把节点的出度的参数传递到对应的入度节点中
 * @note 参数只传递，并不会执行对应的节点
 * @param n
 * @param outInfo
 */
void DAWorkFlowExecuterPrivate::sendParam(DAAbstractNode::SharedPointer& n, const QList< DAAbstractNode::LinkInfo >& outInfo)
{
    for (const DAAbstractNode::LinkInfo& li : qAsConst(outInfo)) {
        QVariant v = n->getOutputData(li.key);
        for (const QPair< QString, DAAbstractNode::SharedPointer >& pair : qAsConst(li.nodes)) {
            pair.second->setInputData(pair.first, v);
            auto ite = _nodeIndegreeSetCount.find(pair.second);
            if (ite == _nodeIndegreeSetCount.end()) {
                ite = _nodeIndegreeSetCount.insert(pair.second, 1);
            } else {
                ++(ite.value());
            }
        }
    }
}

/**
 * @brief 查询输出连接到的节点，并查看节点是否满足执行条件，如果满足则执行
 * @param outInfo
 */
void DAWorkFlowExecuterPrivate::transmit(const QList< DAAbstractNode::LinkInfo >& outInfo)
{
    for (const DAAbstractNode::LinkInfo& li : qAsConst(outInfo)) {
        for (const QPair< QString, DAAbstractNode::SharedPointer >& pair : qAsConst(li.nodes)) {
            if (pair.second->getInputNodesCount() == _nodeIndegreeSetCount[ pair.second ]) {
                //达成执行条件
                q_ptr->executeNode(pair.second);
            }
        }
    }
}

//====================================
// DAWorkFlowExecuter
//====================================

DAWorkFlowExecuter::DAWorkFlowExecuter(QObject* p) : QObject(p), d_ptr(new DAWorkFlowExecuterPrivate(this))
{
    qDebug() << "create DAWorkFlowExecuter";
}

DAWorkFlowExecuter::~DAWorkFlowExecuter()
{
    qDebug() << "destory DAWorkFlowExecuter";
}

/**
 * @brief 设置开始节点
 * @param n
 */
void DAWorkFlowExecuter::setStartNode(DAAbstractNode::SharedPointer n)
{
    d_ptr->_startNode = n;
}

/**
 * @brief 设置工作流
 * @param wf
 */
void DAWorkFlowExecuter::setWorkFlow(DAWorkFlow* wf)
{
    d_ptr->_workflow      = wf;
    d_ptr->_callbackStart = wf->getStartWorkflowCallback();
    d_ptr->_callbackEnd   = wf->getEndWorkflowCallback();
}

/**
 * @brief 获取全局节点
 * @return
 */
QList< DAAbstractNode::SharedPointer > DAWorkFlowExecuter::getGlobalNodes() const
{
    return d_ptr->_globalNodes;
}

/**
 * @brief 获取孤立节点
 * @return
 */
QList< DAAbstractNode::SharedPointer > DAWorkFlowExecuter::getIsolatedNodesNodes() const
{
    return d_ptr->_isolatedNodes;
}

/**
 * @brief 开始执行节点运算
 *
 * 此函数有DAWorkFlow的exec函数触发
 * @param n
 */
void DAWorkFlowExecuter::startExecute()
{
    //! 首先执行注册的prepareStartexec回调
    for (DAWorkFlow::CallbackPrepareStartExecute& fn : d_ptr->_callbackStart) {
        if (!fn(this)) {
            emit finished(false);
            return;
        }
    }

    d_ptr->prepareStartExec();
    if (d_ptr->_startNode) {
        //如果指定了开始节点，就从开始节点开始执行
        executeNode(d_ptr->_startNode);
    } else {
        //否则自动查找节点开始执行
        //执行全局节点，全局节点只执行不传递，也就是说执行节点后，节点的连线并不会执行
        for (const DAAbstractNode::SharedPointer& n : qAsConst(d_ptr->_globalNodes)) {
            executeNodeNotTransmit(n);
        }
        //开始执行孤立节点
        for (const DAAbstractNode::SharedPointer& n : qAsConst(d_ptr->_isolatedNodes)) {
            executeNode(n);
        }
        //开始执行开始节点
        for (const DAAbstractNode::SharedPointer& n : qAsConst(d_ptr->_beginNodes)) {
            if (!d_ptr->_globalNodes.contains(n)) {
                //如果开始节点并不是全局节点，正常执行
                executeNode(n);
            } else {
                //既是开始节点也是全局节点，这时候全局节点已经执行并传递了数据，只需执行传递操作
                d_ptr->transmit(n->getAllOutputLinkInfo());
            }
        }
    }

    //! 最后执行注册的prepareEndexec回调
    for (DAWorkFlow::CallbackPrepareEndExecute& fn : d_ptr->_callbackEnd) {
        if (fn) {
            if (!fn(this)) {
                emit finished(false);
                return;
            }
        }
    }

    emit finished(true);
}

/**
 * @brief 节点执行的槽函数
 *
 * 执行完成后会发射 @sa nodeExecuteFinished 信号
 *
 * 关于节点执行等待问题：
 *
 * 如果有如下链路，需要进行等待
 *
 * [1]--->[2.1]--->[2.2]--->[4]
 *    |                    ↑
 *    └-->[3.1]------------┘
 *
 * 上面这个链路1节点分出2.1和3.1
 *
 * 2.1经过2.2才到4，3.1直接到4，因此，4的执行必须等待2.2的执行
 *
 * 复杂一点的情况如：
 *
 *              ┌———>[4.1]—┐
 *              |          ↓
 * [1]--->[2.1]=┙->[2.2]--->[5]
 *    |           ↑        ↑
 *    └-->[3.1]===┙--------┘
 *
 * 也就是说，入度不全，不能继续传递
 *
 * @param n
 */
void DAWorkFlowExecuter::executeNode(DAAbstractNode::SharedPointer n)
{
    qDebug() << tr("execute node, name=%1,type=%2").arg(n->getNodeName(), n->metaData().getNodePrototype());
    bool state = n->exec();
    emit nodeExecuteFinished(n, state);
    //获取节点的输出
    QList< DAAbstractNode::LinkInfo > outInfo = n->getAllOutputLinkInfo();
    //给输出的节点传参数
    d_ptr->sendParam(n, outInfo);
    //执行输出节点，输出节点的执行要满足入度数量和入度节点链接数量一致才能执行
    d_ptr->transmit(outInfo);
}

/**
 * @brief 执行节点但不会执行它的输出对应的节点，这个函数用于执行节点后并传递参数，但不会触发下个节点
 *
 * 通常用于执行全局节点
 *
 * @note 对于既是全局节点也是开始节点的情况
 *
 * 有一种情况就是节点既是全局节点也是开始节点,节点是不会执行传递
 *
 * @param n
 */
void DAWorkFlowExecuter::executeNodeNotTransmit(DAAbstractNode::SharedPointer n)
{
    qDebug() << tr("execute node(not transmit), name=%1,type=%2").arg(n->getNodeName(), n->metaData().getNodePrototype());
    bool state = n->exec();
    emit nodeExecuteFinished(n, state);
    //获取节点的输出
    QList< DAAbstractNode::LinkInfo > outInfo = n->getAllOutputLinkInfo();
    //给输出的节点传参数
    d_ptr->sendParam(n, outInfo);
}
}  // end of namespace DA
