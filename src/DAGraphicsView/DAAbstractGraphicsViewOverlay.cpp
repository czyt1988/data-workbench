#include "DAAbstractGraphicsViewOverlay.h"
namespace DA
{
DAAbstractGraphicsViewOverlay::DAAbstractGraphicsViewOverlay(QWidget* parent) : DAAbstractWidgetOverlay(parent)
{
}

DAAbstractGraphicsViewOverlay::~DAAbstractGraphicsViewOverlay()
{
}

QRect DAAbstractGraphicsViewOverlay::overlayRect() const
{
	const QWidget* widget = parentWidget();
	if (widget) {
		return widget->contentsRect();
	}
	return QRect();
}
}  // end ns da
