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
	DAChartSeriesResult res = extractSeries();
	if (!res.isOk()) {
		warnSeriesError(res.error);
		return nullptr;
	}
	if (res.points.empty()) {
		return nullptr;
	}
	QwtPlotBarChart* item = new QwtPlotBarChart();
	item->setSamples(res.points);
	// 多 y 时截断提示与曲线（柱状图同组柱重叠，仅取第一条 y）
	const QList< QPair< DAData, QString > > ys = getYList();
	if (ys.size() > 1 && !ys.first().second.isEmpty()) {
		item->setTitle(ys.first().second);
	} else {
		item->setTitle(getNameHint());
	}
	return item;
}
}
