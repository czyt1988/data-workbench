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

//////////////////////////////////////////////

class DAGraphicsSceneWithUndoStackPrivate
{
    DA_IMPL_PUBLIC(DAGraphicsSceneWithUndoStack)
public:
    DAGraphicsSceneWithUndoStackPrivate(DAGraphicsSceneWithUndoStack* p);
    //绘制背景缓存
    void renderBackgroundCache();
    void renderBackgroundCache(QPainter* painter, const QRect& rect);

public:
    QUndoStack _undoStack;
    bool _isMovingItems;  /// 正在进行item的移动
    QPointF _lastMouseScenePos;
    QPointF _lastMousePressScenePos;                    ///< 记录点击时的位置
    _DANodeGraphicsSceneItemMoveingInfos _movingInfos;  ///< 记录移动的信息
    bool _enableSnapToGrid;                             ///< 允许对齐网格
    bool _showGridLine;                                 ///< 是否显示网格
    QSize _gridSize;                                    ///< 网格大小
    QPen _gridLinePen;                                  ///< 绘制网格的画笔
    QPixmap _backgroundCache;                           ///< 背景缓存，不用每次都绘制
    bool _isPaintBackgroundInCache;                     ///< 背景使用缓冲绘制
};
}

////////////////////////////////////////////////
///
////////////////////////////////////////////////
using namespace DA;

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

DAGraphicsSceneWithUndoStackPrivate::DAGraphicsSceneWithUndoStackPrivate(DAGraphicsSceneWithUndoStack* p)
    : q_ptr(p), _isMovingItems(false), _enableSnapToGrid(true), _showGridLine(true), _gridSize(10, 10), _isPaintBackgroundInCache(false)
{
    _gridLinePen.setStyle(Qt::SolidLine);
    _gridLinePen.setColor(QColor(219, 219, 219));
    _gridLinePen.setCapStyle(Qt::RoundCap);
    _gridLinePen.setWidthF(0.5);
}

void DAGraphicsSceneWithUndoStackPrivate::renderBackgroundCache()
{
    if (!_showGridLine) {
        return;
    }
    QRectF sr = q_ptr->sceneRect();
    if (!sr.isValid()) {
        qDebug() << "sceneRect is invalid";
        return;
    }
    QRect scr = sr.toRect();
    QPixmap pixmap(scr.width(), scr.height());
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderBackgroundCache(&painter, pixmap.rect());
    _backgroundCache = std::move(pixmap);
}

/**
 * @brief 渲染背景缓存
 */
void DAGraphicsSceneWithUndoStackPrivate::renderBackgroundCache(QPainter* painter, const QRect& rect)
{
    painter->setPen(_gridLinePen);
    qreal left = int(rect.left()) - (int(rect.left()) % _gridSize.width());
    qreal top  = int(rect.top()) - (int(rect.top()) % _gridSize.height());

    for (qreal x = left; x < rect.right(); x += _gridSize.width()) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    //绘制横线
    for (qreal y = top; y < rect.bottom(); y += _gridSize.height()) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}

////////////////////////////////////////////////

DAGraphicsSceneWithUndoStack::DAGraphicsSceneWithUndoStack(QObject* p)
    : QGraphicsScene(p), d_ptr(new DAGraphicsSceneWithUndoStackPrivate(this))
{
}

DAGraphicsSceneWithUndoStack::DAGraphicsSceneWithUndoStack(const QRectF& sceneRect, QObject* p)
    : QGraphicsScene(sceneRect, p), d_ptr(new DAGraphicsSceneWithUndoStackPrivate(this))
{
}

DAGraphicsSceneWithUndoStack::DAGraphicsSceneWithUndoStack(qreal x, qreal y, qreal width, qreal height, QObject* p)
    : QGraphicsScene(x, y, width, height, p), d_ptr(new DAGraphicsSceneWithUndoStackPrivate(this))
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
    return d_ptr->_isMovingItems;
}

/**
 * @brief 获取当前鼠标在scene的位置
 * @return
 */
QPointF DAGraphicsSceneWithUndoStack::getCurrentMouseScenePos() const
{
    return (d_ptr->_lastMouseScenePos);
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
 * @brief 是否允许对齐网格
 * @param on
 * @note 此操作对未对齐的item是不会起作用
 */
void DAGraphicsSceneWithUndoStack::setEnableSnapToGrid(bool on)
{
    d_ptr->_enableSnapToGrid = on;
}
/**
 * @brief 是否允许对齐网格
 * @return
 */
bool DAGraphicsSceneWithUndoStack::isEnableSnapToGrid() const
{
    return d_ptr->_enableSnapToGrid;
}
/**
 * @brief 设置网格尺寸
 * @param gs
 */
void DAGraphicsSceneWithUndoStack::setGridSize(const QSize& gs)
{
    d_ptr->_gridSize = gs;
}
/**
 * @brief 网格尺寸
 * @return
 */
QSize DAGraphicsSceneWithUndoStack::getGridSize() const
{
    return d_ptr->_gridSize;
}

/**
 * @brief 显示网格线
 * @param on
 */
void DAGraphicsSceneWithUndoStack::showGridLine(bool on)
{
    d_ptr->_showGridLine = on;
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
    return d_ptr->_showGridLine;
}
/**
 * @brief 设置网格画笔
 * @param p
 */
void DAGraphicsSceneWithUndoStack::setGridLinePen(const QPen& p)
{
    d_ptr->_gridLinePen = p;
}
/**
 * @brief 获取网格画笔
 * @return
 */
QPen DAGraphicsSceneWithUndoStack::getGridLinePen() const
{
    return d_ptr->_gridLinePen;
}

/**
 * @brief 设置绘制背景使用缓冲
 * @param on
 */
void DAGraphicsSceneWithUndoStack::setPaintBackgroundInCache(bool on)
{
    d_ptr->_isPaintBackgroundInCache = on;
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
    return d_ptr->_isPaintBackgroundInCache;
}

/**
 * @brief 获取DANodeGraphicsScene内部维护的undoStack
 * @return
 */
QUndoStack& DAGraphicsSceneWithUndoStack::undoStack()
{
    return d_ptr->_undoStack;
}

const QUndoStack& DAGraphicsSceneWithUndoStack::undoStack() const
{
    return d_ptr->_undoStack;
}
/**
 * @brief 获取undostack指针
 * @return
 */
QUndoStack* DAGraphicsSceneWithUndoStack::getUndoStack() const
{
    return &(d_ptr->_undoStack);
}

/**
 * @brief 在StackGroup中激活undoStack
 */
void DAGraphicsSceneWithUndoStack::setUndoStackActive()
{
    if (!d_ptr->_undoStack.isActive()) {
        d_ptr->_undoStack.setActive(true);
    }
}

/**
 * @brief 等同s->undoStack().push(cmd);
 * @param cmd
 */
void DAGraphicsSceneWithUndoStack::push(QUndoCommand* cmd)
{
    d_ptr->_undoStack.push(cmd);
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
    d_ptr->_lastMousePressScenePos = mouseEvent->scenePos();
    //先传递下去使得能处理选中状态
    QGraphicsScene::mousePressEvent(mouseEvent);
    if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
        QGraphicsItem* positem = itemAt(d_ptr->_lastMousePressScenePos, QTransform());
        if (!isItemCanMove(positem, d_ptr->_lastMousePressScenePos)) {
            d_ptr->_isMovingItems = false;
            return;
        }
        //说明这个是移动
        //获取选中的可移动单元
        QList< QGraphicsItem* > mits = getSelectedMovableItems();
        if (mits.isEmpty()) {
            d_ptr->_isMovingItems = false;
            return;
        }
        // todo.如果点击的是链接和control point，不属于移动
        for (QGraphicsItem* its : qAsConst(mits)) {
            DAGraphicsResizeableItem* ri = dynamic_cast< DAGraphicsResizeableItem* >(its);
            if (ri) {
                if (DAGraphicsResizeableItem::NotUnderAnyControlType
                    != ri->getControlPointByPos(ri->mapFromScene(mouseEvent->scenePos()))) {
                    //说明点击在了控制点上，需要跳过
                    d_ptr->_isMovingItems = false;
                    return;
                }
            }
        }
        d_ptr->_isMovingItems = true;
        //
        d_ptr->_movingInfos.clear();
        for (QGraphicsItem* its : qAsConst(mits)) {
            d_ptr->_movingInfos.appendStartPos(its);
        }
    }
}

void DAGraphicsSceneWithUndoStack::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (nullptr == mouseEvent) {
        return;
    }
    d_ptr->_lastMouseScenePos = mouseEvent->scenePos();
    QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void DAGraphicsSceneWithUndoStack::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (nullptr == mouseEvent) {
        return;
    }
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
    if (d_ptr->_isMovingItems) {
        d_ptr->_isMovingItems = false;
        QPointF releasePos    = mouseEvent->scenePos();
        if (qFuzzyCompare(releasePos.x(), d_ptr->_lastMousePressScenePos.x())
            && qFuzzyCompare(releasePos.y(), d_ptr->_lastMousePressScenePos.y())) {
            //位置相等，不做处理
            return;
        }
        //位置不等，属于正常移动
        d_ptr->_movingInfos.updateEndPos();
        DA::DACommandsForGraphicsItemsMoved* cmd = new DA::DACommandsForGraphicsItemsMoved(d_ptr->_movingInfos.items,
                                                                                           d_ptr->_movingInfos.startsPos,
                                                                                           d_ptr->_movingInfos.endsPos,
                                                                                           true);
        push(cmd);
        //位置改变信号
        //        qDebug() << "emit itemsPositionChanged";
        //如果 移动 过程 鼠标移出scene在释放，可能无法捕获
        emit itemsPositionChanged(d_ptr->_movingInfos.items, d_ptr->_movingInfos.startsPos, d_ptr->_movingInfos.endsPos);
    }
}

void DAGraphicsSceneWithUndoStack::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsScene::drawBackground(painter, rect);
    if (isShowGridLine()) {
        if (isPaintBackgroundInCache()) {
            if (d_ptr->_backgroundCache.size() != sceneRect().toRect().size()) {
                d_ptr->renderBackgroundCache();
            }
            const QPointF& pixmapTopleft = rect.topLeft() - sceneRect().topLeft();
            qreal x                      = int(pixmapTopleft.x()) - (int(pixmapTopleft.x()) % d_ptr->_gridSize.width());
            qreal y = int(pixmapTopleft.y()) - (int(pixmapTopleft.y()) % d_ptr->_gridSize.height());
            painter->drawPixmap(rect, d_ptr->_backgroundCache, QRectF(x, y, rect.width(), rect.height()));

        } else {
            d_ptr->renderBackgroundCache(painter, rect.toRect());
        }
    }
    //直接绘制网格线在放大时很卡
}
