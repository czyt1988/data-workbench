#include "DAChartCurveItemSettingWidget.h"
#include "ui_DAChartCurveItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "qwt_text.h"
#include "qwt_plot.h"

#ifndef DAChartCurveItemSettingWidget_DEBUG_PRINT
#define DAChartCurveItemSettingWidget_DEBUG_PRINT 1
#endif

namespace DA
{
DAChartCurveItemSettingWidget::DAChartCurveItemSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartCurveItemSettingWidget)
{
	ui->setupUi(this);
	resetUI();
	connect(ui->comboBoxCurveStyle,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartCurveItemSettingWidget::onCurveStyleCurrentIndexChanged);
	connect(ui->symbolEditWidget,
            &DAChartSymbolEditWidget::symbolStyleChanged,
            this,
            &DAChartCurveItemSettingWidget::onSymbolStyleChanged);
	connect(ui->symbolEditWidget,
            &DAChartSymbolEditWidget::symbolSizeChanged,
            this,
            &DAChartCurveItemSettingWidget::onSymbolSizeChanged);
	connect(ui->symbolEditWidget,
            &DAChartSymbolEditWidget::symbolColorChanged,
            this,
            &DAChartCurveItemSettingWidget::onSymbolColorChanged);
	connect(ui->symbolEditWidget,
            &DAChartSymbolEditWidget::symbolOutlinePenChanged,
            this,
            &DAChartCurveItemSettingWidget::onSymbolOutlinePenChanged);
	connect(ui->brushEditWidget, &DABrushEditWidget::brushChanged, this, &DAChartCurveItemSettingWidget::onBrushChanged);
	connect(ui->buttonGroupOrientation,
            QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
            this,
            &DAChartCurveItemSettingWidget::onButtonGroupOrientationClicked);
	connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartCurveItemSettingWidget::onCurvePenChanged);
}

DAChartCurveItemSettingWidget::~DAChartCurveItemSettingWidget()
{
	delete ui;
}

/**
 * @brief 根据QwtPlotCurve更新ui
 * @param item
 */
void DAChartCurveItemSettingWidget::updateUI(QwtPlotItem* item)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::updateUI=" << quintptr(item);
#endif
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotCurve) {
        return;
    }
    QwtPlotCurve* curItem = static_cast< QwtPlotCurve* >(item);
    ui->checkBoxFitted->setChecked(curItem->testCurveAttribute(QwtPlotCurve::Fitted));
    ui->checkBoxInverted->setChecked(curItem->testCurveAttribute(QwtPlotCurve::Inverted));
    setLegendAttribute(curItem->legendAttributes());
    setCurvePen(curItem->pen());
    QBrush b = curItem->brush();
	if (b != Qt::NoBrush) {
		enableFillEdit(true);
        setFillBrush(curItem->brush());
	} else {
		enableFillEdit(false);
        setFillBrush(curItem->brush());
	}
    setBaseLine(curItem->baseline());
    setOrientation(curItem->orientation());
}

/**
 * @brief 根据ui更新QwtPlotCurve
 * @param item
 */
void DAChartCurveItemSettingWidget::applySetting(QwtPlotCurve* item)
{
	// plot item
    ui->widgetItemSetting->applySetting(item);
	// plot curve
	item->setTitle(getTitle());
	auto s = getCurveStyle();
	if (item->style() != s) {
		item->setStyle(s);
	}
	item->setCurveAttribute(QwtPlotCurve::Fitted, ui->checkBoxFitted->isChecked());
	item->setCurveAttribute(QwtPlotCurve::Inverted, ui->checkBoxInverted->isChecked());
	auto la = getLegendAttribute();
	if (item->legendAttributes() != la) {
		item->setLegendAttributes(la);
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
	// Orientation
	auto ori = getOrientation();
	if (item->orientation() != ori) {
		item->setOrientation(ori);
	}
}

/**
 * @brief 重置CurveStyle ComboBox
 */
void DAChartCurveItemSettingWidget::resetCurveStyleComboBox()
{
	ui->comboBoxCurveStyle->clear();
	ui->comboBoxCurveStyle->addItem(tr("Lines"), static_cast< int >(QwtPlotCurve::Lines));
	ui->comboBoxCurveStyle->addItem(tr("Sticks"), static_cast< int >(QwtPlotCurve::Sticks));
	ui->comboBoxCurveStyle->addItem(tr("Steps"), static_cast< int >(QwtPlotCurve::Steps));
	ui->comboBoxCurveStyle->addItem(tr("Dots"), static_cast< int >(QwtPlotCurve::Dots));
	ui->comboBoxCurveStyle->addItem(tr("No Curve"), static_cast< int >(QwtPlotCurve::NoCurve));
	ui->comboBoxCurveStyle->setCurrentIndex(0);
}

/**
 * @brief DAChartCurveItemSettingWidget::onCurveStyleCurrentIndexChanged
 * @param index
 */
void DAChartCurveItemSettingWidget::onCurveStyleCurrentIndexChanged(int index)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::onCurveStyleCurrentIndexChanged=" << index;
#endif
	QwtPlotCurve::CurveStyle s = static_cast< QwtPlotCurve::CurveStyle >(ui->comboBoxCurveStyle->currentData().toInt());
	switch (s) {
	case QwtPlotCurve::Lines:
	case QwtPlotCurve::NoCurve:
	case QwtPlotCurve::Dots:
	case QwtPlotCurve::UserCurve:
		ui->checkBoxEnableFill->setEnabled(true);
		ui->checkBoxInverted->setEnabled(false);
		break;
	case QwtPlotCurve::Steps:
		ui->checkBoxEnableFill->setEnabled(true);
		ui->checkBoxInverted->setEnabled(true);
		break;
	case QwtPlotCurve::Sticks:
		ui->checkBoxEnableFill->setEnabled(false);
		ui->checkBoxInverted->setEnabled(false);
		break;
	}
}

void DAChartCurveItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
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
void DAChartCurveItemSettingWidget::setTitle(const QString& t)
{
    ui->widgetItemSetting->setItemTitle(t);
}

/**
 * @brief 曲线标题
 * @return
 */
QString DAChartCurveItemSettingWidget::getTitle() const
{
    return ui->widgetItemSetting->getItemTitle();
}

/**
 * @brief 设置CurveStyle
 * @param v
 */
void DAChartCurveItemSettingWidget::setCurveStyle(QwtPlotCurve::CurveStyle v)
{
	int i = ui->comboBoxCurveStyle->findData(static_cast< int >(v));
	if (i >= 0) {
		ui->comboBoxCurveStyle->setCurrentIndex(i);
	}
}

/**
 * @brief 获取CurveStyle
 * @return
 */
QwtPlotCurve::CurveStyle DAChartCurveItemSettingWidget::getCurveStyle() const
{
    return static_cast< QwtPlotCurve::CurveStyle >(ui->comboBoxCurveStyle->currentData().toInt());
}

/**
 * @brief 设置Curve Attribute
 * @param v
 */
void DAChartCurveItemSettingWidget::setCurveAttribute(QwtPlotCurve::CurveAttribute v)
{
	ui->checkBoxFitted->setChecked(QwtPlotCurve::Fitted & v);
	ui->checkBoxInverted->setChecked(QwtPlotCurve::Inverted & v);
}

/**
 * @brief 获取Curve Attribute
 * @return
 */
QwtPlotCurve::CurveAttribute DAChartCurveItemSettingWidget::getCurveAttribute() const
{
	QwtPlotCurve::CurveAttribute v(QwtPlotCurve::Fitted);
	ui->checkBoxFitted->isChecked() ? (v | QwtPlotCurve::Fitted) : (v & (~QwtPlotCurve::Fitted));
	ui->checkBoxInverted->isChecked() ? (v | QwtPlotCurve::Inverted) : (v & (~QwtPlotCurve::Inverted));
	return v;
}

/**
 * @brief 设置 Legend Attribute
 * @param v
 */
void DAChartCurveItemSettingWidget::setLegendAttribute(QwtPlotCurve::LegendAttributes v)
{
	ui->checkBoxLegendShowLine->setChecked(v.testFlag(QwtPlotCurve::LegendShowLine));
	ui->checkBoxLegendShowSymbol->setChecked(v.testFlag(QwtPlotCurve::LegendShowSymbol));
	ui->checkBoxLegendShowBrush->setChecked(v.testFlag(QwtPlotCurve::LegendShowBrush));
}

/**
 * @brief 获取 Legend Attribute
 * @return
 */
QwtPlotCurve::LegendAttributes DAChartCurveItemSettingWidget::getLegendAttribute() const
{
	QwtPlotCurve::LegendAttributes v(QwtPlotCurve::LegendNoAttribute);
	v.setFlag(QwtPlotCurve::LegendShowLine, ui->checkBoxLegendShowLine->isChecked());
	v.setFlag(QwtPlotCurve::LegendShowSymbol, ui->checkBoxLegendShowSymbol->isChecked());
	v.setFlag(QwtPlotCurve::LegendShowBrush, ui->checkBoxLegendShowBrush->isChecked());
	return v;
}

/**
 * @brief 开启marker编辑
 * @param on
 */
void DAChartCurveItemSettingWidget::enableMarkerEdit(bool on)
{
    ui->checkBoxEnableMarker->setChecked(on);
}

/**
 * @brief 是否由marker
 * @return
 */
bool DAChartCurveItemSettingWidget::isEnableMarkerEdit() const
{
    return ui->checkBoxEnableMarker->isChecked();
}

/**
 * @brief 开启填充编辑
 * @param on
 */
void DAChartCurveItemSettingWidget::enableFillEdit(bool on)
{
    ui->checkBoxEnableFill->setChecked(on);
}

/**
 * @brief 是否开启填充编辑
 * @return
 */
bool DAChartCurveItemSettingWidget::isEnableFillEdit() const
{
    return ui->checkBoxEnableFill->isChecked();
}

/**
 * @brief 画笔
 * @param v
 */
void DAChartCurveItemSettingWidget::setCurvePen(const QPen& v)
{
    ui->penEditWidget->setCurrentPen(v);
}

/**
 * @brief 画笔
 * @return
 */
QPen DAChartCurveItemSettingWidget::getCurvePen() const
{
    return ui->penEditWidget->getCurrentPen();
}

/**
 * @brief 填充
 * @param v
 */
void DAChartCurveItemSettingWidget::setFillBrush(const QBrush& v)
{
    ui->brushEditWidget->setCurrentBrush(v);
}

/**
 * @brief 填充
 * @return
 */
QBrush DAChartCurveItemSettingWidget::getFillBrush() const
{
    return ui->brushEditWidget->getCurrentBrush();
}

/**
 * @brief 基线
 * @param v
 */
void DAChartCurveItemSettingWidget::setBaseLine(double v)
{
    ui->lineEditBaseLine->setText(QString::number(v));
}

/**
 * @brief 基线
 * @return
 */
double DAChartCurveItemSettingWidget::getBaseLine() const
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
bool DAChartCurveItemSettingWidget::isHaveBaseLine() const
{
    return !(ui->lineEditBaseLine->text().isEmpty());
}

void DAChartCurveItemSettingWidget::setOrientation(Qt::Orientation v)
{
    ui->radioButtonHorizontal->setChecked(v == Qt::Horizontal);
}

Qt::Orientation DAChartCurveItemSettingWidget::getOrientation() const
{
    return (ui->radioButtonHorizontal->isChecked() ? Qt::Horizontal : Qt::Vertical);
}

/**
 * @brief 重置ui
 */
void DAChartCurveItemSettingWidget::resetUI()
{
	resetCurveStyleComboBox();
	ui->checkBoxInverted->setEnabled(false);
	ui->symbolEditWidget->setSymbolStyle(QwtSymbol::Rect);
	ui->symbolEditWidget->setSymbolSize(4);
	ui->symbolEditWidget->setSymbolOutlinePen(Qt::NoPen);
	ui->symbolEditWidget->setSymbolColor(Qt::black);
	enableMarkerEdit(false);
	enableFillEdit(false);
	setBaseLine(0.0);
	setOrientation(Qt::Horizontal);
}

/**
 * @brief 获取item plot widget
 * @return
 */
DAChartPlotItemSettingWidget* DAChartCurveItemSettingWidget::getItemSettingWidget() const
{
    return ui->widgetItemSetting;
}

void DAChartCurveItemSettingWidget::on_checkBoxFitted_clicked(bool checked)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::on_checkBoxFitted_clicked=" << checked;
#endif
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	c->setCurveAttribute(QwtPlotCurve::Fitted, checked);
}

void DAChartCurveItemSettingWidget::on_checkBoxInverted_clicked(bool checked)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::on_checkBoxInverted_clicked=" << checked;
#endif
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	c->setCurveAttribute(QwtPlotCurve::Inverted, checked);
}

void DAChartCurveItemSettingWidget::on_checkBoxLegendShowLine_clicked(bool checked)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::on_checkBoxLegendShowLine_clicked=" << checked;
#endif
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	c->setLegendAttribute(QwtPlotCurve::LegendShowLine, checked);
}

void DAChartCurveItemSettingWidget::on_checkBoxLegendShowSymbol_clicked(bool checked)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::on_checkBoxLegendShowSymbol_clicked=" << checked;
#endif
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	c->setLegendAttribute(QwtPlotCurve::LegendShowSymbol, checked);
}

void DAChartCurveItemSettingWidget::on_checkBoxLegendShowBrush_clicked(bool checked)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::on_checkBoxLegendShowBrush_clicked=" << checked;
#endif
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	c->setLegendAttribute(QwtPlotCurve::LegendShowBrush, checked);
}

void DAChartCurveItemSettingWidget::on_checkBoxEnableMarker_clicked(bool checked)
{
#if DAChartCurveItemSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartCurveItemSettingWidget::on_checkBoxEnableMarker_clicked=" << checked;
#endif
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	if (checked) {
		QwtSymbol* symbol = ui->symbolEditWidget->createSymbol();
		c->setSymbol(symbol);
	} else {
		c->setSymbol(nullptr);
	}
}

void DAChartCurveItemSettingWidget::onSymbolStyleChanged(QwtSymbol::Style s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartCurveItemSettingWidget::onSymbolSizeChanged(int s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartCurveItemSettingWidget::onSymbolColorChanged(const QColor& s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartCurveItemSettingWidget::onSymbolOutlinePenChanged(const QPen& s)
{
	Q_UNUSED(s);
	on_checkBoxEnableMarker_clicked(ui->checkBoxEnableMarker->isChecked());
}

void DAChartCurveItemSettingWidget::onBrushChanged(const QBrush& b)
{
	Q_UNUSED(b);
	on_checkBoxEnableFill_clicked(ui->checkBoxEnableFill->isChecked());
}

void DAChartCurveItemSettingWidget::on_checkBoxEnableFill_clicked(bool checked)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	if (checked) {
		c->setBrush(getFillBrush());
	} else {
		c->setBrush(Qt::NoBrush);
	}
}

void DAChartCurveItemSettingWidget::on_lineEditBaseLine_editingFinished()
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	double bl       = getBaseLine();
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	if (!qFuzzyCompare(bl, c->baseline())) {
		c->setBaseline(bl);
	}
}

void DAChartCurveItemSettingWidget::onButtonGroupOrientationClicked(QAbstractButton* b)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	auto ori        = getOrientation();
	if (c->orientation() != ori) {
		c->setOrientation(ori);
	}
}

void DAChartCurveItemSettingWidget::onCurvePenChanged(const QPen& p)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	QwtPlotCurve* c = s_cast< QwtPlotCurve* >();
	c->setPen(p);
}

}
