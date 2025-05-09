#include "DAChartCommonItemsSettingWidget.h"
#include "ui_DAChartCommonItemsSettingWidget.h"
#include "DAChartBarItemSettingWidget.h"
#include "DAChartCurveItemSettingWidget.h"
#include "DAChartErrorBarItemSettingWidget.h"
#include "DAChartGridSettingWidget.h"
#include "DAChartTradingCurveItemSettingWidget.h"
#include "DAChartLegendItemSettingWidget.h"
#include "DAChartSpectrogramItemSettingWidget.h"

namespace DA
{

class DAChartCommonItemsSettingWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartCommonItemsSettingWidget)
public:
	PrivateData(DAChartCommonItemsSettingWidget* p);

public:
    DAChartCurveItemSettingWidget* widgetCurveItem { nullptr };
    DAChartBarItemSettingWidget* widgetBarItem { nullptr };
    DAChartErrorBarItemSettingWidget* widgetErrorBarItem { nullptr };
    DAChartSpectrogramItemSettingWidget* widgetSpectrogramItem{ nullptr };
    DAChartLegendItemSettingWidget* widgetLegendItem { nullptr };
    DAChartGridSettingWidget* widgetGridItem { nullptr };
    DAChartTradingCurveItemSettingWidget* widgetTradingCurveItem { nullptr };
};

DAChartCommonItemsSettingWidget::PrivateData::PrivateData(DAChartCommonItemsSettingWidget* p) : q_ptr(p)
{
}

//===============================================================
// DAChartCommonItemsSettingWidget
//===============================================================

DAChartCommonItemsSettingWidget::DAChartCommonItemsSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartCommonItemsSettingWidget)
{
	ui->setupUi(this);
	DA_D(d);
    d->widgetCurveItem = new DAChartCurveItemSettingWidget();
    d->widgetCurveItem->setObjectName(QStringLiteral("widgetCurveItem"));
    d->widgetBarItem = new DAChartBarItemSettingWidget();
    d->widgetBarItem->setObjectName(QStringLiteral("widgetBarItem"));
    d->widgetErrorBarItem = new DAChartErrorBarItemSettingWidget();
    d->widgetErrorBarItem->setObjectName(QStringLiteral("widgetErrorBarItem"));
	d->widgetSpectrogramItem = new DA::DAChartSpectrogramItemSettingWidget();
	d->widgetSpectrogramItem->setObjectName(QString::fromUtf8("widgetSpectrogramItem"));
    d->widgetLegendItem = new DAChartLegendItemSettingWidget();
    d->widgetLegendItem->setObjectName(QStringLiteral("widgetLegendItem"));
    d->widgetGridItem = new DAChartGridSettingWidget();
    d->widgetGridItem->setObjectName(QStringLiteral("widgetGridItem"));
    d->widgetTradingCurveItem = new DAChartTradingCurveItemSettingWidget();
    d->widgetTradingCurveItem->setObjectName(QStringLiteral("widgetTradingCurveItem"));

	ui->stackedWidget->addWidget(d->widgetCurveItem);
	ui->stackedWidget->addWidget(d->widgetBarItem);
	ui->stackedWidget->addWidget(d->widgetErrorBarItem);
	ui->stackedWidget->addWidget(d->widgetSpectrogramItem);
	ui->stackedWidget->addWidget(d->widgetLegendItem);
	ui->stackedWidget->addWidget(d->widgetGridItem);
    ui->stackedWidget->addWidget(d->widgetTradingCurveItem);
}

DAChartCommonItemsSettingWidget::~DAChartCommonItemsSettingWidget()
{
	delete ui;
}

void DAChartCommonItemsSettingWidget::plotItemSet(QwtPlotItem* item)
{
	DA_D(d);
	if (nullptr == item) {
		return;
	}
	switch (item->rtti()) {

	//! For QwtPlotGrid
	case QwtPlotItem::Rtti_PlotGrid: {
		ui->stackedWidget->setCurrentWidget(d->widgetGridItem);
		d->widgetGridItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotScaleItem
	case QwtPlotItem::Rtti_PlotScale: {
		ui->stackedWidget->setCurrentWidget(d->widgetCurveItem);
		d->widgetCurveItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotLegendItem
	case QwtPlotItem::Rtti_PlotLegend: {
		ui->stackedWidget->setCurrentWidget(d->widgetLegendItem);
		d->widgetLegendItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotMarker
	case QwtPlotItem::Rtti_PlotMarker: {
		break;
	}

	//! For QwtPlotCurve
	case QwtPlotItem::Rtti_PlotCurve: {
		ui->stackedWidget->setCurrentWidget(d->widgetCurveItem);
		d->widgetCurveItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotSpectroCurve
	case QwtPlotItem::Rtti_PlotSpectroCurve: {
		break;
	}

	//! For QwtPlotIntervalCurve
	case QwtPlotItem::Rtti_PlotIntervalCurve: {
		ui->stackedWidget->setCurrentWidget(d->widgetErrorBarItem);
		d->widgetErrorBarItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotHistogram
	case QwtPlotItem::Rtti_PlotHistogram: {
		break;
	}

	//! For QwtPlotSpectrogram
	case QwtPlotItem::Rtti_PlotSpectrogram: {
		break;
	}

	//! For QwtPlotGraphicItem, QwtPlotSvgItem
	case QwtPlotItem::Rtti_PlotGraphic: {
		break;
	}

	//! For QwtPlotTradingCurve
	case QwtPlotItem::Rtti_PlotTradingCurve: {
        ui->stackedWidget->setCurrentWidget(d->widgetTradingCurveItem);
        d->widgetTradingCurveItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotBarChart
	case QwtPlotItem::Rtti_PlotBarChart: {
		ui->stackedWidget->setCurrentWidget(d->widgetBarItem);
		d->widgetBarItem->setPlotItem(item);

		break;
	}

	//! For QwtPlotMultiBarChart
	case QwtPlotItem::Rtti_PlotMultiBarChart: {
		break;
	}

	//! For QwtPlotShapeItem
	case QwtPlotItem::Rtti_PlotShape: {
		break;
	}

	//! For QwtPlotTextLabel
	case QwtPlotItem::Rtti_PlotTextLabel: {
		break;
	}

	//! For QwtPlotZoneItem
	case QwtPlotItem::Rtti_PlotZone: {
		break;
	}

	//! For QwtPlotVectorField
	case QwtPlotItem::Rtti_PlotVectorField: {
		break;
	}
	default:
		break;
	}
}

}  // end DA
