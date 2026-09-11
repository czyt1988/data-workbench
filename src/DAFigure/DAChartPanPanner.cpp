#include "DAChartPanPanner.h"
// qt
#include <QMouseEvent>

namespace DA
{
/**
 * @brief 构造函数
 * @param canvas 关联的画布
 */
DAChartPanPanner::DAChartPanPanner(QWidget* canvas) : QwtPlotPanner(canvas)
{
}

/**
 * @brief 鼠标按下事件处理
 * @param mouseEvent 鼠标事件
 * @details QwtPlotPanner 通过 MouseSelect1 鼠标模式匹配单一按钮触发拖动，
 *          这里在左键或中键按下时把鼠标模式动态切换为当前按钮，
 *          使两个按钮都能触发拖动；带修饰键（Ctrl/Shift 等）的按下不匹配
 *          Qt::NoModifier，不会触发拖动，留给其他交互使用。
 *          松开时 QwtPickerDragPointMachine 不检查按钮，可正常结束拖动。
 */
void DAChartPanPanner::widgetMousePressEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton || mouseEvent->button() == Qt::MiddleButton) {
        setMouseButton(mouseEvent->button(), Qt::NoModifier);
    }
    QwtPlotPanner::widgetMousePressEvent(mouseEvent);
}
}  // namespace DA
