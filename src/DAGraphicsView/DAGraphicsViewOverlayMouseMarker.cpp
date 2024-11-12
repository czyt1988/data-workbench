#include "DAGraphicsViewOverlayMouseMarker.h"
#include <QPainter>
#include <QDebug>
#include <QGraphicsView>
namespace DA
{
DAGraphicsViewOverlayMouseMarker::DAGraphicsViewOverlayMouseMarker(QGraphicsView* parent)
	: DAAbstractGraphicsViewOverlay(parent)
{
	setMaskMode(DAAbstractWidgetOverlay::MaskHint);
}

DAGraphicsViewOverlayMouseMarker::~DAGraphicsViewOverlayMouseMarker()
{
}

void DAGraphicsViewOverlayMouseMarker::drawOverlay(QPainter* painter) const
{
	if (!isActive() || mDrawPen == Qt::NoPen) {
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

QRegion DAGraphicsViewOverlayMouseMarker::maskHint() const
{
	QRegion mask;
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

DAGraphicsViewOverlayMouseMarker::MarkerStyle DAGraphicsViewOverlayMouseMarker::getMarkerStyle() const
{
	return mMarkerStyle;
}

void DAGraphicsViewOverlayMouseMarker::setMarkerStyle(MarkerStyle v)
{
	mMarkerStyle = v;
}

QPen DAGraphicsViewOverlayMouseMarker::getDrawPen() const
{
	return mDrawPen;
}

void DAGraphicsViewOverlayMouseMarker::setDrawPen(const QPen& v)
{
	mDrawPen = v;
}
}  // end ns da
