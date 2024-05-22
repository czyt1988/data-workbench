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
	setCurrentValueType(QMetaType::QString);
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

QMetaType::Type DAStandardNodeConstValueGraphicsItem::getCurrentValueType() const
{
	return mValueType;
}

void DAStandardNodeConstValueGraphicsItem::setCurrentValueType(QMetaType::Type v)
{
	switch (v) {
	case QMetaType::QString:
	case QMetaType::Int:
	case QMetaType::UInt:
	case QMetaType::LongLong:
	case QMetaType::ULongLong:
	case QMetaType::Double:
	case QMetaType::Char:
	case QMetaType::Bool: {
		// 生成窗口
	} break;
	default:
		v = QMetaType::QString;
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
