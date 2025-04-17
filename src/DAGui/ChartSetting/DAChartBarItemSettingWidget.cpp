#include "DAChartBarItemSettingWidget.h"
#include "ui_DAChartBarItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "DASignalBlockers.hpp"
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
    connect(ui->brushEditWidget, &DABrushEditWidget::brushChanged, this, &DAChartBarItemSettingWidget::onFillBrushChanged);
	connect(ui->comboBoxLayoutPolicy,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartBarItemSettingWidget::onLayoutPolicyChanged);
	connect(ui->spinBoxSpacing,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartBarItemSettingWidget::onSpacingValueChanged);
	connect(ui->spinBoxMargin,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartBarItemSettingWidget::onMarginValueChanged);
	connect(ui->doubleSpinBoxLayoutHint,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartBarItemSettingWidget::onLayoutHintValueChanged);
    connect(ui->groupBoxFill, &QGroupBox::clicked, this, &DAChartBarItemSettingWidget::onGroupBoxFillClicked);
    connect(ui->groupBoxEdge, &QGroupBox::clicked, this, &DAChartBarItemSettingWidget::onGroupBoxEdgeClicked);
    connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartBarItemSettingWidget::onEdgePenChanged);
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
	QwtPlotBarChart* barItem = static_cast< QwtPlotBarChart* >(item);
	updateUI(barItem);
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
    DASignalBlockers b1ocker(ui->brushEditWidget, ui->penEditWidget);
	if (const QwtColumnSymbol* symbol = item->symbol()) {
		// 设置边框笔（从符号的palette和lineWidth构造）
		QPen pen;
        pen.setColor(symbol->palette().color(QPalette::WindowText));  // 边框颜色
        pen.setWidth(symbol->lineWidth());                            // 边框宽度
        setEnableEdgeEdit(true);
        setEdgePen(pen);

		// 设置填充画刷（从符号palette获取）
		QBrush fillBrush(symbol->palette().color(QPalette::Window));
        setEnableFillEdit(true);
		setFillBrush(fillBrush);
	} else {
		// 默认无符号时的样式
        setEnableEdgeEdit(false);
        setEnableFillEdit(false);
	}
	setBaseLine(item->baseline());

	// 设置布局策略相关参数
	QwtPlotAbstractBarChart::LayoutPolicy policy = item->layoutPolicy();
	switch (policy) {
	case QwtPlotAbstractBarChart::AutoAdjustSamples:
		ui->comboBoxLayoutPolicy->setCurrentIndex(0);
		break;
	case QwtPlotAbstractBarChart::ScaleSamplesToAxes:
		ui->comboBoxLayoutPolicy->setCurrentIndex(1);
		break;
	case QwtPlotAbstractBarChart::ScaleSampleToCanvas:
		ui->comboBoxLayoutPolicy->setCurrentIndex(2);
		break;
	case QwtPlotAbstractBarChart::FixedSampleSize:
		ui->comboBoxLayoutPolicy->setCurrentIndex(3);
		break;
	}

	// 设置布局相关参数
	ui->spinBoxSpacing->setValue(item->spacing());
	ui->spinBoxMargin->setValue(item->margin());
	ui->doubleSpinBoxLayoutHint->setValue(item->layoutHint());

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

	if (ui->checkBoxChart->isChecked()) {
		item->setLegendMode(QwtPlotBarChart::LegendChartTitle);
	} else {
		item->setLegendMode(QwtPlotBarChart::LegendBarTitles);
	}

	// 创建新的柱状图符号
	QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Style::Box);
	// 设置符号的画笔和画刷
	QPalette pal = symbol->palette();
	if (isEnableFillEdit()) {
		QBrush brush = getFillBrush();
		pal.setColor(QPalette::Window, brush.color());  // 填充颜色
	} else {
		pal.setColor(QPalette::Window, Qt::transparent);  // 透明填充
	}
	symbol->setPalette(pal);
	item->setSymbol(symbol);

	// baseline
	if (isHaveBaseLine()) {
		double bl = getBaseLine();
		if (!qFuzzyCompare(bl, item->baseline())) {
			item->setBaseline(bl);
		}
	}

	// 更新布局策略
    QwtPlotAbstractBarChart::LayoutPolicy policy = static_cast< QwtPlotAbstractBarChart::LayoutPolicy >(
        ui->comboBoxLayoutPolicy->currentData().toInt());
	item->setLayoutPolicy(policy);

	// 更新布局参数
	item->setSpacing(ui->spinBoxSpacing->value());
	item->setMargin(ui->spinBoxMargin->value());
	item->setLayoutHint(ui->doubleSpinBoxLayoutHint->value());

	// 更新基线
	if (isHaveBaseLine()) {
		double bl = getBaseLine();
		if (!qFuzzyCompare(bl, item->baseline())) {
			item->setBaseline(bl);
		}
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
 * @brief 开启填充编辑
 * @param on
 */
void DAChartBarItemSettingWidget::setEnableFillEdit(bool on)
{
    ui->groupBoxFill->setChecked(on);
}

/**
 * @brief 是否开启填充编辑
 * @return
 */
bool DAChartBarItemSettingWidget::isEnableFillEdit() const
{
    return ui->groupBoxFill->isChecked();
}

/**
 * @brief 是否允许边框设置
 * @param on
 */
void DAChartBarItemSettingWidget::setEnableEdgeEdit(bool on)
{
    ui->groupBoxEdge->setChecked(on);
}

/**
 * @brief 是否允许边框设置
 * @return
 */
bool DAChartBarItemSettingWidget::isEnableEdgeEdit() const
{
    return ui->groupBoxEdge->isChecked();
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
 * @brief 设置边线
 * @param pen
 */
void DAChartBarItemSettingWidget::setEdgePen(const QPen& pen)
{
    ui->penEditWidget->setCurrentPen(pen);
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
 * @brief 获取设置的画笔
 * @return
 */
QPen DAChartBarItemSettingWidget::getEdgePen() const
{
    return ui->penEditWidget->getCurrentPen();
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
    setEnableFillEdit(false);
	setBaseLine(0.0);
	ui->comboBoxLayoutPolicy->clear();
	ui->comboBoxLayoutPolicy->addItem(tr("Auto Adjust Samples"), QwtPlotAbstractBarChart::AutoAdjustSamples);
	ui->comboBoxLayoutPolicy->addItem(tr("Scale Samples To Axes"), QwtPlotAbstractBarChart::ScaleSamplesToAxes);
	ui->comboBoxLayoutPolicy->addItem(tr("Scale Sample To Canvas"), QwtPlotAbstractBarChart::ScaleSampleToCanvas);
	ui->comboBoxLayoutPolicy->addItem(tr("Fixed Sample Size"), QwtPlotAbstractBarChart::FixedSampleSize);
	ui->doubleSpinBoxLayoutHint->setValue(0.0);
	ui->spinBoxSpacing->setValue(0);
	ui->spinBoxMargin->setValue(0);
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

void DAChartBarItemSettingWidget::onFillBrushChanged(const QBrush& b)
{
    Q_UNUSED(b);
    updateSymbolToItem();
}

void DAChartBarItemSettingWidget::onEdgePenChanged(const QPen& p)
{
    Q_UNUSED(p);
    updateSymbolToItem();
}

void DAChartBarItemSettingWidget::onGroupBoxFillClicked(bool on)
{
    Q_UNUSED(on);
    updateSymbolToItem();
}

void DAChartBarItemSettingWidget::onGroupBoxEdgeClicked(bool on)
{
    Q_UNUSED(on);
    updateSymbolToItem();
}

void DAChartBarItemSettingWidget::on_lineEditBaseLine_editingFinished()
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	double bl          = getBaseLine();
	if (!qFuzzyCompare(bl, c->baseline())) {
		c->setBaseline(bl);
	}
}

// 实现布局策略相关接口
void DAChartBarItemSettingWidget::setLayoutPolicy(QwtPlotAbstractBarChart::LayoutPolicy policy)
{
	ui->comboBoxLayoutPolicy->setCurrentIndex(static_cast< int >(policy));
}

QwtPlotAbstractBarChart::LayoutPolicy DAChartBarItemSettingWidget::getLayoutPolicy() const
{
	return static_cast< QwtPlotAbstractBarChart::LayoutPolicy >(ui->comboBoxLayoutPolicy->currentData().toInt());
}

void DAChartBarItemSettingWidget::setLayoutHint(double hint)
{
	ui->doubleSpinBoxLayoutHint->setValue(hint);
}

double DAChartBarItemSettingWidget::getLayoutHint() const
{
	return ui->doubleSpinBoxLayoutHint->value();
}

void DAChartBarItemSettingWidget::setSpacing(int spacing)
{
	ui->spinBoxSpacing->setValue(spacing);
}

int DAChartBarItemSettingWidget::getSpacing() const
{
	return ui->spinBoxSpacing->value();
}

void DAChartBarItemSettingWidget::setMargin(int margin)
{
	ui->spinBoxMargin->setValue(margin);
}

int DAChartBarItemSettingWidget::getMargin() const
{
    return ui->spinBoxMargin->value();
}

/**
 * @brief 获取当前界面选中的QwtColumnSymbol::FrameStyle
 * @return
 */
int DAChartBarItemSettingWidget::getCurrentSelectFrameStyle() const
{
    return QwtColumnSymbol::Plain;
}

/**
 * @brief QWT中QPalette::Dark决定了QwtColumnSymbol的边框颜色，QPalette::Window决定了填充颜色
 */
void DAChartBarItemSettingWidget::updateSymbolToItem()
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    bool isEnabelFill                      = ui->groupBoxFill->isChecked();
    bool isEnableEdge                      = ui->groupBoxEdge->isChecked();
    QwtPlotBarChart* bar                   = s_cast< QwtPlotBarChart* >();
    QwtColumnSymbol::FrameStyle framestyle = static_cast< QwtColumnSymbol::FrameStyle >(getCurrentSelectFrameStyle());
    // 获取当前符号（const指针）
    const QwtColumnSymbol* currentSymbol = bar->symbol();
    std::unique_ptr< QwtColumnSymbol > newSymbol;

    if (currentSymbol) {
        newSymbol = std::make_unique< QwtColumnSymbol >(currentSymbol->style());
        newSymbol->setPalette(currentSymbol->palette());
        newSymbol->setLineWidth(currentSymbol->lineWidth());
    } else {
        // 创建默认符号
        newSymbol = std::make_unique< QwtColumnSymbol >(QwtColumnSymbol::Box);
    }
    newSymbol->setFrameStyle(framestyle);
    // 更新填充状态
    QPalette palette = newSymbol->palette();
    palette.setColor(QPalette::Window, isEnabelFill ? getFillBrush().color() : Qt::transparent);
    if (isEnableEdge) {
        QPen pen = getEdgePen();
        newSymbol->setLineWidth(pen.width());
        palette.setColor(QPalette::Dark, pen.color());
    }
    newSymbol->setPalette(palette);
    bar->setSymbol(newSymbol.release());
}

void DAChartBarItemSettingWidget::onLayoutPolicyChanged(int index)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setLayoutPolicy(
        static_cast< QwtPlotAbstractBarChart::LayoutPolicy >(ui->comboBoxLayoutPolicy->currentData().toInt()));
}

void DAChartBarItemSettingWidget::onSpacingValueChanged(int value)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setSpacing(value);
}

void DAChartBarItemSettingWidget::onMarginValueChanged(int value)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setMargin(value);
}

void DAChartBarItemSettingWidget::onLayoutHintValueChanged(double value)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotBarChart* c = s_cast< QwtPlotBarChart* >();
	c->setLayoutHint(value);
}
}
