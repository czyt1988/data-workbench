#ifndef DAGRAPHICSVIEW_H
#define DAGRAPHICSVIEW_H
#include <QGraphicsView>
#include "DAWorkFlowGlobal.h"
class QWheelEvent;

/**
 * @brief 支持缩放和拖动的GraphicsView
 */
class DAWORKFLOW_API DANodeGraphicsView : public QGraphicsView
{
public:
    /**
     * @brief 缩放设置
     */
    enum ZoomFlag
    {
        ZoomNotUseWheel     = 0x0001,  ///< 不进行滚轮缩放
        ZoomUseWheel        = 0x0002,  ///< 直接用wheel就可以
        ZoomUseWheelAndCtrl = 0x0004   ///< 需要使用ctrl键
    };
    Q_DECLARE_FLAGS(ZoomFlags, ZoomFlag)
    Q_FLAG(ZoomFlag)

    /**
     * @brief 表征拖动状态
     */
    enum PadFlag
    {
        PadDiable              = 0x0001,  ///< 不允许拖动
        PadByWheelMiddleButton = 0x0002   ///< 通过中键移动
    };
    Q_DECLARE_FLAGS(PadFlags, PadFlag)
    Q_FLAG(PadFlag)
public:
    DANodeGraphicsView(QWidget* parent = 0);
    DANodeGraphicsView(QGraphicsScene* scene, QWidget* parent = 0);
    //设置缩放的因子
    void setScaleRange(qreal min, qreal max);
    //获取缩放因子
    qreal getScaleMaxFactor() const;
    qreal getScaleMinFactor() const;
    //放大
    void zoomIn();
    //缩小
    void zoomOut();
    //设置是否可以滚轮缩放
    bool isEnaleWheelZoom() const;
    void setEnaleWheelZoom(bool enaleWheelZoom = true, ZoomFlags zf = ZoomUseWheelAndCtrl);

    //获取鼠标对应的scence的位置坐标
    QPointF getMouseScenePos() const;

    //设置缩放flag
    void setZoomFrags(ZoomFlags zf);
    ZoomFlags getZoomFlags() const;

    //判断是否在拖动
    bool isPadding() const;
    //设置拖动属性
    void setPaddingFrags(PadFlags pf);
    PadFlags getPaddingFrags() const;

protected:
    // void paintEvent(QPaintEvent * event);
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

protected:
    void wheelZoom(QWheelEvent* event);
    //开始拖动
    void startPad(QMouseEvent* event);
    //结束拖动
    void endPad();

private:
    qreal m_scaleMax;
    qreal m_scaleMin;
    qreal m_zoomStep;
    bool m_isPadding;  ///<标记是否开始拖动
    QPointF m_mouseScenePos;
    QPoint m_startPadPos;  ///< 记录开始拖动的位置
    ZoomFlags m_zoomFlags;
    PadFlags m_padFlags;
};

#endif  // GGRAPHICSVIEW_H
