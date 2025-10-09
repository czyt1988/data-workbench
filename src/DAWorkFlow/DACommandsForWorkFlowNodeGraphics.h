#ifndef DACOMMANDSFORWORKFLOWNODEGRAPHICS_H
#define DACOMMANDSFORWORKFLOWNODEGRAPHICS_H
#include "DAWorkFlowAPI.h"
// qt
#include <QUndoCommand>
#include <QPen>
#include <QBrush>
#include <QPointer>
// workflow
#include "DANodeMetaData.h"
#include "DANodeLinkPoint.h"
#include "DAAbstractNode.h"

namespace DA
{
class DAWorkFlow;
class DAAbstractNodeLinkGraphicsItem;
class DAGraphicsPixmapItem;
class DANodeGraphicsScene;
class DAWorkFlowGraphicsScene;
class DAAbstractNodeGraphicsItem;
class DAGraphicsItem;
class DAGraphicsStandardTextItem;

/**
 * @brief 节点创建命令
 * @note 此函数不能继承id进行压缩，因为推入stack后需要操作这个cmd指针
 * @note 此命令会执行addItem
 */
class DAWORKFLOW_API DACommandsForWorkFlowCreateNode : public QUndoCommand
{
public:
	DACommandsForWorkFlowCreateNode(const DANodeMetaData& md,
                                    DANodeGraphicsScene* scene,
                                    const QPointF& pos,
                                    bool addItemToScene  = true,
                                    QUndoCommand* parent = nullptr);
	~DACommandsForWorkFlowCreateNode();
	void redo();
	void undo();
	// 获取创建的item，此函数不能继承id进行压缩，因为推入stack后需要操作这个cmd指针
	DAAbstractNodeGraphicsItem* item() const;
	// 节点
	DAAbstractNode::SharedPointer node() const;

private:
	DANodeGraphicsScene* mScene;
	DANodeMetaData mMetadata;
	QPointF mScenePos;
	DAAbstractNode::SharedPointer mNode;
	bool mAddItemToScene { true };
	DAAbstractNodeGraphicsItem* mItem { nullptr };
	bool mNeedDelete;
	bool mSkipFirstRedo { true };
};

/**
 * @brief 移除节点命令
 */
class DAWORKFLOW_API DACommandsForWorkFlowRemoveNodes : public QUndoCommand
{
public:
	DACommandsForWorkFlowRemoveNodes(DAWorkFlow* wf,
                                     const QList< DAAbstractNode::SharedPointer >& ns,
                                     QUndoCommand* par = nullptr);
	~DACommandsForWorkFlowRemoveNodes();
	void redo();
	void undo();

private:
	QPointer< DAWorkFlow > mWorkflow;
	QList< DAAbstractNode::SharedPointer > mNodes;
};

/**
 * @brief 节点添加命令
 * @note 此命令不会合并
 */
class DAWORKFLOW_API DACommandsForWorkFlowAddNodeItem : public QUndoCommand
{
public:
	DACommandsForWorkFlowAddNodeItem(DANodeGraphicsScene* scene,
                                     DAAbstractNodeGraphicsItem* item,
                                     QUndoCommand* parent = nullptr);
	~DACommandsForWorkFlowAddNodeItem();
	void redo();
	void undo();

private:
	DANodeGraphicsScene* mScene;
	DAAbstractNodeGraphicsItem* mNodeItem;
	DAAbstractNode::SharedPointer mNode;  ///< node要记录下来，否则undo后没有实例持有，会被析构掉
	bool mNeedDelete;
};

/**
 * @brief 节点删除命令
 *
 * 此命令只适合单独删除
 * @note 此命令不会合并
 */
class DAWORKFLOW_API DACommandsForWorkFlowRemoveNodeItem : public QUndoCommand
{
public:
	DACommandsForWorkFlowRemoveNodeItem(DANodeGraphicsScene* scene,
                                        DAAbstractNodeGraphicsItem* item,
                                        QUndoCommand* parent = nullptr);
	~DACommandsForWorkFlowRemoveNodeItem();
	void redo();
	void undo();
	// 获取选择的节点,此函数构造后即可调用
	QList< DAAbstractNodeLinkGraphicsItem* > getRemovedNodeLinkItems() const;

private:
	DANodeGraphicsScene* mScene;
	DAAbstractNodeGraphicsItem* mNodeItem;
	DAAbstractNode::SharedPointer mWillRemoveNode;  ///< 记录删除的节点，这里是为了智能指针保留实例，不至于被析构
	QList< DAAbstractNodeLinkGraphicsItem* > mWillRemoveLink;  ///< 此item仅做保存
	bool mNeedDelete;
};

/**
 * @brief The 节点删除命令
 * @note 此命令不会合并
 */
class DAWORKFLOW_API DACommandsForWorkFlowRemoveSelectNodes : public QUndoCommand
{
public:
	DACommandsForWorkFlowRemoveSelectNodes(DANodeGraphicsScene* scene, QUndoCommand* parent = nullptr);
	~DACommandsForWorkFlowRemoveSelectNodes();
	void redo();
	void undo();
	void classifyItems(DANodeGraphicsScene* scene,
                       QList< DAAbstractNodeGraphicsItem* >& nodeItems,
                       QList< DAAbstractNodeLinkGraphicsItem* >& linkItems,
                       QList< QGraphicsItem* >& normalItem);
	// 获取节点的链接，只返回唯一值
	static QList< DAAbstractNodeLinkGraphicsItem* > getNodesLinks(const QList< DAAbstractNodeGraphicsItem* >& nodeItems);
	// 是否有效
	bool isValid() const;
	// 移除的数量
	int removeCount() const;
	// 获取选择的节点,此函数构造后即可调用
	QList< DAAbstractNodeGraphicsItem* > getRemovedNodeItems() const;
	QList< DAAbstractNodeLinkGraphicsItem* > getRemovedNodeLinkItems() const;
	QList< QGraphicsItem* > getAllRemovedItems() const;

private:
	bool mIsvalid;  ///< 标记这个cmd是否有效，如果选择的内容没有效，则为false
	DANodeGraphicsScene* mScene;
	QList< DAAbstractNodeGraphicsItem* > mSelectNodeItems;
	QList< DAAbstractNodeLinkGraphicsItem* > mWillRemoveLink;  ///< 此item仅做保存
	QList< QGraphicsItem* > mWillRemoveNormal;
	QList< DAAbstractNode::SharedPointer > mWillRemoveNodes;  ///< 记录删除的节点，这里是为了智能指针保留实例，不至于被析构
	bool mNeedDelete;
};

/**
 * @brief 创建link
 */
class DAWORKFLOW_API DACommandsForWorkFlowCreateLink : public QUndoCommand
{
public:
	// 这两个构造函数是在linkitem已经完成且确认连接后执行，在第一次redo时，不会进行操作，第二次才是真实操作
	DACommandsForWorkFlowCreateLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                    DANodeGraphicsScene* sc,
                                    QUndoCommand* parent = nullptr);
	~DACommandsForWorkFlowCreateLink();
	void redo() override;
	void undo() override;

private:
	DAAbstractNodeLinkGraphicsItem* mLinkitem;
	DAAbstractNodeGraphicsItem* mFromitem;
	DAAbstractNodeGraphicsItem* mToitem;
	QString mFromPointName;  ///< 记录名字，每次redo/undo都从item获取linkpoint，以免出现变化
	QString mToPointName;    ///< 记录名字，每次redo/undo都从item获取linkpoint，以免出现变化
	DANodeGraphicsScene* mScene;
	bool mNeedDelete;
	bool mSkipFirstRedo;  ///< 第一次redo跳过,此优先级高于_isFirstRedoAdditem
};

/**
 * @brief 移除link
 */
class DAWORKFLOW_API DACommandsForWorkFlowRemoveLink : public QUndoCommand
{
public:
	// 这两个构造函数是在linkitem已经完成且确认连接后执行，在第一次redo时，不会进行操作，第二次才是真实操作
	DACommandsForWorkFlowRemoveLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                    DANodeGraphicsScene* sc,
                                    QUndoCommand* parent = nullptr);
	~DACommandsForWorkFlowRemoveLink();
	void redo() override;
	void undo() override;

private:
	DAAbstractNodeLinkGraphicsItem* mLinkitem;
	DAAbstractNodeGraphicsItem* mFromitem;
	DAAbstractNodeGraphicsItem* mToitem;
	QString mFromPointName;  ///< 记录名字，每次redo/undo都从item获取linkpoint，以免出现变化
	QString mToPointName;    ///< 记录名字，每次redo/undo都从item获取linkpoint，以免出现变化
	DANodeGraphicsScene* mScene;
	bool mNeedDelete;
};

}  // end DA

#endif  // DACOMMANDSFORWORKFLOWNODEGRAPHICS_H
