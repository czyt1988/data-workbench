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
#include "DAQtContainerUtil.h"
namespace DA
{

//==============================================================
// DACommandsForWorkFlowCreateNode
//==============================================================

DACommandsForWorkFlowCreateNode::DACommandsForWorkFlowCreateNode(const DANodeMetaData& md,
                                                                 DANodeGraphicsScene* scene,
                                                                 const QPointF& pos,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), mScene(scene), mScenePos(pos), mItem(nullptr), mNeedDelete(true)
{
    setText(QObject::tr("Create Node"));
    //! 针对在命令的构造函数中就直接执行了创建或者删除动作的情况，
    //! 创建的命令mNeedDelete初始要为true，否则创建此命令，但没推入stack就会出现内存泄露
    //! 反之亦然，删除的命令，needdelete应该为false
    mNeedDelete = true;
    mMetadata   = md;
    mItem       = scene->createNode(md, pos);
    if (mItem) {
        mNode = mItem->node();
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
    QUndoCommand::redo();  // 此函数会执行子内容的redo/undo
    mNeedDelete = false;   // 标记要delete为false
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
}

void DACommandsForWorkFlowCreateNode::undo()
{
    QUndoCommand::undo();  // 此函数会执行子内容的redo/undo
    mNeedDelete = true;
    if (mNode) {
        DAWorkFlow* wf = mScene->getWorkflow();
        wf->removeNode(mNode);
    }
    if (mItem) {
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

//==============================================================
// DACommandsForWorkFlowRemoveNode
//==============================================================
DACommandsForWorkFlowRemoveSelectNodes::DACommandsForWorkFlowRemoveSelectNodes(DANodeGraphicsScene* scene, QUndoCommand* parent)
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

QList< DAAbstractNodeLinkGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getNodesLinks(const QList< DAAbstractNodeGraphicsItem* >& nodeItems)
{
    QList< DAAbstractNodeLinkGraphicsItem* > res;
    for (DAAbstractNodeGraphicsItem* n : qAsConst(nodeItems)) {
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
 * @brief 获取移除的QGraphicsItem,此函数构造后即可调用
 * @return
 */
QList< QGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getRemovedItems() const
{
    return mWillRemoveNormal;
}

void DACommandsForWorkFlowRemoveSelectNodes::redo()
{
    // 注意QUndoCommand::redo()要放到最前，QUndoCommand::undo()要放到最后
    mNeedDelete = true;
    QUndoCommand::redo();  // 此函数会执行子内容的redo/undo,也就是先删除link
    DAWorkFlow* wf = mScene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : qAsConst(mSelectNodeItems)) {
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
    for (QGraphicsItem* item : qAsConst(mWillRemoveNormal)) {
        mScene->removeItem(item);
    }
}

void DACommandsForWorkFlowRemoveSelectNodes::undo()
{
    mNeedDelete    = false;
    DAWorkFlow* wf = mScene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : qAsConst(mSelectNodeItems)) {
        mScene->addItem(item);
        if (wf) {
            // 因为mWillRemoveNodes保留了节点，item里的weakpoint能获取到智能指针
            wf->addNode(item->node());
        }
    }
    for (QGraphicsItem* item : qAsConst(mWillRemoveNormal)) {
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
