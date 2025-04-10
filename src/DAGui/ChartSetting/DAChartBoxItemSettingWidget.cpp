#include "DAChartBoxItemSettingWidget.h"
#include "ui_DAChartBoxItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "qwt_text.h"
#include "qwt_plot.h"
namespace DA
{
DAChartBoxItemSettingWidget::DAChartBoxItemSettingWidget(QWidget* parent)
	: DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartBoxItemSettingWidget)
{
	ui->setupUi(this);
	resetUI();
	connect(ui->brushEditWidget, &DABrushEditWidget::brushChanged, this, &DAChartBoxItemSettingWidget::onBrushChanged);
	connect(ui->buttonGroupOrientation,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartBoxItemSettingWidget::onButtonGroupOrientationClicked);
	connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartBoxItemSettingWidget::onCurvePenChanged);
}

DAChartBoxItemSettingWidget::~DAChartBoxItemSettingWidget()
{
	delete ui;
}

void DAChartBoxItemSettingWidget::plotItemSet(QwtPlotItem* item)
{
	if (nullptr == item) {
		return;
	}
	if (item->rtti() != QwtPlotItem::Rtti_PlotTradingCurve) {
		return;
	}
	ui->widgetItemSetting->setPlotItem(item);
	QwtPlotTradingCurve* curItem = static_cast< QwtPlotTradingCurve* >(item);
	updateUI(curItem);
}

/**
 * @brief 根据QwtPlotCurve更新ui
 * @param item
 */
void DAChartBoxItemSettingWidget::updateUI(const QwtPlotTradingCurve* item)
{
    ui->widgetItemSetting->updateUI(item);
}

/**
 * @brief 根据ui更新QwtPlotCurve
 * @param item
 */
void DAChartBoxItemSettingWidget::updatePlotItem(QwtPlotTradingCurve* item)
{
	// box item
	ui->widgetItemSetting->updatePlotItem(item);
	// box curve
	item->setTitle(getTitle());
	item->setSymbolStyle(QwtPlotTradingCurve::SymbolStyle::CandleStick);
	// pen
	item->setSymbolPen(getCurvePen());
	// fill
	if (isEnableFillEdit()) {
		item->setSymbolBrush(QwtPlotTradingCurve::Increasing, getFillBrush());
		item->setSymbolBrush(QwtPlotTradingCurve::Decreasing, getFillBrush());
	} else {
		item->setSymbolBrush(QwtPlotTradingCurve::Increasing, Qt::NoBrush);
		item->setSymbolBrush(QwtPlotTradingCurve::Decreasing, Qt::NoBrush);
	}
	// Orientation
	auto ori = getOrientation();
	if (item->orientation() != ori) {
		item->setOrientation(ori);
	}
}

void DAChartBoxItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
{
	if (!on && plotItem == getPlotItem()) {
		resetUI();
	}
	DAAbstractChartItemSettingWidget::plotItemAttached(plotItem, on);
}

/**
 * @brief 曲线标题
 * @param t
 */
void DAChartBoxItemSettingWidget::setTitle(const QString& t)
{
    ui->widgetItemSetting->setItemTitle(t);
}

/**
 * @brief 曲线标题
 * @return
 */
QString DAChartBoxItemSettingWidget::getTitle() const
{
    return ui->widgetItemSetting->getItemTitle();
}

/**
 * @brief 开启填充编辑
 * @param on
 */
void DAChartBoxItemSettingWidget::enableFillEdit(bool on)
{
    ui->checkBoxEnableFill->setChecked(on);
}

/**
 * @brief 是否开启填充编辑
 * @return
 */
bool DAChartBoxItemSettingWidget::isEnableFillEdit() const
{
    return ui->checkBoxEnableFill->isChecked();
}

/**
 * @brief 画笔
 * @param v
 */
void DAChartBoxItemSettingWidget::setCurvePen(const QPen& v)
{
    ui->penEditWidget->setCurrentPen(v);
}

/**
 * @brief 画笔
 * @return
 */
QPen DAChartBoxItemSettingWidget::getCurvePen() const
{
    return ui->penEditWidget->getCurrentPen();
}

/**
 * @brief 填充
 * @param v
 */
void DAChartBoxItemSettingWidget::setFillBrush(const QBrush& v)
{
    ui->brushEditWidget->setCurrentBrush(v);
}

/**
 * @brief 填充
 * @return
 */
QBrush DAChartBoxItemSettingWidget::getFillBrush() const
{
    return ui->brushEditWidget->getCurrentBrush();
}

void DAChartBoxItemSettingWidget::setOrientation(Qt::Orientation v)
{
    ui->radioButtonHorizontal->setChecked(v == Qt::Horizontal);
}

Qt::Orientation DAChartBoxItemSettingWidget::getOrientation() const
{
    return (ui->radioButtonHorizontal->isChecked() ? Qt::Horizontal : Qt::Vertical);
}

/**
 * @brief 重置ui
 */
void DAChartBoxItemSettingWidget::resetUI()
{
	enableFillEdit(false);
	setOrientation(Qt::Horizontal);
}

/**
 * @brief 获取item plot widget
 * @return
 */
DAChartPlotItemSettingWidget* DAChartBoxItemSettingWidget::getItemSettingWidget() const
{
    return ui->widgetItemSetting;
}

void DAChartBoxItemSettingWidget::onBrushChanged(const QBrush& b)
{
	Q_UNUSED(b);
	on_checkBoxEnableFill_clicked(ui->checkBoxEnableFill->isChecked());
}

void DAChartBoxItemSettingWidget::on_checkBoxEnableFill_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotTradingCurve* c = s_cast< QwtPlotTradingCurve* >();
	if (checked) {
		c->setSymbolBrush(QwtPlotTradingCurve::Decreasing, getFillBrush());
		c->setSymbolBrush(QwtPlotTradingCurve::Increasing, getFillBrush());
	} else {
		c->setSymbolBrush(QwtPlotTradingCurve::Increasing, Qt::NoBrush);
		c->setSymbolBrush(QwtPlotTradingCurve::Decreasing, Qt::NoBrush);
	}
}

void DAChartBoxItemSettingWidget::onButtonGroupOrientationClicked(QAbstractButton* b)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotTradingCurve* c = s_cast< QwtPlotTradingCurve* >();
	auto ori               = getOrientation();
	if (c->orientation() != ori) {
		c->setOrientation(ori);
	}
}

void DAChartBoxItemSettingWidget::onCurvePenChanged(const QPen& p)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotTradingCurve* c = s_cast< QwtPlotTradingCurve* >();
	c->setSymbolPen(p);
}
}
