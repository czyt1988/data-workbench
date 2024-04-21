#include "DAChartCommonItemsSettingWidget.h"
#include "ui_DAChartCommonItemsSettingWidget.h"
namespace DA
{
DAChartCommonItemsSettingWidget::DAChartCommonItemsSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartCommonItemsSettingWidget)
{
    ui->setupUi(this);
}

DAChartCommonItemsSettingWidget::~DAChartCommonItemsSettingWidget()
{
    delete ui;
}

void DAChartCommonItemsSettingWidget::plotItemSet(QwtPlotItem* item)
{
	if (nullptr == item) {
		return;
	}
	switch (item->rtti()) {

	//! For QwtPlotGrid
	case QwtPlotItem::Rtti_PlotGrid: {
		ui->stackedWidget->setCurrentWidget(ui->widgetGridItem);
		ui->widgetGridItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotScaleItem
	case QwtPlotItem::Rtti_PlotScale: {
		break;
	}

	//! For QwtPlotLegendItem
	case QwtPlotItem::Rtti_PlotLegend: {
		ui->stackedWidget->setCurrentWidget(ui->widgetLegendItem);
		ui->widgetLegendItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotMarker
	case QwtPlotItem::Rtti_PlotMarker: {
		break;
	}

	//! For QwtPlotCurve
	case QwtPlotItem::Rtti_PlotCurve: {
		ui->stackedWidget->setCurrentWidget(ui->widgetCurveItem);
		ui->widgetCurveItem->setPlotItem(item);
		break;
	}

	//! For QwtPlotSpectroCurve
	case QwtPlotItem::Rtti_PlotSpectroCurve: {
		break;
	}

	//! For QwtPlotIntervalCurve
	case QwtPlotItem::Rtti_PlotIntervalCurve: {
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
		break;
	}

	//! For QwtPlotBarChart
	case QwtPlotItem::Rtti_PlotBarChart: {
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
