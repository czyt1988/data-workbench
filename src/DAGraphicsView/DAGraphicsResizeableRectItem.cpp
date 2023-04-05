#include "DAGraphicsResizeableRectItem.h"
#include <QPainter>
#include <QDomDocument>
#include <QDomElement>
namespace DA
{

class DAGraphicsResizeableRectItemPrivate
{
    DA_IMPL_PUBLIC(DAGraphicsResizeableRectItem)
public:
    DAGraphicsResizeableRectItemPrivate(DAGraphicsResizeableRectItem* p);
    QPen getTextPen() const;
    void setTextPen(const QPen& p);

public:
    QString _text;
    Qt::Alignment _textAl;
    QPen _textPen;
};

DAGraphicsResizeableRectItemPrivate::DAGraphicsResizeableRectItemPrivate(DAGraphicsResizeableRectItem* p)
    : q_ptr(p), _textAl(Qt::AlignCenter)
{
    _textPen.setColor(Qt::black);
}

QPen DAGraphicsResizeableRectItemPrivate::getTextPen() const
{
    return _textPen;
}

void DAGraphicsResizeableRectItemPrivate::setTextPen(const QPen& p)
{
    _textPen = p;
}
//===================================================
// DAGraphicsResizeableRectItem
//===================================================
DAGraphicsResizeableRectItem::DAGraphicsResizeableRectItem(QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), d_ptr(new DAGraphicsResizeableRectItemPrivate(this))
{
    setShowBackground(false);
    setShowBorder(true);
    setBorderPen(QPen(QColor(Qt::black)));
}

DAGraphicsResizeableRectItem::~DAGraphicsResizeableRectItem()
{
}

void DAGraphicsResizeableRectItem::setText(const QString& t)
{
    d_ptr->_text = t;
}

QString DAGraphicsResizeableRectItem::getText() const
{
    return d_ptr->_text;
}

void DAGraphicsResizeableRectItem::setTextAlignment(Qt::Alignment al)
{
    d_ptr->_textAl = al;
}

Qt::Alignment DAGraphicsResizeableRectItem::getTextAlignment() const
{
    return d_ptr->_textAl;
}

QPen DAGraphicsResizeableRectItem::getTextPen() const
{
    return d_ptr->getTextPen();
}

void DAGraphicsResizeableRectItem::setTextPen(const QPen& p)
{
    d_ptr->setTextPen(p);
}

bool DAGraphicsResizeableRectItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    DAGraphicsResizeableItem::saveToXml(doc, parentElement);
    QDomElement rectEle = doc->createElement("rect-info");

    QDomElement textEle = doc->createElement("text");
    textEle.setAttribute("al", enumToString(d_ptr->_textAl));
    textEle.appendChild(doc->createTextNode(d_ptr->_text));
    rectEle.appendChild(textEle);
    // text-pen
    rectEle.appendChild(DAXMLFileInterface::makeElement(d_ptr->_textPen, "text-pen", doc));
    parentElement->appendChild(rectEle);
    return true;
}

bool DAGraphicsResizeableRectItem::loadFromXml(const QDomElement* itemElement)
{
    if (!DAGraphicsResizeableItem::loadFromXml(itemElement)) {
        return false;
    }
    QDomElement rectEle = itemElement->firstChildElement("rect-info");
    if (rectEle.isNull()) {
        return false;
    }
    QDomElement textEle = itemElement->firstChildElement("text");
    if (!textEle.isNull()) {
        d_ptr->_textAl = stringToEnum(textEle.attribute("al"), Qt::AlignCenter);
        d_ptr->_text   = textEle.text();
    }
    return true;
}

void DAGraphicsResizeableRectItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    if (!(d_ptr->_text.isEmpty())) {
        painter->setPen(d_ptr->_textPen);
        painter->drawText(bodyRect, d_ptr->_textAl, d_ptr->_text);
    }
    painter->restore();
}

}
