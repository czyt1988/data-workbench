#include "DAGraphicsView.h"
#include <QWheelEvent>
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
#include <QKeyEvent>
#include "DAGraphicsScene.h"

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
	qreal mScaleValue { 1.0 };  ///< 记录缩放的值
	bool mIsPadding { false };  ///< 标记是否开始拖动
	QPointF mMouseScenePos;
	QPoint mStartPadPos;  ///< 记录开始拖动的位置
	DAGraphicsView::ZoomFlags mZoomFlags { DAGraphicsView::ZoomUseWheelAndCtrl };
	DAGraphicsView::PadFlags mPadFlags { DAGraphicsView::PadByWheelMiddleButton };
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
//        qDebug() << tr("isPadding horizontalScrollBar value=%1,dx=%2,verticalScrollBar value=%3 dy=%4")
//                            .arg(horizontalScrollBar()->value())
//                            .arg(event->x() - d_ptr->mStartPadPos.x())
//                            .arg(verticalScrollBar()->value())
//                            .arg(event->y() - d_ptr->mStartPadPos.y());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#else
#endif
		horizontalScrollBar()->setValue(horizontalScrollBar()->value()
										- (Qt5Qt6Compat_QXXEvent_x(event) - d_ptr->mStartPadPos.x()));
		verticalScrollBar()->setValue(verticalScrollBar()->value()
									  - (Qt5Qt6Compat_QXXEvent_x(event) - d_ptr->mStartPadPos.y()));
		d_ptr->mStartPadPos = Qt5Qt6Compat_QXXEvent_Pos(event);
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
	}
	QGraphicsView::mouseReleaseEvent(event);
}

/**
 * @brief 处理按钮事件
 * @param event
 */
void DAGraphicsView::keyPressEvent(QKeyEvent* event)
{
	if (event->modifiers().testFlag(Qt::ControlModifier)) {
		// Ctrl键
		switch (event->key()) {
		case Qt::Key_Equal:  // Ctrl + 放大
			zoomIn();
			event->accept();
			return;
		case Qt::Key_Minus:  // Ctrl - 缩小
			zoomOut();
			event->accept();
			return;
		case Qt::Key_A:  // Ctrl + A
			selectAll();
			event->accept();
			return;
		default:
			break;
		}
	} else {
		//        switch (event->key()) {
		//        default:
		//            break;
		//        }
	}
	// 向下传递
	QGraphicsView::keyPressEvent(event);
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
 * @brief 设置全部可见尺寸
 * @note 如果是空场景，此函数无动作
 */
void DAGraphicsView::setWholeView()
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
