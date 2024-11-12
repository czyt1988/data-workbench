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
    if(!mIsInstalled){
        mIsInstalled = tryInstall();
    }
	if (obj && (obj == mScene)) {
		switch (event->type()) {
		case QEvent::GraphicsSceneMouseMove: {
			QGraphicsSceneMouseEvent* sme = static_cast< QGraphicsSceneMouseEvent* >(event);
			//这时是Scene的鼠标事件，需要转换到view上面
            QPointF sp = sme->scenePos();
			viewMouseMove(view()->mapFromScene(sp));
			break;
		}
		case QEvent::GraphicsSceneMousePress: {
			QGraphicsSceneMouseEvent* sme = static_cast< QGraphicsSceneMouseEvent* >(event);
			//这时是Scene的鼠标事件，需要转换到view上面
            QPointF sp = sme->scenePos();
			viewMousePress(view()->mapFromScene(sp));
			break;
		}
		case QEvent::GraphicsSceneMouseRelease: {
			QGraphicsSceneMouseEvent* sme = static_cast< QGraphicsSceneMouseEvent* >(event);
			//这时是Scene的鼠标事件，需要转换到view上面
            QPointF sp = sme->scenePos();
			viewMouseRelease(view()->mapFromScene(sp));
			break;
		}
		default:
			break;
		}
	}
	return false;
}

QGraphicsView* DAAbstractGraphicsViewOverlay::view() const
{
	return qobject_cast< QGraphicsView* >(parentWidget());
}

bool DAAbstractGraphicsViewOverlay::isValid() const
{
	return !mScene.isNull();
}

void DAAbstractGraphicsViewOverlay::viewMouseMove(const QPoint& viewPos,const QPointF& secnePos)
{
	qDebug() << "viewMouseMove:" << pos;
	mMousePos = pos;
	if (isActive()) {
		updateOverlay();
	}
}

void DAAbstractGraphicsViewOverlay::viewMousePress(const QPoint& viewPos,const QPointF& secnePos)
{
	mMousePos = pos;
	if (isActive()) {
		updateOverlay();
	}
}

void DAAbstractGraphicsViewOverlay::viewMouseRelease(const QPoint& viewPos,const QPointF& secnePos)
{
	mMousePos = pos;
	if (isActive()) {
		updateOverlay();
	}
}

bool DAAbstractGraphicsViewOverlay::tryInstall()
{
	QGraphicsView* v   = view();
	QGraphicsScene* sc = v->scene();
	if (!sc) {
		return false;
	}
	if (sc == mScene) {
		// sc不为空且和原来的一样，返回true，表示安装完成
		return true;
	}
	if (mScene) {
        //如果scene非空，且不和sc相等，说明view切换了场景，需要移除监控
		mScene->removeEventFilter(this);
	}
	mScene = sc;
	mScene->installEventFilter(this);
	return true;
}
}  // end ns da
