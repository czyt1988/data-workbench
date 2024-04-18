#include "DAChartPlotSettingWidget.h"
#include "ui_DAChartPlotSettingWidget.h"
namespace DA
{
DAChartPlotSettingWidget::DAChartPlotSettingWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::DAChartPlotSettingWidget)
{
	ui->setupUi(this);
}

DAChartPlotSettingWidget::~DAChartPlotSettingWidget()
{
	delete ui;
}

void DAChartPlotSettingWidget::setChartWidget(DAChartWidget* w)
{
	mChartPlot = w;
	if (!w) {
		return;
	}
	ui->widgetYLeftAxisSetting->setChart(w, QwtPlot::yLeft);
	if (w->axisEnabled(QwtPlot::yLeft)) {
		ui->groupBoxYLeft->setCollapsed(false);
	}
}
}  // end da
