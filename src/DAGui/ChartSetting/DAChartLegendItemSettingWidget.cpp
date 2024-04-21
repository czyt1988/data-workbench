#include "DAChartLegendItemSettingWidget.h"
#include "ui_DAChartLegendItemSettingWidget.h"
namespace DA
{
DAChartLegendItemSettingWidget::DAChartLegendItemSettingWidget(QWidget* parent)
	: DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartLegendItemSettingWidget)
{
	ui->setupUi(this);
}

DAChartLegendItemSettingWidget::~DAChartLegendItemSettingWidget()
{
	delete ui;
}

void DAChartLegendItemSettingWidget::changeEvent(QEvent* e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}
}
