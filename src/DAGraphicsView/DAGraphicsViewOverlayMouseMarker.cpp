#include "DAGraphicsViewOverlayMouseMarker.h"
#include <QPainter>
#include <QDebug>
#include <QGraphicsView>
namespace DA
{
/**
 * @brief 构造函数
 * @param parent 父 QGraphicsView
 */
DAGraphicsViewOverlayMouseMarker::DAGraphicsViewOverlayMouseMarker(QGraphicsView* parent)
	: DAAbstractGraphicsViewOverlay(parent)
{
	setMaskMode(DAAbstractWidgetOverlay::MaskHint);
}

/**
 * @brief 析构函数
 */
DAGraphicsViewOverlayMouseMarker::~DAGraphicsViewOverlayMouseMarker()
{
}

/**
 * @brief 绘制鼠标标记覆盖层
 * @param painter 绘图设备
 */
void DAGraphicsViewOverlayMouseMarker::drawOverlay(QPainter* painter) const
{
	if (!isActive() || mDrawPen == Qt::NoPen || mMarkerStyle == NoMarkerStyle) {
		return;
	}
	const QRect r  = overlayRect();
	const QPoint p = getMousePos();
	painter->setPen(mDrawPen);
	switch (mMarkerStyle) {
	case HLine:
		painter->drawLine(r.left(), p.y(), r.right(), p.y());
		break;
	case VLine:
		painter->drawLine(p.x(), r.top(), p.x(), r.bottom());
		break;
	case CrossLine:
		painter->drawLine(r.left(), p.y(), r.right(), p.y());
		painter->drawLine(p.x(), r.top(), p.x(), r.bottom());
		break;
	default:
		break;
	}
}

/**
 * @brief 返回遮罩提示区域，用于指定需要重绘的标记区域
 * @return 标记区域对应的 QRegion
 */
QRegion DAGraphicsViewOverlayMouseMarker::maskHint() const
{
	QRegion mask;
	if (mMarkerStyle == NoMarkerStyle) {
		return mask;
	}
	const int pw   = mDrawPen.width();
	const QRect r  = overlayRect();
	const QPoint p = getMousePos();
	switch (mMarkerStyle) {
	case HLine:
		mask += maskRegionVOrHLine(QLine(r.left(), p.y(), r.right(), p.y()), pw);
		break;
	case VLine:
		mask += maskRegionVOrHLine(QLine(p.x(), r.top(), p.x(), r.bottom()), pw);
		break;
	case CrossLine:
		mask += maskRegionVOrHLine(QLine(r.left(), p.y(), r.right(), p.y()), pw);
		mask += maskRegionVOrHLine(QLine(p.x(), r.top(), p.x(), r.bottom()), pw);
		break;
	default:
		break;
	}

	return mask;
}

/**
 * @brief 获取/设置鼠标标记样式
 * @return 当前标记样式
 * @param v 要设置的标记样式
 * @sa MarkerStyle
 */
DAGraphicsViewOverlayMouseMarker::MarkerStyle DAGraphicsViewOverlayMouseMarker::getMarkerStyle() const
{
	return mMarkerStyle;
}

void DAGraphicsViewOverlayMouseMarker::setMarkerStyle(MarkerStyle v)
{
	mMarkerStyle = v;
}

/**
 * @brief 获取/设置绘制画笔
 * @return 当前绘制画笔
 * @param v 要设置的画笔
 */
QPen DAGraphicsViewOverlayMouseMarker::getDrawPen() const
{
	return mDrawPen;
}

void DAGraphicsViewOverlayMouseMarker::setDrawPen(const QPen& v)
{
	mDrawPen = v;
}
}  // end ns da
