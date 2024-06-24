#include "DAAbstractGraphicsSceneAction.h"
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
	return false;
}

bool DAAbstractGraphicsSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	return false;
}

bool DAAbstractGraphicsSceneAction::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	return false;
}

void DAAbstractGraphicsSceneAction::endAction()
{

}

void DAAbstractGraphicsSceneAction::end()
{

}
}
