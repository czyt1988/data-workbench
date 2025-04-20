#include "DAChartAddBarWidget.h"
#include "qwt_plot_barchart.h"
namespace DA
{
DAChartAddBarWidget::DAChartAddBarWidget(QWidget* parent) : DAChartAddXYSeriesWidget(parent)
{
}

DAChartAddBarWidget::~DAChartAddBarWidget()
{
}

QwtPlotItem* DAChartAddBarWidget::createPlotItem()
{
	QVector< QPointF > xy = getSeries();
	if (xy.empty()) {
		return nullptr;
	}
	QwtPlotBarChart* item = new QwtPlotBarChart();
	item->setSamples(xy);
	return item;
}
}
