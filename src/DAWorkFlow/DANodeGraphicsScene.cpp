#include "DANodeGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QScopedPointer>
#include <QPointer>
#include <QPainter>
#include "DAGraphicsResizeablePixmapItem.h"
#include "DAStandardNodeLinkGraphicsItem.h"
#include "DACommandsForGraphics.h"
#include "DACommandsForWorkFlowNodeGraphics.h"
#include "DAGraphicsResizeableTextItem.h"
#include "DAStandardGraphicsTextItem.h"
#include "DAAbstractNode.h"
#include "DAWorkFlow.h"
#include "DAGraphicsResizeableRectItem.h"
#include "DAStandardGraphicsTextItem.h"
namespace DA
{
class DANodeGraphicsScene::PrivateData
{
    DA_DECLARE_PUBLIC(DANodeGraphicsScene)
public:
    PrivateData(DANodeGraphicsScene* p);

public:
    bool mIsStartLink { false };  ///< 正在进行链接
    QScopedPointer< DAAbstractNodeLinkGraphicsItem > mLinkingItem;
    QPointer< DAWorkFlow > mWorkflow;
    QPointF mLastMousePressPos;  ///< 记录鼠标点击的位置
};

////////////////////////////////////////////////
///
////////////////////////////////////////////////
DANodeGraphicsScene::PrivateData::PrivateData(DANodeGraphicsScene* p) : q_ptr(p)
{
}

///////////////////////////////////

DANodeGraphicsScene::DANodeGraphicsScene(QObject* p) : DAGraphicsSceneWithUndoStack(p), DA_PIMPL_CONSTRUCT
{
    initConnect();
}

DANodeGraphicsScene::DANodeGraphicsScene(const QRectF& sceneRect, QObject* p)
    : DAGraphicsSceneWithUndoStack(sceneRect, p), DA_PIMPL_CONSTRUCT
{
    initConnect();
}

DANodeGraphicsScene::DANodeGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject* p)
    : DAGraphicsSceneWithUndoStack(x, y, width, height, p), DA_PIMPL_CONSTRUCT
{
    initConnect();
}

DANodeGraphicsScene::~DANodeGraphicsScene()
{
}

/**
 * @brief 判断当前是否正在进行链接
 * @return
 */
bool DANodeGraphicsScene::isStartLink() const
{
    return (d_ptr->mIsStartLink);
}

/**
 * @brief 取消链接，如果此时没在进行链接，不做处理
 */
void DANodeGraphicsScene::cancelLink()
{
    if (d_ptr->mIsStartLink) {
        //除开左键的所有按键都是取消
        DAAbstractNodeLinkGraphicsItem* linkItem = d_ptr->mLinkingItem.get();
        if (linkItem) {
            removeItem(linkItem);
        }
        DAAbstractNodeGraphicsItem* it = linkItem->fromNodeItem();
        if (it) {
            it->prepareLinkOutputFailed(linkItem->fromNodeLinkPoint());
        }
        it = linkItem->toNodeItem();
        if (it) {
            it->prepareLinkInputFailed(linkItem->toNodeLinkPoint(), linkItem);
        }
        d_ptr->mLinkingItem.reset();
        d_ptr->mIsStartLink = false;
    }
}

/**
 * @brief 判断是否这个item是正在连线的item
 *
 * 正在连线过程，会有一个item，这个item会被暂时加入scene，如果链接线失败，会把这个item删除
 *
 * 这个函数主要是在批量删除item时，判断item是否是正在链接的那个item
 * @param linkItem
 * @return
 */
bool DANodeGraphicsScene::isLinkingItem(DAAbstractNodeLinkGraphicsItem* linkItem) const
{
    return (linkItem == d_ptr->mLinkingItem.get());
}

/**
 * @brief 设置工作流
 * @param wf
 * @note DANodeGraphicsScene不负责管理DAWorkFlow的所有权
 */
void DANodeGraphicsScene::setWorkFlow(DAWorkFlow* wf)
{
    if (d_ptr->mWorkflow) {
        disconnect(d_ptr->mWorkflow.data(), &DAWorkFlow::nodeNameChanged, this, &DANodeGraphicsScene::onNodeNameChanged);
    }
    d_ptr->mWorkflow = wf;
    if (wf) {
        connect(wf, &DAWorkFlow::nodeNameChanged, this, &DANodeGraphicsScene::onNodeNameChanged);
    }
}

/**
 * @brief 获取工作流
 * @return
 */
DAWorkFlow* DANodeGraphicsScene::getWorkflow()
{
    return d_ptr->mWorkflow.data();
}

/**
 * @brief 通过node找到item
 * @param n
 * @return
 */
DAAbstractNodeGraphicsItem* DANodeGraphicsScene::findItemByNode(DAAbstractNode* n)
{
    QList< QGraphicsItem* > its = items();
    for (QGraphicsItem* i : qAsConst(its)) {
        DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i);
        if (ni) {
            if (ni->rawNode() == n) {
                return ni;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 获取选中的NodeGraphicsItem,如果没有返回nullptr，如果选中多个，返回第一个
 * @return
 */
DAAbstractNodeGraphicsItem* DANodeGraphicsScene::getSelectedNodeGraphicsItem() const
{
    QList< QGraphicsItem* > sits = selectedItems();
    for (QGraphicsItem* i : qAsConst(sits)) {
        if (DAAbstractNodeGraphicsItem* gi = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
            return gi;
        }
    }
    return nullptr;
}

/**
 * @brief 获取所有的NodeGraphicsItems
 * @return
 */
QList< DAAbstractNodeGraphicsItem* > DANodeGraphicsScene::getNodeGraphicsItems() const
{
    QList< DAAbstractNodeGraphicsItem* > res;
    QList< QGraphicsItem* > its = items();
    for (QGraphicsItem* i : qAsConst(its)) {
        DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i);
        if (ni) {
            res.append(ni);
        }
    }
    return res;
}

/**
 * @brief 获取所有的文本
 * @return
 * @sa createText_
 */
QList< DAStandardGraphicsTextItem* > DANodeGraphicsScene::getTextGraphicsItems() const
{
    QList< DAStandardGraphicsTextItem* > res;
    QList< QGraphicsItem* > its = items();
    for (QGraphicsItem* i : qAsConst(its)) {
        DAStandardGraphicsTextItem* ni = dynamic_cast< DAStandardGraphicsTextItem* >(i);
        if (ni) {
            res.append(ni);
        }
    }
    return res;
}

/**
 * @brief 获取选中的NodeGraphicsItem,如果没有返回一个空list
 * @return
 */
QList< DAAbstractNodeGraphicsItem* > DANodeGraphicsScene::getSelectedNodeGraphicsItems() const
{
    QList< DAAbstractNodeGraphicsItem* > res;
    QList< QGraphicsItem* > sits = selectedItems();
    for (QGraphicsItem* i : qAsConst(sits)) {
        if (DAAbstractNodeGraphicsItem* gi = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
            res.append(gi);
        }
    }
    return res;
}
/**
 * @brief 获取选中的NodeLinkGraphicsItem,如果没有返回nullptr，如果选中多个，返回第一个
 * @return
 */
DAAbstractNodeLinkGraphicsItem* DANodeGraphicsScene::getSelectedNodeLinkGraphicsItem() const
{
    QList< QGraphicsItem* > sits = selectedItems();
    for (QGraphicsItem* i : qAsConst(sits)) {
        if (DAAbstractNodeLinkGraphicsItem* gi = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(i)) {
            return gi;
        }
    }
    return nullptr;
}

/**
 * @brief 获取除了连接线以外的item
 * @return
 */
QList< QGraphicsItem* > DANodeGraphicsScene::getGraphicsItemsWithoutLink() const
{
    QList< QGraphicsItem* > res;
    QList< QGraphicsItem* > ites = items();
    for (QGraphicsItem* i : qAsConst(ites)) {
        if (nullptr == dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(i)) {
            res.append(i);
        }
    }
    return res;
}

/**
 * @brief 删除选中的item，此函数支持redo/undo
 *
 * @return 返回删除的数量，如果没有删除，返回0
 *
 * @note 有些连带删除，例如选择了一个node，它有连线，删除节点会把连线也删除，这时候返回的删除结果会比较多
 */
int DANodeGraphicsScene::removeSelectedItems_()
{
    //此操作比较复杂，首先要找到非node的item，这些直接就可以删除，然后找到nodeitem，先删除link，再删除node
    // QGraphicsItem不进行处理,因此普通的item需要调用addItem_而不是addItem
    //需要先找到节点下面所以得link，和选中的link进行分组
    //! 注意： 如果处于链接状态（isStartLink() == true）那么不能删除正在链接的那个元素的节点，否则会导致异常
    qDebug() << "removeSelectedItems_";
    //移除元素过程中，先要删除链接
    cancelLink();
    QScopedPointer< DA::DACommandsForWorkFlowRemoveSelectNodes > cmd(new DA::DACommandsForWorkFlowRemoveSelectNodes(this));
    int rc = cmd->removeCount();
    if (!cmd->isValid()) {
        qDebug() << "remove select is invalid";
        return 0;
    }
    DA::DACommandsForWorkFlowRemoveSelectNodes* rawcmd = cmd.take();
    push(rawcmd);
    QList< DAAbstractNodeGraphicsItem* > rn = rawcmd->getRemovedNodeItems();
    if (rn.size() > 0) {
        emit nodeItemsRemoved(rn);
    }
    QList< DAAbstractNodeLinkGraphicsItem* > rl = rawcmd->getRemovedNodeLinkItems();
    if (rl.size() > 0) {
        emit nodeLinksRemoved(rl);
    }
    QList< QGraphicsItem* > gl = rawcmd->getRemovedItems();
    if (gl.size() > 0) {
        emit itemRemoved(gl);
    }
    return rc;
}

/**
 * @brief 创建节点（不带回退功能）
 * @note 注意，节点会记录在工作流中,如果返回的是nullptr，则不会记录
 * @param md
 * @param pos
 * @return
 */
DAAbstractNodeGraphicsItem* DANodeGraphicsScene::createNode(const DANodeMetaData& md, const QPointF& pos)
{
    if (nullptr == d_ptr->mWorkflow) {
        return nullptr;
    }
    DAAbstractNode::SharedPointer n = d_ptr->mWorkflow->createNode(md);

    DAAbstractNodeGraphicsItem* nodeitem = nullptr;
    if (nullptr == n) {
        qCritical() << QObject::tr("can not create node,metadata name=%1(%2)").arg(md.getNodeName(), md.getNodePrototype());
    } else {
        nodeitem = n->createGraphicsItem();
        if (nodeitem) {
            nodeitem->setPos(pos);
            d_ptr->mWorkflow->addNode(n);
        }
    }
    return nodeitem;
}

/**
 * @brief 通过node元对象创建工作流节点
 * @param md
 * @return 如果创建失败，返回nullptr
 */
DAAbstractNodeGraphicsItem* DANodeGraphicsScene::createNode_(const DANodeMetaData& md, const QPointF& pos)
{
    QScopedPointer< DA::DACommandsForWorkFlowCreateNode > cmd(new DA::DACommandsForWorkFlowCreateNode(md, this, pos));
    DAAbstractNodeGraphicsItem* item = cmd->item();
    if (item == nullptr) {
        return nullptr;
    }
    push(cmd.take());
    return item;
}

/**
 * @brief 创建并加入一个文本框
 * @param pos
 * @return
 * @sa getTextGraphicsItems
 */
DAStandardGraphicsTextItem* DANodeGraphicsScene::createText_(const QString& str)
{
    DAStandardGraphicsTextItem* item = new DAStandardGraphicsTextItem();
    if (!str.isEmpty()) {
        item->setPlainText(str);
    }
    addItem_(item);
    return (item);
}

/**
 * @brief 在画布中创建一个矩形
 * @param p 矩形的位置
 * @return
 */
DAGraphicsResizeableRectItem* DANodeGraphicsScene::createRect_(const QPointF& p)
{
    DAGraphicsResizeableRectItem* item = new DAGraphicsResizeableRectItem();
    addItem_(item);
    if (!p.isNull()) {
        item->setPos(p);
    }
    return (item);
}

void DANodeGraphicsScene::initConnect()
{
    qRegisterMetaType< DANodeLinkPoint >("DANodeLinkPoint");
    connect(this, &DANodeGraphicsScene::nodeItemLinkPointSelected, this, &DANodeGraphicsScene::onNodeItemLinkPointSelected);
    connect(this, &QGraphicsScene::selectionChanged, this, &DANodeGraphicsScene::onItemSelectionChanged);
}

/**
 * @brief itemlink都没用节点连接时会调用这个函数，发出
 * @param link
 */
void DANodeGraphicsScene::callNodeItemLinkIsEmpty(DAAbstractNodeLinkGraphicsItem* link)
{
    removeItem(link);
    emit nodeLinkItemIsEmpty(link);
}

/**
 * @brief 这是链接点选中的信号
 * @param item
 * @param lp
 * @param event
 */
void DANodeGraphicsScene::onNodeItemLinkPointSelected(DA::DAAbstractNodeGraphicsItem* item,
                                                      const DA::DANodeLinkPoint& lp,
                                                      QGraphicsSceneMouseEvent* event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        if (isStartLink()) {
            //说明此时处于开始连接状态，接收到点击需要判断是否接受
            if (lp.isOutput() && d_ptr->mLinkingItem.isNull()) {
                //连接的结束从in结束，out就退出
                event->ignore();
                return;
            }
            //此时连接到to点
            if (!(d_ptr->mLinkingItem->attachTo(item, lp))) {
                //连接失败
                return;
            }
            //连接成功，把item脱离管理
            DAAbstractNodeLinkGraphicsItem* linkitem = d_ptr->mLinkingItem.take();
            //通知item链接入口成功了
            linkitem->updateBoundingRect();
            //由于开始链接的的时候已经additem了，因此，这个cmd的第三个参数设置为false，也就是第一次执行redo不用additem
            DA::DACommandsForWorkFlowCreateLink* cmd = new DA::DACommandsForWorkFlowCreateLink(linkitem, this, true);
            push(cmd);
            d_ptr->mIsStartLink = false;
            // qDebug() << "linkingItem success attach to " << item->rawNode()->getNodeName();
        } else {
            //说明此时处于正常状态，判断是否开始连线
            if (lp.isInput()) {
                //连接的开始从out开始，in就退出
                event->ignore();
                return;
            }
            //此时说明开始进行连线
            d_ptr->mIsStartLink = true;
            d_ptr->mLinkingItem.reset(item->createLinkItem(lp));
            if (nullptr == d_ptr->mLinkingItem) {
                //连接不成功
                //通知item链接出口的时候失败了
                d_ptr->mIsStartLink = false;
                d_ptr->mLinkingItem.reset();
                return;
            }
            if (!(d_ptr->mLinkingItem->attachFrom(item, lp))) {
                d_ptr->mIsStartLink = false;
                d_ptr->mLinkingItem.reset();
                return;
            }
            //把item加入
            //这里不能直接用addItem_，因为item的操作要反作用节点
            addItem(d_ptr->mLinkingItem.get());
            d_ptr->mLinkingItem->setPos(item->mapToScene(lp.position));
            d_ptr->mLinkingItem->updateBoundingRect();
        }
    } else {
        //除开左键的所有按键都是取消
        cancelLink();
    }
}

/**
 * @brief item选择改变
 */
void DANodeGraphicsScene::onItemSelectionChanged()
{
    QList< QGraphicsItem* > sits = selectedItems();
    for (QGraphicsItem* i : qAsConst(sits)) {
        if (DAAbstractNodeGraphicsItem* gi = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
            emit selectNodeItemChanged(gi);
        } else if (DAAbstractNodeLinkGraphicsItem* gi = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(i)) {
            emit selectNodeLinkChanged(gi);
        }
    }
}

/**
 * @brief 节点名字改变触发的槽
 * @param node
 * @param oldName
 * @param newName
 */
void DANodeGraphicsScene::onNodeNameChanged(DAAbstractNode::SharedPointer node, const QString& oldName, const QString& newName)
{
    //查找对应的item并让其改变文字
    Q_UNUSED(oldName);
    DAAbstractNodeGraphicsItem* it = findItemByNode(node.get());
    if (it) {
        it->prepareNodeNameChanged(newName);
    }
}

void DANodeGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    d_ptr->mLastMousePressPos = mouseEvent->scenePos();
    if (mouseEvent->isAccepted()) {
        //说明上级已经接受了鼠标事件，这里不应该处理
        if (isStartLink()) {
            cancelLink();
        }
        DAGraphicsSceneWithUndoStack::mousePressEvent(mouseEvent);
        return;
    }
    if (!mouseEvent->buttons().testFlag(Qt::LeftButton)) {
        //除开左键的所有按键都是取消
        if (isStartLink()) {
            cancelLink();
        }
    } else {
        //左键点击
        QGraphicsItem* positem = itemAt(d_ptr->mLastMousePressPos, QTransform());
        if (nullptr == positem) {
            DAGraphicsSceneWithUndoStack::mousePressEvent(mouseEvent);
            return;
        }
        //看看是否点击到了节点item
        DAAbstractNodeGraphicsItem* nodeItem = dynamic_cast< DAAbstractNodeGraphicsItem* >(positem);
        if (nullptr == nodeItem) {
            //点击的不是节点item就退出
            DAGraphicsSceneWithUndoStack::mousePressEvent(mouseEvent);
            return;
        }
        //如果点击到了DAAbstractNodeGraphicsItem，要看看是否点击到了连接点
        QPointF itempos = nodeItem->mapFromScene(d_ptr->mLastMousePressPos);
        if (isStartLink()) {
            //开始链接状态，此时点击的是input
            nodeItem->prepareLinkInput(itempos, d_ptr->mLinkingItem.get());
        } else {
            //非开始链接状态，此时点击的是output
            nodeItem->prepareLinkOutput(itempos);
        }
        DANodeLinkPoint lp = nodeItem->getLinkPointByPos(itempos);
        if (!lp.isValid()) {
            //说明没有点击到连接点，正常传递到DAGraphicsSceneWithUndoStack
            DAGraphicsSceneWithUndoStack::mousePressEvent(mouseEvent);
            return;
        }
        emit nodeItemLinkPointSelected(nodeItem, lp, mouseEvent);
        return;
    }
    DAGraphicsSceneWithUndoStack::mousePressEvent(mouseEvent);
}

void DANodeGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (isStartLink() && !(d_ptr->mLinkingItem.isNull())) {
        //此时正值连接中，把鼠标的位置发送到link中
        d_ptr->mLinkingItem->updateBoundingRect();
    }
    DAGraphicsSceneWithUndoStack::mouseMoveEvent(mouseEvent);
}

void DANodeGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    DAGraphicsSceneWithUndoStack::mouseReleaseEvent(mouseEvent);
}
}  // end DA
