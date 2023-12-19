#include "DAGraphicsItem.h"
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QEvent>
namespace DA
{

class DAGraphicsItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsItem)
public:
    PrivateData(DAGraphicsItem* p);
    bool mIsShowBorder { false };      ///< 是否显示边框
    bool mIsShowBackground { false };  ///< 是否显示背景
    QPen mBorderPen;                   ///< 边框画笔
    QBrush mBackgroundBrush;           ///< 背景画刷
};

DAGraphicsItem::PrivateData::PrivateData(DAGraphicsItem* p) : q_ptr(p)
{
}

//===================================================
// DAGraphicsItem
//===================================================
DAGraphicsItem::DAGraphicsItem(QGraphicsItem* parent) : QGraphicsObject(parent), DA_PIMPL_CONSTRUCT
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
    QPointF scPos       = scenePos();
    infoEle.setAttribute("x", scPos.x());
    infoEle.setAttribute("y", scPos.y());
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
        // 没有找到info节点，返回错误
        return false;
    }
    qreal realValue;
    qreal realValue2;

    if (getStringRealValue(infoEle.attribute("x", "0"), realValue) && getStringRealValue(infoEle.attribute("y", "0"), realValue2)) {
        setScenePos(realValue, realValue2);
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
        // 解析shapeInfoEle信息
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
    d_ptr->mBorderPen = p;
}

/**
 * @brief 获取当前边框画笔
 * @return
 */
QPen DAGraphicsItem::getBorderPen() const
{
    return d_ptr->mBorderPen;
}

/**
 * @brief 设置显示边框
 * @param on
 */
void DAGraphicsItem::setShowBorder(bool on)
{
    d_ptr->mIsShowBorder = on;
}

/**
 * @brief 是否显示边框
 * @return
 */
bool DAGraphicsItem::isShowBorder() const
{
    return d_ptr->mIsShowBorder;
}

/**
 * @brief 设置背景
 * @param b
 */
void DAGraphicsItem::setBackgroundBrush(const QBrush& b)
{
    d_ptr->mBackgroundBrush = b;
}

/**
 * @brief 获取背景
 * @return
 */
QBrush DAGraphicsItem::getBackgroundBrush() const
{
    return d_ptr->mBackgroundBrush;
}

/**
 * @brief 设置显示背景
 * @param on
 */
void DAGraphicsItem::setShowBackground(bool on)
{
    d_ptr->mIsShowBackground = on;
}

/**
 * @brief 是否显示背景
 * @return
 */
bool DAGraphicsItem::isShowBackground() const
{
    return d_ptr->mIsShowBackground;
}

/**
   @brief 分组的位置发生了改变
    此函数是在分组后才会回调，分组后，分组的移动对于item来说是不移动的，这时候无法触发ItemPositionChanged的改变，从而导致一些异常，因此需要分组告诉子对象分组移动了
   @param pos 分组的位置，如果要获取当前item的位置，获取parent，后再map
 */
void DAGraphicsItem::groupPositionChanged(const QPointF& p)
{
    Q_UNUSED(p);
}

void DAGraphicsItem::setScenePos(const QPointF& p)
{
    setPos(mapToParent(mapFromScene(p)));
}

void DAGraphicsItem::setScenePos(qreal x, qreal y)
{
    setScenePos(QPointF(x, y));
}

// bool DAGraphicsItem::sceneEvent(QEvent* event)
//{
//     qDebug() << "DAGraphicsItem::sceneEvent" << event->type();
//     return QGraphicsObject::sceneEvent(event);
// }

}  // end of DA
