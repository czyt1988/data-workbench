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

DACommandsForGraphicsItemAdd* DAGraphicsCommandsFactory::createItemAdd(QGraphicsItem* item, QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemAdd(item, scene(), parent);
}

DACommandsForGraphicsItemsAdd* DAGraphicsCommandsFactory::createItemsAdd(const QList< QGraphicsItem* > its,
																		 QUndoCommand* parent)
{
	return new DACommandsForGraphicsItemsAdd(its, scene(), parent);
}

DACommandsForGraphicsItemRemove* DAGraphicsCommandsFactory::createItemRemove(QGraphicsItem* item, QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemRemove(item, scene(), parent);
}

DACommandsForGraphicsItemsRemove* DAGraphicsCommandsFactory::createItemsRemove(const QList< QGraphicsItem* > its,
																			   QUndoCommand* parent)
{
	return new DACommandsForGraphicsItemsRemove(its, scene(), parent);
}

DACommandsForGraphicsItemMoved* DAGraphicsCommandsFactory::createItemMoved(QGraphicsItem* item,
                                                                           const QPointF& start,
                                                                           const QPointF& end,
                                                                           bool skipfirst,
                                                                           QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemMoved(item, start, end, skipfirst, parent);
}

DACommandsForGraphicsItemMoved_Merge* DAGraphicsCommandsFactory::createItemMoved_Merge(QGraphicsItem* item,
                                                                                       const QPointF& start,
                                                                                       const QPointF& end,
                                                                                       bool skipfirst,
                                                                                       QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemMoved_Merge(item, start, end, skipfirst, parent);
}

DACommandsForGraphicsItemsMoved* DAGraphicsCommandsFactory::createItemsMoved(const QList< QGraphicsItem* >& items,
                                                                             const QList< QPointF >& starts,
                                                                             const QList< QPointF >& ends,
                                                                             bool skipfirst,
                                                                             QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemsMoved(items, starts, ends, skipfirst, parent);
}

DACommandsForGraphicsItemsMoved_Merge* DAGraphicsCommandsFactory::createItemsMoved_Merge(const QList< QGraphicsItem* >& items,
                                                                                         const QList< QPointF >& starts,
                                                                                         const QList< QPointF >& ends,
                                                                                         bool skipfirst,
                                                                                         QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemsMoved_Merge(items, starts, ends, skipfirst, parent);
}

DACommandsForGraphicsItemResized* DAGraphicsCommandsFactory::createItemResized(DAGraphicsResizeableItem* item,
                                                                               const QPointF& oldpos,
                                                                               const QSizeF& oldSize,
                                                                               const QPointF& newpos,
                                                                               const QSizeF& newSize,
                                                                               bool skipfirst,
                                                                               QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemResized(item, oldpos, oldSize, newpos, newSize, skipfirst, parent);
}

DACommandsForGraphicsItemResized* DAGraphicsCommandsFactory::createItemResized(DAGraphicsResizeableItem* item,
                                                                               const QSizeF& oldSize,
                                                                               const QSizeF& newSize,
                                                                               QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemResized(item, oldSize, newSize, parent);
}

DACommandsForGraphicsItemResizeWidth* DAGraphicsCommandsFactory::createItemResizeWidth(DAGraphicsResizeableItem* item,
                                                                                       const qreal& oldWidth,
                                                                                       const qreal& newWidth,
                                                                                       QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemResizeWidth(item, oldWidth, newWidth, parent);
}

DACommandsForGraphicsItemResizeHeight* DAGraphicsCommandsFactory::createItemResizeHeight(DAGraphicsResizeableItem* item,
                                                                                         const qreal& oldHeight,
                                                                                         const qreal& newHeight,
                                                                                         QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemResizeHeight(item, oldHeight, newHeight, parent);
}

DACommandsForGraphicsItemRotation* DAGraphicsCommandsFactory::createItemRotation(DAGraphicsResizeableItem* item,
                                                                                 const qreal& oldRotation,
                                                                                 const qreal& newRotation,
                                                                                 QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemRotation(item, oldRotation, newRotation, parent);
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
