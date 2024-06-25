#include "DAAbstractGraphicsSceneAction.h"
#include "DAGraphicsScene.h"

namespace DA
{
DAAbstractGraphicsSceneAction::DAAbstractGraphicsSceneAction(DAGraphicsScene* sc) : mScene(sc)
{
}

DAAbstractGraphicsSceneAction::~DAAbstractGraphicsSceneAction()
{
}

DAGraphicsScene* DAAbstractGraphicsSceneAction::scene() const
{
	return mScene;
}

void DAAbstractGraphicsSceneAction::beginActive()
{
}

bool DAAbstractGraphicsSceneAction::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent);
	return false;
}

bool DAAbstractGraphicsSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent);
	return false;
}

bool DAAbstractGraphicsSceneAction::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent);
	return false;
}

void DAAbstractGraphicsSceneAction::endAction()
{
}

void DAAbstractGraphicsSceneAction::end()
{
    auto sc = scene();
    if (sc) {
        sc->clearSceneAction();
    }
}
}
