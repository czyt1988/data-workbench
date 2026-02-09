#include "DAChartAddIntervalCurveWidget.h"
#include "qwt_plot_intervalcurve.h"
namespace DA
{

DAChartAddIntervalCurveWidget::DAChartAddIntervalCurveWidget(QWidget* parent) : DAChartAddXYESeriesWidget(parent)
{
}

DAChartAddIntervalCurveWidget::~DAChartAddIntervalCurveWidget()
{
}

QwtPlotItem* DAChartAddIntervalCurveWidget::createPlotItem()
{
	QVector< QwtIntervalSample > xye = getSeries();
	if (xye.empty()) {
		return nullptr;
	}
	QwtPlotIntervalCurve* item = new QwtPlotIntervalCurve();
	item->setSamples(xye);
	return item;
}

}  // end DA namespace
