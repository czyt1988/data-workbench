#include "DAStandardNodeRectGraphicsItem.h"
#include <QPainter>
#include "DANodePalette.h"
namespace DA
{

class DAStandardNodeRectGraphicsItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAStandardNodeRectGraphicsItem)
public:
    PrivateData(DAStandardNodeRectGraphicsItem* p);

public:
    QString mText;
    Qt::Alignment mTextAlignment { Qt::AlignCenter };
};

DAStandardNodeRectGraphicsItem::PrivateData::PrivateData(DAStandardNodeRectGraphicsItem* p) : q_ptr(p)
{
}

//===================================================
// DAStandardNodeRectGraphicsItem
//===================================================

DAStandardNodeRectGraphicsItem::DAStandardNodeRectGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), DA_PIMPL_CONSTRUCT
{
}

DAStandardNodeRectGraphicsItem::~DAStandardNodeRectGraphicsItem()
{
}

void DAStandardNodeRectGraphicsItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    const DANodePalette& pl = getNodePalette();
    painter->setPen(pl.getBorderColor());
    painter->setBrush(pl.getBackgroundBrush());
    painter->drawRect(bodyRect);
    if (!(d_ptr->mText.isEmpty())) {
        painter->drawText(bodyRect, d_ptr->mTextAlignment, d_ptr->mText);
    }
    painter->restore();
}

void DAStandardNodeRectGraphicsItem::setText(const QString& t)
{
    d_ptr->mText = t;
}

QString DAStandardNodeRectGraphicsItem::getText() const
{
    return d_ptr->mText;
}

void DAStandardNodeRectGraphicsItem::setTextAlignment(Qt::Alignment al)
{
    d_ptr->mTextAlignment = al;
}

Qt::Alignment DAStandardNodeRectGraphicsItem::getTextAlignment() const
{
    return d_ptr->mTextAlignment;
}

}  // end of DA
