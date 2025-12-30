#include "DAChartLegendItemSettingWidget.h"
#include "ui_DAChartLegendItemSettingWidget.h"
#include "qwt_plot_legenditem.h"
#include "DASignalBlockers.hpp"
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
    connect(ui->spinBoxMargin,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartLegendItemSettingWidget::onSpinBoxMarginValueChanged);
    connect(ui->spinBoxSpacing,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartLegendItemSettingWidget::onSpinBoxSpacingValueChanged);
    connect(ui->spinBoxItemMargin,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartLegendItemSettingWidget::onSpinBoxItemMarginValueChanged);
    connect(ui->spinBoxItemSpacing,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartLegendItemSettingWidget::onSpinBoxItemSpacingValueChanged);
    connect(ui->spinBoxMaxColumns,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartLegendItemSettingWidget::onSpinBoxMaxColumnsValueChanged);
    connect(ui->doubleSpinBoxRadius,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartLegendItemSettingWidget::onDoubleSpinBoxRadiusValueChanged);
    connect(ui->widgetBorderPen, &DAPenEditWidget::penChanged, this, &DAChartLegendItemSettingWidget::onBorderPenChanged);
    connect(ui->widgetFont,
            &DAFontEditPannelWidget::currentFontChanged,
            this,
            &DAChartLegendItemSettingWidget::onLegendFontChanged);
    connect(ui->widgetFont,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAChartLegendItemSettingWidget::onLegendFontColorChanged);
    connect(ui->widgetBKBrush, &DABrushEditWidget::brushChanged, this, &DAChartLegendItemSettingWidget::onLegendBKBrushChanged);
}

DAChartLegendItemSettingWidget::~DAChartLegendItemSettingWidget()
{
    delete ui;
}

void DAChartLegendItemSettingWidget::updateUI(QwtPlotItem* item)
{
    if (item == nullptr) {
        return;
    }
    if (item->rtti() == QwtPlotItem::Rtti_PlotLegend) {
        QwtPlotLegendItem* legend = static_cast< QwtPlotLegendItem* >(item);
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
        ui->widgetPosition->setAligmentPosition(legend->alignmentInCanvas());
        ui->spinBoxHorizontalOffset->setValue(legend->offsetInCanvas(Qt::Horizontal));
        ui->spinBoxVerticalOffset->setValue(legend->offsetInCanvas(Qt::Vertical));
        ui->spinBoxMargin->setValue(legend->margin());
        ui->spinBoxSpacing->setValue(legend->spacing());
        ui->spinBoxItemMargin->setValue(legend->itemMargin());
        ui->spinBoxItemSpacing->setValue(legend->itemSpacing());
        ui->spinBoxMaxColumns->setValue(legend->maxColumns());
        ui->doubleSpinBoxRadius->setValue(legend->borderRadius());
        ui->widgetBorderPen->setCurrentPen(legend->borderPen());
        ui->widgetFont->setCurrentFont(legend->font());
        ui->widgetFont->setCurrentFontColor(legend->textPen().color());
        ui->widgetBKBrush->setCurrentBrush(legend->backgroundBrush());
    }
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
    replot();
}

void DAChartLegendItemSettingWidget::onSpinBoxHorizontalOffsetValueChanged(int v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setOffsetInCanvas(Qt::Horizontal, v);
    replot();
}

void DAChartLegendItemSettingWidget::onSpinBoxVerticalOffsetValueChanged(int v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setOffsetInCanvas(Qt::Vertical, v);
    replot();
}

void DAChartLegendItemSettingWidget::onSpinBoxMarginValueChanged(int v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setMargin(v);
    replot();
}

void DAChartLegendItemSettingWidget::onSpinBoxSpacingValueChanged(int v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setSpacing(v);
    replot();
}

void DAChartLegendItemSettingWidget::onSpinBoxItemMarginValueChanged(int v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setItemMargin(v);
    replot();
}

void DAChartLegendItemSettingWidget::onSpinBoxItemSpacingValueChanged(int v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setItemSpacing(v);
    replot();
}

void DAChartLegendItemSettingWidget::onSpinBoxMaxColumnsValueChanged(int v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setMaxColumns(v);
    replot();
}

void DAChartLegendItemSettingWidget::onDoubleSpinBoxRadiusValueChanged(double v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setBorderRadius(v);
    replot();
}

void DAChartLegendItemSettingWidget::onBorderPenChanged(const QPen& v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setBorderPen(v);
    replot();
}

void DAChartLegendItemSettingWidget::onLegendFontChanged(const QFont& v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setFont(v);
    replot();
}

void DAChartLegendItemSettingWidget::onLegendFontColorChanged(const QColor& v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    auto p                    = legend->textPen();
    p.setColor(v);
    legend->setTextPen(p);
    replot();
}

void DAChartLegendItemSettingWidget::onLegendBKBrushChanged(const QBrush& v)
{
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotLegend)) {
        return;
    }
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    legend->setBackgroundBrush(v);
    replot();
}
}
