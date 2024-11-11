#include "DAAbstractGraphicsViewAction.h"
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include "DAGraphicsView.h"

namespace DA
{

DAAbstractGraphicsViewAction::DAAbstractGraphicsViewAction(DAGraphicsView* v) : mView(v)
{
}

DAAbstractGraphicsViewAction::~DAAbstractGraphicsViewAction()
{
}

DAGraphicsView* DAAbstractGraphicsViewAction::view() const
{
	return mView;
}

void DAAbstractGraphicsViewAction::destroy()
{
	auto v = view();
	if (v) {
		v->clearViewAction();
	}
}

void DAAbstractGraphicsViewAction::beginActive()
{
}

void DAAbstractGraphicsViewAction::endAction()
{
}

void DAAbstractGraphicsViewAction::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
}

bool DAAbstractGraphicsViewAction::keyPressEvent(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

bool DAAbstractGraphicsViewAction::keyReleaseEvent(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

bool DAAbstractGraphicsViewAction::mouseDoubleClickEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

bool DAAbstractGraphicsViewAction::mouseMoveEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

bool DAAbstractGraphicsViewAction::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

bool DAAbstractGraphicsViewAction::mouseReleaseEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

}
