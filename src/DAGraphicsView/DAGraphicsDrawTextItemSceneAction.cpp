#include "DAGraphicsDrawTextItemSceneAction.h"
#include "DAGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include "DAGraphicsTextItem.h"
#include "DAGraphicsRubberBandItem.h"
namespace DA
{

/**
 * @brief 构造文本绘制场景动作
 * @param sc 关联的图形场景
 */
DAGraphicsDrawTextItemSceneAction::DAGraphicsDrawTextItemSceneAction(DAGraphicsScene* sc)
    : DAAbstractGraphicsSceneAction(sc)
{
	static QPixmap s_default_cursor_pixmap = svgToPixmap(":/DAGraphicsView/svg/draw-rect.svg", QSize(20, 20));
	setCursorPixmap(s_default_cursor_pixmap);
}

/**
 * @brief 析构函数，恢复鼠标光标
 */
DAGraphicsDrawTextItemSceneAction::~DAGraphicsDrawTextItemSceneAction()
{
	restoreCursor();
}

/**
 * @brief 开始激活，设置绘制文本样式的鼠标光标
 */
void DAGraphicsDrawTextItemSceneAction::beginActive()
{
	// 开始激活，把cursor设置为绘制文本样式
	setupCursor();
}

/**
 * @brief 鼠标按下事件处理，开始绘制文本区域的矩形框
 * @param mouseEvent 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
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

/**
 * @brief 鼠标释放事件处理，完成文本项的创建
 * @param mouseEvent 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
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
            destroy();
            return true;
        }
    }
    return false;
}

/**
 * @brief 鼠标移动事件处理，更新橡皮筋选区位置
 * @param mouseEvent 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAGraphicsDrawTextItemSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (mRubberBand) {
		mRubberBand->setCurrentMousePos(mouseEvent->scenePos());
	}
	return DAAbstractGraphicsSceneAction::mouseMoveEvent(mouseEvent);
}
}
