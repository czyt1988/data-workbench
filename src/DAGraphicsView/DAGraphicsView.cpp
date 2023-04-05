#include "DAGraphicsView.h"
#include <QMatrix>
#include <QWheelEvent>
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
#include <QKeyEvent>
#include "DAGraphicsSceneWithUndoStack.h"

////////////////////////////////////////////////
///
////////////////////////////////////////////////

using namespace DA;

////////////////////////////////////////////////
/// DAGraphicsView
////////////////////////////////////////////////

DAGraphicsView::DAGraphicsView(QWidget* parent)
    : QGraphicsView(parent), m_scaleMax(3), m_scaleMin(0.333), m_zoomStep(0.1), m_scaleValue(1.0), m_isPadding(false)
{
    init();
}

DAGraphicsView::DAGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent), m_scaleMax(3), m_scaleMin(0.333), m_zoomStep(0.1), m_isPadding(false)
{
    init();
}

void DAGraphicsView::init()
{
    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    // setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setDragMode(QGraphicsView::RubberBandDrag);
    // setDragMode(QGraphicsView::ScrollHandDrag);
    m_zoomFlags = ZoomUseWheelAndCtrl;
    m_padFlags  = PadByWheelMiddleButton;
}

void DAGraphicsView::setScaleRange(qreal min, qreal max)
{
    m_scaleMin = min;
    m_scaleMax = max;
}

qreal DAGraphicsView::getScaleMaxFactor() const
{
    return (m_scaleMax);
}

qreal DAGraphicsView::getScaleMinFactor() const
{
    return (m_scaleMin);
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
    //    scale(1 + m_zoomStep, 1 + m_zoomStep);

    qreal scale = m_scaleValue + m_zoomStep;
    if (scale >= m_scaleMax) {
        scale = m_scaleMax;
    }
    m_scaleValue = scale;
    setTransform(QTransform::fromScale(m_scaleValue, m_scaleValue));
}

/**
 * @brief 缩小
 */
void DAGraphicsView::zoomOut()
{
    //    qreal scaleFactor = this->matrix().m11();
    //    if (scaleFactor <= m_scaleMin) {
    //        return;
    //    }
    //    scale(1 - m_scaleMin, 1 - m_scaleMin);

    qreal scale = m_scaleValue - m_zoomStep;
    if (scale < m_scaleMin) {
        scale = m_scaleMin;
    }
    m_scaleValue = scale;
    setTransform(QTransform::fromScale(m_scaleValue, m_scaleValue));
}

/**
 * @brief 中键滚动
 * @param event
 */
void DAGraphicsView::wheelEvent(QWheelEvent* event)
{
    if (m_zoomFlags.testFlag(ZoomNotUseWheel)) {
        QGraphicsView::wheelEvent(event);
        return;
    }
    if (m_zoomFlags.testFlag(ZoomUseWheelAndCtrl)) {
        //通过ctrl来缩放，需要判断是否按住了ctrl
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            wheelZoom(event);
            event->accept();
            return;
        }
    } else if (m_zoomFlags.testFlag(ZoomUseWheel)) {
        wheelZoom(event);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void DAGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    m_mouseScenePos = mapToScene(event->pos());
    if (isPadding()) {
        //        qDebug() << tr("isPadding horizontalScrollBar value=%1,dx=%2,verticalScrollBar value=%3 dy=%4")
        //                            .arg(horizontalScrollBar()->value())
        //                            .arg(event->x() - m_startPadPos.x())
        //                            .arg(verticalScrollBar()->value())
        //                            .arg(event->y() - m_startPadPos.y());
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - m_startPadPos.x()));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - m_startPadPos.y()));
        m_startPadPos = event->pos();
        //移动状态不把事件向下传递
        event->accept();
    }
    QGraphicsView::mouseMoveEvent(event);
}

void DAGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (m_padFlags.testFlag(PadDiable)) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    if (m_padFlags.testFlag(PadByWheelMiddleButton)) {
        if (event->button() == Qt::MiddleButton) {
            //设置了中间拖动
            startPad(event);
            //把事件截断
            event->accept();
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void DAGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (isPadding()) {
        if (m_padFlags.testFlag(PadByWheelMiddleButton)) {
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
    if (event->delta() > 0) {
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
    m_isPadding   = true;
    m_startPadPos = event->pos();
    setCursor(Qt::ClosedHandCursor);
    setDragMode(QGraphicsView::NoDrag);
}

/**
 * @brief 结束拖动
 * @param event
 */
void DAGraphicsView::endPad()
{
    m_isPadding = false;
    setCursor(Qt::ArrowCursor);
    setDragMode(QGraphicsView::RubberBandDrag);
}

/**
 * @brief 获取鼠标对应的scence的位置坐标
 * @return
 */
QPointF DAGraphicsView::getMouseScenePos() const
{
    return m_mouseScenePos;
}

/**
 * @brief 设置zoomflags
 * @param zf
 */
void DAGraphicsView::setZoomFrags(ZoomFlags zf)
{
    m_zoomFlags = zf;
}

/**
 * @brief 获取zoomflags
 * @return
 */
DAGraphicsView::ZoomFlags DAGraphicsView::getZoomFlags() const
{
    return m_zoomFlags;
}

/**
 * @brief 判断是否在拖动
 * @return
 */
bool DAGraphicsView::isPadding() const
{
    return m_isPadding;
}

/**
 * @brief 设置拖动属性
 * @param pf
 */
void DAGraphicsView::setPaddingFrags(PadFlags pf)
{
    m_padFlags = pf;
}

/**
 * @brief 获取拖动属性
 * @return
 */
DAGraphicsView::PadFlags DAGraphicsView::getPaddingFrags() const
{
    return m_padFlags;
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
    DAGraphicsSceneWithUndoStack* s = qobject_cast< DAGraphicsSceneWithUndoStack* >(scene());
    if (s) {
        // DAGraphicsSceneWithUndoStack的selectAll只发射一次selectionChanged信号
        s->selectAll();
    } else {
        //非DAGraphicsSceneWithUndoStack，就执行选中所有
        QList< QGraphicsItem* > its = items();
        for (QGraphicsItem* i : its) {
            if (!i->isSelected() && i->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
                //只有没有被选上，且是可选的才会执行选中动作
                i->setSelected(true);
            }
        }
    }
}

bool DAGraphicsView::isEnaleWheelZoom() const
{
    return !m_zoomFlags.testFlag(ZoomNotUseWheel);
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
