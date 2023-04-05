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
    : QUndoCommand(parent), _scene(scene), _scenePos(pos), _item(nullptr), _needDelete(false)
{
    setText(QObject::tr("Create Node"));
    _metadata = md;
    _item     = scene->createNode(md, pos);
    _node     = _item->node();

    //    DAWorkFlow* wf = _scene->getWorkflow();
    //    _node          = wf->createNode(_metadata);
    //    if (nullptr == _node) {
    //        qCritical() << QObject::tr("can not create node,metadata name=%1(%2)").arg(_metadata.getNodeName(), _metadata.getNodePrototype());
    //    } else {
    //        _item = _node->createGraphicsItem();
    //        _item->setPos(pos);
    //    }
}

DACommandsForWorkFlowCreateNode::~DACommandsForWorkFlowCreateNode()
{
    if (_needDelete) {
        //_node是智能指针自动删除
        if (_item) {
            delete _item;
        }
    }
}

void DACommandsForWorkFlowCreateNode::redo()
{
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo
    if (_item) {
        _scene->addItem(_item);
    }
    if (_node) {
        DAWorkFlow* wf = _scene->getWorkflow();
        wf->addNode(_node);
    }
    _needDelete = false;
}

void DACommandsForWorkFlowCreateNode::undo()
{
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
    if (_node) {
        DAWorkFlow* wf = _scene->getWorkflow();
        wf->removeNode(_node);
    }
    if (_item) {
        _scene->removeItem(_item);
    }
    _needDelete = true;
}

DAAbstractNodeGraphicsItem* DACommandsForWorkFlowCreateNode::item() const
{
    return _item;
}

//==============================================================
// DACommandsForWorkFlowRemoveNode
//==============================================================
DACommandsForWorkFlowRemoveSelectNodes::DACommandsForWorkFlowRemoveSelectNodes(DANodeGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), _isvalid(false), _scene(scene), _needDelete(false)
{
    setText(QObject::tr("Remove Select Nodes"));
    classifyItems(scene, _selNodeItems, _willRemoveLink, _willRemoveNormal);
    QList< DAAbstractNodeLinkGraphicsItem* > nodeLinks = getNodesLinks(_selNodeItems);
    //这时候得到所有需要删除的link
    //也要做一次去重
    _willRemoveLink = nodeLinks + _willRemoveLink;
    _willRemoveLink = _willRemoveLink.toSet().toList();
    for (DAAbstractNodeLinkGraphicsItem* lk : qAsConst(_willRemoveLink)) {
        new DACommandsForWorkFlowRemoveLink(lk, scene, this);
    }
    _isvalid = (nodeLinks.size() > 0) || (_willRemoveLink.size() > 0) || _selNodeItems.size() > 0 || _willRemoveNormal.size() > 0;
}

DACommandsForWorkFlowRemoveSelectNodes::~DACommandsForWorkFlowRemoveSelectNodes()
{
    if (_needDelete) {
        for (DAAbstractNodeGraphicsItem* item : qAsConst(_selNodeItems)) {
            delete item;
        }
        for (QGraphicsItem* item : qAsConst(_willRemoveNormal)) {
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
    for (QGraphicsItem* i : qAsConst(its)) {
        if (DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
            nodeItems.append(ni);
        } else if (DAAbstractNodeLinkGraphicsItem* li = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(i)) {
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
    return _isvalid;
}
/**
 * @brief 获取移除的数量，此函数构造后即可调用
 * @return
 */
int DACommandsForWorkFlowRemoveSelectNodes::removeCount() const
{
    return _selNodeItems.size() + childCount() + _willRemoveNormal.size();
}

/**
 * @brief 获取选择的节点,此函数构造后即可调用
 * @return
 */
QList< DAAbstractNodeGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getRemovedNodeItems() const
{
    return _selNodeItems;
}
/**
 * @brief 获取移除的link,此函数构造后即可调用
 * @return
 */
QList< DAAbstractNodeLinkGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getRemovedNodeLinkItems() const
{
    return _willRemoveLink;
}
/**
 * @brief 获取移除的QGraphicsItem,此函数构造后即可调用
 * @return
 */
QList< QGraphicsItem* > DACommandsForWorkFlowRemoveSelectNodes::getRemovedItems() const
{
    return _willRemoveNormal;
}

void DACommandsForWorkFlowRemoveSelectNodes::redo()
{
    //注意QUndoCommand::redo()要放到最前，QUndoCommand::undo()要放到最后
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo,也就是先删除link
    DAWorkFlow* wf = _scene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : qAsConst(_selNodeItems)) {
        _scene->removeItem(item);
        if (wf) {
            wf->removeNode(item->node());
        }
    }
    for (QGraphicsItem* item : qAsConst(_willRemoveNormal)) {
        _scene->removeItem(item);
    }
    _needDelete = true;
}

void DACommandsForWorkFlowRemoveSelectNodes::undo()
{
    DAWorkFlow* wf = _scene->getWorkflow();
    for (DAAbstractNodeGraphicsItem* item : qAsConst(_selNodeItems)) {
        _scene->addItem(item);
        if (wf) {
            wf->addNode(item->node());
        }
    }
    for (QGraphicsItem* item : qAsConst(_willRemoveNormal)) {
        _scene->addItem(item);
    }
    _needDelete = false;
    //注意undo要放到最后
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
}

//==============================================================
// DACommandsForWorkFlowCreateLink
//==============================================================

DACommandsForWorkFlowCreateLink::DACommandsForWorkFlowCreateLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), _linkitem(linkitem), _scene(sc), _needDelete(false), _isFirstRedoNotAdditem(true), _skipFirstRedo(true)
{
    setText(QObject::tr("Create Link"));
    _fromitem      = linkitem->fromNodeItem();
    _fromPointName = linkitem->fromNodeLinkPoint().name;
    _toitem        = linkitem->toNodeItem();
    _toPointName   = linkitem->toNodeLinkPoint().name;
}

DACommandsForWorkFlowCreateLink::DACommandsForWorkFlowCreateLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 bool isFirstRedoNotAdditem,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), _linkitem(linkitem), _scene(sc), _needDelete(false), _isFirstRedoNotAdditem(isFirstRedoNotAdditem)
{
    setText(QObject::tr("Create Link"));
    _fromitem      = linkitem->fromNodeItem();
    _fromPointName = linkitem->fromNodeLinkPoint().name;
    _toitem        = linkitem->toNodeItem();
    _toPointName   = linkitem->toNodeLinkPoint().name;
}

DACommandsForWorkFlowCreateLink::~DACommandsForWorkFlowCreateLink()
{
    if (_needDelete) {
        delete _linkitem;
    }
}

void DACommandsForWorkFlowCreateLink::redo()
{
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo
    if (_skipFirstRedo) {
        _skipFirstRedo = false;
        if (_isFirstRedoNotAdditem) {
            _isFirstRedoNotAdditem = false;
        }
        return;
    }
    if (_isFirstRedoNotAdditem) {
        //此参数为true说明第一次调用不用添加item，后面都不管
        _isFirstRedoNotAdditem = false;
        return;
    }
    _linkitem->attachFrom(_fromitem, _fromPointName);
    _linkitem->attachTo(_toitem, _toPointName);
    _scene->addItem(_linkitem);
    _needDelete = false;
}

void DACommandsForWorkFlowCreateLink::undo()
{
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
    _scene->removeItem(_linkitem);
    _linkitem->detachFrom();
    _linkitem->detachTo();
    _needDelete = true;
}

//==============================================================
// DACommandsForWorkFlowRemoveLink
//==============================================================

DACommandsForWorkFlowRemoveLink::DACommandsForWorkFlowRemoveLink(DAAbstractNodeLinkGraphicsItem* linkitem,
                                                                 DANodeGraphicsScene* sc,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent), _linkitem(linkitem), _scene(sc), _needDelete(false)
{
    setText(QObject::tr("Remove Link"));
    _fromitem      = linkitem->fromNodeItem();
    _fromPointName = linkitem->fromNodeLinkPoint().name;
    _toitem        = linkitem->toNodeItem();
    _toPointName   = linkitem->toNodeLinkPoint().name;
}

DACommandsForWorkFlowRemoveLink::~DACommandsForWorkFlowRemoveLink()
{
    if (_needDelete) {
        delete _linkitem;
    }
}

void DACommandsForWorkFlowRemoveLink::redo()
{
    QUndoCommand::redo();  //此函数会执行子内容的redo/undo
    _scene->removeItem(_linkitem);
    _linkitem->detachFrom();
    _linkitem->detachTo();
    _needDelete = true;
}

void DACommandsForWorkFlowRemoveLink::undo()
{
    QUndoCommand::undo();  //此函数会执行子内容的redo/undo
    _linkitem->attachFrom(_fromitem, _fromPointName);
    _linkitem->attachTo(_toitem, _toPointName);
    _scene->addItem(_linkitem);
    _needDelete = false;
}

}  // end DA
