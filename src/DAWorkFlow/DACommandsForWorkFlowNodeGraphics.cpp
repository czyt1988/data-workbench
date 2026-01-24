#include "DACommandsForWorkFlowNodeGraphics.h"
// qt
#include <QObject>
// workflow
#include "DANodeGraphicsScene.h"
#include "DAGraphicsPixmapItem.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAWorkFlow.h"
#include "DAGraphicsItem.h"
#include "DAGraphicsStandardTextItem.h"
#include "DAQtContainerUtil.hpp"
namespace DA
{

//==============================================================
// DACommandsForWorkFlowCreateNode
//==============================================================

DACommandsForWorkFlowCreateNode::DACommandsForWorkFlowCreateNode(const DANodeMetaData& md,
                                                                 DANodeGraphicsScene* scene,
                                                                 const QPointF& pos,
                                                                 bool addItemToScene,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mScene(scene), mScenePos(pos), mAddItemToScene(addItemToScene), mItem(nullptr), mNeedDelete(true)
{
	setText(QObject::tr("Create Node"));
	//! 针对在命令的构造函数中就直接执行了创建或者删除动作的情况，
	//! 创建的命令mNeedDelete初始要为true，否则创建此命令，但没推入stack就会出现内存泄露
	//! 反之亦然，删除的命令，needdelete应该为false
	mNeedDelete = true;
	mMetadata   = md;
	mItem       = scene->createNode(md, pos, addItemToScene);
	mNode       = mItem->node();
}

DACommandsForWorkFlowCreateNode::~DACommandsForWorkFlowCreateNode()
{
	if (mNeedDelete) {
		//_node是智能指针自动删除
		if (mItem) {
			delete mItem;
		}
	}
}

void DACommandsForWorkFlowCreateNode::redo()
{
	QUndoCommand::redo();  // 此函数会执行子内容的redo/undo
	mNeedDelete = false;   // 标记要delete为false
	if (mSkipFirstRedo) {
		//! 关键:第一次执行要跳过redo，否则会重复添加节点
		mSkipFirstRedo = false;
		return;
	}
	if (mItem && mAddItemToScene) {
		mScene->addItem(mItem);
	}
	if (mNode) {
		DAWorkFlow* wf = mScene->getWorkflow();
		wf->addNode(mNode);
	}
}

void DACommandsForWorkFlowCreateNode::undo()
{
	QUndoCommand::undo();  // 此函数会执行子内容的redo/undo
	mNeedDelete = true;
	if (mNode) {
		DAWorkFlow* wf = mScene->getWorkflow();
		wf->removeNode(mNode);
	}
	if (mItem && mAddItemToScene) {
		mScene->removeItem(mItem);
	}
}

DAAbstractNodeGraphicsItem* DACommandsForWorkFlowCreateNode::item() const
{
	return mItem;
}

DAAbstractNode::SharedPointer DACommandsForWorkFlowCreateNode::node() const
{
	return mNode;
}

//----------------------------------------------------
// DACommandsForWorkFlowRemoveNodes
//----------------------------------------------------
DACommandsForWorkFlowRemoveNodes::DACommandsForWorkFlowRemoveNodes(DAWorkFlow* wf,
                                                                   const QList< DAAbstractNode::SharedPointer >& ns,
                                                                   QUndoCommand* par)
    : QUndoCommand(par), mWorkflow(wf), mNodes(ns)
{
    setText(QObject::tr("Remove Nodes"));
}

DACommandsForWorkFlowRemoveNodes::~DACommandsForWorkFlowRemoveNodes()
{
}

void DACommandsForWorkFlowRemoveNodes::redo()
{
	if (mWorkflow) {
        for (const auto& n : std::as_const(mNodes)) {
			mWorkflow->removeNode(n);
		}
	}
}

void DACommandsForWorkFlowRemoveNodes::undo()
{
	if (mWorkflow) {
        for (const auto& n : std::as_const(mNodes)) {
			mWorkflow->addNode(n);
		}
	}
}

//----------------------------------------------------
// DACommandsForWorkFlowAddNodeItem
//----------------------------------------------------

DACommandsForWorkFlowAddNodeItem::DACommandsForWorkFlowAddNodeItem(DANodeGraphicsScene* scene,
                                                                   DAAbstractNodeGraphicsItem* item,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mScene(scene), mNodeItem(item)
{
	setText(QObject::tr("Add node Item"));  // cn:添加节点
	//! 针对在命令的构造函数中就直接执行了创建或者删除动作的情况，
	//! 创建的命令mNeedDelete初始要为true，否则创建此命令，但没推入stack就会出现内存泄露
	//! 但要注意redo时一定要把mNeedDelete设置好
	//! 反之亦然，删除的命令，needdelete应该为false
	mNeedDelete = true;
}

DACommandsForWorkFlowAddNodeItem::~DACommandsForWorkFlowAddNodeItem()
{
	if (mNeedDelete) {
		if (mNodeItem) {
			delete mNodeItem;
		}
	}
}

void DACommandsForWorkFlowAddNodeItem::redo()
{
	DAWorkFlow* wf = mScene->getWorkflow();
	if (wf) {
		//! 这里要把node保存下来，node是智能指针，如果用户正常操作添加，addNode_，node的智能指针是有其它地方的实例不会析构
		//! 但是，如果是打开工程，打开后再删除，这样ndoe是没有其它地方的实例，wf->removeNode会直接把node析构了，
		//! 如果这里直接removeNode,节点就会被析构，
		//! 因此，为了避免node析构，这里要把node再保存下来
		mNode = mNodeItem->node();
		wf->addNode(mNode);
	}
	mScene->addItem(mNodeItem);
	mNeedDelete = false;
}

void DACommandsForWorkFlowAddNodeItem::undo()
{
	mScene->removeItem(mNodeItem);
	DAWorkFlow* wf = mScene->getWorkflow();
	if (wf) {
		wf->removeNode(mNode);
	}
	mNeedDelete = true;
}
//===============================================================
// DACommandsForWorkFlowRemoveNodeItem
//===============================================================
DACommandsForWorkFlowRemoveNodeItem::DACommandsForWorkFlowRemoveNodeItem(DANodeGraphicsScene* scene,
                                                                         DAAbstractNodeGraphicsItem* item,
                                                                         QUndoCommand* parent)
    : QUndoCommand(parent), mScene(scene), mNodeItem(item)
{
	setText(QObject::tr("Remove Node Item"));
	//! 针对在命令的构造函数中就直接执行了创建或者删除动作的情况，
	//! 创建的命令mNeedDelete初始要为true，否则创建此命令，但没推入stack就会出现内存泄露
	//! 反之亦然，删除的命令，needdelete应该为false
	mNeedDelete = false;

	mWillRemoveLink = item->getLinkItems();
    for (DAAbstractNodeLinkGraphicsItem* lk : std::as_const(mWillRemoveLink)) {
		new DACommandsForWorkFlowRemoveLink(lk, scene, this);
	}
}

DACommandsForWorkFlowRemoveNodeItem::~DACommandsForWorkFlowRemoveNodeItem()
{
	if (mNeedDelete) {
		if (mNodeItem) {
			delete mNodeItem;
		}
	}
}

void DACommandsForWorkFlowRemoveNodeItem::redo()
{  // 注意QUndoCommand::redo()要放到最前，QUndoCommand::undo()要放到最后
	mNeedDelete = true;
	QUndoCommand::redo();  // 此函数会执行子内容的redo/undo,也就是先删除link
	mScene->removeItem(mNodeItem);
	DAWorkFlow* wf = mScene->getWorkflow();
	if (wf) {
		//! 这里要把node保存下来，node是智能指针，如果用户正常操作添加，addNode_，node的智能指针是有其它地方的实例不会析构
		//! 但是，如果是打开工程，打开后再删除，这样ndoe是没有其它地方的实例，wf->removeNode会直接把node析构了，
		//! 如果这里直接removeNode,节点就会被析构，
		//! 因此，为了避免node析构，这里要把node再保存下来
		mWillRemoveNode = mNodeItem->node();
		wf->removeNode(mWillRemoveNode);
	}
}

void DACommandsForWorkFlowRemoveNodeItem::undo()
{
	mNeedDelete = false;
	mScene->addItem(mNodeItem);
	DAWorkFlow* wf = mScene->getWorkflow();
	if (wf) {
		// 因为mWillRemoveNodes保留了节点，item里的weakpoint能获取到智能指针
		wf->addNode(mNodeItem->node());
	}
	mWillRemoveNode = nullptr;
	// 注意undo要放到最后
	QUndoCommand::undo();  // 此函数会执行子内容的redo/undo
}

QList< DAAbstractNodeLinkGraphicsItem* > DACommandsForWorkFlowRemoveNodeItem::getRemovedNodeLinkItems() const
{
	return mWillRemoveLink;
}

//==============================================================
// DACommandsForWorkFlowRemoveNode
//==============================================================
DACommandsForWorkFlowRemoveSelectNodes::DACommandsForWorkFlowRemoveSelectNodes(DANodeGraphicsScene* scene,
                                                                               QUndoCommand* parent)
    : QUndoCommand(parent), mIsvalid(false), mScene(scene), mNeedDelete(false)
{
	setText(QObject::tr("Remove Select Nodes"));
	//! 针对在命令的构造函数中就直接执行了创建或者删除动作的情况，
	//! 创建的命令mNeedDelete初始要为true，否则创建此命令，但没推入stack就会出现内存泄露
	//! 反之亦然，删除的命令，needdelete应该为false
	mNeedDelete = false;
	classifyItems(mScene, mSelectNodeItems, mWillRemoveLink, mWillRemoveNormal);
	QList< DAAbstractNodeLinkGraphicsItem* > nodeLinks = getNodesLinks(mSelectNodeItems);
	// 这时候得到所有需要删除的link
	// 也要做一次去重
	mWillRemoveLink = nodeLinks + mWillRemoveLink;
	mWillRemoveLink = unique_qlist(mWillRemoveLink);
    for (DAAbstractNodeLinkGraphicsItem* lk : std::as_const(mWillRemoveLink)) {
		new DACommandsForWorkFlowRemoveLink(lk, scene, this);
	}
	mIsvalid = (nodeLinks.size() > 0) || (mWillRemoveLink.size() > 0) || mSelectNodeItems.size() > 0
			   || mWillRemoveNormal.size() > 0;
}

DACommandsForWorkFlowRemoveSelectNodes::~DACommandsForWorkFlowRemoveSelectNodes()
{
	if (mNeedDelete) {
        for (DAAbstractNodeGraphicsItem* item : std::as_const(mSelectNodeItems)) {
			delete item;
		}
        for (QGraphicsItem* item : std::as_const(mWillRemoveNormal)) {
			delete item;
		}
	}
}
/**
 * @brief 把选中的item分类
 * @param scene
 * @param nodeItems
 * @param linkItems
 */
void DACommandsForWorkFlowRemoveSelectNodes::classifyItems(DANodeGraphicsScene* scene,
                                                           QList< DAAbstractNodeGraphicsItem* >& nodeItems,
                                                           QList< DAAbstractNodeLinkGraphicsItem* >& linkItems,
                                                           QList< QGraphicsItem* >& normalItem)
{
	QList< QGraphicsItem* > its = scene->selectedItems();
	if (its.size() <= 0) {
		return;
	}
	bool isStartLink = scene->isStartLink();
    for (QGraphicsItem* i : std::as_const(its)) {
		if (DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
			nodeItems.append(ni);
		} else if (DAAbstractNodeLinkGraphicsItem* li = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(i)) {
			if (isStartLink) {
				if (scene->getCurrentLinkItem() == li) {
					// 连接线要跳过
					scene->cancelLink();
					continue;
				}
			}
			linkItems.append(li);
		} else {
			normalItem.append(i);
		}
	}
}

QList< DAAbstractNodeLinkGraphicsItem* >
DACommandsForWorkFlowRemoveSelectNodes::getNodesLinks(const QList< DAAbstractNodeGraphicsItem* >& nodeItems)
{
	QList< DAAbstractNodeLinkGraphicsItem* > res;
    for (DAAbstractNodeGraphicsItem* n : std::as_const(nodeItems)) {
		res += n->getLinkItems();
	}
	return unique_qlist(res);
}

bool DACommandsForWorkFlowRemoveSelectNodes::isValid() const
{
    return mIsvalid;
}
/**
 * @brief 获取移除的数量，此函数构造后即可调用
 * @return
 */
int DACommandsForWorkFlowRemoveSelectNodes::removeCount() const
{
    return mSelectNodeItems.size() + childCount() + mWillRemoveNormal.size();
}

/**
 * @brief 获取选择的节点,此函数构造后即可调用
 * @return
 */
QList< DAAbstractNodeGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getRemovedNodeItems() const
{
    return mSelectNodeItems;
}
/**
 * @brief 获取移除的link,此函数构造后即可调用
 * @return
 */
QList< DAAbstractNodeLinkGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getRemovedNodeLinkItems() const
{
    return mWillRemoveLink;
}
/**
 * @brief 获取所有移除的QGraphicsItem
 * @return
 */
QList< QGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getAllRemovedItems() const
{
	QList< QGraphicsItem* > res = mWillRemoveNormal;
    for (DAAbstractNodeGraphicsItem* i : std::as_const(mSelectNodeItems)) {
		res.append(i);
	}
    for (DAAbstractNodeLinkGraphicsItem* i : std::as_const(mWillRemoveLink)) {
		res.append(i);
	}
	return res;
}

void DACommandsForWorkFlowRemoveSelectNodes::redo()
{
	// 注意QUndoCommand::redo()要放到最前，QUndoCommand::undo()要放到最后
	mNeedDelete = true;
	QUndoCommand::redo();  // 此函数会执行子内容的redo/undo,也就是先删除link
	DAWorkFlow* wf = mScene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : std::as_const(mSelectNodeItems)) {
		mScene->removeItem(item);
		if (wf) {
			//! 这里要把node保存下来，node是智能指针，如果用户正常操作添加，addNode_，node的智能指针是有其它地方的实例不会析构
			//! 但是，如果是打开工程，打开后再删除，这样ndoe是没有其它地方的实例，如果这里直接removeNode,节点就会被析构，
			//! 因此，为了避免node析构，这里要把node再保存下来
			auto n = item->node();
			mWillRemoveNodes.append(n);
			wf->removeNode(n);
		}
	}
    for (QGraphicsItem* item : std::as_const(mWillRemoveNormal)) {
		mScene->removeItem(item);
	}
}

void DACommandsForWorkFlowRemoveSelectNodes::undo()
{
	mNeedDelete    = false;
	DAWorkFlow* wf = mScene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : std::as_const(mSelectNodeItems)) {
		mScene->addItem(item);
		if (wf) {
			// 因为mWillRemoveNodes保留了节点，item里的weakpoint能获取到智能指针
			wf->addNode(item->node());
		}
	}
    for (QGraphicsItem* item : std::as_const(mWillRemoveNormal)) {
		mScene->addItem(item);
	}
	mWillRemoveNodes.clear();
	// 注意undo要放到最后
	QUndoCommand::undo();  // 此函数会执行子内容的redo/undo
}

//==============================================================
// DACommandsForWorkFlowCreateLink
//==============================================================

DACommandsForWorkFlowCreateLink::DACommandsForWorkFlowCreateLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mLinkitem(linkitem), mScene(sc), mNeedDelete(false), mSkipFirstRedo(true)
{
	setText(QObject::tr("Create Link"));
	//! 针对在命令的构造函数中就直接执行了创建或者删除动作的情况，
	//! 创建的命令mNeedDelete初始要为true，否则创建此命令，但没推入stack就会出现内存泄露
	//! 反之亦然，删除的命令，needdelete应该为false
	//! 但这里不一样
	//! 这里的mlinkItem是外面传入的，并非在构造函数创建，因此初始mNeedDelete = false
	mNeedDelete    = false;
	mFromitem      = linkitem->fromNodeItem();
	mFromPointName = linkitem->fromNodeLinkPoint().name;
	mToitem        = linkitem->toNodeItem();
	mToPointName   = linkitem->toNodeLinkPoint().name;
	if (mLinkitem->scene() != mScene) {
		mScene->addItem(mLinkitem);
	}
}

DACommandsForWorkFlowCreateLink::~DACommandsForWorkFlowCreateLink()
{
	if (mNeedDelete) {
		delete mLinkitem;
	}
}

void DACommandsForWorkFlowCreateLink::redo()
{
	QUndoCommand::redo();  // 此函数会执行子内容的redo/undo
	mNeedDelete = false;
	if (mSkipFirstRedo) {
		mSkipFirstRedo = false;
		return;
	}
	mLinkitem->attachFrom(mFromitem, mFromPointName);
	mLinkitem->attachTo(mToitem, mToPointName);
	mScene->addItem(mLinkitem);
}

void DACommandsForWorkFlowCreateLink::undo()
{
	QUndoCommand::undo();  // 此函数会执行子内容的redo/undo
	mNeedDelete = true;
	mScene->removeItem(mLinkitem);
	mLinkitem->detachFrom();
	mLinkitem->detachTo();
}

//==============================================================
// DACommandsForWorkFlowRemoveLink
//==============================================================

/**
 * @brief DACommandsForWorkFlowRemoveLink::DACommandsForWorkFlowRemoveLink
 *
 * 此类在@sa DACommandsForWorkFlowRemoveSelectNodes 中调用
 * @param linkitem
 * @param sc
 * @param parent
 */
DACommandsForWorkFlowRemoveLink::DACommandsForWorkFlowRemoveLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mLinkitem(linkitem), mScene(sc), mNeedDelete(false)
{
	setText(QObject::tr("Remove Link"));
	//! 针对在命令的构造函数中就直接执行了创建或者删除动作的情况，
	//! 创建的命令mNeedDelete初始要为true，否则创建此命令，但没推入stack就会出现内存泄露
	//! 反之亦然，删除的命令，needdelete应该为false
	mNeedDelete    = false;
	mFromitem      = linkitem->fromNodeItem();
	mFromPointName = linkitem->fromNodeLinkPoint().name;
	mToitem        = linkitem->toNodeItem();
	mToPointName   = linkitem->toNodeLinkPoint().name;
}

DACommandsForWorkFlowRemoveLink::~DACommandsForWorkFlowRemoveLink()
{
	if (mNeedDelete) {
		delete mLinkitem;
	}
}

void DACommandsForWorkFlowRemoveLink::redo()
{
	QUndoCommand::redo();  // 此函数会执行子内容的redo/undo
	mScene->removeItem(mLinkitem);
	mLinkitem->detachFrom();
	mLinkitem->detachTo();
	mNeedDelete = true;
}

void DACommandsForWorkFlowRemoveLink::undo()
{
	QUndoCommand::undo();  // 此函数会执行子内容的redo/undo
	mLinkitem->attachFrom(mFromitem, mFromPointName);
	mLinkitem->attachTo(mToitem, mToPointName);
	mScene->addItem(mLinkitem);
	mNeedDelete = false;
}

}  // end DA
