#include "DAGraphicsRubberBandItem.h"
#include <QGraphicsScene>

namespace DA
{

DAGraphicsRubberBandItem::DAGraphicsRubberBandItem(QGraphicsItem* parent) : QGraphicsRectItem(parent)
{
}

void DAGraphicsRubberBandItem::setBeginScenePos(const QPointF& scenePos)
{
	setPos(scenePos);
}

void DAGraphicsRubberBandItem::setCurrentMousePos(const QPointF& scenePos)
{
	QRectF r(pos(), scenePos);
	r.moveTo(QPointF(0, 0));
	setRect(r);
}

/**
 * @brief 适应试图尺寸，保证当前点击的开始位置就位于鼠标所在位置
 *
 * 这个主要在场景比较小的情况下适用
 */
void DAGraphicsRubberBandItem::adjustBeginPosToMouse(QGraphicsScene* sc, const QPoint& screenPos, const QPointF& scenePos)
{
}

}  // end namespace DA
