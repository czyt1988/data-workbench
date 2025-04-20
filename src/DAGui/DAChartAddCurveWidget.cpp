#include "DAChartAddCurveWidget.h"
#include <QMessageBox>
#include "qwt_plot_curve.h"
namespace DA
{

//===================================================
// DAChartAddCurveWidget
//===================================================

DAChartAddCurveWidget::DAChartAddCurveWidget(QWidget* parent) : DAChartAddXYSeriesWidget(parent)
{
}

DAChartAddCurveWidget::~DAChartAddCurveWidget()
{
}

/**
 * @brief 此函数创建QwtPlotCurve
 * @return
 */
QwtPlotItem* DAChartAddCurveWidget::createPlotItem()
{
	QVector< QPointF > xy = getSeries();
	if (xy.empty()) {
		return nullptr;
	}
	QwtPlotCurve* item = new QwtPlotCurve();
	item->setSamples(xy);
	return item;
}

}  // end DA
