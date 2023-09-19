#include "DAGraphicsSceneWithUndoStack.h"
#include <QGraphicsSceneMouseEvent>
#include "DACommandsForGraphics.h"
#include "DAGraphicsResizeableItem.h"
#include <QPainter>
#include <QDebug>

namespace DA
{

class _DANodeGraphicsSceneItemMoveingInfos
{
public:
    QList< QGraphicsItem* > items;
    QList< QPointF > startsPos;
    QList< QPointF > endsPos;
    //添加开始的位置
    void appendStartPos(QGraphicsItem* item, const QPointF& start);
    void appendStartPos(QGraphicsItem* item);
    //刷新结束位置
    void updateEndPos();
    //清除信息
    void clear();
};

//===================================================
// DAGraphicsSceneWithUndoStack::PrivateData
//===================================================

class DAGraphicsSceneWithUndoStack::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsSceneWithUndoStack)
public:
    PrivateData(DAGraphicsSceneWithUndoStack* p);
    //绘制背景缓存
    void renderBackgroundCache();
    void renderBackgroundCache(QPainter* painter, const QRect& rect);

public:
    QUndoStack mUndoStack;
    bool mIsMovingItems { false };  /// 正在进行item的移动
    QPointF mLastMouseScenePos;
    QPointF mLastMousePressScenePos;                    ///< 记录点击时的位置
    _DANodeGraphicsSceneItemMoveingInfos mMovingInfos;  ///< 记录移动的信息
    bool mEnableSnapToGrid { false };                   ///< 允许对齐网格
    bool mShowGridLine { true };                        ///< 是否显示网格
    QSize mGridSize { 10, 10 };                         ///< 网格大小
    QPen mGridLinePen;                                  ///< 绘制网格的画笔
    QPixmap mBackgroundCache;                           ///< 背景缓存，不用每次都绘制
    bool mIsPaintBackgroundInCache { false };           ///< 背景使用缓冲绘制
    std::unique_ptr< DAGraphicsLinkItem > mLinkItem;    ///< 连接线
    DAGraphicsSceneWithUndoStack::LinkMode mLinkMode { DAGraphicsSceneWithUndoStack::LinkModeAutoStartEndFollowMouseClick };  ///< 当前链接模式记录
    ///< 标记连接线移动过，这个变量是在LinkModeAutoStartEndFollowMouseClick模式下，
    /// 用户调用beginLink函数后，有可能接下来就马上触发mousePressedEvent而结束链接，因此，在LinkModeAutoStartEndFollowMouseClick模式下
    /// beginLink函数调用时会把mLinkItemIsMoved设置为false，只有接收到mouseMove事件后，此变量变为true，在mousePressedEvent才会进行结束判断
    bool mLinkItemIsMoved { false };
};

////////////////////////////////////////////////
/// _DANodeGraphicsSceneItemMoveingInfos
////////////////////////////////////////////////

/**
 * @brief 添加开始的位置
 * @param item
 * @param start
 */
void _DANodeGraphicsSceneItemMoveingInfos::appendStartPos(QGraphicsItem* item, const QPointF& start)
{
    items.append(item);
    startsPos.append(start);
}

/**
 * @brief 刷新结束位置
 */
void _DANodeGraphicsSceneItemMoveingInfos::appendStartPos(QGraphicsItem* item)
{
    items.append(item);
    startsPos.append(item->pos());
}

void _DANodeGraphicsSceneItemMoveingInfos::updateEndPos()
{
    for (QGraphicsItem* i : qAsConst(items)) {
        endsPos.append(i->pos());
    }
}

/**
 * @brief 清除信息
 */
void _DANodeGraphicsSceneItemMoveingInfos::clear()
{
    items.clear();
    startsPos.clear();
    endsPos.clear();
}

////////////////////////////////////////////////
//==============================
// DAGraphicsSceneWithUndoStackPrivate
// 实现
//==============================
////////////////////////////////////////////////

DAGraphicsSceneWithUndoStack::PrivateData::PrivateData(DAGraphicsSceneWithUndoStack* p) : q_ptr(p)
{
    mGridLinePen.setStyle(Qt::SolidLine);
    mGridLinePen.setColor(QColor(219, 219, 219));
    mGridLinePen.setCapStyle(Qt::RoundCap);
    mGridLinePen.setWidthF(0.5);
}

void DAGraphicsSceneWithUndoStack::PrivateData::renderBackgroundCache()
{
    if (!mShowGridLine) {
        return;
    }
    QRectF sr = q_ptr->sceneRect();
    if (!sr.isValid()) {
        qDebug() << "sceneRect is invalid";
        return;
    }
    QRect scr = sr.toRect();
    QPixmap pixmap(scr.width(), scr.height());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderBackgroundCache(&painter, pixmap.rect());
    mBackgroundCache = std::move(pixmap);
}

/**
 * @brief 渲染背景缓存
 */
void DAGraphicsSceneWithUndoStack::PrivateData::renderBackgroundCache(QPainter* painter, const QRect& rect)
{
    painter->setPen(mGridLinePen);
    qreal left = int(rect.left()) - (int(rect.left()) % mGridSize.width());
    qreal top  = int(rect.top()) - (int(rect.top()) % mGridSize.height());

    for (qreal x = left; x < rect.right(); x += mGridSize.width()) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    //绘制横线
    for (qreal y = top; y < rect.bottom(); y += mGridSize.height()) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}

//===============================================================
// DAGraphicsSceneWithUndoStack
//===============================================================

DAGraphicsSceneWithUndoStack::DAGraphicsSceneWithUndoStack(QObject* p) : QGraphicsScene(p), DA_PIMPL_CONSTRUCT
{
}

DAGraphicsSceneWithUndoStack::DAGraphicsSceneWithUndoStack(const QRectF& sceneRect, QObject* p)
    : QGraphicsScene(sceneRect, p), DA_PIMPL_CONSTRUCT
{
}

DAGraphicsSceneWithUndoStack::DAGraphicsSceneWithUndoStack(qreal x, qreal y, qreal width, qreal height, QObject* p)
    : QGraphicsScene(x, y, width, height, p), DA_PIMPL_CONSTRUCT
{
}

DAGraphicsSceneWithUndoStack::~DAGraphicsSceneWithUndoStack()
{
}

/**
 * @brief 判断是否正在移动item
 * @return
 */
bool DAGraphicsSceneWithUndoStack::isMovingItems() const
{
    return d_ptr->mIsMovingItems;
}

/**
 * @brief 获取当前鼠标在scene的位置
 * @return
 */
QPointF DAGraphicsSceneWithUndoStack::getCurrentMouseScenePos() const
{
    return (d_ptr->mLastMouseScenePos);
}

/**
 * @brief 获取选中且能移动的item
 * @return
 */
QList< QGraphicsItem* > DAGraphicsSceneWithUndoStack::getSelectedMovableItems()
{
    QList< QGraphicsItem* > res;
    QList< QGraphicsItem* > its = selectedItems();
    for (QGraphicsItem* it : qAsConst(its)) {
        if (it->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
            res.append(it);
        }
    }
    return res;
}

/**
 * @brief 等同additem，但使用redo/undo来添加，可以进行redo/undo操作
 * @param item
 * @param autopush 自动推入redo/undo栈，对于无需操作返回的cmd，此值需要设置为true，否则需要手动调用push函数，把返回的cmd推入
 * @return 返回执行的命令
 */
QUndoCommand* DAGraphicsSceneWithUndoStack::addItem_(QGraphicsItem* item, bool autopush)
{
    DA::DACommandsForGraphicsItemAdd* cmd = new DA::DACommandsForGraphicsItemAdd(item, this);
    if (autopush) {
        push(cmd);
    }
    return cmd;
}

/**
 * @brief 等同removeItem，但使用redo/undo来添加，可以进行redo/undo操作
 * @param item
 * @param autopush 自动推入redo/undo栈，对于无需操作返回的cmd，此值需要设置为true，否则需要手动调用push函数，把返回的cmd推入
 * @return
 */
QUndoCommand* DAGraphicsSceneWithUndoStack::removeItem_(QGraphicsItem* item, bool autopush)
{
    DA::DACommandsForGraphicsItemRemove* cmd = new DA::DACommandsForGraphicsItemRemove(item, this);
    if (autopush) {
        push(cmd);
    }
    return cmd;
}

/**
 * @brief 导出为pixmap
 * @return
 */
QPixmap DAGraphicsSceneWithUndoStack::toPixamp()
{
    QRectF br = itemsBoundingRect();
    QPixmap res(br.size().toSize() + QSize(10, 10));
    res.fill(Qt::transparent);
    QPainter painter(&res);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    render(&painter, QRectF(10, 10, br.width(), br.height()), br);
    return res;
}

/**
 * @brief 开始链接模式
 *
 * 再开始链接模式，鼠标移动事件会改变当前链接线的末端位置，直到点击鼠标左键进行确认（endLink）,
 * 或者点击鼠标右键取消链接（cancelLink）
 * @param linkItem 连接线图元
 */
void DAGraphicsSceneWithUndoStack::beginLink(DAGraphicsLinkItem* linkItem, LinkMode lm)
{
    if (nullptr == linkItem) {
        cancelLink();
        return;
    }
    d_ptr->mLinkItem.reset(linkItem);
    if (linkItem->scene() != this) {
        addItem(linkItem);
    }
    switch (lm) {
    case LinkModeAutoStartEndFollowMouseClick: {  //开端为当前鼠标位置，末端跟随鼠标移动，在下个鼠标左键点击时结束连线
        linkItem->setStartScenePosition(getCurrentMouseScenePos());
        d_ptr->mLinkItemIsMoved = false;
    } break;
    default:
        break;
    }

    linkItem->updateBoundingRect();
}

/**
 * @brief 判断当前是否是链接模式
 * @return
 */
bool DAGraphicsSceneWithUndoStack::isStartLink() const
{
    return (d_ptr->mLinkItem != nullptr);
}

/**
 * @brief 结束链接模式
 *
 * 结束链接模式会正常记录当前的连线
 *
 * 如果是要取消当前的连接线，使用@sa cancelLink
 */
void DAGraphicsSceneWithUndoStack::endLink()
{
    if (!isStartLink()) {
        return;
    }
    //把item脱离智能指针管理
    DAGraphicsLinkItem* linkItem = d_ptr->mLinkItem.release();
    linkItem->updateBoundingRect();
    DACommandsForGraphicsItemAdd* cmd = new DACommandsForGraphicsItemAdd(linkItem, this);
    push(cmd);
    emit completeLink(linkItem);
}

/**
 * @brief 取消链接模式
 */
void DAGraphicsSceneWithUndoStack::cancelLink()
{
    if (!isStartLink()) {
        return;
    }
    DAGraphicsLinkItem* linkItem = d_ptr->mLinkItem.get();
    removeItem(linkItem);
    d_ptr->mLinkItem.reset();
}

/**
 * @brief 获取当前正在进行连线的连接线item
 * @note 注意，此函数在@sa beginLink 调用之后才会有指针返回，在调用@sa endLink 或 @sa cancelLink 后都返回nullptr
 * @return 返回beginLink设置的指针
 */
DAGraphicsLinkItem* DAGraphicsSceneWithUndoStack::getCurrentLinkItem() const
{
    return d_ptr->mLinkItem.get();
}

/**
 * @brief 是否允许对齐网格
 * @param on
 * @note 此操作对未对齐的item是不会起作用
 */
void DAGraphicsSceneWithUndoStack::setEnableSnapToGrid(bool on)
{
    d_ptr->mEnableSnapToGrid = on;
}
/**
 * @brief 是否允许对齐网格
 * @return
 */
bool DAGraphicsSceneWithUndoStack::isEnableSnapToGrid() const
{
    return d_ptr->mEnableSnapToGrid;
}
/**
 * @brief 设置网格尺寸
 * @param gs
 */
void DAGraphicsSceneWithUndoStack::setGridSize(const QSize& gs)
{
    d_ptr->mGridSize = gs;
}
/**
 * @brief 网格尺寸
 * @return
 */
QSize DAGraphicsSceneWithUndoStack::getGridSize() const
{
    return d_ptr->mGridSize;
}

/**
 * @brief 显示网格线
 * @param on
 */
void DAGraphicsSceneWithUndoStack::showGridLine(bool on)
{
    d_ptr->mShowGridLine = on;
}

/**
 * @brief 选中所有item
 *
 * @note 此函数会发射一次selectionChanged信号
 */
void DAGraphicsSceneWithUndoStack::selectAll()
{
    int selectCnt = 0;
    {
        //避免每个选中都触发selectionChanged信号
        QSignalBlocker b(this);
        QList< QGraphicsItem* > its = items();
        for (QGraphicsItem* i : its) {
            if (!i->isSelected() && i->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
                //只有没有被选上，且是可选的才会执行选中动作
                i->setSelected(true);
                ++selectCnt;
            }
        }
    }
    if (selectCnt > 0) {
        emit selectionChanged();
    }
}
/**
 * @brief 是否显示网格线
 * @return
 */
bool DAGraphicsSceneWithUndoStack::isShowGridLine()
{
    return d_ptr->mShowGridLine;
}
/**
 * @brief 设置网格画笔
 * @param p
 */
void DAGraphicsSceneWithUndoStack::setGridLinePen(const QPen& p)
{
    d_ptr->mGridLinePen = p;
}
/**
 * @brief 获取网格画笔
 * @return
 */
QPen DAGraphicsSceneWithUndoStack::getGridLinePen() const
{
    return d_ptr->mGridLinePen;
}

/**
 * @brief 设置绘制背景使用缓冲
 * @param on
 */
void DAGraphicsSceneWithUndoStack::setPaintBackgroundInCache(bool on)
{
    d_ptr->mIsPaintBackgroundInCache = on;
    if (on) {
        d_ptr->renderBackgroundCache();
    }
}

/**
 * @brief 是否绘制背景使用缓冲
 * @return
 */
bool DAGraphicsSceneWithUndoStack::isPaintBackgroundInCache() const
{
    return d_ptr->mIsPaintBackgroundInCache;
}

/**
 * @brief 获取DANodeGraphicsScene内部维护的undoStack
 * @return
 */
QUndoStack& DAGraphicsSceneWithUndoStack::undoStack()
{
    return d_ptr->mUndoStack;
}

const QUndoStack& DAGraphicsSceneWithUndoStack::undoStack() const
{
    return d_ptr->mUndoStack;
}
/**
 * @brief 获取undostack指针
 * @return
 */
QUndoStack* DAGraphicsSceneWithUndoStack::getUndoStack() const
{
    return &(d_ptr->mUndoStack);
}

/**
 * @brief 在StackGroup中激活undoStack
 */
void DAGraphicsSceneWithUndoStack::setUndoStackActive()
{
    if (!d_ptr->mUndoStack.isActive()) {
        d_ptr->mUndoStack.setActive(true);
    }
}

/**
 * @brief 等同s->undoStack().push(cmd);
 * @param cmd
 */
void DAGraphicsSceneWithUndoStack::push(QUndoCommand* cmd)
{
    d_ptr->mUndoStack.push(cmd);
}

/**
 * @brief 判断点击的item是否可以移动
 *
 * 对于一些特殊的item，可以通过继承此函数来否决掉DAGraphicsSceneWithUndoStack对item移动的判断，否则会对item进行移动
 * 最简单的实现如下：
 * @code
 * bool DAGraphicsSceneWithUndoStack::isItemCanMove(const QGraphicsItem* positem, const QPointF& scenePos)
 * {
 *     //没选中
 *     if (positem == nullptr) {
 *         return false;
 *     }
 *     //选中了但不可移动，也不行
 *     if (!positem->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
 *         return false;
 *     }
 *     return true;
 * }
 * @endcode
 * @param selitems
 * @return
 */
bool DAGraphicsSceneWithUndoStack::isItemCanMove(QGraphicsItem* positem, const QPointF& scenePos)
{
    //没选中
    if (positem == nullptr) {
        return false;
    }
    //选中了但不可移动，也不行
    if (!positem->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
        return false;
    }
    //还要确认一下是否点在了DAGraphicsResizeableItem的控制点上，点在控制点上是不能移动的
    DAGraphicsResizeableItem* resizeitem = qgraphicsitem_cast< DAGraphicsResizeableItem* >(positem);
    if (resizeitem) {
        DAGraphicsResizeableItem::ControlType t = resizeitem->getControlPointByPos(resizeitem->mapFromScene(scenePos));
        if (t != DAGraphicsResizeableItem::NotUnderAnyControlType) {
            //说明点击在了控制点上，也取消移动
            return false;
        }
    }
    return true;
}

/**
 * @brief 调用此函数 主动触发itemsPositionChanged信号，这个函数用于 继承此类例如实现了键盘移动item，主动触发此信号
 * @param items
 * @param oldPos
 * @param newPos
 */
void DAGraphicsSceneWithUndoStack::emitItemsPositionChanged(const QList< QGraphicsItem* >& items,
                                                            const QList< QPointF >& oldPos,
                                                            const QList< QPointF >& newPos)
{
    emit itemsPositionChanged(items, oldPos, newPos);
}
/**
 * @brief 调用此函数 主动触发itemBodySizeChanged信号
 * @param item
 * @param oldSize
 * @param newSize
 */
void DAGraphicsSceneWithUndoStack::emitItemBodySizeChanged(DAGraphicsResizeableItem* item, const QSizeF& oldSize, const QSizeF& newSize)
{
    emit itemBodySizeChanged(item, oldSize, newSize);
}
/**
 * @brief 调用此函数 主动触发itemRotationed信号
 * @param item
 * @param rotation
 */
void DAGraphicsSceneWithUndoStack::emitItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation)
{
    emit itemRotationChanged(item, rotation);
}

void DAGraphicsSceneWithUndoStack::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    //记录鼠标点击的位置
    d_ptr->mLastMousePressScenePos = mouseEvent->scenePos();
    //先传递下去使得能处理选中状态
    QGraphicsScene::mousePressEvent(mouseEvent);
    if (isStartLink()) {
        //链接线模式，处理连接线
        if (mouseEvent->buttons().testFlag(Qt::RightButton)) {
            //右键点击是取消
            cancelLink();
        } else if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
            //左键点击
            switch (d_ptr->mLinkMode) {
            case LinkModeAutoStartEndFollowMouseClick: {
                //结束链接
                if (d_ptr->mLinkItemIsMoved) {
                    endLink();
                }
            } break;
            default:
                break;
            }
        }
    } else {
        if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
            //! 1. 在链接模式下处理链接线的点击

            //!  2.记录选中的所有图元，如果点击的是改变尺寸的点，这个就不执行记录
            QGraphicsItem* positem = itemAt(d_ptr->mLastMousePressScenePos, QTransform());
            if (!isItemCanMove(positem, d_ptr->mLastMousePressScenePos)) {
                d_ptr->mIsMovingItems = false;
                return;
            }
            //说明这个是移动
            //获取选中的可移动单元
            QList< QGraphicsItem* > mits = getSelectedMovableItems();
            if (mits.isEmpty()) {
                d_ptr->mIsMovingItems = false;
                return;
            }
            // todo.如果点击的是链接和control point，不属于移动
            for (QGraphicsItem* its : qAsConst(mits)) {
                DAGraphicsResizeableItem* ri = dynamic_cast< DAGraphicsResizeableItem* >(its);
                if (ri) {
                    if (DAGraphicsResizeableItem::NotUnderAnyControlType
                        != ri->getControlPointByPos(ri->mapFromScene(mouseEvent->scenePos()))) {
                        //说明点击在了控制点上，需要跳过
                        d_ptr->mIsMovingItems = false;
                        return;
                    }
                }
            }
            d_ptr->mIsMovingItems = true;
            //
            d_ptr->mMovingInfos.clear();
            for (QGraphicsItem* its : qAsConst(mits)) {
                d_ptr->mMovingInfos.appendStartPos(its);
            }
        }
    }
}

void DAGraphicsSceneWithUndoStack::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (nullptr == mouseEvent) {
        return;
    }
    d_ptr->mLastMouseScenePos = mouseEvent->scenePos();
    if (isStartLink()) {
        switch (d_ptr->mLinkMode) {
        case LinkModeAutoStartEndFollowMouseClick: {  //开端为当前鼠标位置，末端跟随鼠标移动，在下个鼠标左键点击时结束连线
            d_ptr->mLinkItem->setEndScenePosition(d_ptr->mLastMouseScenePos);
            d_ptr->mLinkItem->updateBoundingRect();
            d_ptr->mLinkItemIsMoved = true;
        } break;
        default:
            break;
        }
    }
    QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void DAGraphicsSceneWithUndoStack::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (nullptr == mouseEvent) {
        return;
    }
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
    if (d_ptr->mIsMovingItems) {
        d_ptr->mIsMovingItems = false;
        QPointF releasePos    = mouseEvent->scenePos();
        if (qFuzzyCompare(releasePos.x(), d_ptr->mLastMousePressScenePos.x())
            && qFuzzyCompare(releasePos.y(), d_ptr->mLastMousePressScenePos.y())) {
            //位置相等，不做处理
            return;
        }
        //位置不等，属于正常移动
        d_ptr->mMovingInfos.updateEndPos();
        DA::DACommandsForGraphicsItemsMoved* cmd = new DA::DACommandsForGraphicsItemsMoved(d_ptr->mMovingInfos.items,
                                                                                           d_ptr->mMovingInfos.startsPos,
                                                                                           d_ptr->mMovingInfos.endsPos,
                                                                                           true);
        push(cmd);
        //位置改变信号
        //        qDebug() << "emit itemsPositionChanged";
        //如果 移动 过程 鼠标移出scene在释放，可能无法捕获
        emit itemsPositionChanged(d_ptr->mMovingInfos.items, d_ptr->mMovingInfos.startsPos, d_ptr->mMovingInfos.endsPos);
    }
}

void DAGraphicsSceneWithUndoStack::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsScene::drawBackground(painter, rect);
    if (isShowGridLine()) {
        if (isPaintBackgroundInCache()) {
            if (d_ptr->mBackgroundCache.size() != sceneRect().toRect().size()) {
                d_ptr->renderBackgroundCache();
            }
            const QPointF& pixmapTopleft = rect.topLeft() - sceneRect().topLeft();
            qreal x                      = int(pixmapTopleft.x()) - (int(pixmapTopleft.x()) % d_ptr->mGridSize.width());
            qreal y = int(pixmapTopleft.y()) - (int(pixmapTopleft.y()) % d_ptr->mGridSize.height());
            painter->drawPixmap(rect, d_ptr->mBackgroundCache, QRectF(x, y, rect.width(), rect.height()));

        } else {
            d_ptr->renderBackgroundCache(painter, rect.toRect());
        }
    }
    //直接绘制网格线在放大时很卡
}

}  // end DA
