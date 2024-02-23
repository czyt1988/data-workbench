#include "DAGraphicsTextItem.h"
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QTextItem>
#include <QGraphicsSceneResizeEvent>
#include "DAGraphicsStandardTextItem.h"
namespace DA
{
class DAGraphicsTextItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsTextItem)
public:
    PrivateData(DAGraphicsTextItem* p);
    DAGraphicsStandardTextItem* mTextItem { nullptr };
    bool mEnableRelativePosition { false };
    qreal mRelativeX { 0.0 };
    qreal mRelativeY { 0.0 };
    Qt::Corner mRelativeCorner { Qt::TopLeftCorner };
};

DAGraphicsTextItem::PrivateData::PrivateData(DAGraphicsTextItem* p) : q_ptr(p)
{
    mTextItem = new DAGraphicsStandardTextItem(p);
}

//----------------------------------------------------
// DAGraphicsTextItem
//----------------------------------------------------

DAGraphicsTextItem::DAGraphicsTextItem(QGraphicsItem* parent) : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
    //    m_textItem->setFocusProxy(this);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFocusProxy(d_ptr->mTextItem);
}

DAGraphicsTextItem::DAGraphicsTextItem(const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
    //    m_textItem->setFocusProxy(this);
    d_ptr->mTextItem->setFont(f);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFocusProxy(d_ptr->mTextItem);
}

DAGraphicsTextItem::DAGraphicsTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
    //    m_textItem->setFocusProxy(this);
    d_ptr->mTextItem->setFont(f);
    setText(str);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFlags(ItemUsesExtendedStyleOption);
    setFocusProxy(d_ptr->mTextItem);
}

void DAGraphicsTextItem::setEditMode(bool on)
{
    d_ptr->mTextItem->setEditMode(on);
}

bool DAGraphicsTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    DAGraphicsResizeableItem::saveToXml(doc, parentElement);
    d_ptr->mTextItem->saveToXml(doc, parentElement);
    return true;
}

bool DAGraphicsTextItem::loadFromXml(const QDomElement* itemElement)
{
    DAGraphicsResizeableItem::loadFromXml(itemElement);
    return d_ptr->mTextItem->loadFromXml(itemElement);
}

DAGraphicsStandardTextItem* DAGraphicsTextItem::textItem() const
{
    return d_ptr->mTextItem;
}

void DAGraphicsTextItem::setBodySize(const QSizeF& s)
{
    DAGraphicsResizeableItem::setBodySize(s);
    QRectF br = boundingRect();
    d_ptr->mTextItem->setPos(br.topLeft());
    d_ptr->mTextItem->setTextWidth(br.width());
    d_ptr->mTextItem->adjustSize();
}

void DAGraphicsTextItem::setText(const QString& v)
{
    d_ptr->mTextItem->setPlainText(v);
}

QString DAGraphicsTextItem::getText() const
{
    return d_ptr->mTextItem->toPlainText();
}

void DAGraphicsTextItem::setRelativePosition(qreal xp, qreal yp, Qt::Corner parentCorner)
{
    d_ptr->mRelativeX      = xp;
    d_ptr->mRelativeY      = yp;
    d_ptr->mRelativeCorner = parentCorner;
}

void DAGraphicsTextItem::setEnableRelativePosition(bool on)
{
    d_ptr->mEnableRelativePosition = on;
}

bool DAGraphicsTextItem::isEnableRelativePosition() const
{
    return d_ptr->mEnableRelativePosition;
}

void DAGraphicsTextItem::updateRelativePosition()
{
}

/**
 * @brief paintBody
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAGraphicsTextItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    DAGraphicsResizeableItem::paintBody(painter, option, widget, bodyRect);
}

}  // end da
