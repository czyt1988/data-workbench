#ifndef DACHARTADDTRADINGCURVEWIDGET_H
#define DACHARTADDTRADINGCURVEWIDGET_H
#include "DAChartAddOHLCSeriesWidget.h"
namespace DA
{

/**
 * @brief 添加TradingCurve
 */
class DAChartAddTradingCurveWidget : public DAChartAddOHLCSeriesWidget
{
public:
    DAChartAddTradingCurveWidget(QWidget* parent = nullptr);
    // 创建item
    virtual QwtPlotItem* createPlotItem() override;
};
}
#endif  // DACHARTADDTRADINGCURVEWIDGET_H
