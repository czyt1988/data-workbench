#include "DAAbstractGraphicsViewAction.h"
#include "DAAbstractGraphicsViewAction.h"
#include "DAAbstractGraphicsViewAction.h"
#include "DAAbstractGraphicsViewAction.h"
#include "DAAbstractGraphicsViewAction.h"
#include "DAAbstractGraphicsViewAction.h"
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include "DAGraphicsView.h"

namespace DA{

DAAbstractGraphicsViewAction::DAAbstractGraphicsViewAction(DAGraphicsView * v):mView(v)
{
}

DAAbstractGraphicsViewAction::~DAAbstractGraphicsViewAction()
{
}

DAGraphicsView * DAAbstractGraphicsViewAction::view() const
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

/**
 * @brief 捕获绘图事件.
 *  
 * @param 绘图事件
 * @return 返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
 * @note 正常情况下，此函数都不应该返回true
 */
bool DAAbstractGraphicsViewAction::paintEvent(QPaintEvent * event)
{
    return false;
}

bool DAAbstractGraphicsViewAction::keyPressEvent(QKeyEvent * event)
{
    return false;
}

bool DAAbstractGraphicsViewAction::keyReleaseEvent(QKeyEvent * event)
{
    return false;
}

bool DAAbstractGraphicsViewAction::mouseDoubleClickEvent(QMouseEvent * event)
{
    return false;
}

bool DAAbstractGraphicsViewAction::mouseMoveEvent(QMouseEvent * event)
{
    return false;
}

bool DAAbstractGraphicsViewAction::mousePressEvent(QMouseEvent * event)
{
    return false;
}

bool DAAbstractGraphicsViewAction::mouseReleaseEvent(QMouseEvent * event)
{
    return false;
}

}