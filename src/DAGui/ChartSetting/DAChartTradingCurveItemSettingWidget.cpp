#include "DAChartTradingCurveItemSettingWidget.h"
#include "ui_DAChartTradingCurveItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "qwt_text.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot.h"
#include "DASignalBlockers.hpp"
namespace DA
{
DAChartTradingCurveItemSettingWidget::DAChartTradingCurveItemSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartTradingCurveItemSettingWidget)
{
	ui->setupUi(this);
	resetUI();
    connect(ui->brushEditWidgetIncreasing,
            &DABrushEditWidget::brushChanged,
            this,
            &DAChartTradingCurveItemSettingWidget::onIncreasingBrushChanged);
    connect(ui->brushEditWidgetDecreasing,
            &DABrushEditWidget::brushChanged,
            this,
            &DAChartTradingCurveItemSettingWidget::onDecreasingBrushChanged);
	connect(ui->buttonGroupOrientation,
            QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
            this,
            &DAChartTradingCurveItemSettingWidget::onButtonGroupOrientationClicked);
    connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartTradingCurveItemSettingWidget::onCurvePenChanged);
    connect(ui->doubleSpinBoxExtern,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartTradingCurveItemSettingWidget::onDoubleSpinBoxExternValueChanged);
    connect(ui->doubleSpinBoxMin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartTradingCurveItemSettingWidget::onDoubleSpinBoxMinValueChanged);
    connect(ui->doubleSpinBoxMax,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartTradingCurveItemSettingWidget::onDoubleSpinBoxMaxValueChanged);
    connect(ui->radioButtonBar, &QAbstractButton::clicked, this, &DAChartTradingCurveItemSettingWidget::onRadioButtonBarClicked);
    connect(ui->radioButtonStick,
            &QAbstractButton::clicked,
            this,
            &DAChartTradingCurveItemSettingWidget::onRadioButtonStickClicked);
}

DAChartTradingCurveItemSettingWidget::~DAChartTradingCurveItemSettingWidget()
{
	delete ui;
}

void DAChartTradingCurveItemSettingWidget::updateUI(QwtPlotItem* item)
{
	if (nullptr == item) {
		return;
	}
	if (item->rtti() != QwtPlotItem::Rtti_PlotTradingCurve) {
		return;
	}
	QwtPlotTradingCurve* curItem = static_cast< QwtPlotTradingCurve* >(item);
    DASignalBlockers blockers(ui->penEditWidget,
                              ui->brushEditWidgetDecreasing,
                              ui->brushEditWidgetIncreasing,
                              ui->doubleSpinBoxExtern,
                              ui->doubleSpinBoxMax,
                              ui->doubleSpinBoxMin,
                              ui->radioButtonStick,
                              ui->radioButtonBar,
                              ui->radioButtonHorizontal,
                              ui->radioButtonVertical);
    ui->widgetItemSetting->updateUI(item);
    // style
    QwtPlotTradingCurve::SymbolStyle s = curItem->symbolStyle();
    ui->radioButtonBar->setChecked(s == QwtPlotTradingCurve::Bar);
    ui->radioButtonStick->setChecked(s == QwtPlotTradingCurve::CandleStick);
    // pen
    ui->penEditWidget->setCurrentPen(curItem->symbolPen());
    // fill
    ui->brushEditWidgetIncreasing->setCurrentBrush(curItem->symbolBrush(QwtPlotTradingCurve::Increasing));
    ui->brushEditWidgetDecreasing->setCurrentBrush(curItem->symbolBrush(QwtPlotTradingCurve::Decreasing));
    // size
    ui->doubleSpinBoxExtern->setValue(curItem->symbolExtent());
    ui->doubleSpinBoxMin->setValue(curItem->minSymbolWidth());
    ui->doubleSpinBoxMax->setValue(curItem->maxSymbolWidth());
    //
    ui->radioButtonHorizontal->setChecked(curItem->orientation() == Qt::Horizontal);
    ui->radioButtonVertical->setChecked(curItem->orientation() == Qt::Vertical);
}

/**
 * @brief 根据ui更新QwtPlotCurve
 * @param item
 */
void DAChartTradingCurveItemSettingWidget::applySetting(QwtPlotTradingCurve* item)
{
	// box item
    ui->widgetItemSetting->applySetting(item);
    // style
    if (ui->radioButtonBar->isChecked()) {
        if (item->symbolStyle() != QwtPlotTradingCurve::Bar) {
            item->setSymbolStyle(QwtPlotTradingCurve::Bar);
        }
    } else {
        if (item->symbolStyle() != QwtPlotTradingCurve::CandleStick) {
            item->setSymbolStyle(QwtPlotTradingCurve::CandleStick);
        }
    }
	// pen
    item->setSymbolPen(ui->penEditWidget->getCurrentPen());
	// fill
    updateSymbolFillBrushFromUI(item);

	// Orientation
    updateOrientationFromUI(item);
}

void DAChartTradingCurveItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
{
	if (!on && plotItem == getPlotItem()) {
		resetUI();
	}
	DAAbstractChartItemSettingWidget::plotItemAttached(plotItem, on);
}

Qt::Orientation DAChartTradingCurveItemSettingWidget::getOrientationFromUI() const
{
    return (ui->radioButtonHorizontal->isChecked() ? Qt::Horizontal : Qt::Vertical);
}

/**
 * @brief 重置ui
 */
void DAChartTradingCurveItemSettingWidget::resetUI()
{
    DASignalBlockers blockers(ui->penEditWidget,
                              ui->brushEditWidgetDecreasing,
                              ui->brushEditWidgetIncreasing,
                              ui->doubleSpinBoxExtern,
                              ui->doubleSpinBoxMax,
                              ui->doubleSpinBoxMin,
                              ui->radioButtonHorizontal,
                              ui->radioButtonVertical);
    ui->radioButtonBar->setChecked(false);
    ui->radioButtonStick->setChecked(true);
    ui->radioButtonHorizontal->setChecked(false);
    ui->radioButtonVertical->setChecked(true);
    ui->penEditWidget->setCurrentPen(QPen(Qt::black));
    ui->brushEditWidgetIncreasing->setCurrentBrush(QBrush(Qt::red));
    ui->brushEditWidgetDecreasing->setCurrentBrush(QBrush(Qt::green));
    ui->doubleSpinBoxExtern->setValue(0.6);
    ui->doubleSpinBoxMin->setValue(2.0);
    ui->doubleSpinBoxMax->setValue(0.0);
}

/**
 * @brief 获取item plot widget
 * @return
 */
DAChartPlotItemSettingWidget* DAChartTradingCurveItemSettingWidget::getItemSettingWidget() const
{
    return ui->widgetItemSetting;
}

/**
 * @brief 从界面更新数据到symbol brush
 */
void DAChartTradingCurveItemSettingWidget::updateSymbolFillBrushFromUI(QwtPlotTradingCurve* c)
{
    c->setSymbolBrush(QwtPlotTradingCurve::Increasing, ui->brushEditWidgetIncreasing->getCurrentBrush());
    c->setSymbolBrush(QwtPlotTradingCurve::Decreasing, ui->brushEditWidgetDecreasing->getCurrentBrush());
}

void DAChartTradingCurveItemSettingWidget::updateOrientationFromUI(QwtPlotTradingCurve* c)
{
    auto ori = getOrientationFromUI();
    if (c->orientation() != ori) {
        c->setOrientation(ori);
    }
}

void DAChartTradingCurveItemSettingWidget::onRadioButtonBarClicked(bool on)
{
    if (on) {
        DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
        QwtPlotTradingCurve* item = s_cast< QwtPlotTradingCurve* >();
        item->setSymbolStyle(QwtPlotTradingCurve::Bar);
    }
}

void DAChartTradingCurveItemSettingWidget::onRadioButtonStickClicked(bool on)
{
    if (on) {
        DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
        QwtPlotTradingCurve* item = s_cast< QwtPlotTradingCurve* >();
        item->setSymbolStyle(QwtPlotTradingCurve::CandleStick);
    }
}

void DAChartTradingCurveItemSettingWidget::onIncreasingBrushChanged(const QBrush& b)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotTradingCurve* c = s_cast< QwtPlotTradingCurve* >();
    c->setSymbolBrush(QwtPlotTradingCurve::Increasing, b);
}

void DAChartTradingCurveItemSettingWidget::onDecreasingBrushChanged(const QBrush& b)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotTradingCurve* c = s_cast< QwtPlotTradingCurve* >();
    c->setSymbolBrush(QwtPlotTradingCurve::Decreasing, b);
}

void DAChartTradingCurveItemSettingWidget::onButtonGroupOrientationClicked(QAbstractButton* b)
{
    Q_UNUSED(b);
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    updateOrientationFromUI(s_cast< QwtPlotTradingCurve* >());
}

void DAChartTradingCurveItemSettingWidget::onCurvePenChanged(const QPen& p)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotTradingCurve* c = s_cast< QwtPlotTradingCurve* >();
    c->setSymbolPen(p);
}

void DAChartTradingCurveItemSettingWidget::onDoubleSpinBoxExternValueChanged(double v)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotTradingCurve* item = s_cast< QwtPlotTradingCurve* >();
    item->setSymbolExtent(v);
}

void DAChartTradingCurveItemSettingWidget::onDoubleSpinBoxMinValueChanged(double v)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotTradingCurve* item = s_cast< QwtPlotTradingCurve* >();
    item->setMinSymbolWidth(v);
}

void DAChartTradingCurveItemSettingWidget::onDoubleSpinBoxMaxValueChanged(double v)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotTradingCurve* item = s_cast< QwtPlotTradingCurve* >();
    item->setMaxSymbolWidth(v);
}
}
