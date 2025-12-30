#include "DAChartSpectrogramItemSettingWidget.h"
#include "ui_DAChartSpectrogramItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "qwt_color_map.h"
#include "qwt_text.h"
#include "qwt_plot.h"
namespace DA
{
DAChartSpectrogramItemSettingWidget::DAChartSpectrogramItemSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartSpectrogramItemSettingWidget)
{
    ui->setupUi(this);
    resetUI();
    connect(ui->comboBoxDisplayMode,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartSpectrogramItemSettingWidget::onDisplayModeCurrentIndexChanged);
    connect(ui->FromColorButton,
            &DAColorPickerButton::colorChanged,
            this,
            &DAChartSpectrogramItemSettingWidget::onFromColorChanged);
    connect(ui->FromColorButton, &DAColorPickerButton::colorChanged, this, &DAChartSpectrogramItemSettingWidget::onToColorChanged);
    connect(ui->Penwidget, &DAPenEditWidget::penChanged, this, &DAChartSpectrogramItemSettingWidget::onCurvePenChanged);
}

DAChartSpectrogramItemSettingWidget::~DAChartSpectrogramItemSettingWidget()
{
    delete ui;
}

void DAChartSpectrogramItemSettingWidget::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotSpectrogram) {
        return;
    }
    // 自动会调用widgetItemSetting的updateUI，这里必须调用setPlotItem，否则widgetItemSetting不会持有item的指针
    ui->widgetItemSetting->setPlotItem(item);
    ui->widgetItemSetting->setPlotItem(item);
    QwtPlotSpectrogram* curItem = static_cast< QwtPlotSpectrogram* >(item);

    setCurvePen(curItem->defaultContourPen());

    const QwtColorMap* currentMap      = curItem->colorMap();
    const QwtLinearColorMap* linearMap = dynamic_cast< const QwtLinearColorMap* >(currentMap);
    if (linearMap) {
        QColor color1 = linearMap->color1();
        QColor color2 = linearMap->color2();
        setFromColor(color1);
        setToColor(color2);
    } else {
        setFromColor(Qt::blue);
        setToColor(Qt::red);
    }
}

/**
 * @brief 根据ui更新QwtPlotCurve
 * @param item
 */
void DAChartSpectrogramItemSettingWidget::applySetting(QwtPlotSpectrogram* item)
{
    // plot item
    ui->widgetItemSetting->applySetting(item);
    // plot displaymode
    item->setTitle(getTitle());
    auto s = getDisplayMode();
    item->setDisplayMode(s);
    // color
    item->setColorMap(new QwtLinearColorMap(getFromColor(), getToColor()));
    // Pen
    if (ui->comboBoxDisplayMode->currentText() == "ContourMode") {
        item->setDefaultContourPen(getCurvePen());
    }
    replot();
}

/**
 * @brief 重置CurveStyle ComboBox
 */
void DAChartSpectrogramItemSettingWidget::resetDisplayModeComboBox()
{
    ui->comboBoxDisplayMode->clear();
    ui->comboBoxDisplayMode->addItem(tr("Image Mode"), static_cast< int >(QwtPlotSpectrogram::ImageMode));
    ui->comboBoxDisplayMode->addItem(tr("Contour Mode"), static_cast< int >(QwtPlotSpectrogram::ContourMode));
    ui->comboBoxDisplayMode->setCurrentIndex(0);
}

/**
 * @brief DAChartSpectrogramItemSettingWidget::onCurveStyleCurrentIndexChanged
 * @param index
 */
void DAChartSpectrogramItemSettingWidget::onDisplayModeCurrentIndexChanged(int index)
{

    QwtPlotSpectrogram::DisplayMode s =
        static_cast< QwtPlotSpectrogram::DisplayMode >(ui->comboBoxDisplayMode->currentData().toInt());
    switch (s) {
    case QwtPlotSpectrogram::ImageMode:
        ui->Penwidget->setEnabled(false);
    case QwtPlotSpectrogram::ContourMode:
        ui->Penwidget->setEnabled(true);
        break;
    }
}

void DAChartSpectrogramItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
{
    if (!on && plotItem == getPlotItem()) {
        resetUI();
    }
    ui->widgetItemSetting->plotItemAttached(plotItem, on);
    DAAbstractChartItemSettingWidget::plotItemAttached(plotItem, on);
}

/**
 * @brief 曲线标题
 * @param t
 */
void DAChartSpectrogramItemSettingWidget::setTitle(const QString& t)
{
    ui->widgetItemSetting->setItemTitle(t);
}

/**
 * @brief 曲线标题
 * @return
 */
QString DAChartSpectrogramItemSettingWidget::getTitle() const
{
    return ui->widgetItemSetting->getItemTitle();
}

/**
 * @brief 设置DisplayMode
 * @param v
 */
void DAChartSpectrogramItemSettingWidget::setDisplayMode(QwtPlotSpectrogram::DisplayMode v)
{
    int i = ui->comboBoxDisplayMode->findData(static_cast< int >(v));
    if (i >= 0) {
        ui->comboBoxDisplayMode->setCurrentIndex(i);
    }
}

/**
 * @brief 获取DisplayMode
 * @return
 */
QwtPlotSpectrogram::DisplayMode DAChartSpectrogramItemSettingWidget::getDisplayMode() const
{
    return static_cast< QwtPlotSpectrogram::DisplayMode >(ui->comboBoxDisplayMode->currentData().toInt());
}

/**
 * @brief 画笔
 * @param v
 */
void DAChartSpectrogramItemSettingWidget::setCurvePen(const QPen& v)
{
    ui->Penwidget->setCurrentPen(v);
}

/**
 * @brief 画笔
 * @return
 */
QPen DAChartSpectrogramItemSettingWidget::getCurvePen() const
{
    return ui->Penwidget->getCurrentPen();
}

/**
 * @brief 设置颜色
 * @param v
 */
void DAChartSpectrogramItemSettingWidget::setFromColor(const QColor& v)
{
    ui->FromColorButton->setColor(v);
}

/**
 * @brief 获取颜色
 * @return
 */
QColor DAChartSpectrogramItemSettingWidget::getFromColor() const
{
    return ui->FromColorButton->color();
}

/**
 * @brief 设置颜色
 * @param v
 */
void DAChartSpectrogramItemSettingWidget::setToColor(const QColor& v)
{
    ui->ToColorButton->setColor(v);
}

/**
 * @brief 获取颜色
 * @return
 */
QColor DAChartSpectrogramItemSettingWidget::getToColor() const
{
    return ui->ToColorButton->color();
}

/**
 * @brief 重置ui
 */
void DAChartSpectrogramItemSettingWidget::resetUI()
{
    resetDisplayModeComboBox();
    setFromColor(Qt::blue);
    setToColor(Qt::red);
    setCurvePen(QPen(Qt::black, 1.0, Qt::SolidLine));
}

/**
 * @brief 获取item plot widget
 * @return
 */
DAChartPlotItemSettingWidget* DAChartSpectrogramItemSettingWidget::getItemSettingWidget() const
{
    return ui->widgetItemSetting;
}

void DAChartSpectrogramItemSettingWidget::onCurvePenChanged(const QPen& p)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotSpectrogram* c = s_cast< QwtPlotSpectrogram* >();
    c->setDefaultContourPen(p);
    replot();
}

void DAChartSpectrogramItemSettingWidget::onFromColorChanged(const QPen& p)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotSpectrogram* c = s_cast< QwtPlotSpectrogram* >();
    c->setColorMap(new QwtLinearColorMap(getFromColor(), getToColor()));
    replot();
}

void DAChartSpectrogramItemSettingWidget::onToColorChanged(const QPen& p)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotSpectrogram* c = s_cast< QwtPlotSpectrogram* >();
    c->setColorMap(new QwtLinearColorMap(getFromColor(), getToColor()));
    replot();
}
}
