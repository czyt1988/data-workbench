#ifndef DACHARTCANVAS_H
#define DACHARTCANVAS_H
#include "DAFigureAPI.h"
#include "qwt_plot_canvas.h"
class QPaintEvent;
namespace DA
{
DA_IMPL_FORWARD_DECL(DAChartCanvas)

/**
 * @brief 重写了paint背景的方法
 */
class DAFIGURE_API DAChartCanvas : public QwtPlotCanvas
{
    Q_OBJECT
public:
    explicit DAChartCanvas(QwtPlot* p = nullptr);
    virtual ~DAChartCanvas();
};
}  // End Of Namespace DA
#endif  // DACHARTCANVAS_H
