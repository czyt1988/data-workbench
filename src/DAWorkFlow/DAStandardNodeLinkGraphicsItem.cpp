#include "DAStandardNodeLinkGraphicsItem.h"
namespace DA
{

//===================================================
// DAStandardNodeLinkGraphicsItem
//===================================================
DAStandardNodeLinkGraphicsItem::DAStandardNodeLinkGraphicsItem(QGraphicsItem* p) : DAAbstractNodeLinkGraphicsItem(p)
{
}

DAStandardNodeLinkGraphicsItem::DAStandardNodeLinkGraphicsItem(DAAbstractNodeGraphicsItem* from, DANodeLinkPoint pl, QGraphicsItem* p)
    : DAAbstractNodeLinkGraphicsItem(from, pl, p)
{
}

}  // end DA
