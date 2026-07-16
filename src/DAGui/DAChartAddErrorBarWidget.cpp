#include "DAChartAddErrorBarWidget.h"
#include "qwt_plot_intervalcurve.h"
namespace DA
{

DAChartAddErrorBarWidget::DAChartAddErrorBarWidget(QWidget* parent) : DAChartAddXYESeriesWidget(parent)
{
}

DAChartAddErrorBarWidget::~DAChartAddErrorBarWidget()
{
}

QwtPlotItem* DAChartAddErrorBarWidget::createPlotItem()
{
	QVector< QwtIntervalSample > xye = getSeries();
	if (xye.empty()) {
		return nullptr;
	}
	QwtPlotIntervalCurve* item = new QwtPlotIntervalCurve();
	item->setSamples(xye);
	item->setTitle(tr("Error Bar"));  // cn:误差棒
	// 误差棒样式：填充区间上下边界，默认 Tube 样式即合适
	return item;
}

}  // namespace DA
