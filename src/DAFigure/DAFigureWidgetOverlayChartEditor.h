#ifndef DAFIGUREWIDGETOVERLAYCHARTEDITOR_H
#define DAFIGUREWIDGETOVERLAYCHARTEDITOR_H
#include "DAFigureAPI.h"
#include "DAFigureWidgetOverlay.h"
namespace DA
{

///
/// \brief 用于辅助显示figure的子chart位置编辑的覆盖辅助窗体
///
class DAFIGURE_API DAFigureWidgetOverlayChartEditor : public DAFigureWidgetOverlay
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureWidgetOverlayChartEditor)
public:
    DAFigureWidgetOverlayChartEditor(DAFigureWidget* fig);
    ~DAFigureWidgetOverlayChartEditor();

    ///
    /// \brief 用于标记矩形的区域
    ///
    enum ControlType
    {
        ControlLineTop,
        ControlLineBottom,
        ControlLineLeft,
        ControlLineRight,
        ControlPointTopLeft,
        ControlPointTopRight,
        ControlPointBottomLeft,
        ControlPointBottomRight,
        Inner,
        OutSide
    };
    Q_ENUM(ControlType)
public:
    //根据点和矩形的关系，返回图标的样式
    static Qt::CursorShape controlTypeToCursor(ControlType rr);
    static ControlType getPositionControlType(const QPoint& pos, const QRect& region, int err = 1);
    static bool isPointInRectEdget(const QPoint& pos, const QRect& region, int err = 1);

    //设置边框的画笔
    void setBorderPen(const QPen& p);
    QPen getBorderPen() const;
    //控制点的填充
    void setControlPointBrush(const QBrush& b);
    QBrush getControlPointBrush() const;
    //控制点尺寸
    QSize getControlPointSize() const;
    void setControlPointSize(const QSize& c);
    //选择下一个窗口作为激活窗体
    void selectNextWidget(bool forward = true);
    //选择下一个绘图作为激活窗体
    void selectNextChart(bool forward = true);
    //获取当前激活的窗口
    QWidget* getCurrentActiveWidget() const;
    //显示占比数值
    void showPercentText(bool on = true);
public slots:
    //改变激活窗口
    void setActiveWidget(QWidget* w);

protected:
    virtual void drawOverlay(QPainter* p) const;
    virtual QRegion maskHint() const;
    virtual bool eventFilter(QObject* obj, QEvent* event);
    //绘制
    virtual void drawChartEditMode(QPainter* painter, const QRect& chartRect) const;
signals:

    /**
     * @brief 窗口尺寸发生改变信号
     * @param w 窗口
     * @param oldGeometry 旧的位置
     * @param newGeometry 新的位置
     */
    void widgetGeometryChanged(QWidget* w, const QRect& oldGeometry, const QRect& newGeometry);
    /**
     * @brief 激活窗口发生变化的信号
     * @param oldActive 如果之前没有激活窗口，此指针有可能是nullptr
     * @param newActive 如果没有新的激活窗口，此指针有可能是nullptr
     */
    void activeWidgetChanged(QWidget* oldActive, QWidget* newActive);

private:
    bool onMouseMoveEvent(QMouseEvent* me);
    bool onMouseReleaseEvent(QMouseEvent* me);
    bool onMousePressedEvent(QMouseEvent* me);
    bool onHoverMoveEvent(QHoverEvent* me);
    bool onKeyPressedEvent(QKeyEvent* ke);
};
}  // end DA namespace
#endif  // SAFIGURECHARTRUBBERBANDEDITOVERLAY_H
