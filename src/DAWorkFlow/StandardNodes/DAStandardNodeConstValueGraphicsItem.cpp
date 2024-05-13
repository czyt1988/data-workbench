#include "DAStandardNodeConstValueGraphicsItem.h"
#include <QPainter>
#include <QLineEdit>
#include <QDebug>
#include "DANodeLinkPointDrawDelegate.h"
namespace DA
{

DAStandardNodeConstValueGraphicsItem::DAStandardNodeConstValueGraphicsItem(DAStandardNodeConstValue* n, QGraphicsItem* p)
	: DAStandardNodeTextGraphicsItem(n, p)
{
	setShowBorder(false);
	setEditable(true);
	setSelectable(true);
	setMovable(true);
	setCurrentValueType(QVariant::String);
	setAutoAdjustSize(false);
	setBodySize(QSize(150, 30));
	getLinkPointDrawDelegate()->showLinkPointText(false);  // 不显示
}

DAStandardNodeConstValueGraphicsItem::~DAStandardNodeConstValueGraphicsItem()
{
}

void DAStandardNodeConstValueGraphicsItem::nodeDisplayNameChanged(const QString& name)
{
}

QVariant::Type DAStandardNodeConstValueGraphicsItem::getCurrentValueType() const
{
	return mValueType;
}

void DAStandardNodeConstValueGraphicsItem::setCurrentValueType(QVariant::Type v)
{
	switch (v) {
	case QVariant::String:
	case QVariant::Int:
	case QVariant::UInt:
	case QVariant::LongLong:
	case QVariant::ULongLong:
	case QVariant::Double:
	case QVariant::Char:
	case QVariant::Bool: {
		// 生成窗口
	} break;
	default:
		v = QVariant::String;
		break;
	}
	mValueType = v;
}

void DAStandardNodeConstValueGraphicsItem::paintBody(QPainter* painter,
                                                     const QStyleOptionGraphicsItem* option,
                                                     QWidget* widget,
                                                     const QRectF& bodyRect)
{
	// 绘制背景
	painter->save();
	painter->setPen(getBorderPen());
	painter->setBrush(getBackgroundBrush());
	painter->drawRoundedRect(bodyRect, mRectRadius, mRectRadius);
	painter->restore();
	//! 绘制连接点
	paintLinkPoints(painter, option, widget);
}

qreal DAStandardNodeConstValueGraphicsItem::getRectRadius() const
{
	return mRectRadius;
}

void DAStandardNodeConstValueGraphicsItem::setRectRadius(qreal newRectRadius)
{
	if (qFuzzyCompare(mRectRadius, newRectRadius))
		return;
	mRectRadius = newRectRadius;
}
}
