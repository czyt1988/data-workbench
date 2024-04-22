#include "DAChartLegendItemSettingWidget.h"
#include "ui_DAChartLegendItemSettingWidget.h"
#include "qwt_plot_legenditem.h"
#include "DASignalBlockers.h"
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
	connect(ui->spinBoxHorizontalOffset,
			QOverload< int >::of(&QSpinBox::valueChanged),
			this,
			&DAChartLegendItemSettingWidget::onSpinBoxHorizontalOffsetValueChanged);
	connect(ui->spinBoxVerticalOffset,
			QOverload< int >::of(&QSpinBox::valueChanged),
			this,
			&DAChartLegendItemSettingWidget::onSpinBoxVerticalOffsetValueChanged);
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
	DASignalBlockers b1(ui->widgetPosition,
						ui->spinBoxHorizontalOffset,
						ui->spinBoxVerticalOffset,
						ui->spinBoxMargin,
						ui->spinBoxSpacing,
						ui->spinBoxItemMargin,
						ui->spinBoxItemSpacing,
						ui->spinBoxMaxColumns,
						ui->doubleSpinBoxRadius,
						ui->widgetBorderPen,
						ui->widgetFont,
						ui->widgetBKBrush);
	ui->widgetPosition->setAligmentPosition(item->alignmentInCanvas());
	ui->spinBoxHorizontalOffset->setValue(item->offsetInCanvas(Qt::Horizontal));
	ui->spinBoxVerticalOffset->setValue(item->offsetInCanvas(Qt::Vertical));
	ui->spinBoxMargin->setValue(item->margin());
	ui->spinBoxSpacing->setValue(item->spacing());
	ui->spinBoxItemMargin->setValue(item->itemMargin());
	ui->spinBoxItemSpacing->setValue(item->itemSpacing());
	ui->spinBoxMaxColumns->setValue(item->maxColumns());
	ui->doubleSpinBoxRadius->setValue(item->borderRadius());
	ui->widgetBorderPen->setCurrentPen(item->borderPen());
	ui->widgetFont->setCurrentFont(item->font());
	ui->widgetFont->setCurrentFontColor(item->textPen().color());
	ui->widgetBKBrush->setCurrentBrush(item->backgroundBrush());
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

void DAChartLegendItemSettingWidget::onSpinBoxHorizontalOffsetValueChanged(int v)
{
	if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
		return;
	}
	QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
	legend->setOffsetInCanvas(Qt::Horizontal, v);
}

void DAChartLegendItemSettingWidget::onSpinBoxVerticalOffsetValueChanged(int v)
{
}
}
