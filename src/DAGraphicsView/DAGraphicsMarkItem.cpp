#include "DAGraphicsMarkItem.h"
#include "DAGraphicsItemFactory.h"
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <optional>
#include "DAGraphicsScene.h"
namespace DA
{

class DAGraphicsMarkItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsMarkItem)
public:
	PrivateData(DAGraphicsMarkItem* p);
	std::optional< QRectF > optBoundingRect;  ///< 边界，可以设置，如果不设置就以父item为对象
	int markShapeStyle;                       ///< 对应DAGraphicsMarkItem::MarkShape
};

/**
 * @brief PrivateData 构造函数
 * @param p 指向 DAGraphicsMarkItem 的指针
 */
DAGraphicsMarkItem::PrivateData::PrivateData(DAGraphicsMarkItem* p) : q_ptr(p)
{
}

//===================================================
// DAGraphicsMarkItem
//===================================================

/**
 * @brief 构造函数
 * @param parent 父 QGraphicsItem
 */
DAGraphicsMarkItem::DAGraphicsMarkItem(QGraphicsItem* parent) : DAGraphicsItem(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
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
	DAGraphicsItem::saveToXml(doc, parentElement, ver);
	return true;
}

/**
 * @brief 从xml中加载
 * @param itemElement
 * @return
 */
bool DAGraphicsMarkItem::loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)
{
	DAGraphicsItem::loadFromXml(parentElement, ver);
	return true;
}

/**
 * @brief 设置标记的边界矩形
 * @param r 边界矩形，若为空则清除自定义边界
 */
void DAGraphicsMarkItem::setMarkBoundingRect(const QRectF& r)
{
	if (r.isNull()) {
		d_ptr->optBoundingRect = std::nullopt;
	} else {
		d_ptr->optBoundingRect = r;
	}
}

/**
 * @brief 获取/设置标记形状样式
 * @param shapeStyle 标记形状样式，对应 DAGraphicsMarkItem::MarkShape
 * @return 标记形状样式
 */
void DAGraphicsMarkItem::setMarkShape(int shapeStyle)
{
	d_ptr->markShapeStyle = shapeStyle;
}

int DAGraphicsMarkItem::getMarkShape() const
{
	return d_ptr->markShapeStyle;
}

/**
 * @brief 绘制标记图形
 * @param painter 画家对象
 * @param option 样式选项
 * @param widget 关联的 widget
 */
void DAGraphicsMarkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setPen(getBorderPen());
	painter->setBrush(getBackgroundBrush());
	switch (getMarkShape()) {
	case DAGraphicsMarkItem::ShapeRect: {
		painter->drawRect(option->rect);
	} break;
	case DAGraphicsMarkItem::ShapeCross: {
		// 获取中心
		QPointF center = option->rect.center();
		painter->drawLine(option->rect.left(), center.y(), option->rect.right(), center.y());
		painter->drawLine(center.x(), option->rect.top(), center.x(), option->rect.bottom());
	} break;
	default:
		break;
	}
}

/**
 * @brief 获取标记的边界矩形
 *
 * 若设置了自定义边界则返回自定义值，否则返回父级 item 的边界（略微扩大），
 * 若无父级则返回默认矩形。
 *
 * @return 边界矩形
 */
QRectF DAGraphicsMarkItem::boundingRect() const
{
	if (d_ptr->optBoundingRect) {
		return d_ptr->optBoundingRect.value();
	}
	auto pi = parentItem();
	if (pi) {
		// 略微比父级大一点
		return pi->boundingRect().adjusted(-3, -3, 3, 3);
	}
	return QRectF(-50, -50, 100, 100);
}

}  // end of DA
