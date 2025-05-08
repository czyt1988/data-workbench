#include "DAGraphicsCommandsFactory.h"
#include "DAGraphicsScene.h"
namespace DA
{
DAGraphicsCommandsFactory::DAGraphicsCommandsFactory()
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

void DAGraphicsCommandsFactory::setScene(DAGraphicsScene* s)
{
    mScene = s;
}

DAGraphicsScene* DAGraphicsCommandsFactory::scene() const
{
    return mScene;
}
}  // end DA
