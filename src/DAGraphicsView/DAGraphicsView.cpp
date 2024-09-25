#include "DAGraphicsView.h"
#include <QWheelEvent>
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
#include <QKeyEvent>
#include "DAGraphicsScene.h"

#define DAGRAPHICSVIEW_DEBUG_PRINT 0

namespace DA
{
class DAGraphicsView::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsView)
public:
	PrivateData(DAGraphicsView* p);

public:
	qreal mScaleMax { 3.0 };
	qreal mScaleMin { 0.333 };
	qreal mZoomStep { 0.1 };
	qreal mScaleValue { 1.0 };          ///< 记录缩放的值
	bool mIsPadding { false };          ///< 标记是否开始拖动
	bool mIsSpacebarPressed { false };  ///< 标记空格是否被按下
	QPointF mMouseScenePos;
	QPoint mStartPadPos;  ///< 记录开始拖动的位置
	DAGraphicsView::ZoomFlags mZoomFlags { DAGraphicsView::ZoomUseWheelAndCtrl };
	DAGraphicsView::PadFlags mPadFlags { DAGraphicsView::PadByWheelMiddleButton
										 | DAGraphicsView::PadBySpaceWithMouseLeftButton };
};

DAGraphicsView::PrivateData::PrivateData(DAGraphicsView* p) : q_ptr(p)
{
}

////////////////////////////////////////////////
/// DAGraphicsView
////////////////////////////////////////////////

DAGraphicsView::DAGraphicsView(QWidget* parent) : QGraphicsView(parent), DA_PIMPL_CONSTRUCT
{
    init();
}

DAGraphicsView::DAGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent), DA_PIMPL_CONSTRUCT
{
    init();
}

DAGraphicsView::~DAGraphicsView()
{
}

void DAGraphicsView::init()
{
	setMouseTracking(true);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setDragMode(QGraphicsView::RubberBandDrag);
	// setDragMode(QGraphicsView::ScrollHandDrag);
}

void DAGraphicsView::setScaleRange(qreal min, qreal max)
{
	d_ptr->mScaleMin = min;
	d_ptr->mScaleMax = max;
}

qreal DAGraphicsView::getScaleMaxFactor() const
{
	return (d_ptr->mScaleMax);
}

qreal DAGraphicsView::getScaleMinFactor() const
{
    return (d_ptr->mScaleMin);
}

/**
 * @brief 放大
 */
void DAGraphicsView::zoomIn()
{
	//    qreal scaleFactor = this->matrix().m11();
	//    if (scaleFactor >= m_scaleMax) {
	//        return;
	//    }
	//    scale(1 + d_ptr->mZoomStep, 1 + d_ptr->mZoomStep);

	qreal scale = d_ptr->mScaleValue + d_ptr->mZoomStep;
	if (scale >= d_ptr->mScaleMax) {
		scale = d_ptr->mScaleMax;
	}
	d_ptr->mScaleValue = scale;
	setTransform(QTransform::fromScale(d_ptr->mScaleValue, d_ptr->mScaleValue));
}

/**
 * @brief 缩小
 */
void DAGraphicsView::zoomOut()
{
	//    qreal scaleFactor = this->matrix().m11();
	//    if (scaleFactor <= d_ptr->mScaleMin) {
	//        return;
	//    }
	//    scale(1 - d_ptr->mScaleMin, 1 - d_ptr->mScaleMin);

	qreal scale = d_ptr->mScaleValue - d_ptr->mZoomStep;
	if (scale < d_ptr->mScaleMin) {
		scale = d_ptr->mScaleMin;
	}
	d_ptr->mScaleValue = scale;
	setTransform(QTransform::fromScale(d_ptr->mScaleValue, d_ptr->mScaleValue));
}

/**
 * @brief 设置全部可见尺寸
 * @note 如果是空场景，此函数无动作
 */
void DAGraphicsView::zoomFit()
{
	QGraphicsScene* sc = scene();
	if (!sc) {
		return;
	}
	QRectF rect;
	QList< QGraphicsItem* > items = sc->items();
	if (items.isEmpty()) {
		return;
	}
	for (const auto& item : qAsConst(items)) {
		QRectF boundingRect = item->boundingRect();
		boundingRect.moveTo(item->scenePos());
		rect = rect.united(boundingRect);
	}
	int space = 10;
	rect.adjust(-space, -space, space, space);
	fitInView(rect, Qt::KeepAspectRatio);
	// 这时要更新scale值m11和m22,这里只取m11
	auto tm            = transform();
	d_ptr->mScaleValue = tm.m11();
	// 这时更新最小scale
	if (d_ptr->mScaleValue < d_ptr->mScaleMin) {
		d_ptr->mScaleMin = d_ptr->mScaleValue;
	}
}

/**
 * @brief 中键滚动
 * @param event
 */
void DAGraphicsView::wheelEvent(QWheelEvent* event)
{
	if (d_ptr->mZoomFlags.testFlag(ZoomNotUseWheel)) {
		QGraphicsView::wheelEvent(event);
		return;
	}
	if (d_ptr->mZoomFlags.testFlag(ZoomUseWheelAndCtrl)) {
		// 通过ctrl来缩放，需要判断是否按住了ctrl
		if (event->modifiers().testFlag(Qt::ControlModifier)) {
			wheelZoom(event);
			event->accept();
			return;
		}
	} else if (d_ptr->mZoomFlags.testFlag(ZoomUseWheel)) {
		wheelZoom(event);
		event->accept();
		return;
	}
	QGraphicsView::wheelEvent(event);
}

void DAGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
	d_ptr->mMouseScenePos = mapToScene(event->pos());
	if (isPadding()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#else
#endif
		int dx = Qt5Qt6Compat_QXXEvent_x(event) - d_ptr->mStartPadPos.x();
		int dy = Qt5Qt6Compat_QXXEvent_y(event) - d_ptr->mStartPadPos.y();
#if DAGRAPHICSVIEW_DEBUG_PRINT
		qDebug() << QString("isPadding begin move,mStartPadPos=(%1,%2) horizontalScrollBar "
							"value=%3,dx=%4,verticalScrollBar value=%5 dy=%6,event.x=%7,y=%8")
						.arg(d_ptr->mStartPadPos.x())
						.arg(d_ptr->mStartPadPos.y())
						.arg(horizontalScrollBar()->value())
						.arg(dx)
						.arg(verticalScrollBar()->value())
						.arg(dy)
						.arg(Qt5Qt6Compat_QXXEvent_x(event))
						.arg(Qt5Qt6Compat_QXXEvent_y(event));
#endif
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - dx);
		verticalScrollBar()->setValue(verticalScrollBar()->value() - dy);
		d_ptr->mStartPadPos = Qt5Qt6Compat_QXXEvent_Pos(event);
#if DAGRAPHICSVIEW_DEBUG_PRINT
		qDebug() << QString(
						"isPadding end,mStartPadPos=(%1,%2) horizontalScrollBar value=%3,verticalScrollBar value=%4")
						.arg(d_ptr->mStartPadPos.x())
						.arg(d_ptr->mStartPadPos.y())
						.arg(horizontalScrollBar()->value())
						.arg(verticalScrollBar()->value());
#endif
		// 移动状态不把事件向下传递
		event->accept();
	}
	QGraphicsView::mouseMoveEvent(event);
}

void DAGraphicsView::mousePressEvent(QMouseEvent* event)
{
	if (d_ptr->mPadFlags.testFlag(PadDiable)) {
		QGraphicsView::mousePressEvent(event);
		return;
	}
	if (d_ptr->mPadFlags.testFlag(PadByWheelMiddleButton)) {
		if (event->button() == Qt::MiddleButton) {
			// 设置了中间拖动
			startPad(event);
			// 把事件截断
			event->accept();
			return;
		}
	}
	if (d_ptr->mPadFlags.testFlag(PadBySpaceWithMouseLeftButton)) {
		// 设置了空格鼠标拖动
		if (event->button() == Qt::LeftButton && d_ptr->mIsSpacebarPressed) {
			qDebug() << "mose press2";
			startPad(event);
			// 把事件截断
			event->accept();
			return;
		}
	}
	QGraphicsView::mousePressEvent(event);
}

void DAGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
	if (isPadding()) {
		if (d_ptr->mPadFlags.testFlag(PadByWheelMiddleButton)) {
			if (event->button() == Qt::MiddleButton) {
				endPad();
				event->accept();
				return;
			}
		}
		if (d_ptr->mPadFlags.testFlag(PadBySpaceWithMouseLeftButton)) {
			// 设置了空格鼠标拖动
			if (event->button() == Qt::LeftButton) {
				endPad();
				event->accept();
				return;
			}
		}
	}
	QGraphicsView::mouseReleaseEvent(event);
}

void DAGraphicsView::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Space) {
		d_ptr->mIsSpacebarPressed = true;
	}
	QGraphicsView::keyPressEvent(event);
}

void DAGraphicsView::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Space) {
		d_ptr->mIsSpacebarPressed = false;
	}
	QGraphicsView::keyReleaseEvent(event);
}

void DAGraphicsView::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	// 获取 view 的当前大小
	// if (QGraphicsScene* sc = scene()) {
	//     QRectF sr          = sc->sceneRect();
	//     QRect viewportRect = viewport()->rect();
	//     if (sr.width() < viewportRect.width() || sr.height() < viewportRect.height()) {
	//         setSceneRect(viewportRect);
	//     }
	// }
}

/**
 * @brief 滚轮事件的缩放
 * @param event
 */
void DAGraphicsView::wheelZoom(QWheelEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	if (event->delta() > 0) {
#else
	if (event->angleDelta().y() > 0) {
#endif
		zoomIn();
	} else {
		zoomOut();
	}
}

/**
 * @brief 开始拖动
 * @param event
 */
void DAGraphicsView::startPad(QMouseEvent* event)
{
	d_ptr->mIsPadding   = true;
	d_ptr->mStartPadPos = event->pos();
	setCursor(Qt::ClosedHandCursor);
	setDragMode(QGraphicsView::NoDrag);
}

/**
 * @brief 结束拖动
 * @param event
 */
void DAGraphicsView::endPad()
{
	d_ptr->mIsPadding = false;
	setCursor(Qt::ArrowCursor);
	setDragMode(QGraphicsView::RubberBandDrag);
}

/**
 * @brief 获取鼠标对应的scence的位置坐标
 * @return
 */
QPointF DAGraphicsView::getMouseScenePos() const
{
    return d_ptr->mMouseScenePos;
}

/**
 * @brief 设置zoomflags
 * @param zf
 */
void DAGraphicsView::setZoomFrags(ZoomFlags zf)
{
    d_ptr->mZoomFlags = zf;
}

/**
 * @brief 获取zoomflags
 * @return
 */
DAGraphicsView::ZoomFlags DAGraphicsView::getZoomFlags() const
{
    return d_ptr->mZoomFlags;
}

/**
 * @brief 判断是否在拖动
 * @return
 */
bool DAGraphicsView::isPadding() const
{
    return d_ptr->mIsPadding;
}

/**
 * @brief 设置拖动属性
 * @param pf
 */
void DAGraphicsView::setPaddingFrags(PadFlags pf)
{
    d_ptr->mPadFlags = pf;
}

/**
 * @brief 获取拖动属性
 * @return
 */
DAGraphicsView::PadFlags DAGraphicsView::getPaddingFrags() const
{
    return d_ptr->mPadFlags;
}

/**
 * @brief 选中的item
 * @return
 */
QList< DAGraphicsItem* > DAGraphicsView::selectedDAItems() const
{
	QGraphicsScene* sc    = scene();
	DAGraphicsScene* dasc = qobject_cast< DAGraphicsScene* >(sc);
	if (!sc) {
		return QList< DAGraphicsItem* >();
	}
	if (dasc) {
		return dasc->selectedDAItems();
	}
	QList< DAGraphicsItem* > res;
	const QList< QGraphicsItem* > its = sc->selectedItems();
	for (QGraphicsItem* item : its) {
		if (DAGraphicsItem* i = dynamic_cast< DAGraphicsItem* >(item)) {
			res.append(i);
		}
	}
	return res;
}

/**
 * @brief 是否空格被按下
 * @return
 */
bool DAGraphicsView::isSpacebarPressed() const
{
    return d_ptr->mIsSpacebarPressed;
}

/**
 * @brief 选中所有可选的item
 */
void DAGraphicsView::selectAll()
{
	DAGraphicsScene* s = qobject_cast< DAGraphicsScene* >(scene());
	if (s) {
		// DAGraphicsSceneWithUndoStack的selectAll只发射一次selectionChanged信号
		s->selectAll();
	} else {
		// 非DAGraphicsSceneWithUndoStack，就执行选中所有
		QList< QGraphicsItem* > its = items();
		for (QGraphicsItem* i : its) {
			if (!i->isSelected() && i->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
				// 只有没有被选上，且是可选的才会执行选中动作
				i->setSelected(true);
			}
		}
	}
}

/**
 * @brief 取消所有选中
 */
void DAGraphicsView::clearSelection()
{
	DAGraphicsScene* s = qobject_cast< DAGraphicsScene* >(scene());
	if (s) {
		// DAGraphicsSceneWithUndoStack的selectAll只发射一次selectionChanged信号
		s->clearSelection();
	} else {
		// 非DAGraphicsSceneWithUndoStack，就执行选中所有
		QList< QGraphicsItem* > its = items();
		for (QGraphicsItem* i : its) {
			if (!i->isSelected() && i->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
				// 只有没有被选上，且是可选的才会执行选中动作
				i->setSelected(false);
			}
		}
	}
}

bool DAGraphicsView::isEnaleWheelZoom() const
{
	return !d_ptr->mZoomFlags.testFlag(ZoomNotUseWheel);
}

void DAGraphicsView::setEnaleWheelZoom(bool enaleWheelZoom, ZoomFlags zf)
{
	if (!enaleWheelZoom) {
		setZoomFrags(ZoomNotUseWheel);
	} else {
		zf.setFlag(ZoomNotUseWheel, false);
		setZoomFrags(zf);
	}
}

}  // end DA
