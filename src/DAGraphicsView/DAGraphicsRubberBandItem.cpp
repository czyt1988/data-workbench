#include "DAGraphicsRubberBandItem.h"
#include <QGraphicsScene>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父项
 */
DAGraphicsRubberBandItem::DAGraphicsRubberBandItem(QGraphicsItem* parent) : QGraphicsRectItem(parent)
{
}

/**
 * @brief 设置选框的开始点位置（通常是mousePress时的场景坐标）
 * @param scenePos 开始点场景坐标
 */
void DAGraphicsRubberBandItem::setBeginScenePos(const QPointF& scenePos)
{
	setPos(scenePos);
}

/**
 * @brief 设置选框跟随鼠标的当前位置，根据开始点和当前点更新矩形区域
 * @param scenePos 当前鼠标场景坐标
 */
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
