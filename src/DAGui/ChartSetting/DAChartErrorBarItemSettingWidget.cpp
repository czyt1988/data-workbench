#include "DAChartErrorBarItemSettingWidget.h"
#include "ui_DAChartErrorBarItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "qwt_text.h"
#include "qwt_plot.h"
namespace DA
{
DAChartErrorBarItemSettingWidget::DAChartErrorBarItemSettingWidget(QWidget* parent)
	: DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartErrorBarItemSettingWidget)
{
	ui->setupUi(this);
	resetUI();
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolStyleChanged,
			this,
			&DAChartErrorBarItemSettingWidget::onSymbolStyleChanged);
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolSizeChanged,
			this,
			&DAChartErrorBarItemSettingWidget::onSymbolSizeChanged);
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolColorChanged,
			this,
			&DAChartErrorBarItemSettingWidget::onSymbolColorChanged);
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolOutlinePenChanged,
			this,
			&DAChartErrorBarItemSettingWidget::onSymbolOutlinePenChanged);
	connect(ui->brushEditWidget, &DABrushEditWidget::brushChanged, this, &DAChartErrorBarItemSettingWidget::onBrushChanged);
	connect(ui->buttonGroupOrientation,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartErrorBarItemSettingWidget::onButtonGroupOrientationClicked);
	connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartErrorBarItemSettingWidget::onCurvePenChanged);
}

DAChartErrorBarItemSettingWidget::~DAChartErrorBarItemSettingWidget()
{
	delete ui;
}

void DAChartErrorBarItemSettingWidget::plotItemSet(QwtPlotItem* item)
{
	if (nullptr == item) {
		return;
	}
	if (item->rtti() != QwtPlotItem::Rtti_PlotIntervalCurve) {
		return;
	}
	ui->widgetItemSetting->setPlotItem(item);
	QwtPlotIntervalCurve* curItem = static_cast< QwtPlotIntervalCurve* >(item);
	updateUI(curItem);
}

/**
 * @brief 根据QwtPlotCurve更新ui
 * @param item
 */
void DAChartErrorBarItemSettingWidget::updateUI(const QwtPlotIntervalCurve* item)
{
	ui->widgetItemSetting->updateUI(item);
	setCurvePen(item->pen());
	QBrush b = item->brush();
	if (b != Qt::NoBrush) {
		enableFillEdit(true);
		setFillBrush(item->brush());
	} else {
		enableFillEdit(false);
		setFillBrush(item->brush());
	}
	setOrientation(item->orientation());
}

/**
 * @brief 根据ui更新QwtPlotCurve
 * @param item
 */
void DAChartErrorBarItemSettingWidget::updatePlotItem(QwtPlotIntervalCurve* item)
{
	// errorbar item
	ui->widgetItemSetting->updatePlotItem(item);
	// errorbar curve
	item->setTitle(getTitle());
	item->setStyle(QwtPlotIntervalCurve::CurveStyle::Tube);
	// pen
	item->setPen(getCurvePen());
	// symbol
	if (isEnableMarkerEdit()) {
		QwtIntervalSymbol* symbol = ui->symbolEditWidget->createIntervalSymbol();
		item->setSymbol(symbol);
	} else {
		item->setSymbol(nullptr);
	}
	// fill
	if (isEnableFillEdit()) {
		item->setBrush(getFillBrush());
	} else {
		item->setBrush(Qt::NoBrush);
	}
	// Orientation
	auto ori = getOrientation();
	if (item->orientation() != ori) {
		item->setOrientation(ori);
	}
}

void DAChartErrorBarItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
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
void DAChartErrorBarItemSettingWidget::setTitle(const QString& t)
{
    ui->widgetItemSetting->setItemTitle(t);
}

/**
 * @brief 曲线标题
 * @return
 */
QString DAChartErrorBarItemSettingWidget::getTitle() const
{
    return ui->widgetItemSetting->getItemTitle();
}

/**
 * @brief 开启marker编辑
 * @param on
 */
void DAChartErrorBarItemSettingWidget::enableMarkerEdit(bool on)
{
    ui->checkBoxEnableMarker->setChecked(on);
}

/**
 * @brief 是否由marker
 * @return
 */
bool DAChartErrorBarItemSettingWidget::isEnableMarkerEdit() const
{
    return ui->checkBoxEnableMarker->isChecked();
}

/**
 * @brief 开启填充编辑
 * @param on
 */
void DAChartErrorBarItemSettingWidget::enableFillEdit(bool on)
{
    ui->checkBoxEnableFill->setChecked(on);
}

/**
 * @brief 是否开启填充编辑
 * @return
 */
bool DAChartErrorBarItemSettingWidget::isEnableFillEdit() const
{
    return ui->checkBoxEnableFill->isChecked();
}

/**
 * @brief 画笔
 * @param v
 */
void DAChartErrorBarItemSettingWidget::setCurvePen(const QPen& v)
{
    ui->penEditWidget->setCurrentPen(v);
}

/**
 * @brief 画笔
 * @return
 */
QPen DAChartErrorBarItemSettingWidget::getCurvePen() const
{
    return ui->penEditWidget->getCurrentPen();
}

/**
 * @brief 填充
 * @param v
 */
void DAChartErrorBarItemSettingWidget::setFillBrush(const QBrush& v)
{
    ui->brushEditWidget->setCurrentBrush(v);
}

/**
 * @brief 填充
 * @return
 */
QBrush DAChartErrorBarItemSettingWidget::getFillBrush() const
{
    return ui->brushEditWidget->getCurrentBrush();
}

void DAChartErrorBarItemSettingWidget::setOrientation(Qt::Orientation v)
{
    ui->radioButtonHorizontal->setChecked(v == Qt::Horizontal);
}

Qt::Orientation DAChartErrorBarItemSettingWidget::getOrientation() const
{
    return (ui->radioButtonHorizontal->isChecked() ? Qt::Horizontal : Qt::Vertical);
}

/**
 * @brief 重置ui
 */
void DAChartErrorBarItemSettingWidget::resetUI()
{
	ui->symbolEditWidget->setSymbolStyle(QwtSymbol::Rect);
	ui->symbolEditWidget->setSymbolSize(4);
	ui->symbolEditWidget->setSymbolOutlinePen(Qt::NoPen);
	ui->symbolEditWidget->setSymbolColor(Qt::black);
	enableMarkerEdit(false);
	enableFillEdit(false);
	setOrientation(Qt::Horizontal);
}

/**
 * @brief 获取item plot widget
 * @return
 */
DAChartPlotItemSettingWidget* DAChartErrorBarItemSettingWidget::getItemSettingWidget() const
{
    return ui->widgetItemSetting;
}

void DAChartErrorBarItemSettingWidget::on_checkBoxEnableMarker_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
	if (checked) {
		QwtIntervalSymbol* symbol = ui->symbolEditWidget->createIntervalSymbol();
		c->setSymbol(symbol);
	} else {
		c->setSymbol(nullptr);
	}
}

void DAChartErrorBarItemSettingWidget::onSymbolStyleChanged(QwtSymbol::Style s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartErrorBarItemSettingWidget::onSymbolSizeChanged(int s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartErrorBarItemSettingWidget::onSymbolColorChanged(const QColor& s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartErrorBarItemSettingWidget::onSymbolOutlinePenChanged(const QPen& s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartErrorBarItemSettingWidget::onBrushChanged(const QBrush& b)
{
	Q_UNUSED(b);
	on_checkBoxEnableFill_clicked(ui->checkBoxEnableFill->isChecked());
}

void DAChartErrorBarItemSettingWidget::on_checkBoxEnableFill_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
	if (checked) {
		c->setBrush(getFillBrush());
	} else {
		c->setBrush(Qt::NoBrush);
	}
}

void DAChartErrorBarItemSettingWidget::onButtonGroupOrientationClicked(QAbstractButton* b)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
	auto ori                = getOrientation();
	if (c->orientation() != ori) {
		c->setOrientation(ori);
	}
}

void DAChartErrorBarItemSettingWidget::onCurvePenChanged(const QPen& p)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
	c->setPen(p);
}
}
