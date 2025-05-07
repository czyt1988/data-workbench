#include "DAGraphicsCommandsFactory.h"
namespace DA
{
DAGraphicsCommandsFactory::DAGraphicsCommandsFactory()
{
}

DAGraphicsCommandsFactory::~DAGraphicsCommandsFactory()
{
}

DACommandsForGraphicsItemAdd* DAGraphicsCommandsFactory::createItemAdd(QGraphicsItem* item,
                                                                       QGraphicsScene* scene,
                                                                       QUndoCommand* parent)
{
    return new DACommandsForGraphicsItemAdd(item, scene, parent);
}

DACommandsForGraphicsItemsAdd* DAGraphicsCommandsFactory::createItemsAdd(const QList< QGraphicsItem* > its,
                                                                         QGraphicsScene* scene,
                                                                         QUndoCommand* parent)
{
    return new DA::DACommandsForGraphicsItemsAdd(its, scene, parent);
}
}  // end DA
