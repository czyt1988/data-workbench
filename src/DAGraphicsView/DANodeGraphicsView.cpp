#include "DANodeGraphicsView.h"
#include <QMatrix>
#include <QWheelEvent>
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
DANodeGraphicsView::DANodeGraphicsView(QWidget* parent)
    : QGraphicsView(parent), m_scaleMax(3), m_scaleMin(0.333), m_zoomStep(0.1), m_isPadding(false)
{
    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    m_zoomFlags = ZoomUseWheelAndCtrl;
    m_padFlags  = PadByWheelMiddleButton;
}

DANodeGraphicsView::DANodeGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent), m_scaleMax(3), m_scaleMin(0.333), m_zoomStep(0.1), m_isPadding(false)
{
    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_zoomFlags = ZoomUseWheelAndCtrl;
    m_padFlags  = PadByWheelMiddleButton;
}

void DANodeGraphicsView::setScaleRange(qreal min, qreal max)
{
    m_scaleMin = min;
    m_scaleMax = max;
}

qreal DANodeGraphicsView::getScaleMaxFactor() const
{
    return (m_scaleMax);
}

qreal DANodeGraphicsView::getScaleMinFactor() const
{
    return (m_scaleMin);
}

/**
 * @brief 放大
 */
void DANodeGraphicsView::zoomIn()
{
    qreal scaleFactor = this->matrix().m11();
    if (scaleFactor >= m_scaleMax) {
        return;
    }
    scale(1 + m_zoomStep, 1 + m_zoomStep);
}

/**
 * @brief 缩小
 */
void DANodeGraphicsView::zoomOut()
{
    qreal scaleFactor = this->matrix().m11();
    if (scaleFactor <= m_scaleMin) {
        return;
    }
    scale(1 - m_scaleMin, 1 - m_scaleMin);
}

/**
 * @brief 中键滚动
 * @param event
 */
void DANodeGraphicsView::wheelEvent(QWheelEvent* event)
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
    return;
}

void DANodeGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    QGraphicsView::mouseMoveEvent(event);
    m_mouseScenePos = mapToScene(event->pos());
    if (isPadding()) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - m_startPadPos.x()));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - m_startPadPos.y()));
        m_startPadPos = event->pos();
        event->accept();
        return;
    }
}

void DANodeGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (m_padFlags.testFlag(PadDiable) || isPadding()) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    if (m_padFlags.testFlag(PadByWheelMiddleButton)) {
        if (event->button() == Qt::MiddleButton) {
            //设置了中间拖动
            startPad(event);
            event->accept();
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void DANodeGraphicsView::mouseReleaseEvent(QMouseEvent* event)
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
 * @brief 滚轮事件的缩放
 * @param event
 */
void DANodeGraphicsView::wheelZoom(QWheelEvent* event)
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
void DANodeGraphicsView::startPad(QMouseEvent* event)
{
    m_isPadding   = true;
    m_startPadPos = event->pos();
    setCursor(Qt::ClosedHandCursor);
}

/**
 * @brief 结束拖动
 * @param event
 */
void DANodeGraphicsView::endPad()
{
    m_isPadding = false;
    setCursor(Qt::ArrowCursor);
}

/**
 * @brief 获取鼠标对应的scence的位置坐标
 * @return
 */
QPointF DANodeGraphicsView::getMouseScenePos() const
{
    return m_mouseScenePos;
}

/**
 * @brief 设置zoomflags
 * @param zf
 */
void DANodeGraphicsView::setZoomFrags(ZoomFlags zf)
{
    m_zoomFlags = zf;
}

/**
 * @brief 获取zoomflags
 * @return
 */
DANodeGraphicsView::ZoomFlags DANodeGraphicsView::getZoomFlags() const
{
    return m_zoomFlags;
}

/**
 * @brief 判断是否在拖动
 * @return
 */
bool DANodeGraphicsView::isPadding() const
{
    return m_isPadding;
}

/**
 * @brief 设置拖动属性
 * @param pf
 */
void DANodeGraphicsView::setPaddingFrags(PadFlags pf)
{
    m_padFlags = pf;
}

/**
 * @brief 获取拖动属性
 * @return
 */
DANodeGraphicsView::PadFlags DANodeGraphicsView::getPaddingFrags() const
{
    return m_padFlags;
}

bool DANodeGraphicsView::isEnaleWheelZoom() const
{
    return !m_zoomFlags.testFlag(ZoomNotUseWheel);
}

void DANodeGraphicsView::setEnaleWheelZoom(bool enaleWheelZoom, ZoomFlags zf)
{
    if (!enaleWheelZoom) {
        setZoomFrags(ZoomNotUseWheel);
    } else {
        zf.setFlag(ZoomNotUseWheel, false);
        setZoomFrags(zf);
    }
}
