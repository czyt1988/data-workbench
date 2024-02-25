#include "DAGraphicsRectItem.h"
#include <QPainter>
#include <QDomDocument>
#include <QDomElement>
namespace DA
{

//===================================================
// DAGraphicsRectItem::PrivateData
//===================================================
class DAGraphicsRectItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsRectItem)
public:
    PrivateData(DAGraphicsRectItem* p);
    QPen getTextPen() const;
    void setTextPen(const QPen& p);

public:
    QString mText;
    Qt::Alignment mTextAlignment { Qt::AlignCenter };
    QPen mTextPen { Qt::black };
};

DAGraphicsRectItem::PrivateData::PrivateData(DAGraphicsRectItem* p) : q_ptr(p)
{
}

QPen DAGraphicsRectItem::PrivateData::getTextPen() const
{
    return mTextPen;
}

void DAGraphicsRectItem::PrivateData::setTextPen(const QPen& p)
{
    mTextPen = p;
}
//===================================================
// DAGraphicsRectItem
//===================================================
DAGraphicsRectItem::DAGraphicsRectItem(QGraphicsItem* parent) : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
    enableShowBackground(false);
    enableShowBorder(true);
    setBorderPen(QPen(QColor(Qt::black)));
}

DAGraphicsRectItem::~DAGraphicsRectItem()
{
}

void DAGraphicsRectItem::setText(const QString& t)
{
    d_ptr->mText = t;
}

QString DAGraphicsRectItem::getText() const
{
    return d_ptr->mText;
}

void DAGraphicsRectItem::setTextAlignment(Qt::Alignment al)
{
    d_ptr->mTextAlignment = al;
}

Qt::Alignment DAGraphicsRectItem::getTextAlignment() const
{
    return d_ptr->mTextAlignment;
}

QPen DAGraphicsRectItem::getTextPen() const
{
    return d_ptr->getTextPen();
}

void DAGraphicsRectItem::setTextPen(const QPen& p)
{
    d_ptr->setTextPen(p);
}

bool DAGraphicsRectItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    DAGraphicsResizeableItem::saveToXml(doc, parentElement);
    QDomElement rectEle = doc->createElement("rect-info");

    QDomElement textEle = doc->createElement("text");
    textEle.setAttribute("al", enumToString(d_ptr->mTextAlignment));
    textEle.appendChild(doc->createTextNode(d_ptr->mText));
    rectEle.appendChild(textEle);
    // text-pen
    rectEle.appendChild(DAXMLFileInterface::makeElement(d_ptr->mTextPen, "text-pen", doc));
    parentElement->appendChild(rectEle);
    return true;
}

bool DAGraphicsRectItem::loadFromXml(const QDomElement* itemElement)
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
        d_ptr->mTextAlignment = stringToEnum(textEle.attribute("al"), Qt::AlignCenter);
        d_ptr->mText          = textEle.text();
    }
    return true;
}

void DAGraphicsRectItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    if (!(d_ptr->mText.isEmpty())) {
        painter->setPen(d_ptr->mTextPen);
        painter->drawText(bodyRect, d_ptr->mTextAlignment, d_ptr->mText);
    }
    painter->restore();
}

}
