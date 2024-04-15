#include "DAChartCurveItemSettingWidget.h"
#include "ui_DAChartCurveItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "qwt_text.h"
#include "qwt_plot.h"
namespace DA
{
DAChartCurveItemSettingWidget::DAChartCurveItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartCurveItemSettingWidget)
{
    ui->setupUi(this);
    resetCurveStyleComboBox();
    connect(ui->comboBoxCurveStyle,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartCurveItemSettingWidget::onCurveStyleCurrentIndexChanged);
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

DAChartCurveItemSettingWidget::~DAChartCurveItemSettingWidget()
{
    delete ui;
}

/**
 * @brief 根据QwtPlotCurve更新ui
 * @param item
 */
void DAChartCurveItemSettingWidget::updateUI(const QwtPlotCurve* item)
{
    setTitle(item->title().text());
    ui->checkBoxFitted->setChecked(item->testCurveAttribute(QwtPlotCurve::Fitted));
    ui->checkBoxInverted->setChecked(item->testCurveAttribute(QwtPlotCurve::Inverted));
    setLegendAttribute(item->legendAttributes());
    setCurvePen(item->pen());
    QBrush b = item->brush();
    if (b != Qt::NoBrush) {
        enableFillEdit(true);
        setFillBrush(item->brush());
    } else {
        enableFillEdit(false);
        setFillBrush(item->brush());
    }
    setBaseLine(item->baseline());
    setOrientation(item->orientation());
}

/**
 * @brief 根据ui更新QwtPlotCurve
 * @param item
 */
void DAChartCurveItemSettingWidget::updatePlotItem(QwtPlotCurve* item)
{
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

void DAChartCurveItemSettingWidget::setPlotItem(QwtPlotCurve* item)
{
    mItem = item;
    // 如果item有plot，则把plot设置进来，plot可以知道item是否被delete
    QwtPlot* oldPlot = mPlot.data();
    QwtPlot* newPlot = nullptr;
    if (item) {
        newPlot = item->plot();
    }
    if (oldPlot == newPlot) {
        return;
    }
    if (oldPlot) {
        disconnect(oldPlot, &QwtPlot::itemAttached, this, &DAChartPlotItemSettingWidget::onPlotItemAttached);
    }
    if (newPlot) {
        connect(mPlot.data(), &QwtPlot::itemAttached, this, &DAChartPlotItemSettingWidget::onPlotItemAttached);
    }
}

QwtPlotCurve* DAChartCurveItemSettingWidget::getPlotItem() const
{
    return mItem;
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

/**
 * @brief 曲线标题
 * @param t
 */
void DAChartCurveItemSettingWidget::setTitle(const QString& t)
{
    ui->lineEditTitle->setText(t);
}

/**
 * @brief 曲线标题
 * @return
 */
QString DAChartCurveItemSettingWidget::getTitle() const
{
    return ui->lineEditTitle->text();
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

}
