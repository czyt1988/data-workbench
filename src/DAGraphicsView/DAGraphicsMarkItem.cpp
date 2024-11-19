#include "DAGraphicsMarkItem.h"
#include "DAGraphicsItemFactory.h"
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include "DAGraphicsScene.h"
namespace DA
{

class DAGraphicsMarkItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsMarkItem)
public:
	PrivateData(DAGraphicsMarkItem* p);
    int markShapeStyle;///< 对应DAGraphicsMarkItem::MarkShape
};

DAGraphicsMarkItem::PrivateData::PrivateData(DAGraphicsMarkItem* p) : q_ptr(p)
{

}

//===================================================
// DAGraphicsMarkItem
//===================================================
DAGraphicsMarkItem::DAGraphicsMarkItem(QGraphicsItem* parent) : DAGraphicsItem(parent), DA_PIMPL_CONSTRUCT
{
}

DAGraphicsMarkItem::~DAGraphicsMarkItem()
{
}

/**
 * @brief 保存到xml中
 * @param doc
 * @param parentElement
 * @return
 */
bool DAGraphicsMarkItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	

	return true;
}

void DAGraphicsMarkItem::setMarkShape(int shapeStyle)
{
    d_ptr->markShapeStyle = shapeStyle;
}

int DAGraphicsMarkItem::getMarkShape() const
{
    return d_ptr->markShapeStyle;
}

void DAGraphicsMarkItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    qDebug() << option->rect;
    painter->setPen(getBorderPen());
    painter->setBrush(getBackgroundBrush());
    switch (getMarkShape()) {
    case DAGraphicsMarkItem::ShapeRect: {
        painter->drawRect(option->rect);
    }break;
    case DAGraphicsMarkItem::ShapeCross: {
        //获取parent的中心
    }break;
    default:
        break;
    }
}

QRectF DAGraphicsMarkItem::boundingRect() const
{
    auto pi = parentItem();
    if (pi) {
        return pi->boundingRect().adjusted(-3, -3, 3, 3);
    }
    return QRectF(-50,-50,100,100);
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
	
	return true;
}


}  // end of DA
