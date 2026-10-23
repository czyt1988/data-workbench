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
	// 多 y 时截断提示与曲线（柱状图同组柱重叠，仅取第一条 y）
	if (getXYInfo().y.size() > 1) {
		QString name;
		QList< QVector< QPointF > > xys;
		QStringList names;
		if (getMultiSeries(xys, names) && !names.isEmpty()) {
			name = names.first();
		}
		item->setTitle(name);
	} else {
		item->setTitle(getNameHint());
	}
	return item;
}
}
