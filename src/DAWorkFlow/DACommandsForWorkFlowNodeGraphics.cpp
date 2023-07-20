#include "DACommandsForWorkFlowNodeGraphics.h"
// qt
#include <QObject>
// workflow
#include "DANodeGraphicsScene.h"
#include "DAGraphicsResizeablePixmapItem.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAWorkFlow.h"
#include "DAGraphicsItem.h"
#include "DAStandardGraphicsTextItem.h"
namespace DA
{

//==============================================================
// DACommandsForWorkFlowCreateNode
//==============================================================

DACommandsForWorkFlowCreateNode::DACommandsForWorkFlowCreateNode(const DANodeMetaData& md,
                                                                 DANodeGraphicsScene* scene,
                                                                 const QPointF& pos,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mScene(scene), mScenePos(pos), mItem(nullptr), mNeedDelete(false)
{
    setText(QObject::tr("Create Node"));
    mMetadata = md;
    mItem     = scene->createNode(md, pos);
    mNode     = mItem->node();
    if (mItem) {
        mScene->addItem(mItem);
    }
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
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo
    if (mSkipFirstRedo) {
        //! 关键:第一次执行要跳过redo，否则会重复添加节点
        mSkipFirstRedo = false;
        return;
    }
    if (mItem) {
        mScene->addItem(mItem);
    }
    if (mNode) {
        DAWorkFlow* wf = mScene->getWorkflow();
        wf->addNode(mNode);
    }
    mNeedDelete = false;
}

void DACommandsForWorkFlowCreateNode::undo()
{
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
    if (mNode) {
        DAWorkFlow* wf = mScene->getWorkflow();
        wf->removeNode(mNode);
    }
    if (mItem) {
        mScene->removeItem(mItem);
    }
    mNeedDelete = true;
}

DAAbstractNodeGraphicsItem* DACommandsForWorkFlowCreateNode::item() const
{
    return mItem;
}

//==============================================================
// DACommandsForWorkFlowRemoveNode
//==============================================================
DACommandsForWorkFlowRemoveSelectNodes::DACommandsForWorkFlowRemoveSelectNodes(DANodeGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), mIsvalid(false), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Remove Select Nodes"));
    classifyItems(scene, mSelectNodeItems, mWillRemoveLink, mWillRemoveNormal);
    QList< DAAbstractNodeLinkGraphicsItem* > nodeLinks = getNodesLinks(mSelectNodeItems);
    //这时候得到所有需要删除的link
    //也要做一次去重
    mWillRemoveLink = nodeLinks + mWillRemoveLink;
    mWillRemoveLink = mWillRemoveLink.toSet().toList();
    for (DAAbstractNodeLinkGraphicsItem* lk : qAsConst(mWillRemoveLink)) {
        new DACommandsForWorkFlowRemoveLink(lk, scene, this);
    }
    mIsvalid = (nodeLinks.size() > 0) || (mWillRemoveLink.size() > 0) || mSelectNodeItems.size() > 0
               || mWillRemoveNormal.size() > 0;
}

DACommandsForWorkFlowRemoveSelectNodes::~DACommandsForWorkFlowRemoveSelectNodes()
{
    if (mNeedDelete) {
        for (DAAbstractNodeGraphicsItem* item : qAsConst(mSelectNodeItems)) {
            delete item;
        }
        for (QGraphicsItem* item : qAsConst(mWillRemoveNormal)) {
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
    for (QGraphicsItem* i : qAsConst(its)) {
        if (DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
            nodeItems.append(ni);
        } else if (DAAbstractNodeLinkGraphicsItem* li = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(i)) {
            if (isStartLink) {
                if (scene->isLinkingItem(li)) {
                    //连接线要跳过
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

QList< DAAbstractNodeLinkGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getNodesLinks(const QList< DAAbstractNodeGraphicsItem* >& nodeItems)
{
    QList< DAAbstractNodeLinkGraphicsItem* > res;
    for (DAAbstractNodeGraphicsItem* n : qAsConst(nodeItems)) {
        res += n->getLinkItems();
    }
    return res.toSet().toList();
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
 * @brief 获取移除的QGraphicsItem,此函数构造后即可调用
 * @return
 */
QList< QGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getRemovedItems() const
{
    return mWillRemoveNormal;
}

void DACommandsForWorkFlowRemoveSelectNodes::redo()
{
    //注意QUndoCommand::redo()要放到最前，QUndoCommand::undo()要放到最后
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo,也就是先删除link
    DAWorkFlow* wf = mScene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : qAsConst(mSelectNodeItems)) {
        mScene->removeItem(item);
        if (wf) {
            wf->removeNode(item->node());
        }
    }
    for (QGraphicsItem* item : qAsConst(mWillRemoveNormal)) {
        mScene->removeItem(item);
    }
    mNeedDelete = true;
}

void DACommandsForWorkFlowRemoveSelectNodes::undo()
{
    DAWorkFlow* wf = mScene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : qAsConst(mSelectNodeItems)) {
        mScene->addItem(item);
        if (wf) {
            wf->addNode(item->node());
        }
    }
    for (QGraphicsItem* item : qAsConst(mWillRemoveNormal)) {
        mScene->addItem(item);
    }
    mNeedDelete = false;
    //注意undo要放到最后
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
}

//==============================================================
// DACommandsForWorkFlowCreateLink
//==============================================================

DACommandsForWorkFlowCreateLink::DACommandsForWorkFlowCreateLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mLinkitem(linkitem), mScene(sc), mNeedDelete(false), mIsFirstRedoNotAdditem(true), mSkipFirstRedo(true)
{
    setText(QObject::tr("Create Link"));
    mFromitem      = linkitem->fromNodeItem();
    mFromPointName = linkitem->fromNodeLinkPoint().name;
    mToitem        = linkitem->toNodeItem();
    mToPointName   = linkitem->toNodeLinkPoint().name;
}

DACommandsForWorkFlowCreateLink::DACommandsForWorkFlowCreateLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 bool isFirstRedoNotAdditem,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mLinkitem(linkitem), mScene(sc), mNeedDelete(false), mIsFirstRedoNotAdditem(isFirstRedoNotAdditem)
{
    setText(QObject::tr("Create Link"));
    mFromitem      = linkitem->fromNodeItem();
    mFromPointName = linkitem->fromNodeLinkPoint().name;
    mToitem        = linkitem->toNodeItem();
    mToPointName   = linkitem->toNodeLinkPoint().name;
}

DACommandsForWorkFlowCreateLink::~DACommandsForWorkFlowCreateLink()
{
    if (mNeedDelete) {
        delete mLinkitem;
    }
}

void DACommandsForWorkFlowCreateLink::redo()
{
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo
    if (mSkipFirstRedo) {
        mSkipFirstRedo = false;
        if (mIsFirstRedoNotAdditem) {
            mIsFirstRedoNotAdditem = false;
        }
        return;
    }
    if (mIsFirstRedoNotAdditem) {
        //此参数为true说明第一次调用不用添加item，后面都不管
        mIsFirstRedoNotAdditem = false;
        return;
    }
    mLinkitem->attachFrom(mFromitem, mFromPointName);
    mLinkitem->attachTo(mToitem, mToPointName);
    mScene->addItem(mLinkitem);
    mNeedDelete = false;
}

void DACommandsForWorkFlowCreateLink::undo()
{
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
    mScene->removeItem(mLinkitem);
    mLinkitem->detachFrom();
    mLinkitem->detachTo();
    mNeedDelete = true;
}

//==============================================================
// DACommandsForWorkFlowRemoveLink
//==============================================================

DACommandsForWorkFlowRemoveLink::DACommandsForWorkFlowRemoveLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mLinkitem(linkitem), mScene(sc), mNeedDelete(false)
{
    setText(QObject::tr("Remove Link"));
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
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo
    mScene->removeItem(mLinkitem);
    mLinkitem->detachFrom();
    mLinkitem->detachTo();
    mNeedDelete = true;
}

void DACommandsForWorkFlowRemoveLink::undo()
{
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
    mLinkitem->attachFrom(mFromitem, mFromPointName);
    mLinkitem->attachTo(mToitem, mToPointName);
    mScene->addItem(mLinkitem);
    mNeedDelete = false;
}

}  // end DA
