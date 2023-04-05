#include "DAStandardNodeLinkGraphicsItem.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

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
