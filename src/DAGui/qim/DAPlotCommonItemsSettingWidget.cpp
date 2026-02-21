#include "DAPlotCommonItemsSettingWidget.h"
#include "ui_DAPlotCommonItemsSettingWidget.h"
#include "plot/QImPlotItemNode.h"
namespace DA
{

class DAPlotCommonItemsSettingWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAPlotCommonItemsSettingWidget)
public:
	PrivateData(DAPlotCommonItemsSettingWidget* p);

public:
	// DAChartCurveItemSettingWidget* widgetCurveItem { nullptr };
	// DAChartBarItemSettingWidget* widgetBarItem { nullptr };
	// DAChartErrorBarItemSettingWidget* widgetErrorBarItem { nullptr };
	// DAChartSpectrogramItemSettingWidget* widgetSpectrogramItem { nullptr };
	// DAChartLegendItemSettingWidget* widgetLegendItem { nullptr };
	// DAChartGridSettingWidget* widgetGridItem { nullptr };
	// DAChartTradingCurveItemSettingWidget* widgetTradingCurveItem { nullptr };
};

DAPlotCommonItemsSettingWidget::PrivateData::PrivateData(DAPlotCommonItemsSettingWidget* p) : q_ptr(p)
{
}

//===============================================================
// DAChartCommonItemsSettingWidget
//===============================================================

DAPlotCommonItemsSettingWidget::DAPlotCommonItemsSettingWidget(QWidget* parent)
	: DAAbstractPlotItemSettingWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAPlotCommonItemsSettingWidget)
{
	ui->setupUi(this);
	DA_D(d);
	//    d->widgetCurveItem = new DAChartCurveItemSettingWidget();
	//    d->widgetCurveItem->setObjectName(QStringLiteral("widgetCurveItem"));
	//    d->widgetBarItem = new DAChartBarItemSettingWidget();
	//    d->widgetBarItem->setObjectName(QStringLiteral("widgetBarItem"));
	//    d->widgetErrorBarItem = new DAChartErrorBarItemSettingWidget();
	//    d->widgetErrorBarItem->setObjectName(QStringLiteral("widgetErrorBarItem"));
	// d->widgetSpectrogramItem = new DA::DAChartSpectrogramItemSettingWidget();
	// d->widgetSpectrogramItem->setObjectName(QString::fromUtf8("widgetSpectrogramItem"));
	//    d->widgetLegendItem = new DAChartLegendItemSettingWidget();
	//    d->widgetLegendItem->setObjectName(QStringLiteral("widgetLegendItem"));
	//    d->widgetGridItem = new DAChartGridSettingWidget();
	//    d->widgetGridItem->setObjectName(QStringLiteral("widgetGridItem"));
	//    d->widgetTradingCurveItem = new DAChartTradingCurveItemSettingWidget();
	//    d->widgetTradingCurveItem->setObjectName(QStringLiteral("widgetTradingCurveItem"));

	// ui->stackedWidget->addWidget(d->widgetCurveItem);
	// ui->stackedWidget->addWidget(d->widgetBarItem);
	// ui->stackedWidget->addWidget(d->widgetErrorBarItem);
	// ui->stackedWidget->addWidget(d->widgetSpectrogramItem);
	// ui->stackedWidget->addWidget(d->widgetLegendItem);
	// ui->stackedWidget->addWidget(d->widgetGridItem);
	//    ui->stackedWidget->addWidget(d->widgetTradingCurveItem);
}

DAPlotCommonItemsSettingWidget::~DAPlotCommonItemsSettingWidget()
{
	delete ui;
}

void DAPlotCommonItemsSettingWidget::updateUI(QIM::QImPlotItemNode* item)
{
	DA_D(d);
	if (nullptr == item) {
		return;
	}
	switch (item->type()) {

	default:
		break;
	}
}

}  // end DA
