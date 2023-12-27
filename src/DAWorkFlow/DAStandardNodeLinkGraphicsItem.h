#ifndef DASTANDARDNODELINKGRAPHICSITEM_H
#define DASTANDARDNODELINKGRAPHICSITEM_H
#include "DAAbstractNodeLinkGraphicsItem.h"
#include <QtCore/qglobal.h>

namespace DA
{
/**
 * @brief 标准的连线
 */
class DAWORKFLOW_API DAStandardNodeLinkGraphicsItem : public DAAbstractNodeLinkGraphicsItem
{
    Q_OBJECT
public:
    DAStandardNodeLinkGraphicsItem(QGraphicsItem* p = nullptr);
    DAStandardNodeLinkGraphicsItem(DAAbstractNodeGraphicsItem* from, DANodeLinkPoint pl, QGraphicsItem* p = nullptr);
};
}  // end of namespace DA
#endif  // FCSTANDARDNODELINKGRAPHICSITEM_H
