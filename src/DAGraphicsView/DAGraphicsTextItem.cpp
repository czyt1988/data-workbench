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

DAGraphicsTextItem::~DAGraphicsTextItem()
{
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

/**
 * @brief 设置文本
 * @param v
 */
void DAGraphicsTextItem::setText(const QString& v)
{
    d_ptr->mTextItem->setPlainText(v);
}

/**
 * @brief 文本
 * @return
 */
QString DAGraphicsTextItem::getText() const
{
    return d_ptr->mTextItem->toPlainText();
}

/**
 * @brief 设置相对父窗口的相对定位
 * @param xp x位置相对父级item的宽度占比，如parentCorner=Qt::TopLeftCorner,xp=0.2
 * 那么textitem的x定位设置为parentItem.boundingRect().topLeft().x() + parentItem.width() * xp
 * @param yp y位置相对父级item的宽度占比，如parentCorner=Qt::TopLeftCorner,yp=0.1
 * 那么textitem的y定位设置为parentItem.boundingRect().topLeft().y() + parentItem.width() * yp5
 */
void DAGraphicsTextItem::setRelativePosition(qreal xp, qreal yp)
{
    d_ptr->mRelativeX = xp;
    d_ptr->mRelativeY = yp;
}

QPointF DAGraphicsTextItem::getRelativePosition() const
{
    return QPointF(d_ptr->mRelativeX, d_ptr->mRelativeY);
}

/**
 * @brief 设置是否开启相对定位
 * @note 相对定位需要有父级item，否则无效
 * @param on
 */
void DAGraphicsTextItem::setEnableRelativePosition(bool on)
{
    d_ptr->mEnableRelativePosition = on;
}

/**
 * @brief 是否开启相对定位
 * @return
 */
bool DAGraphicsTextItem::isEnableRelativePosition() const
{
    return d_ptr->mEnableRelativePosition;
}

/**
 * @brief 更新相对位置
 */
void DAGraphicsTextItem::updateRelativePosition()
{
    QGraphicsItem* pi = parentItem();
    if (nullptr == pi) {
        return;
    }
    QRectF br = pi->boundingRect();
    QPointF p(br.width() * d_ptr->mRelativeX, br.height() * d_ptr->mRelativeY);
    switch (d_ptr->mRelativeCorner) {
    case Qt::TopLeftCorner:
        p = br.topLeft() + p;
        break;
    case Qt::TopRightCorner:
        p = br.topRight() + p;
        break;
    case Qt::BottomLeftCorner:
        p = br.bottomLeft() + p;
        break;
    case Qt::BottomRightCorner:
        p = br.bottomRight() + p;
        break;
    default:
        break;
    }
    setPos(p);
}

void DAGraphicsTextItem::getRelativePositionCorner(Qt::Corner v)
{
    d_ptr->mRelativeCorner = v;
}

/**
 * @brief 获取相对位置对应父级item的位置
 * @return
 */
Qt::Corner DAGraphicsTextItem::getRelativePositionCorner() const
{
    return d_ptr->mRelativeCorner;
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
    // DAGraphicsResizeableItem::paintBody(painter, option, widget, bodyRect);
}

}  // end da
