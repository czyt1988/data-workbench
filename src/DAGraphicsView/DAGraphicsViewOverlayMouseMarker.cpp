#include "DAGraphicsViewOverlayMouseMarker.h"
#include <QPainter>

namespace DA
{
DAGraphicsViewOverlayMouseMarker::DAGraphicsViewOverlayMouseMarker(QWidget* parent)
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
	QRect r = overlayRect();
	painter->setPen(mDrawPen);
	switch (mMarkerStyle) {
	case HLine:
		painter->drawLine(r.left(), mPoint.y(), r.right(), mPoint.y());
		break;
	case VLine:
		painter->drawLine(mPoint.x(), r.top(), mPoint.x(), r.bottom());
		break;
	case CrossLine:
		painter->drawLine(r.left(), mPoint.y(), r.right(), mPoint.y());
		painter->drawLine(mPoint.x(), r.top(), mPoint.x(), r.bottom());
		break;
	default:
		break;
	}
}

QRegion DAGraphicsViewOverlayMouseMarker::maskHint() const
{
	QRegion mask;
	const int pw  = mDrawPen.width();
	const QRect r = overlayRect();
	switch (mMarkerStyle) {
	case HLine:
		mask += maskRegionVOrHLine(QLine(r.left(), mPoint.y(), r.right(), mPoint.y()), pw);
		break;
	case VLine:
		mask += maskRegionVOrHLine(QLine(mPoint.x(), r.top(), mPoint.x(), r.bottom()), pw);
		break;
	case CrossLine:
		mask += maskRegionVOrHLine(QLine(r.left(), mPoint.y(), r.right(), mPoint.y()), pw);
		mask += maskRegionVOrHLine(QLine(mPoint.x(), r.top(), mPoint.x(), r.bottom()), pw);
		break;
	default:
		break;
	}
	return mask;
}

bool DAGraphicsViewOverlayMouseMarker::isActive() const
{
	return mIsActive;
}

void DAGraphicsViewOverlayMouseMarker::setActive(bool v)
{
	mIsActive = v;
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
