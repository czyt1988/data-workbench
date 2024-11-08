#include "DAGraphicsMouseCrossLineViewAction.h"
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>
#include "DAGraphicsView.h"
namespace DA
{
DAGraphicsMouseCrossLineViewAction::DAGraphicsMouseCrossLineViewAction(DAGraphicsView* v, ActionTypes type)
    : DAAbstractGraphicsViewAction(v)
{
}

DAGraphicsMouseCrossLineViewAction::~DAGraphicsMouseCrossLineViewAction()
{
}

void DAGraphicsMouseCrossLineViewAction::setActionTypes(ActionTypes t)
{
	mType = t;
}

DAGraphicsMouseCrossLineViewAction::ActionTypes DAGraphicsMouseCrossLineViewAction::getActionTypes() const
{
	return mType;
}

void DAGraphicsMouseCrossLineViewAction::setDrawPen(const QPen& c)
{
	mDrawPen = c;
}

QPen DAGraphicsMouseCrossLineViewAction::getDrawPen() const
{
	return mDrawPen;
}

void DAGraphicsMouseCrossLineViewAction::setDrawColor(const QColor& c)
{
	mDrawPen.setColor(c);
}

QColor DAGraphicsMouseCrossLineViewAction::getDrawColor() const
{
	return mDrawPen.color();
}

void DAGraphicsMouseCrossLineViewAction::setCrossViewPos(const QPoint& p)
{
	mPos = p;
}

QPoint DAGraphicsMouseCrossLineViewAction::getCrossViewPos() const
{
	return mPos;
}

void DAGraphicsMouseCrossLineViewAction::setCrossScenePos(const QPointF& p)
{
	DAGraphicsView* v = view();
	QPoint vp         = v->mapFromScene(p);
	QRectF f(0, 0, 100, 100);
	f.moveCenter(p);
	v->ensureVisible(f);
	mPos = vp;
}

void DAGraphicsMouseCrossLineViewAction::setValid(bool on)
{
    mValid = on;
}

bool DAGraphicsMouseCrossLineViewAction::isValid() const
{
    return mValid;
}

void DAGraphicsMouseCrossLineViewAction::beginActive()
{
    setValid(true);
}

void DAGraphicsMouseCrossLineViewAction::endAction()
{
}

bool DAGraphicsMouseCrossLineViewAction::paintEvent(QPaintEvent* event)
{
    if(!mValid){
        return false;
    }
    QRect viewRect = view()->rect();
	if (mPos.isNull() || !viewRect.contains(mPos)) {
		return false;
	}
	QPainter painter(view());
	painter.setPen(mDrawPen);
    //绘制水平
    painter.drawLine(QPoint(viewRect.x(), mPos.y()), QPoint(viewRect.x()+viewRect.width(), mPos.y()));
    //绘制竖直线
    painter.drawLine(QPoint(mPos.x(), viewRect.y()), QPoint(mPos.x(), viewRect.y()+viewRect.height()));
	return false;
}

bool DAGraphicsMouseCrossLineViewAction::keyPressEvent(QKeyEvent* event)
{
    if(OneTimeMarking == mType){
        setValid(false);
    }
	return false;
}

bool DAGraphicsMouseCrossLineViewAction::mouseMoveEvent(QMouseEvent* event)
{
    if(FollowMouse == mType){
        //更新位置
        mPos = event->pos();
    }
	return false;
}

bool DAGraphicsMouseCrossLineViewAction::mousePressEvent(QMouseEvent* event)
{
    if(OneTimeMarking == mType){
        setValid(false);
    }else if(FollowMouse == mType){
        //更新位置
        mPos = event->pos();
    }
	return false;
}

bool DAGraphicsMouseCrossLineViewAction::mouseReleaseEvent(QMouseEvent* event)
{
    if(OneTimeMarking == mType){
        setValid(false);
    }else if(FollowMouse == mType){
        //更新位置
        mPos = event->pos();
    }
	return false;
}

}  // end DA
