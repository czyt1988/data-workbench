#ifndef DACHARTPANPANNER_H
#define DACHARTPANPANNER_H
#include "DAFigureAPI.h"
#include "qwt_plot_panner.h"
namespace DA
{
/**
 * @brief 支持鼠标左键和中键拖动画布的平移器
 */
class DAFIGURE_API DAChartPanPanner : public QwtPlotPanner
{
    Q_OBJECT
public:
    DAChartPanPanner(QWidget* canvas);

protected:
    virtual void widgetMousePressEvent(QMouseEvent* mouseEvent) override;
};
}  // namespace DA
#endif  // DACHARTPANPANNER_H
