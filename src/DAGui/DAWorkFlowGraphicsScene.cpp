#include "DAWorkFlowGraphicsScene.h"
// Qt
#include <QApplication>
#include <QMatrix>
#include <QKeyEvent>
#include <QDebug>
#include <QGraphicsSceneDragDropEvent>
#include <DAStandardGraphicsTextItem.h>
// workflow
#include "DAAbstractNode.h"
#include "DANodeMimeData.h"
#include "DANodeMetaData.h"
#include "DAGraphicsResizeablePixmapItem.h"
#include "DAGraphicsResizeableRectItem.h"
#include "DAGraphicsResizeableTextItem.h"
#include "DACommandsForWorkFlow.h"

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DAWorkFlowGraphicsScene
//===================================================
DAWorkFlowGraphicsScene::DAWorkFlowGraphicsScene(QObject* parent)
    : DANodeGraphicsScene(parent)
    , _backgroundPixmapItem(nullptr)
    , _mouseAction(NoMouseAction)
    , _isMouseActionContinuoue(false)
    , m_enableItemMoveWithBackground(false)
{
    m_textFont = QApplication::font();
}

DAWorkFlowGraphicsScene::~DAWorkFlowGraphicsScene()
{
    qDebug() << "destory DAWorkFlowGraphicsScene";
}

/**
 * @brief 添加一个背景图,如果多次调用，此函数返回的QGraphicsPixmapItem* 是一样的，也就是只会创建一个QGraphicsPixmapItem*
 * @param pixmap
 * @return
 */
DAGraphicsResizeablePixmapItem* DAWorkFlowGraphicsScene::setBackgroundPixmap(const QPixmap& pixmap)
{
    DACommandWorkFlowSceneAddBackgroundPixmap* cmd = new DACommandWorkFlowSceneAddBackgroundPixmap(this, pixmap);
    undoStack().push(cmd);
    return _backgroundPixmapItem;
}
/**
 * @brief 获取背景图item
 * @return 如果没有设置返回一个nullptr
 */
DAGraphicsResizeablePixmapItem* DAWorkFlowGraphicsScene::getBackgroundPixmapItem()
{
    if (nullptr == _backgroundPixmapItem) {
        return nullptr;
    }
    QList< QGraphicsItem* > its = items();
    for (const QGraphicsItem* i : its) {
        if (i == _backgroundPixmapItem) {
            return _backgroundPixmapItem;
        }
    }
    //在所有item中没有找到，说明_backgroundPixmapItem已经被删除，因此惰性赋值为nullptr
    _backgroundPixmapItem = nullptr;
    return nullptr;
}

/**
 * @brief 创建一个新的图片item
 *
 * @note 原来的并不会删除
 * @return
 */
DAGraphicsResizeablePixmapItem* DAWorkFlowGraphicsScene::createBackgroundPixmapItem()
{
    DAGraphicsResizeablePixmapItem* item = new DAGraphicsResizeablePixmapItem();
    setBackgroundPixmapItem(item);
    return item;
}

/**
 * @brief 设置鼠标动作
 *
 * 一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
 * @param mf 鼠标动作
 * @param continuous 是否连续执行
 */
void DAWorkFlowGraphicsScene::setMouseAction(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continuous)
{
    _mouseAction             = mf;
    _isMouseActionContinuoue = continuous;
}
/**
 * @brief 获取当前的鼠标动作标记
 * @return
 */
DAWorkFlowGraphicsScene::MouseActionFlag DAWorkFlowGraphicsScene::getMouseAction() const
{
    return _mouseAction;
}
/**
 * @brief 鼠标动作是否连续执行
 * @return
 */
bool DAWorkFlowGraphicsScene::isMouseActionContinuoue() const
{
    return _isMouseActionContinuoue;
}

/**
 * @brief 设置背景item，如果外部调用getBackgroundPixmapItem并删除，需要通过此函数把保存的item设置为null
 * @note 此函数不会对item执行additem操作，仅仅只是交由DAWorkFlowGraphicsScene保存
 * @param item
 */
void DAWorkFlowGraphicsScene::setBackgroundPixmapItem(DAGraphicsResizeablePixmapItem* item)
{
    _backgroundPixmapItem = item;
#if DA_USE_QGRAPHICSOBJECT
    connect(_backgroundPixmapItem, &DAGraphicsResizeablePixmapItem::xChanged, this, &DAWorkFlowGraphicsScene::backgroundPixmapItemXChanged);
    connect(_backgroundPixmapItem, &DAGraphicsResizeablePixmapItem::yChanged, this, &DAWorkFlowGraphicsScene::backgroundPixmapItemYChanged);
    connect(_backgroundPixmapItem, &DAGraphicsResizeablePixmapItem::itemBodySizeChanged, this, &DAWorkFlowGraphicsScene::backgroundPixmapItemBodyItemBodySizeChanged);
#endif
    item->setZValue(-9999);
    addItem(_backgroundPixmapItem);
#if DA_USE_QGRAPHICSOBJECT
    _backgroundPixmapItemLastPos = _backgroundPixmapItem->pos();
    _pixmapResizeLastPos         = _backgroundPixmapItem->pos();
#endif
}

/**
 * @brief 移出当前的背景item
 * @note 不会对item进行删除操作
 */
DAGraphicsResizeablePixmapItem* DAWorkFlowGraphicsScene::removeBackgroundPixmapItem()
{
#if DA_USE_QGRAPHICSOBJECT
    disconnect(_backgroundPixmapItem, &DAGraphicsResizeablePixmapItem::xChanged, this, &DAWorkFlowGraphicsScene::backgroundPixmapItemXChanged);
    disconnect(_backgroundPixmapItem, &DAGraphicsResizeablePixmapItem::yChanged, this, &DAWorkFlowGraphicsScene::backgroundPixmapItemYChanged);
    disconnect(_backgroundPixmapItem,
               &DAGraphicsResizeablePixmapItem::itemBodySizeChanged,
               this,
               &DAWorkFlowGraphicsScene::backgroundPixmapItemBodyItemBodySizeChanged);
#endif
    removeItem(_backgroundPixmapItem);
    _backgroundPixmapItem = nullptr;
    return _backgroundPixmapItem;
}

/**
 * @brief 允许item跟随背景图移动
 * @param on
 */
void DAWorkFlowGraphicsScene::enableItemMoveWithBackground(bool on)
{
    m_enableItemMoveWithBackground = on;
}

/**
 * @brief DAWorkFlowGraphicsScene::isEnableItemMoveWithBackground
 * @return
 */
bool DAWorkFlowGraphicsScene::isEnableItemMoveWithBackground() const
{
    return m_enableItemMoveWithBackground;
}

DAGraphicsResizeablePixmapItem* DAWorkFlowGraphicsScene::ensureGetBackgroundPixmapItem()
{
    if (nullptr == _backgroundPixmapItem) {
        createBackgroundPixmapItem();
    }
    return _backgroundPixmapItem;
}

void DAWorkFlowGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
        //说明有节点的meta数据拖入
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DAWorkFlowGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    event->acceptProposedAction();
}

void DAWorkFlowGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
{
    event->acceptProposedAction();
}

void DAWorkFlowGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
        //说明有节点的meta数据拖入
        const DANodeMimeData* nodemime = qobject_cast< const DANodeMimeData* >(event->mimeData());
        if (nullptr == nodemime) {
            return;
        }
        clearSelection();
        DANodeMetaData nodemeta = nodemime->getNodeMetaData();
        createNode_(nodemeta, event->scenePos());
    }
}

void DAWorkFlowGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (_mouseAction == NoMouseAction) {
        //说明没有鼠标动作要执行，正常传递鼠标事件
        DANodeGraphicsScene::mousePressEvent(mouseEvent);
        return;
    }
    //说明有鼠标事件要执行
    QPointF pos = mouseEvent->scenePos();
    switch (_mouseAction) {
    case StartAddText: {
        DAStandardGraphicsTextItem* item = createText_();
        item->setPos(pos);
        item->setFont(m_textFont);
        item->setDefaultTextColor(m_textColor);
        item->setEditMode(true);
        item->setFocus();
        emit mouseActionFinished(_mouseAction);
    } break;
    case StartAddRect: {
        DAGraphicsResizeableRectItem* item = createRect_(pos);
        item->setSelected(true);
        item->setZValue(-10);
        emit mouseActionFinished(_mouseAction);
    }
    default:
        break;
    }
    if (!_isMouseActionContinuoue) {
        _mouseAction = NoMouseAction;
    }
    mouseEvent->accept();  //接受事件，通知下面的mousePressEvent函数事件已经接收，无需执行动作
}

void DAWorkFlowGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    DANodeGraphicsScene::mouseReleaseEvent(mouseEvent);
}

#if DA_USE_QGRAPHICSOBJECT
void DAWorkFlowGraphicsScene::backgroundPixmapItemXChanged()
{
    if (!isEnableItemMoveWithBackground()) {
        _backgroundPixmapItemLastPos = _backgroundPixmapItem->pos();
        return;
    }
    QPointF newPos = _backgroundPixmapItem->pos();
    QPointF sub(newPos - _backgroundPixmapItemLastPos);
    //
    QList< QGraphicsItem* > itemsWithoutLink = getGraphicsItemsWithoutLink();

    for (QGraphicsItem* item : qAsConst(itemsWithoutLink)) {
        if (item == getBackgroundPixmapItem()) {
            // PixmapItem 也会获取到，避免递归
            continue;
        }
        if (nullptr != item->parentItem()) {
            continue;
        }
        QPointF tempPos = sub + item->pos();
        item->setPos(tempPos);
    }

    _backgroundPixmapItemLastPos = newPos;
}

void DAWorkFlowGraphicsScene::backgroundPixmapItemYChanged()
{
    if (!isEnableItemMoveWithBackground()) {
        _backgroundPixmapItemLastPos = _backgroundPixmapItem->pos();
        return;
    }
    QPointF newPos = _backgroundPixmapItem->pos();
    QPointF sub(newPos - _backgroundPixmapItemLastPos);
    QList< QGraphicsItem* > itemsWithoutLink = getGraphicsItemsWithoutLink();

    for (QGraphicsItem* item : qAsConst(itemsWithoutLink)) {
        if (item == getBackgroundPixmapItem()) {
            // PixmapItem 也会获取到，避免递归
            continue;
        }
        if (nullptr != item->parentItem()) {
            continue;
        }
        QPointF tempPos = sub + item->pos();
        item->setPos(tempPos);
    }
    _backgroundPixmapItemLastPos = newPos;
}

void DAWorkFlowGraphicsScene::backgroundPixmapItemBodyItemBodySizeChanged(const QSizeF& oldsize, const QSizeF& newsize)
{
    QPointF lastPost = _pixmapResizeLastPos;
    QPointF newPos   = _backgroundPixmapItem->pos();
    qDebug() << "oldPos=" << _backgroundPixmapItemLastPos << " newPos=" << newPos;
    qDebug() << "oldsize=" << oldsize << " newsize=" << newsize;

    //    QList< DAAbstractNodeGraphicsItem* > list = getNodeGraphicsItems();
    //    for (auto item : qAsConst(list)) {
    //        QPointF itemPos = item->pos();
    //        qDebug() << "itemPos=" << itemPos << " lastPost=" << _pixmapResizeLastPos;
    //        qreal xSub = 0;
    //        qreal ySub = 0;
    //        if (lastPost.x() < 0) {
    //            xSub = itemPos.x() + lastPost.x();
    //        } else {
    //            xSub = itemPos.x() - lastPost.x();
    //        }
    //        if (lastPost.x() < 0) {
    //            ySub = itemPos.y() + lastPost.y();
    //        } else {
    //            ySub = itemPos.y() - lastPost.y();
    //        }
    //        qreal dx = (xSub / oldsize.width()) * newsize.width();
    //        qreal dy = (ySub / oldsize.height()) * newsize.height();
    //        item->setPos(QPointF(dx, dy));
    //    }

    //    _pixmapResizeLastPos = newPos;
}

QColor DAWorkFlowGraphicsScene::getDefaultTextColor() const
{
    return m_textColor;
}

void DAWorkFlowGraphicsScene::setDefaultTextColor(const QColor& c)
{
    m_textColor = c;
}

QFont DAWorkFlowGraphicsScene::getDefaultTextFont() const
{
    return m_textFont;
}

void DAWorkFlowGraphicsScene::setDefaultTextFont(const QFont& f)
{
    m_textFont = f;
}
#endif
