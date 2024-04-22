#include "DAChartLegendItemSettingWidget.h"
#include "ui_DAChartLegendItemSettingWidget.h"
#include "qwt_plot_legenditem.h"
namespace DA
{
DAChartLegendItemSettingWidget::DAChartLegendItemSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartLegendItemSettingWidget)
{
	ui->setupUi(this);
    connect(ui->widgetPosition,
            &DAAligmentPositionEditWidget::aligmentPositionChanged,
            this,
            &DAChartLegendItemSettingWidget::onAligmentPositionChanged);
}

DAChartLegendItemSettingWidget::~DAChartLegendItemSettingWidget()
{
    delete ui;
}

void DAChartLegendItemSettingWidget::plotItemSet(QwtPlotItem* item)
{
    if (item == nullptr) {
        return;
    }
    if (item->rtti() == QwtPlotItem::Rtti_PlotLegend) {
        QwtPlotLegendItem* legend = static_cast< QwtPlotLegendItem* >(item);
        updateUI(legend);
    }
}

void DAChartLegendItemSettingWidget::updateUI(const QwtPlotLegendItem* item)
{
    if (!item) {
        return;
    }
    QSignalBlocker b(ui->widgetPosition);
    ui->widgetPosition->setAligmentPosition(item->alignmentInCanvas());
    ui->spinBoxHorizontalOffset->setValue(item->offsetInCanvas(Qt::Horizontal));
    ui->spinBoxVerticalOffset->setValue(item->offsetInCanvas(Qt::Vertical));
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

void DAChartLegendItemSettingWidget::onAligmentPositionChanged(Qt::Alignment al)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setAlignmentInCanvas(al);
}
}
