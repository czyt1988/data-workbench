#ifndef DAWORKFLOW_H
#define DAWORKFLOW_H
#include <QtCore/qglobal.h>
#include <memory>
#include <QObject>
#include "DAWorkFlowGlobal.h"
#include "DAAbstractNode.h"
class QDomDocument;
class QDomElement;

namespace DA
{
class DAWorkFlowExecuter;
class DAAbstractNodeFactory;

/**
 * @brief 基本的工作流,这个也是总工厂，汇总了所有插件的工厂
 */
class DAWORKFLOW_API DAWorkFlow : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAWorkFlow)
    friend class DAAbstractNode;

public:
    using CallbackPrepareStartExecute = std::function< bool(DAWorkFlowExecuter*) >;
    using CallbackPrepareEndExecute   = std::function< bool(DAWorkFlowExecuter*) >;

public:
    DAWorkFlow(QObject* p = nullptr);
    virtual ~DAWorkFlow();
    //工厂相关
    void registFactory(DAAbstractNodeFactory* factory);
    void registFactorys(const QList< DAAbstractNodeFactory* > factorys);
    //获取所有的工厂
    QList< DAAbstractNodeFactory* > factorys() const;
    //获取使用到的工厂
    QList< DAAbstractNodeFactory* > usedFactorys() const;
    //获取工程的数量
    int getFactoryCount() const;
    //通过工厂名字获取工厂
    DAAbstractNodeFactory* getFactory(const QString& factoryPrototypes);
    //通过protoType获取DANodeMetaData
    DANodeMetaData getNodeMetaData(const QString& protoType) const;
    //创建节点，会触发信号nodeCreated,FCWorkFlow保留节点的内存管理权
    DAAbstractNode::SharedPointer createNode(const DANodeMetaData& md);
    //添加节点
    void addNode(DAAbstractNode::SharedPointer n);
    //获取节点列表
    QList< DAAbstractNode::SharedPointer > nodes() const;
    //清空节点
    void clear();
    //删除节点
    void removeNode(const DAAbstractNode::SharedPointer& n);
    //是否在workflow存在id
    bool hasNodeID(const DAAbstractNode::IdType id);
    //通过节点ID获取节点Item
    DAAbstractNode::SharedPointer getNode(const DAAbstractNode::IdType id);
    //通过节点ID获取节点Item
    DAAbstractNodeGraphicsItem* getNodeGraphicsItem(const DAAbstractNode::IdType id);
    //把扩展信息保存到xml上
    virtual void saveExternInfoToXml(QDomDocument* doc, QDomElement* nodeElement) const;
    //从xml加载扩展信息
    virtual void loadExternInfoFromXml(const QDomElement* nodeElement);
    //设置开始节点
    void setStartNode(DAAbstractNode::SharedPointer p);
    //获取开始执行的节点
    DAAbstractNode::SharedPointer getStartNode() const;
    //判断工作量是否再运行
    bool isRunning() const;
    //工作流中节点的数量
    int size() const;
    //判断是否为空
    bool isEmpty() const;
    //获取最后发生的错误信息
    QString getLastErrorString() const;
    //注册开始执行的回调
    void registStartWorkflowCallback(CallbackPrepareStartExecute fn);
    //注册开始结束的回调
    void registEndWorkflowCallback(CallbackPrepareEndExecute fn);
    //获取所有注册的开始回调
    QList< CallbackPrepareStartExecute > getStartWorkflowCallback() const;
    //获取所有注册的结束回调
    QList< CallbackPrepareEndExecute > getEndWorkflowCallback() const;
public slots:
    //运行工作流
    void exec();
    //终止
    void terminate();

protected:
    //主动触发nodeNameChanged信号
    void emitNodeNameChanged(DAAbstractNode::SharedPointer node, const QString& oldName, const QString& newName);
signals:

    /**
     * @brief 节点添加的信号
     * @param node
     */
    void nodeAdded(DAAbstractNode::SharedPointer node);
    /**
     * @brief 节点在工作流中开始被删除
     * @param node
     */
    void nodeStartRemove(DAAbstractNode::SharedPointer node);
    /**
     * @brief 节点名字变更
     * @param node
     * @param oldName
     * @param newName
     */
    void nodeNameChanged(DAAbstractNode::SharedPointer node, const QString& oldName, const QString& newName);
    /**
     * @brief 清空信号
     */
    void workflowCleared();

    /**
     * @brief 开始执行，exec函数调用后会触发此信号
     */
    void startExecute();

    /**
     * @brief 开始停止工作流
     */
    void terminateExecute();
    /**
     * @brief 执行到某个节点发射的信号
     * @param n
     */
    void nodeExecuteFinished(DAAbstractNode::SharedPointer n, bool state);

    /**
     * @brief 工作流执行完毕信号
     * @param success 成功全部执行完成为true
     */
    void finished(bool success);
private slots:
    //工厂删除时触发的信号，用于清除工厂相关的信息
    void onFactoryDestory(QObject* fac);
    //执行器执行结束
    void onExecuteFinished(bool success);
};
}  // end of namespace DA
#endif  // FCWORKFLOW_H
