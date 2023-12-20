#include "DAGraphicsTextItem.h"
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QTextItem>
#include "DAStandardGraphicsTextItem.h"
namespace DA
{
DAGraphicsTextItem::DAGraphicsTextItem(QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), m_textItem(new DAStandardGraphicsTextItem(this))
{
    //    m_textItem->setFocusProxy(this);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFocusProxy(m_textItem);
}

DAGraphicsTextItem::DAGraphicsTextItem(const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), m_textItem(new DAStandardGraphicsTextItem(f, this))
{
    //    m_textItem->setFocusProxy(this);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFocusProxy(m_textItem);
}

DAGraphicsTextItem::DAGraphicsTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), m_textItem(new DAStandardGraphicsTextItem(str, f, this))
{
    //    m_textItem->setFocusProxy(this);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setFlags(ItemUsesExtendedStyleOption);
    setFocusProxy(m_textItem);
}

void DAGraphicsTextItem::setEditMode(bool on)
{
    m_textItem->setEditMode(on);
}

bool DAGraphicsTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    DAGraphicsResizeableItem::saveToXml(doc, parentElement);
    m_textItem->saveToXml(doc, parentElement);
    return true;
}

bool DAGraphicsTextItem::loadFromXml(const QDomElement* itemElement)
{
    DAGraphicsResizeableItem::loadFromXml(itemElement);
    return m_textItem->loadFromXml(itemElement);
}

DAStandardGraphicsTextItem* DAGraphicsTextItem::textItem() const
{
    return m_textItem;
}

void DAGraphicsTextItem::setBodySize(const QSizeF& s)
{
    DAGraphicsResizeableItem::setBodySize(s);
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
    // TODO:
    Q_UNUSED(bodyRect);
    // m_textItem->paint(painter, option, widget);
}

}  // end da
