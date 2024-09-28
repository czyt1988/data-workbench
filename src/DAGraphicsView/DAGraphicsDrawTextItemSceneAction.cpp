#include "DAGraphicsDrawTextItemSceneAction.h"
#include "DAGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include "DAGraphicsTextItem.h"
#include "DAGraphicsRubberBandItem.h"
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
        if (!mIsStarted) {
            mStartPoint = mouseEvent->scenePos();
            mIsStarted  = true;
			if (!mRubberBand) {
				mRubberBand = std::make_unique< DAGraphicsRubberBandItem >();
				scene()->addItem(mRubberBand.get());
			}
			mRubberBand->setBeginScenePos(mStartPoint);
			return true;
        }
    }
    return false;
}

bool DAGraphicsDrawTextItemSceneAction::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton) {
        if (mIsStarted) {
            // 说明已经点击过，这里是要完成矩形的创建
            mIsStarted               = false;
            QPointF endPos           = mouseEvent->scenePos();
            QPointF itemPos          = topLeftPoint(mStartPoint, endPos);
            DAGraphicsTextItem* item = scene()->createText_();
            item->setPos(itemPos);
            item->setBodySize(pointRectSize(mStartPoint, endPos));
            item->setSelected(true);
            mRubberBand->hide();
            scene()->removeItem(mRubberBand.get());
            mRubberBand.reset();
            end();
            return true;
        }
    }
    return false;
}

bool DAGraphicsDrawTextItemSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (mRubberBand) {
		mRubberBand->setCurrentMousePos(mouseEvent->scenePos());
	}
	return DAAbstractGraphicsSceneAction::mouseMoveEvent(mouseEvent);
}
}
