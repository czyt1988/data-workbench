#ifndef DAWORKFLOWEXECUTER_H
#define DAWORKFLOWEXECUTER_H
#include <QObject>
#include "DAWorkFlowGlobal.h"
#include "DAAbstractNode.h"

namespace DA
{
/**
 * @brief 工作流任务的执行者，执者和workflow通过信号传递信息，在workflow中，执行者是在一个单独线程中
 */
class DAWORKFLOW_API DAWorkFlowExecuter : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAWorkFlowExecuter)
public:
    DAWorkFlowExecuter(QObject* p = nullptr);
    ~DAWorkFlowExecuter();
    //设置查询的开始点
    void setStartNode(DAAbstractNode::SharedPointer n);
    //设置workflow
    void setWorkFlow(DAWorkFlow* wf);
    //获取全局节点
    QList< DAAbstractNode::SharedPointer > getGlobalNodes() const;
    //获取孤立节点
    QList< DAAbstractNode::SharedPointer > getIsolatedNodesNodes() const;
    //判断是否在请求结束
    bool isTerminateRequest() const;
public slots:
    //开始执行
    void startExecute();
    //请求终止
    void terminateRequest();
    //单独执行某个节点
    void executeNode(DAAbstractNode::SharedPointer n);
private slots:
    //执行节点但不会执行它的输出对应的节点，这个函数用于执行全局节点
    void executeNodeNotTransmit(DAAbstractNode::SharedPointer n);
signals:
    /**
     * @brief 节点执行完成返回的结果
     * @param n
     */
    void nodeExecuteFinished(DAAbstractNode::SharedPointer n, bool state);
    /**
     * @brief 完成执行发射此信号
     */
    void finished(bool success);

private:
};

}  // end of namespace DA

#endif  // DAWORKFLOWEXECUTER_H
