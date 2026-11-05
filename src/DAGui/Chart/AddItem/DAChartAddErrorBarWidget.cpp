#include "Chart/AddItem/DAChartAddErrorBarWidget.h"
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
	const bool programmatic = hasProgrammaticSeries();
	QVector< QwtIntervalSample > xye = programmatic ? mProgrammaticSeries : getSeries();
	if (xye.empty()) {
		return nullptr;
	}
	QwtPlotIntervalCurve* item = new QwtPlotIntervalCurve();
	item->setSamples(xye);
	item->setTitle(programmatic && !mProgrammaticTitle.isEmpty() ? mProgrammaticTitle
	                                                            : tr("Error Bar"));  // cn:误差棒
	// 误差棒样式：填充区间上下边界，默认 Tube 样式即合适
	return item;
}

/**
 * @brief 设置编程式区间样本（快速绘图路径）
 */
void DAChartAddErrorBarWidget::setProgrammaticSeries(const QVector< QwtIntervalSample >& samples,
                                                     const QString& title)
{
	mProgrammaticSeries = samples;
	mProgrammaticTitle  = title;
}

/**
 * @brief 是否已设置编程式数据
 */
bool DAChartAddErrorBarWidget::hasProgrammaticSeries() const
{
	return !mProgrammaticSeries.isEmpty();
}

}  // namespace DA
