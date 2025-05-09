#include "DAGraphicsCommandsFactory.h"
#include "DAGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
namespace DA
{

class DAGraphicsCommandsFactory::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAGraphicsCommandsFactory)
public:
    PrivateData(DAGraphicsCommandsFactory* p);
    DAGraphicsScene* scene { nullptr };
    QPointF sceneMousePressPos;    ///< 鼠标按下的场景位置
    QPointF sceneMouseReleasePos;  ///< 鼠标释放的场景位置
    QList< QGraphicsItem* > movingItems;
    QList< QPointF > startsPos;
    QList< QPointF > endsPos;
};

DAGraphicsCommandsFactory::PrivateData::PrivateData(DAGraphicsCommandsFactory* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DAGraphicsCommandsFactory
//----------------------------------------------------
DAGraphicsCommandsFactory::DAGraphicsCommandsFactory() : DA_PIMPL_CONSTRUCT
{
}

DAGraphicsCommandsFactory::~DAGraphicsCommandsFactory()
{
}

DACommandsForGraphicsItemAdd* DAGraphicsCommandsFactory::createItemAdd(QGraphicsItem* item)
{
    return new DACommandsForGraphicsItemAdd(item, scene());
}

DACommandsForGraphicsItemsAdd* DAGraphicsCommandsFactory::createItemsAdd(const QList< QGraphicsItem* > its)
{
    return new DACommandsForGraphicsItemsAdd(its, scene());
}

DACommandsForGraphicsItemRemove* DAGraphicsCommandsFactory::createItemRemove(QGraphicsItem* item, QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemRemove(item, scene(), parent);
}

DACommandsForGraphicsItemsRemove* DAGraphicsCommandsFactory::createItemsRemove(const QList< QGraphicsItem* > its)
{
    return new DACommandsForGraphicsItemsRemove(its, scene());
}

DACommandsForGraphicsItemMoved* DAGraphicsCommandsFactory::createItemMoved(QGraphicsItem* item,
                                                                           const QPointF& start,
                                                                           const QPointF& end,
                                                                           bool skipfirst)
{
    return new DACommandsForGraphicsItemMoved(item, start, end, skipfirst);
}

DACommandsForGraphicsItemMoved_Merge* DAGraphicsCommandsFactory::createItemMoved_Merge(QGraphicsItem* item,
                                                                                       const QPointF& start,
                                                                                       const QPointF& end,
                                                                                       bool skipfirst)
{
    return new DACommandsForGraphicsItemMoved_Merge(item, start, end, skipfirst);
}

DACommandsForGraphicsItemsMoved* DAGraphicsCommandsFactory::createItemsMoved(const QList< QGraphicsItem* >& items,
                                                                             const QList< QPointF >& starts,
                                                                             const QList< QPointF >& ends,
                                                                             bool skipfirst)
{
    return new DACommandsForGraphicsItemsMoved(items, starts, ends, skipfirst);
}

/**
 * @brief 针对鼠标移动的移动命令
 * @param mouseReleaseEEvent
 * @return
 * @note 注意，此函数会返回nullptr，代表没有需要推入的命令
 */
DACommandsForGraphicsItemsMoved* DAGraphicsCommandsFactory::createItemsMoved(QGraphicsSceneMouseEvent* mouseReleaseEEvent)
{
    // 这里先获取释放时的位置
    if (!mouseReleaseEEvent) {
        return nullptr;
    }
    DA_D(d);
    QPointF releasePos = mouseReleaseEEvent->scenePos();
    if (qFuzzyCompare(releasePos.x(), d->sceneMousePressPos.x())
        && qFuzzyCompare(releasePos.y(), d->sceneMousePressPos.y())) {
        // 位置相等，不做处理
        return nullptr;
    }
    QList< QPointF > endsPos;
    for (QGraphicsItem* i : qAsConst(d->movingItems)) {
        endsPos.append(i->pos());
    }
    return createItemsMoved(d->movingItems, d->startsPos, d->endsPos, true);
}

DACommandsForGraphicsItemsMoved_Merge* DAGraphicsCommandsFactory::createItemsMoved_Merge(const QList< QGraphicsItem* >& items,
                                                                                         const QList< QPointF >& starts,
                                                                                         const QList< QPointF >& ends,
                                                                                         bool skipfirst)
{
    return new DACommandsForGraphicsItemsMoved_Merge(items, starts, ends, skipfirst);
}

DACommandsForGraphicsItemResized* DAGraphicsCommandsFactory::createItemResized(DAGraphicsResizeableItem* item,
                                                                               const QPointF& oldpos,
                                                                               const QSizeF& oldSize,
                                                                               const QPointF& newpos,
                                                                               const QSizeF& newSize,
                                                                               bool skipfirst)
{
    return new DACommandsForGraphicsItemResized(item, oldpos, oldSize, newpos, newSize, skipfirst);
}

DACommandsForGraphicsItemResized* DAGraphicsCommandsFactory::createItemResized(DAGraphicsResizeableItem* item,
                                                                               const QSizeF& oldSize,
                                                                               const QSizeF& newSize)
{
    return new DACommandsForGraphicsItemResized(item, oldSize, newSize);
}

DACommandsForGraphicsItemResizeWidth* DAGraphicsCommandsFactory::createItemResizeWidth(DAGraphicsResizeableItem* item,
                                                                                       const qreal& oldWidth,
                                                                                       const qreal& newWidth)
{
    return new DACommandsForGraphicsItemResizeWidth(item, oldWidth, newWidth);
}

DACommandsForGraphicsItemResizeHeight* DAGraphicsCommandsFactory::createItemResizeHeight(DAGraphicsResizeableItem* item,
                                                                                         const qreal& oldHeight,
                                                                                         const qreal& newHeight)
{
    return new DACommandsForGraphicsItemResizeHeight(item, oldHeight, newHeight);
}

DACommandsForGraphicsItemRotation* DAGraphicsCommandsFactory::createItemRotation(DAGraphicsResizeableItem* item,
                                                                                 const qreal& oldRotation,
                                                                                 const qreal& newRotation)
{
    return new DACommandsForGraphicsItemRotation(item, oldRotation, newRotation);
}

DACommandsForGraphicsItemGrouping* DAGraphicsCommandsFactory::createItemGrouping(const QList< QGraphicsItem* >& groupingitems)
{
    return new DACommandsForGraphicsItemGrouping(scene(), groupingitems);
}

DACommandsForGraphicsItemUngrouping* DAGraphicsCommandsFactory::createItemUngrouping(QGraphicsItemGroup* group)
{
    return new DACommandsForGraphicsItemUngrouping(scene(), group);
}

void DAGraphicsCommandsFactory::sceneMousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (!mouseEvent) {
        return;
    }
    if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
        DA_D(d);
        d->movingItems.clear();
        d->startsPos.clear();
        d->endsPos.clear();

        d->sceneMousePressPos        = mouseEvent->scenePos();
        QList< QGraphicsItem* > mits = d->scene->getSelectedMovableItems();
        for (QGraphicsItem* its : qAsConst(mits)) {
            d->movingItems.append(its);
            d->startsPos.append(its->pos());
        }
    }
}

void DAGraphicsCommandsFactory::sceneMouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent);
}

void DAGraphicsCommandsFactory::sceneMouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (mouseEvent) {
        d_ptr->sceneMouseReleasePos = mouseEvent->scenePos();
    }
}

void DAGraphicsCommandsFactory::setScene(DAGraphicsScene* s)
{
    d_ptr->scene = s;
}

DAGraphicsScene* DAGraphicsCommandsFactory::scene() const
{
    return d_ptr->scene;
}

}  // end DA
