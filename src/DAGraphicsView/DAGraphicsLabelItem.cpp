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
DAGraphicsLabelItem::DAGraphicsLabelItem(QGraphicsItem* parent) : QGraphicsSimpleTextItem(parent), DA_PIMPL_CONSTRUCT
{
}

DAGraphicsLabelItem::DAGraphicsLabelItem(const QString& str, QGraphicsItem* parent)
	: QGraphicsSimpleTextItem(str, parent), DA_PIMPL_CONSTRUCT
{
}

DAGraphicsLabelItem::~DAGraphicsLabelItem()
{
}

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
	e.setAttribute("id", getItemID());
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

bool DAGraphicsLabelItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	QDomElement infoEle = itemElement->firstChildElement("label-item");
	if (infoEle.isNull()) {
		// 没有找到label-item节点，返回错误
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

uint64_t DAGraphicsLabelItem::getItemID() const
{
	return d_ptr->mID;
}

void DAGraphicsLabelItem::setItemID(uint64_t id)
{
	d_ptr->mID = id;
}

void DAGraphicsLabelItem::setRelativePosition(qreal xp, qreal yp)
{
	d_ptr->mRelativePos = std::make_optional< QPointF >(xp, yp);
}

QPointF DAGraphicsLabelItem::getRelativePosition() const
{
	return d_ptr->mRelativePos.value_or(QPointF());
}

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

void DAGraphicsLabelItem::setOriginPoint(DAShapeKeyPoint originPoint)
{
	d_ptr->mOriginPoint = originPoint;
}

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
	qDebug() << "parentRect:" << parentRect << ",boundingRect:" << br << ",offset:" << offset
			 << ",itemwillMovePoint:" << itemwillMovePoint;
}

void DAGraphicsLabelItem::setSelectable(bool on)
{
	setFlag(ItemIsSelectable, on);
}

QVariant DAGraphicsLabelItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if (change == QGraphicsItem::ItemSceneHasChanged) {
		updatePosition();
	}
	return QGraphicsSimpleTextItem::itemChange(change, value);
}

}  // end DA namespace
