#include "DAGraphicsDrawTextItemSceneAction.h"
#include "DAGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include "DAGraphicsTextItem.h"
namespace DA
{
DAGraphicsDrawTextItemSceneAction::DAGraphicsDrawTextItemSceneAction(DAGraphicsScene* sc)
	: DAAbstractGraphicsSceneAction(sc)
{
	static QPixmap s_default_cursor_pixmap = svgToPixmap(":/DAGraphicsView/svg/draw-rect.svg", QSize(20, 20));
	setCursorPixmap(s_default_cursor_pixmap);
}

DAGraphicsDrawTextItemSceneAction::~DAGraphicsDrawTextItemSceneAction()
{
	restoreCursor();
}

void DAGraphicsDrawTextItemSceneAction::beginActive()
{
	// 开始激活，把cursor设置为绘制文本样式
	setupCursor();
}

bool DAGraphicsDrawTextItemSceneAction::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (mouseEvent->button() == Qt::LeftButton) {
		// 左键点击
		if (!mHaveBeingPressed) {
			mStartPoint       = mouseEvent->scenePos();
			mHaveBeingPressed = true;
			return true;
		} else {
			// 说明已经点击过，这里是要完成矩形的创建
			QPointF endPos           = mouseEvent->scenePos();
			QPointF itemPos          = topLeftPoint(mStartPoint, endPos);
			DAGraphicsTextItem* item = scene()->createText_();
			item->setPos(itemPos);
			item->setBodySize(pointRectSize(mStartPoint, endPos));
			item->setSelected(true);
			end();
			return true;
		}
	} else if (mouseEvent->button() == Qt::RightButton) {
		end();
		return false;
	}
	return false;
}

bool DAGraphicsDrawTextItemSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	return DAAbstractGraphicsSceneAction::mouseMoveEvent(mouseEvent);
}
}
