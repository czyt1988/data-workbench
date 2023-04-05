#include "DAGraphicsResizeableTextItem.h"
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QTextItem>
#include "DAStandardGraphicsTextItem.h"
namespace DA
{
DAGraphicsResizeableTextItem::DAGraphicsResizeableTextItem(QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), m_textItem(new DAStandardGraphicsTextItem(this))
{
    //    m_textItem->setFocusProxy(this);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFocusProxy(m_textItem);
}

DAGraphicsResizeableTextItem::DAGraphicsResizeableTextItem(const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), m_textItem(new DAStandardGraphicsTextItem(f, this))
{
    //    m_textItem->setFocusProxy(this);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFocusProxy(m_textItem);
}

DAGraphicsResizeableTextItem::DAGraphicsResizeableTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), m_textItem(new DAStandardGraphicsTextItem(str, f, this))
{
    //    m_textItem->setFocusProxy(this);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFlags(ItemUsesExtendedStyleOption);
    setFocusProxy(m_textItem);
}

void DAGraphicsResizeableTextItem::setEditMode(bool on)
{
    m_textItem->setEditMode(on);
}

bool DAGraphicsResizeableTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    DAGraphicsResizeableItem::saveToXml(doc, parentElement);
    m_textItem->saveToXml(doc, parentElement);
    return true;
}

bool DAGraphicsResizeableTextItem::loadFromXml(const QDomElement* itemElement)
{
    DAGraphicsResizeableItem::loadFromXml(itemElement);
    return m_textItem->loadFromXml(itemElement);
}

DAStandardGraphicsTextItem* DAGraphicsResizeableTextItem::textItem() const
{
    return m_textItem;
}

void DAGraphicsResizeableTextItem::setBodySize(const QSizeF& s)
{
    DAGraphicsResizeableItem::setBodySize(s);
}

/**
 * @brief DAGraphicsResizeableTextItem::paintBody
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAGraphicsResizeableTextItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    // TODO:
    Q_UNUSED(bodyRect);
    // m_textItem->paint(painter, option, widget);
}

}  // end da
