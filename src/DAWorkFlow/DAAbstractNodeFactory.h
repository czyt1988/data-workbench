#ifndef DAABSTRACTNODEFACTORY_H
#define DAABSTRACTNODEFACTORY_H
#include "DAWorkFlowGlobal.h"
#include <QtCore/qglobal.h>
#include <QObject>
#include "DAAbstractNode.h"
class QDomDocument;
class QDomElement;

namespace DA
{

class DAWorkFlow;
class DANodeGraphicsScene;
class DANodeGraphicsSceneEventListener;
/**
 * @brief FCAbstractNode的工厂基类，所有自定义的node集合最后都需要提供一个工厂
 *
 * 工厂将通过@sa DANodeMetaData 来创建一个DAAbstractNode,DAAbstractNode可以生成DAAbstractNodeGraphicsItem实现前端的渲染，
 * 因此，任何节点都需要实现一个DAAbstractNode和一个DAAbstractNodeGraphicsItem，一个实现逻辑节点的描述，
 * 一个实现前端的渲染,另外DAAbstractNodeGraphicsItem可以生成DAAbstractNodeWidget，用于设置DAAbstractNodeGraphicsItem
 */
class DAWORKFLOW_API DAAbstractNodeFactory : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAbstractNodeFactory)
public:
    DAAbstractNodeFactory(QObject* p = nullptr);
    virtual ~DAAbstractNodeFactory();

    // 工厂设置了workflow，此函数设置为虚函数，在某些工厂可以通过此函数的重载来绑定DAWorkFlow的信号,以及注册回调
    virtual void registWorkflow(DAWorkFlow* wf);

    // 获取工作流
    DAWorkFlow* getWorkFlow() const;

    /**
     * @brief 工厂的唯一标识
     * @note 每个工厂需要保证有唯一的标识，工作流将通过标识查找工厂
     * @note 此类型名字不能进行翻译
     * @return
     */
    virtual QString factoryPrototypes() const = 0;
    /**
     * @brief 工厂名称
     * @note 工厂名称可进行翻译
     * @return
     */
    virtual QString factoryName() const = 0;

    /**
     * @brief 工厂具体描述
     * @note 工厂具体描述可进行翻译
     * @return
     */
    virtual QString factoryDescribe() const = 0;

    /**
     * @brief 工厂函数，创建一个DAAbstractNode，工厂不持有FCAbstractNode的管理权
     * @param meta 元对象
     * @return
     * @note 此函数会在@sa DAWorkFlow::createNode 中调用，用户不要直接调用此函数，
     * 因为@sa DAWorkFlow::createNode 中会有其他的操作
     */
    virtual DAAbstractNode::SharedPointer create(const DANodeMetaData& meta) = 0;

    /**
     * @brief 获取所有注册的Prototypes
     * @return
     */
    virtual QStringList getPrototypes() const = 0;

    /**
     * @brief 获取所有类型的元数据
     * @return
     */
    virtual QList< DANodeMetaData > getNodesMetaData() const = 0;

    // 节点加入workflow的回调
    virtual void nodeAddedToWorkflow(DAAbstractNode::SharedPointer node);

    // 节点删除的工厂回调
    virtual void nodeStartRemove(DAAbstractNode::SharedPointer node);

    // 节点连接成功的回调
    // 注意仅仅是节点的链接完成，这里不要操作graphicsItem,
    // 要处理连接线完全连接两个节点后的情况，使用DAAbstractNodeLinkGraphicsItem::finishedLink来处理
    virtual void nodeLinkSucceed(DAAbstractNode::SharedPointer outNode,
                                 const QString& outKey,
                                 DAAbstractNode::SharedPointer inNode,
                                 const QString& inkey);
    // 节点连线删除的回调
    virtual void nodeLinkDetached(DAAbstractNode::SharedPointer outNode,
                                  const QString& outKey,
                                  DAAbstractNode::SharedPointer inNode,
                                  const QString& inkey);
    // 把扩展信息保存到xml上
    virtual void saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const;
    // 从xml加载扩展信息
    virtual void loadExternInfoFromXml(const QDomElement* factoryExternElement);
    // 创建场景事件监听器，如果需要对场景的事件进行监听，需要继承此函数并返回一个事件监听器
    virtual DANodeGraphicsSceneEventListener* createNodeGraphicsSceneEventListener();
};
}
#endif  // FCABSTRACTNODEFACTORY_H
