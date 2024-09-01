#include "DAGraphicsItem.h"
#include "DAGraphicsItemFactory.h"
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QEvent>
#include "DAGraphicsScene.h"
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
	uint64_t mID { 0 };                ///< id
};

DAGraphicsItem::PrivateData::PrivateData(DAGraphicsItem* p) : q_ptr(p)
{
	// DAGraphicsItemFactory::generateID通过一个uint32_t生成一个uint64_t的id
	union Combine__ {
		uint32_t a;
		void* b;
	};
	Combine__ tmp;
	tmp.b = p;
	mID   = DAGraphicsItemFactory::generateID(tmp.a);
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
bool DAGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	// if (ver.isNull() || (ver.majorVersion() == 1 && ver.minorVersion() <= 1))

	// v1.3.0
	QDomElement infoEle = doc->createElement("info");
	QPointF scPos       = scenePos();
	infoEle.setAttribute("flags", static_cast< int >(flags()));
	infoEle.setAttribute("id", getItemID());
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
bool DAGraphicsItem::loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)
{
	QDomElement infoEle = parentElement->firstChildElement("info");
	if (infoEle.isNull()) {
		// 没有找到info节点，返回错误
		return false;
	}
	qreal realValue;
	qreal realValue2;
	uint64_t llv;

	QString strflags = infoEle.attribute("flags");
	if (!strflags.isEmpty()) {
		bool ok       = false;
		int flagvalue = strflags.toInt(&ok);
		if (ok) {
			GraphicsItemFlags f = static_cast< GraphicsItemFlags >(flagvalue);
			setFlags(f);
		}
	}

	if (getStringULongLongValue(infoEle.attribute("id"), llv)) {
		setItemID(llv);
	}

	if (getStringRealValue(infoEle.attribute("x", "0"), realValue)
		&& getStringRealValue(infoEle.attribute("y", "0"), realValue2)) {
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
					enableShowBackground(isShowBk);
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
 * @brief 设置是否可选中
 * @param on
 */
void DAGraphicsItem::setSelectable(bool on)
{
    setFlag(ItemIsSelectable, on);
}

/**
 * @brief 判断可否被选中
 * @return
 */
bool DAGraphicsItem::isSelectable() const
{
    return flags().testFlag(ItemIsSelectable);
}

/**
 * @brief 设置为是否可移动
 * @param on
 */
void DAGraphicsItem::setMovable(bool on)
{
    setFlag(ItemIsMovable, on);
}

/**
 * @brief 判断是否可以移动
 * @return
 */
bool DAGraphicsItem::isMovable() const
{
    return flags().testFlag(ItemIsMovable);
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
void DAGraphicsItem::enableShowBackground(bool on)
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

uint64_t DAGraphicsItem::getItemID() const
{
    return d_ptr->mID;
}

void DAGraphicsItem::setItemID(uint64_t id)
{
    d_ptr->mID = id;
}

/**
   @brief dynamic_cast为DAGraphicsItem
   @param i
   @return
 */
DAGraphicsItem* DAGraphicsItem::cast(QGraphicsItem* i)
{
	return dynamic_cast< DAGraphicsItem* >(i);
}

DAGraphicsScene* DAGraphicsItem::daScene() const
{
	return dynamic_cast< DAGraphicsScene* >(scene());
}

/**
 * @brief 判断当前场景是否为只读模式，只读模式不允许操作
 *
 * 在继承此类时要通过此函数判断当前的状态
 *
 * 只读模式下，一些值会被过滤，例如：
 * @code
 * QVariant DAGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
 * {
 *     switch (change) {
 *     case QGraphicsItem::ItemRotationChange: {
 *         if (isSceneReadOnly()) {
 *             return rotation();
 *         }
 *     } break;
 *     case QGraphicsItem::ItemPositionChange: {
 *         if (isSceneReadOnly()) {
 *             return pos();
 *         }
 *     } break;
 *     case QGraphicsItem::ItemOpacityChange: {
 *         if (isSceneReadOnly()) {
 *             return opacity();
 *         }
 *     } break;
 *     default:
 *         break;
 *     }
 *     return QGraphicsObject::itemChange(change, value);
 * }
 * @endcode
 * @return
 */
bool DAGraphicsItem::isSceneReadOnly() const
{
	if (DAGraphicsScene* sc = daScene()) {
		return sc->isReadOnly();
	}
	return false;
}

/**
 * @brief 这里主要对只读模式的过滤
 * @param change
 * @param value
 * @return
 */
QVariant DAGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	switch (change) {
	case QGraphicsItem::ItemRotationChange: {
		if (isSceneReadOnly()) {
			return rotation();
		}
	} break;
	case QGraphicsItem::ItemPositionChange: {
		if (isSceneReadOnly()) {
			return pos();
		}
	} break;
	case QGraphicsItem::ItemOpacityChange: {
		if (isSceneReadOnly()) {
			return opacity();
		}
	} break;
	default:
		break;
	}
	return QGraphicsObject::itemChange(change, value);
}

// bool DAGraphicsItem::sceneEvent(QEvent* event)
//{
//     qDebug() << "DAGraphicsItem::sceneEvent" << event->type();
//     return QGraphicsObject::sceneEvent(event);
// }

}  // end of DA
