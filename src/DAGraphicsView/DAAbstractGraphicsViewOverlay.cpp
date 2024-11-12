#include "DAAbstractGraphicsViewOverlay.h"
#include <QMouseEvent>
#include <QHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QGraphicsView>
#include <QGraphicsScene>
namespace DA
{
DAAbstractGraphicsViewOverlay::DAAbstractGraphicsViewOverlay(QGraphicsView* parent) : DAAbstractWidgetOverlay(parent)
{
	parent->setMouseTracking(true);
	mIsInstalled = tryInstall();
	setActive(true);
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
	if (!mIsInstalled) {
		mIsInstalled = tryInstall();
	}
	if (obj && (obj == mViewPort)) {
		switch (event->type()) {
		case QEvent::MouseMove: {
			QMouseEvent* me = static_cast< QMouseEvent* >(event);
			viewMouseMove(view()->mapFromGlobal(mViewPort->mapToGlobal(me->pos())));
			break;
		}
		case QEvent::MouseButtonPress: {
			QMouseEvent* me = static_cast< QMouseEvent* >(event);
			viewMousePress(view()->mapFromGlobal(mViewPort->mapToGlobal(me->pos())));
			break;
		}
		case QEvent::MouseButtonRelease: {
			QMouseEvent* me = static_cast< QMouseEvent* >(event);
			viewMouseRelease(view()->mapFromGlobal(mViewPort->mapToGlobal(me->pos())));
			break;
		}
		default:
			break;
		}
	}
	return DAAbstractWidgetOverlay::eventFilter(obj, event);
}

QGraphicsView* DAAbstractGraphicsViewOverlay::view() const
{
	return qobject_cast< QGraphicsView* >(parentWidget());
}

bool DAAbstractGraphicsViewOverlay::isValid() const
{
	return mViewPort != nullptr;
}

void DAAbstractGraphicsViewOverlay::viewMouseMove(const QPoint& viewPos)
{
	mMousePos = viewPos;
	if (isActive()) {
		updateOverlay();
	}
}

void DAAbstractGraphicsViewOverlay::viewMousePress(const QPoint& viewPos)
{
	mMousePos = viewPos;
	if (isActive()) {
		updateOverlay();
	}
}

void DAAbstractGraphicsViewOverlay::viewMouseRelease(const QPoint& viewPos)
{
	mMousePos = viewPos;
	if (isActive()) {
		updateOverlay();
	}
}

bool DAAbstractGraphicsViewOverlay::tryInstall()
{
	QGraphicsView* v = view();
	QWidget* vp      = v->viewport();
	if (!vp) {
		return false;
	}
	if (vp == mViewPort) {
		// vp不为空且和原来的一样，返回true，表示安装完成
		return true;
	}
	mViewPort = vp;
	mViewPort->installEventFilter(this);
	return true;
}
}  // end ns da
