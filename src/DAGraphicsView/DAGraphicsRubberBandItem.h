#ifndef DAGRAPHICSRUBBERBANDITEM_H
#define DAGRAPHICSRUBBERBANDITEM_H
#include "DAGraphicsViewGlobal.h"
#include <QGraphicsRectItem>

namespace DA
{
/**
 * @brief 橡皮筋（范围选框）item
 */
class DAGRAPHICSVIEW_API DAGraphicsRubberBandItem : public QGraphicsRectItem
{
public:
	DAGraphicsRubberBandItem(QGraphicsItem* parent = nullptr);
	// 选框开始点，一般是mousePress设置的点
	void setBeginScenePos(const QPointF& scenePos);
	// 选框的跟随鼠标点
	void setCurrentMousePos(const QPointF& scenePos);
	// 用于适应尺寸
	void adjustBeginPosToMouse(QGraphicsScene* sc, const QPoint& screenPos, const QPointF& scenePos);
};

}  // end namespace DA
#endif  // DAGRAPHICSRUBBERBANDITEM_H
