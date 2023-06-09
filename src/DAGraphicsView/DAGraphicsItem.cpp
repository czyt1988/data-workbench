﻿#include "DAGraphicsItem.h"
#include <QDomDocument>
#include <QDomElement>

namespace DA
{

class DAGraphicsItemPrivate
{
    DA_IMPL_PUBLIC(DAGraphicsItem)
public:
    DAGraphicsItemPrivate(DAGraphicsItem* p);
    bool _isShowBorder;      ///< 是否显示边框
    QPen _borderPen;         ///< 边框画笔
    bool _isShowBackground;  ///< 是否显示背景
    QBrush _bkBrush;         ///< 背景画刷
};

DAGraphicsItemPrivate::DAGraphicsItemPrivate(DAGraphicsItem* p)
    : q_ptr(p), _isShowBorder(false), _isShowBackground(false)
{
}

DAGraphicsItem::DAGraphicsItem(QGraphicsItem* parent) : QGraphicsObject(parent), d_ptr(new DAGraphicsItemPrivate(this))
{
}

DAGraphicsItem::~DAGraphicsItem()
{
}
/**
 * @brief 保存到xml中
 * @param doc
 * @param parentElement
 * @return
 */
bool DAGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    QDomElement infoEle = doc->createElement("info");
    infoEle.setAttribute("x", x());
    infoEle.setAttribute("y", y());
    infoEle.setAttribute("z", zValue());
    infoEle.setAttribute("acceptDrops", acceptDrops());
    infoEle.setAttribute("enable", isEnabled());
    infoEle.setAttribute("opacity", opacity());
    infoEle.setAttribute("rotation", rotation());
    infoEle.setAttribute("scale", scale());
    QString tt = toolTip();
    if (!tt.isEmpty()) {
        QDomElement toolTipEle = doc->createElement("toolTip");
        toolTipEle.appendChild(doc->createTextNode(tt));
        infoEle.appendChild(toolTipEle);
    }
    QDomElement shapeInfoEle = doc->createElement("shape-info");
    shapeInfoEle.setAttribute("show-border", isShowBorder());
    shapeInfoEle.setAttribute("show-bk", isShowBackground());
    if (isShowBorder()) {
        QDomElement borderPenEle = DAXMLFileInterface::makeElement(getBorderPen(), "border-pen", doc);
        shapeInfoEle.appendChild(borderPenEle);
    }
    if (isShowBackground()) {
        QDomElement bkEle = DAXMLFileInterface::makeElement(getBackgroundBrush(), "bk-brush", doc);
        shapeInfoEle.appendChild(bkEle);
    }
    infoEle.appendChild(shapeInfoEle);
    parentElement->appendChild(infoEle);
    return true;
}

/**
 * @brief 从xml中加载
 * @param itemElement
 * @return
 */
bool DAGraphicsItem::loadFromXml(const QDomElement* parentElement)
{
    QDomElement infoEle = parentElement->firstChildElement("info");
    if (infoEle.isNull()) {
        //没有找到info节点，返回错误
        return false;
    }
    qreal realValue;
    qreal realValue2;

    if (getStringRealValue(infoEle.attribute("x", "0"), realValue) && getStringRealValue(infoEle.attribute("y", "0"), realValue2)) {
        setPos(realValue, realValue2);
    }
    if (getStringRealValue(infoEle.attribute("z", ""), realValue)) {
        setZValue(realValue);
    }
    setAcceptDrops(getStringBoolValue(infoEle.attribute("acceptDrops", "0")));
    setEnabled(getStringBoolValue(infoEle.attribute("enable", "1")));
    if (getStringRealValue(infoEle.attribute("opacity", ""), realValue)) {
        setOpacity(realValue);
    }
    if (getStringRealValue(infoEle.attribute("rotation", ""), realValue)) {
        setRotation(realValue);
    }
    if (getStringRealValue(infoEle.attribute("scale", ""), realValue)) {
        setScale(realValue);
    }
    QDomElement toolTipEle = infoEle.firstChildElement("toolTip");
    if (!toolTipEle.isNull()) {
        setToolTip(toolTipEle.text());
    }
    QDomElement shapeInfoEle = infoEle.firstChildElement("shape-info");
    if (!shapeInfoEle.isNull()) {
        //解析shapeInfoEle信息
        bool isShowBorder = getStringBoolValue(shapeInfoEle.attribute("show-border", "0"));
        bool isShowBk     = getStringBoolValue(shapeInfoEle.attribute("show-bk", "0"));
        if (isShowBorder) {
            QDomElement borderPenEle = shapeInfoEle.firstChildElement("border-pen");
            if (!borderPenEle.isNull()) {
                QPen p;
                if (DAXMLFileInterface::loadElement(p, &borderPenEle)) {
                    setShowBorder(isShowBorder);
                    setBorderPen(p);
                }
            }
        }
        if (isShowBk) {
            QDomElement bkEle = shapeInfoEle.firstChildElement("bk-brush");
            if (!bkEle.isNull()) {
                QBrush b;
                if (DAXMLFileInterface::loadElement(b, &bkEle)) {
                    setShowBackground(isShowBk);
                    setBackgroundBrush(b);
                }
            }
        }
    }
    return true;
}

/**
 * @brief 设置边框画笔
 * @note 设置后不会重绘，重绘用户自己调用update
 * @param p
 */
void DAGraphicsItem::setBorderPen(const QPen& p)
{
    d_ptr->_borderPen = p;
}

/**
 * @brief 获取当前边框画笔
 * @return
 */
QPen DAGraphicsItem::getBorderPen() const
{
    return d_ptr->_borderPen;
}

/**
 * @brief 设置显示边框
 * @param on
 */
void DAGraphicsItem::setShowBorder(bool on)
{
    d_ptr->_isShowBorder = on;
}

/**
 * @brief 是否显示边框
 * @return
 */
bool DAGraphicsItem::isShowBorder() const
{
    return d_ptr->_isShowBorder;
}

/**
 * @brief 设置背景
 * @param b
 */
void DAGraphicsItem::setBackgroundBrush(const QBrush& b)
{
    d_ptr->_bkBrush = b;
}

/**
 * @brief 获取背景
 * @return
 */
QBrush DAGraphicsItem::getBackgroundBrush() const
{
    return d_ptr->_bkBrush;
}

/**
 * @brief 设置显示背景
 * @param on
 */
void DAGraphicsItem::setShowBackground(bool on)
{
    d_ptr->_isShowBackground = on;
}

/**
 * @brief 是否显示背景
 * @return
 */
bool DAGraphicsItem::isShowBackground() const
{
    return d_ptr->_isShowBackground;
}

}  // end of DA
