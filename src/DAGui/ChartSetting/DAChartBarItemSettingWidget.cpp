#include "DAChartBarItemSettingWidget.h"
#include "ui_DAChartBarItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "qwt_plot_barchart.h"
#include "qwt_column_symbol.h"
#include "qwt_text.h"
#include "qwt_plot.h"
namespace DA
{
DAChartBarItemSettingWidget::DAChartBarItemSettingWidget(QWidget* parent)
	: DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartBarItemSettingWidget)
{
	ui->setupUi(this);
	resetUI();
	connect(ui->comboBoxBarStyle,
			QOverload< int >::of(&QComboBox::currentIndexChanged),
			this,
			&DAChartBarItemSettingWidget::onBarStyleCurrentIndexChanged);
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolStyleChanged,
			this,
			&DAChartBarItemSettingWidget::onSymbolStyleChanged);
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolSizeChanged,
			this,
			&DAChartBarItemSettingWidget::onSymbolSizeChanged);
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolColorChanged,
			this,
			&DAChartBarItemSettingWidget::onSymbolColorChanged);
	connect(ui->symbolEditWidget,
			&DAChartSymbolEditWidget::symbolOutlinePenChanged,
			this,
			&DAChartBarItemSettingWidget::onSymbolOutlinePenChanged);
	connect(ui->brushEditWidget, &DABrushEditWidget::brushChanged, this, &DAChartBarItemSettingWidget::onBrushChanged);
	//	connect(ui->buttonGroupOrientation,
	//			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
	//			this,
	//			&DAChartBarItemSettingWidget::onButtonGroupOrientationClicked);
	connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartBarItemSettingWidget::onCurvePenChanged);
}

DAChartBarItemSettingWidget::~DAChartBarItemSettingWidget()
{
	delete ui;
}

void DAChartBarItemSettingWidget::plotItemSet(QwtPlotItem* item)
{
	if (nullptr == item) {
		return;
	}
	if (item->rtti() != QwtPlotItem::Rtti_PlotBarChart) {
		return;
	}
	ui->widgetItemSetting->setPlotItem(item);
	QwtPlotBarChart* curItem = static_cast< QwtPlotBarChart* >(item);
	updateUI(curItem);
}

/**
 * @brief 根据QwtPlotBarChart更新ui
 * @param item
 */
void DAChartBarItemSettingWidget::updateUI(const QwtPlotBarChart* item)
{
	ui->widgetItemSetting->updateUI(item);
	QwtPlotBarChart::LegendMode currentMode = item->legendMode();
	ui->checkBoxChart->setChecked(currentMode & QwtPlotBarChart::LegendChartTitle);
	ui->checkBoxBar->setChecked(currentMode & QwtPlotBarChart::LegendBarTitles);
	if (const QwtColumnSymbol* symbol = item->symbol()) {
		// 设置边框笔（从符号的palette和lineWidth构造）
		QPen pen;
		pen.setColor(symbol->palette().color(QPalette::WindowText));  // 边框颜色
		pen.setWidth(symbol->lineWidth());                            // 边框宽度
		setCurvePen(pen);

		// 设置填充画刷（从符号palette获取）
		QBrush fillBrush(symbol->palette().color(QPalette::Window));
		enableFillEdit(fillBrush != Qt::NoBrush);
		setFillBrush(fillBrush);
	} else {
		// 默认无符号时的样式
		setCurvePen(QPen(Qt::black, 1));
		enableFillEdit(false);
		setFillBrush(Qt::NoBrush);
	}
	setBaseLine(item->baseline());
}

/**
 * @brief 根据ui更新QwtPlotBarChart
 * @param item
 */
void DAChartBarItemSettingWidget::updatePlotItem(QwtPlotBarChart* item)
{
	// plot item
	ui->widgetItemSetting->updatePlotItem(item);
	// plot bar
	item->setTitle(getTitle());
	auto s = getBarStyle();
	if (item->symbol()->style() != s) {
		QwtColumnSymbol* newSymbol = new QwtColumnSymbol(s);  // 创建新对象
		item->setSymbol(newSymbol);
	}
	if (ui->checkBoxChart->isChecked()) {
		item->setLegendMode(QwtPlotBarChart::LegendChartTitle);
	} else {
		item->setLegendMode(QwtPlotBarChart::LegendBarTitles);
	}

	// pen
	item->setPen(getCurvePen());
	// symbol
	if (isEnableMarkerEdit()) {
		QwtSymbol* symbol = ui->symbolEditWidget->createSymbol();
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
	// baseline
	if (isHaveBaseLine()) {
		double bl = getBaseLine();
		if (!qFuzzyCompare(bl, item->baseline())) {
			item->setBaseline(bl);
		}
	}
}

/**
 * @brief 重置BarStyle ComboBox
 */
void DAChartBarItemSettingWidget::resetBarStyleComboBox()
{
	ui->comboBoxBarStyle->clear();
	ui->comboBoxBarStyle->addItem(tr("NoStyle"), static_cast< int >(QwtColumnSymbol::NoStyle));
	ui->comboBoxBarStyle->addItem(tr("Box"), static_cast< int >(QwtColumnSymbol::Box));
	ui->comboBoxBarStyle->setCurrentIndex(0);
}

/**
 * @brief DAChartBarItemSettingWidget::onCurveStyleCurrentIndexChanged
 * @param index
 */
void DAChartBarItemSettingWidget::onBarStyleCurrentIndexChanged(int index)
{
	QwtColumnSymbol::Style s = static_cast< QwtColumnSymbol::Style >(ui->comboBoxBarStyle->currentData().toInt());
	switch (s) {
	case QwtColumnSymbol::NoStyle:
		ui->checkBoxEnableFill->setEnabled(true);
		ui->checkBoxBar->setEnabled(true);
	case QwtColumnSymbol::Box:
		ui->checkBoxEnableFill->setEnabled(true);
		ui->checkBoxBar->setEnabled(false);
	case QwtColumnSymbol::UserStyle:
		ui->checkBoxEnableFill->setEnabled(true);
		ui->checkBoxChart->setEnabled(true);
		break;
	}
}

void DAChartBarItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
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
void DAChartBarItemSettingWidget::setTitle(const QString& t)
{
    ui->widgetItemSetting->setItemTitle(t);
}

/**
 * @brief 曲线标题
 * @return
 */
QString DAChartBarItemSettingWidget::getTitle() const
{
    return ui->widgetItemSetting->getItemTitle();
}

/**
 * @brief 设置BarStyle
 * @param v
 */
void DAChartBarItemSettingWidget::setBarStyle(QwtColumnSymbol::Style v)
{
	int i = ui->comboBoxBarStyle->findData(static_cast< int >(v));
	if (i >= 0) {
		ui->comboBoxBarStyle->setCurrentIndex(i);
	}
}

/**
 * @brief 获取BarStyle
 * @return
 */
QwtColumnSymbol::Style DAChartBarItemSettingWidget::getBarStyle() const
{
    return static_cast< QwtColumnSymbol::Style >(ui->comboBoxBarStyle->currentData().toInt());
}

/**
 * @brief 设置Bar Legend Mode
 * @param v
 */
void DAChartBarItemSettingWidget::setBarLegendMode(QwtPlotBarChart::LegendMode v)
{
	ui->checkBoxChart->setChecked(QwtPlotBarChart::LegendChartTitle & v);
	ui->checkBoxBar->setChecked(QwtPlotBarChart::LegendBarTitles & v);
}

/**
 * @brief 获取Bar Legend Mode
 * @return
 */
QwtPlotBarChart::LegendMode DAChartBarItemSettingWidget::getBarLegendMode() const
{
	QwtPlotBarChart::LegendMode mode = static_cast< QwtPlotBarChart::LegendMode >(0);  // 初始化为无标志

	if (ui->checkBoxChart->isChecked()) {
		mode = static_cast< QwtPlotBarChart::LegendMode >(mode | QwtPlotBarChart::LegendChartTitle);
	}

	if (ui->checkBoxBar->isChecked()) {
		mode = static_cast< QwtPlotBarChart::LegendMode >(mode | QwtPlotBarChart::LegendBarTitles);
	}

	return mode;
}

/**
 * @brief 开启marker编辑
 * @param on
 */
void DAChartBarItemSettingWidget::enableMarkerEdit(bool on)
{
    ui->checkBoxEnableMarker->setChecked(on);
}

/**
 * @brief 是否由marker
 * @return
 */
bool DAChartBarItemSettingWidget::isEnableMarkerEdit() const
{
    return ui->checkBoxEnableMarker->isChecked();
}

/**
 * @brief 开启填充编辑
 * @param on
 */
void DAChartBarItemSettingWidget::enableFillEdit(bool on)
{
    ui->checkBoxEnableFill->setChecked(on);
}

/**
 * @brief 是否开启填充编辑
 * @return
 */
bool DAChartBarItemSettingWidget::isEnableFillEdit() const
{
    return ui->checkBoxEnableFill->isChecked();
}

/**
 * @brief 画笔
 * @param v
 */
void DAChartBarItemSettingWidget::setCurvePen(const QPen& v)
{
    ui->penEditWidget->setCurrentPen(v);
}

/**
 * @brief 画笔
 * @return
 */
QPen DAChartBarItemSettingWidget::getCurvePen() const
{
    return ui->penEditWidget->getCurrentPen();
}

/**
 * @brief 填充
 * @param v
 */
void DAChartBarItemSettingWidget::setFillBrush(const QBrush& v)
{
    ui->brushEditWidget->setCurrentBrush(v);
}

/**
 * @brief 填充
 * @return
 */
QBrush DAChartBarItemSettingWidget::getFillBrush() const
{
    return ui->brushEditWidget->getCurrentBrush();
}

/**
 * @brief 基线
 * @param v
 */
void DAChartBarItemSettingWidget::setBaseLine(double v)
{
    ui->lineEditBaseLine->setText(QString::number(v));
}

/**
 * @brief 基线
 * @return
 */
double DAChartBarItemSettingWidget::getBaseLine() const
{
	bool isok = false;
	double v  = ui->lineEditBaseLine->text().toDouble(&isok);
	if (!isok) {
		return 0.0;
	}
	return v;
}

/**
 * @brief 判断是否设置了基线
 * @return
 */
bool DAChartBarItemSettingWidget::isHaveBaseLine() const
{
    return !(ui->lineEditBaseLine->text().isEmpty());
}

/**
 * @brief 重置ui
 */
void DAChartBarItemSettingWidget::resetUI()
{
	resetBarStyleComboBox();
	//	ui->checkBoxInverted->setEnabled(false);
	ui->symbolEditWidget->setSymbolStyle(QwtSymbol::Rect);
	ui->symbolEditWidget->setSymbolSize(4);
	ui->symbolEditWidget->setSymbolOutlinePen(Qt::NoPen);
	ui->symbolEditWidget->setSymbolColor(Qt::black);
	enableMarkerEdit(false);
	enableFillEdit(false);
	setBaseLine(0.0);
}

/**
 * @brief 获取item plot widget
 * @return
 */
DAChartPlotItemSettingWidget* DAChartBarItemSettingWidget::getItemSettingWidget() const
{
    return ui->widgetItemSetting;
}

void DAChartBarItemSettingWidget::on_checkBoxChart_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	if (checked) {
		c->setLegendMode(QwtPlotBarChart::LegendMode::LegendChartTitle);
	}
}

void DAChartBarItemSettingWidget::on_checkBoxBar_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	if (checked) {
		c->setLegendMode(QwtPlotBarChart::LegendMode::LegendBarTitles);
	}
}

void DAChartBarItemSettingWidget::on_checkBoxLegendShowLine_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setLegendAttribute(QwtPlotBarChart::LegendShowLine, checked);
}

void DAChartBarItemSettingWidget::on_checkBoxLegendShowSymbol_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setLegendAttribute(QwtPlotBarChart::LegendShowSymbol, checked);
}

void DAChartBarItemSettingWidget::on_checkBoxLegendShowBrush_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setLegendAttribute(QwtPlotBarChart::LegendShowBrush, checked);
}

void DAChartBarItemSettingWidget::on_checkBoxEnableMarker_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	if (checked) {
		QwtSymbol* symbol = ui->symbolEditWidget->createSymbol();
		c->setSymbol(symbol);
	} else {
		c->setSymbol(nullptr);
	}
}

void DAChartBarItemSettingWidget::onSymbolStyleChanged(QwtSymbol::Style s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartBarItemSettingWidget::onSymbolSizeChanged(int s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartBarItemSettingWidget::onSymbolColorChanged(const QColor& s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartBarItemSettingWidget::onSymbolOutlinePenChanged(const QPen& s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartBarItemSettingWidget::onBrushChanged(const QBrush& b)
{
	Q_UNUSED(b);
	on_checkBoxEnableFill_clicked(ui->checkBoxEnableFill->isChecked());
}

void DAChartBarItemSettingWidget::on_checkBoxEnableFill_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	if (checked) {
		c->setBrush(getFillBrush());
	} else {
		c->setBrush(Qt::NoBrush);
	}
}

void DAChartBarItemSettingWidget::on_lineEditBaseLine_editingFinished()
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	double bl          = getBaseLine();
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	if (!qFuzzyCompare(bl, c->baseline())) {
		c->setBaseline(bl);
	}
}

void DAChartBarItemSettingWidget::onButtonGroupOrientationClicked(QAbstractButton* b)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	auto ori           = getOrientation();
	if (c->orientation() != ori) {
		c->setOrientation(ori);
	}
}

void DAChartBarItemSettingWidget::onCurvePenChanged(const QPen& p)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setPen(p);
}
}
