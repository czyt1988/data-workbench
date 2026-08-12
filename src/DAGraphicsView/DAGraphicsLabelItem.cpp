#include "DAGraphicsLabelItem.h"
#include <optional>
#include <QGraphicsScene>
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include "DAGraphicsItemFactory.h"
namespace DA
{
class DAGraphicsLabelItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsLabelItem)
public:
	PrivateData(DAGraphicsLabelItem* p);

public:
	uint64_t mID { 0 };
	std::optional< QPointF > mRelativePos;
	DAShapeKeyPoint mOriginPoint { DAShapeKeyPoint::Center };
};

/**
 * @brief PrivateData 构造函数
 * @param p 父对象指针
 */
DAGraphicsLabelItem::PrivateData::PrivateData(DAGraphicsLabelItem* p) : q_ptr(p)
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
//===============================================================
// DAGraphicsLabelItem
//===============================================================

/**
 * @brief 构造函数
 * @param parent 父图形项
 */
DAGraphicsLabelItem::DAGraphicsLabelItem(QGraphicsItem* parent) : QGraphicsSimpleTextItem(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 构造函数
 * @param str 标签文本
 * @param parent 父图形项
 */
DAGraphicsLabelItem::DAGraphicsLabelItem(const QString& str, QGraphicsItem* parent)
	: QGraphicsSimpleTextItem(str, parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAGraphicsLabelItem::~DAGraphicsLabelItem()
{
}

/**
 * @brief 将标签项保存到XML节点
 * @param doc XML文档对象
 * @param parentElement 父XML元素
 * @param ver 版本号
 * @return 保存成功返回true
 */
bool DAGraphicsLabelItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	QDomElement e = doc->createElement("label-item");
	bool on       = isHaveRelativePosition();
	if (on) {
		e.setAttribute("relative", on);
		QPointF rp = getRelativePosition();
		e.setAttribute("rx", rp.x());
		e.setAttribute("ry", rp.y());
	}
	e.setAttribute("flags", static_cast< int >(flags()));
	e.setAttribute("id", static_cast< qulonglong >(getItemID()));
	e.setAttribute("x", x());
	e.setAttribute("y", y());
	e.setAttribute("z", zValue());
	e.setAttribute("opacity", opacity());
	e.setAttribute("rotation", rotation());
	e.setAttribute("scale", scale());
	QDomElement textEle = DAXMLFileInterface::makeElement(text(), "text", doc);
	e.appendChild(textEle);
	QDomElement penEle = DAXMLFileInterface::makeElement(pen(), "pen", doc);
	e.appendChild(penEle);
	QDomElement brushEle = DAXMLFileInterface::makeElement(brush(), "brush", doc);
	e.appendChild(brushEle);

	parentElement->appendChild(e);
	return true;
}

/**
 * @brief 从XML元素加载标签项
 * @param itemElement 包含标签项信息的XML元素
 * @param ver 版本号
 * @return 加载成功返回true，未找到label-item节点返回false
 */
bool DAGraphicsLabelItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	QDomElement infoEle = itemElement->firstChildElement("label-item");
	if (infoEle.isNull()) {
		// 没有找到label-item节点，返回错误
		return false;
	}
	qreal realValue;
	qreal realValue2;
	qulonglong llv;

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
		setPos(realValue, realValue2);
	}
	if (getStringRealValue(infoEle.attribute("opacity", ""), realValue)) {
		setOpacity(realValue);
	}
	if (getStringRealValue(infoEle.attribute("rotation", ""), realValue)) {
		setRotation(realValue);
	}
	if (getStringRealValue(infoEle.attribute("scale", ""), realValue)) {
		setScale(realValue);
	}
	QDomElement textEle = infoEle.firstChildElement("text");
	if (!textEle.isNull()) {
		setText(textEle.text());
	}
	QDomElement penEle = infoEle.firstChildElement("pen");
	if (!penEle.isNull()) {
		QPen p;
		if (DAXMLFileInterface::loadElement(p, &penEle)) {
			setPen(p);
		}
	}
	QDomElement bkEle = infoEle.firstChildElement("brush");
	if (!bkEle.isNull()) {
		QBrush b;
		if (DAXMLFileInterface::loadElement(b, &bkEle)) {
			setBrush(b);
		}
	}
	return true;
}

/**
 * @brief 获取标签项的唯一ID
 * @return 标签项ID
 */
uint64_t DAGraphicsLabelItem::getItemID() const
{
	return d_ptr->mID;
}

/**
 * @brief 设置标签项的唯一ID
 * @param id 要设置的ID
 */
void DAGraphicsLabelItem::setItemID(uint64_t id)
{
	d_ptr->mID = id;
}

/**
 * @brief 设置相对位置
 * @param xp 相对x坐标
 * @param yp 相对y坐标
 */
void DAGraphicsLabelItem::setRelativePosition(qreal xp, qreal yp)
{
	d_ptr->mRelativePos = std::make_optional< QPointF >(xp, yp);
}

/**
 * @brief 获取相对位置
 * @return 相对位置坐标，若未设置则返回(0,0)
 */
QPointF DAGraphicsLabelItem::getRelativePosition() const
{
	return d_ptr->mRelativePos.value_or(QPointF());
}

/**
 * @brief 判断是否设置了相对位置
 * @return 已设置返回true，否则返回false
 */
bool DAGraphicsLabelItem::isHaveRelativePosition() const
{
    return d_ptr->mRelativePos.has_value();
}

/**
 * @brief 设置相对贴附位置
 *
 * @code
 * TopLeft        TopCenter          TopRight
 * (0,0)----------(0.5,0)------------(1,0)
 * |                                   |
 * |CenterLeft   Center     CenterRight|
 * (0,0.5)       (0.5,0.5)          (1,0.5)
 * |                                   |
 * |BottomLeft BottomCenter BottomRight|
 * (0,1)----------(0.5,1)------------(1,1)
 * @endcode

 * @param parentAttachPoint
 */
void DAGraphicsLabelItem::setAttachPoint(DAShapeKeyPoint parentAttachPoint)
{
	switch (parentAttachPoint.value()) {
	case DAShapeKeyPoint::TopLeft:
		setRelativePosition(0.0, 0.0);
		break;
	case DAShapeKeyPoint::TopCenter:
		setRelativePosition(0.5, 0.0);
		break;
	case DAShapeKeyPoint::TopRight:
		setRelativePosition(1.0, 0.0);
		break;
	case DAShapeKeyPoint::CenterLeft:
		setRelativePosition(0.0, 0.5);
		break;
	case DAShapeKeyPoint::Center:
		setRelativePosition(0.5, 0.5);
		break;
	case DAShapeKeyPoint::CenterRight:
		setRelativePosition(1.0, 0.5);
		break;
	case DAShapeKeyPoint::BottomLeft:
		setRelativePosition(0.0, 1.0);
		break;
	case DAShapeKeyPoint::BottomCenter:
		setRelativePosition(0.5, 1.0);
		break;
	case DAShapeKeyPoint::BottomRight:
		setRelativePosition(1.0, 1.0);
		break;
	default:
		break;
	}
}

/**
 * @brief 设置原点位置
 * @param originPoint 原点对应的关键点位置
 */
void DAGraphicsLabelItem::setOriginPoint(DAShapeKeyPoint originPoint)
{
	d_ptr->mOriginPoint = originPoint;
}

/**
 * @brief 获取原点位置
 * @return 原点对应的关键点位置
 */
DAShapeKeyPoint DAGraphicsLabelItem::getOriginPoint() const
{
    return d_ptr->mOriginPoint;
}

/**
 * @brief 更新位置
 */
void DAGraphicsLabelItem::updatePosition()
{
	if (!isHaveRelativePosition()) {
		return;
	}
	QRectF parentRect;
	QGraphicsItem* pi = parentItem();
	if (pi) {
		parentRect = pi->boundingRect();
	} else {
		auto sc = scene();
		if (sc) {
			parentRect = sc->sceneRect();
		} else {
			return;
		}
	}
	QPointF itemwillMovePoint = QPointF(parentRect.x() + (parentRect.width() * d_ptr->mRelativePos->x()),
										parentRect.y() + (parentRect.height() * d_ptr->mRelativePos->y()));
	QRectF br                 = boundingRect();
	QPointF offset            = d_ptr->mOriginPoint.rectKeyPoint(br);
	itemwillMovePoint -= offset;
	setPos(itemwillMovePoint);
}

/**
 * @brief 设置是否可选择
 * @param on 为true时启用可选择
 */
void DAGraphicsLabelItem::setSelectable(bool on)
{
	setFlag(ItemIsSelectable, on);
}

/**
 * @brief 图形项变化时的回调函数
 * @param change 变化类型
 * @param value 变化值
 * @return 变化后的值
 */
QVariant DAGraphicsLabelItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if (change == QGraphicsItem::ItemSceneHasChanged) {
		updatePosition();
	}
	return QGraphicsSimpleTextItem::itemChange(change, value);
}

}  // end DA namespace
