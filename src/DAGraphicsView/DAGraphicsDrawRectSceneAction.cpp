#include "DAGraphicsDrawRectSceneAction.h"
#include "DAGraphicsScene.h"
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include "DAGraphicsRectItem.h"
#include "DAGraphicsRubberBandItem.h"
// 通过任意两点，获取构成矩形的尺寸
namespace DA
{

/**
 * @brief 构造函数，初始化绘制矩形场景动作
 * @param sc 关联的图形场景
 */
DAGraphicsDrawRectSceneAction::DAGraphicsDrawRectSceneAction(DAGraphicsScene* sc) : DAAbstractGraphicsSceneAction(sc)
{
	static QPixmap s_default_cursor_pixmap = svgToPixmap(":/DAGraphicsView/svg/draw-rect.svg", QSize(20, 20));
	setCursorPixmap(s_default_cursor_pixmap);
}

/**
 * @brief 析构函数，恢复鼠标光标
 */
DAGraphicsDrawRectSceneAction::~DAGraphicsDrawRectSceneAction()
{
	restoreCursor();
}

/**
 * @brief 激活动作，设置鼠标光标为绘制矩形样式
 */
void DAGraphicsDrawRectSceneAction::beginActive()
{
	// 开始激活，把cursor设置为绘制矩形样式
	setupCursor();
}

/**
 * @brief 鼠标按下事件，开始绘制矩形
 * @param mouseEvent 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAGraphicsDrawRectSceneAction::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
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
 * @brief 鼠标释放事件，完成矩形创建
 * @param mouseEvent 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAGraphicsDrawRectSceneAction::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (mouseEvent->button() == Qt::LeftButton) {
		if (mIsStarted) {
			// 说明已经点击过，这里是要完成矩形的创建
			mIsStarted      = false;
			QPointF endPos  = mouseEvent->scenePos();
			QPointF rectPos = topLeftPoint(mStartPoint, endPos);
			QSizeF drawSize = pointRectSize(mStartPoint, endPos);
			// 如果绘制太小，默认绘制100*100
			qDebug() << "drawSize" << drawSize;
			if (drawSize.width() <= 5) {
				drawSize.setWidth(100);
			}
			if (drawSize.height() <= 5) {
				drawSize.setHeight(100);
			}
			DAGraphicsRectItem* item = scene()->createRect_(rectPos);
			item->setBodySize(drawSize);
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
 * @brief 鼠标移动事件，更新橡皮筋选区位置
 * @param mouseEvent 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAGraphicsDrawRectSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (mRubberBand) {
		mRubberBand->setCurrentMousePos(mouseEvent->scenePos());
	}
	return DAAbstractGraphicsSceneAction::mouseMoveEvent(mouseEvent);
}

}  // end namespace DA
