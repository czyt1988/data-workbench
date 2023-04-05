#include "DAStandardNodeRectGraphicsItem.h"
#include <QPainter>
#include "DANodePalette.h"
namespace DA
{

class DAStandardNodeRectGraphicsItemPrivate
{
    DA_IMPL_PUBLIC(DAStandardNodeRectGraphicsItem)
public:
    DAStandardNodeRectGraphicsItemPrivate(DAStandardNodeRectGraphicsItem* p);

public:
    QString _text;
    Qt::Alignment _textAl;
};

DAStandardNodeRectGraphicsItemPrivate::DAStandardNodeRectGraphicsItemPrivate(DAStandardNodeRectGraphicsItem* p)
    : q_ptr(p), _textAl(Qt::AlignCenter)
{
}

//===================================================
// DAStandardNodeRectGraphicsItem
//===================================================

DAStandardNodeRectGraphicsItem::DAStandardNodeRectGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), d_ptr(new DAStandardNodeRectGraphicsItemPrivate(this))
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
    if (!(d_ptr->_text.isEmpty())) {
        painter->drawText(bodyRect, d_ptr->_textAl, d_ptr->_text);
    }
    painter->restore();
}

void DAStandardNodeRectGraphicsItem::setText(const QString& t)
{
    d_ptr->_text = t;
}

QString DAStandardNodeRectGraphicsItem::getText() const
{
    return d_ptr->_text;
}

void DAStandardNodeRectGraphicsItem::setTextAlignment(Qt::Alignment al)
{
    d_ptr->_textAl = al;
}

Qt::Alignment DAStandardNodeRectGraphicsItem::getTextAlignment() const
{
    return d_ptr->_textAl;
}

}  // end of DA
