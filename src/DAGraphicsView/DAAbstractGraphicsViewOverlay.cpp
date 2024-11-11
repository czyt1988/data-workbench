#include "DAAbstractGraphicsViewOverlay.h"
#include <QMouseEvent>
#include <QHoverEvent>
#include <QDebug>
#include <QGraphicsView>
namespace DA
{
DAAbstractGraphicsViewOverlay::DAAbstractGraphicsViewOverlay(QGraphicsView* parent) : DAAbstractWidgetOverlay(parent)
{
	setActive(true);
    //QGraphicsView要捕获hover move而不是mouse move
    parent->setAttribute(Qt::WA_Hover, true);
    parent->setMouseTracking(true);
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

QPoint DAAbstractGraphicsViewOverlay::getMousePos() const
{
	return mMousePos;
}

bool DAAbstractGraphicsViewOverlay::isActive() const
{
	return mIsActive;
}

void DAAbstractGraphicsViewOverlay::setActive(bool v)
{
	mIsActive = v;
	setMouseTracking(v);
}

bool DAAbstractGraphicsViewOverlay::eventFilter(QObject* obj, QEvent* event)
{
	if (obj && obj == parentWidget()) {
		qDebug() << "eventFilter ok,even type = " << event->type();
		switch (event->type()) {
		case QEvent::HoverMove: {
			QHoverEvent* he = static_cast< QHoverEvent* >(event);
			//这时是viewport的鼠标移动事件，需要转换到view上面
			viewMouseMove(he->pos());
			break;
		}
		case QEvent::MouseButtonPress: {
			viewMousePressEvent(static_cast< QMouseEvent* >(event));
			break;
		}
		case QEvent::MouseButtonRelease: {
			viewMouseReleaseEvent(static_cast< QMouseEvent* >(event));
			break;
		}
		default:
			break;
		}
	}
	return false;
}

void DAAbstractGraphicsViewOverlay::viewMouseMove(const QPoint& pos)
{
	qDebug() << "viewMouseMove:" << pos;
	mMousePos = pos;
	if (isActive()) {
		updateOverlay();
	}
}

void DAAbstractGraphicsViewOverlay::viewMousePressEvent(QMouseEvent* event)
{
	qDebug() << "viewMousePressEvent";
	if (event) {
		mMousePos = event->pos();
		if (isActive()) {
			updateOverlay();
		}
	}
}

void DAAbstractGraphicsViewOverlay::viewMouseReleaseEvent(QMouseEvent* event)
{
	if (event) {
		mMousePos = event->pos();
		if (isActive()) {
			updateOverlay();
		}
	}
}
}  // end ns da
