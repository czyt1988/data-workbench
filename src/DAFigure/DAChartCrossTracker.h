#ifndef DACHARTCROSSTRACKER_H
#define DACHARTCROSSTRACKER_H
#include "DAFigureAPI.h"
#include "qwt_plot_picker.h"
#include "qwt_text.h"
namespace DA
{
/**
 * @brief 十字标记线
 */
class DAFIGURE_API DAChartCrossTracker : public QwtPlotPicker
{
public:
    DAChartCrossTracker(QWidget* w);
    DAChartCrossTracker(int xAxis, int yAxis, QWidget* w);

protected:
    virtual QwtText trackerTextF(const QPointF& pos) const;

private:
    void init();
};
}  // End Of Namespace DA
#endif  // DACHARTCROSSTRACKER_H
