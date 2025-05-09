#include "DAChartAddTradingCurveWidget.h"
#include "qwt_samples.h"
#include "qwt_plot_tradingcurve.h"
namespace DA
{
DAChartAddTradingCurveWidget::DAChartAddTradingCurveWidget(QWidget* parent) : DAChartAddOHLCSeriesWidget(parent)
{
}

QwtPlotItem* DAChartAddTradingCurveWidget::createPlotItem()
{
    QVector< QwtOHLCSample > ohlc = getSeries();
    if (ohlc.empty()) {
        return nullptr;
    }
    QwtPlotTradingCurve* item = new QwtPlotTradingCurve();
    item->setSamples(ohlc);
    return item;
}

}
