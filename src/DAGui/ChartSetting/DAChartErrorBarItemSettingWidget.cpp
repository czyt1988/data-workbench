#include "DAChartErrorBarItemSettingWidget.h"
#include "ui_DAChartErrorBarItemSettingWidget.h"
#include <QMessageBox>
#include <QDebug>
#include "DASignalBlockers.hpp"
#include "qwt_text.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_interval_symbol.h"
namespace DA
{
DAChartErrorBarItemSettingWidget::DAChartErrorBarItemSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartErrorBarItemSettingWidget)
{
    ui->setupUi(this);
    resetUI();
    connect(ui->groupBoxErrorBar, &QGroupBox::clicked, this, &DAChartErrorBarItemSettingWidget::onGroupBoxErrorBarEnable);

    connect(ui->groupBoxFill, &QGroupBox::clicked, this, &DAChartErrorBarItemSettingWidget::onGroupBoxFillEnable);
    connect(ui->groupBoxPen, &QGroupBox::clicked, this, &DAChartErrorBarItemSettingWidget::onGroupBoxPenEnable);

    connect(ui->brushEditWidget, &DABrushEditWidget::brushChanged, this, &DAChartErrorBarItemSettingWidget::onBrushChanged);
    connect(ui->buttonGroupOrientation,
            QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
            this,
            &DAChartErrorBarItemSettingWidget::onButtonGroupOrientationClicked);
    connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartErrorBarItemSettingWidget::onCurvePenChanged);
    connect(ui->penEditWidgetToErrorBar,
            &DAPenEditWidget::penChanged,
            this,
            &DAChartErrorBarItemSettingWidget::onPenEditWidgetToErrorBarChanged);
    connect(ui->brushEditWidgetToErrorBar,
            &DABrushEditWidget::brushChanged,
            this,
            &DAChartErrorBarItemSettingWidget::onBrushEditWidgetToErrorBarChanged);
    connect(ui->buttonGroupBarStyle,
            QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
            this,
            &DAChartErrorBarItemSettingWidget::onBarStyleButtonClicked);
    connect(ui->spinBoxErrorBarPenWidth,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAChartErrorBarItemSettingWidget::onErrorBarPenWidthChanged);
}

DAChartErrorBarItemSettingWidget::~DAChartErrorBarItemSettingWidget()
{
    delete ui;
}

/**
 * @brief 根据QwtPlotCurve更新ui
 * @param item
 */
void DAChartErrorBarItemSettingWidget::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotIntervalCurve) {
        return;
    }
    QwtPlotIntervalCurve* curItem = static_cast< QwtPlotIntervalCurve* >(item);
    DASignalBlockers blocker(ui->brushEditWidget, ui->penEditWidget);
    // 自动会调用widgetItemSetting的updateUI，这里必须调用setPlotItem，否则widgetItemSetting不会持有item的指针
    ui->widgetItemSetting->setPlotItem(item);
    setCurvePen(curItem->pen());
    QBrush b = curItem->brush();
    if (b != Qt::NoBrush) {
        enableFillEdit(true);
        setFillBrush(curItem->brush());
    } else {
        enableFillEdit(false);
        setFillBrush(curItem->brush());
    }
    setOrientation(curItem->orientation());
}

/**
 * @brief 根据ui更新QwtPlotCurve
 * @param item
 */
void DAChartErrorBarItemSettingWidget::applySetting(QwtPlotIntervalCurve* item)
{
    // errorbar item
    ui->widgetItemSetting->applySetting(item);
    // errorbar curve
    item->setTitle(getTitle());
    item->setStyle(QwtPlotIntervalCurve::CurveStyle::Tube);
    // pen
    item->setPen(getCurvePen());
    // symbol
    applySymbolSetting(item);
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

/**
 * @brief 从界面更新到QwtPlotIntervalCurve
 * @param item
 */
void DAChartErrorBarItemSettingWidget::applySymbolSetting(QwtPlotIntervalCurve* item)
{
    if (isEnableErrorBarEdit()) {
        QwtIntervalSymbol* symbol = createIntervalSymbolFromUI();
        item->setSymbol(symbol);
    } else {
        item->setSymbol(nullptr);
    }
}

void DAChartErrorBarItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
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
void DAChartErrorBarItemSettingWidget::enableErrorBarEdit(bool on)
{
    ui->groupBoxErrorBar->setChecked(on);
}

/**
 * @brief 是否由marker
 * @return
 */
bool DAChartErrorBarItemSettingWidget::isEnableErrorBarEdit() const
{
    return ui->groupBoxErrorBar->isChecked();
}

/**
 * @brief 开启填充编辑
 * @param on
 */
void DAChartErrorBarItemSettingWidget::enableFillEdit(bool on)
{
    ui->groupBoxFill->setChecked(on);
}

/**
 * @brief 是否开启填充编辑
 * @return
 */
bool DAChartErrorBarItemSettingWidget::isEnableFillEdit() const
{
    return ui->groupBoxFill->isChecked();
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
    enableErrorBarEdit(false);
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

/**
 * @brief 重ui设置创建QwtIntervalSymbol
 * @return
 */
QwtIntervalSymbol* DAChartErrorBarItemSettingWidget::createIntervalSymbolFromUI()
{
    std::unique_ptr< QwtIntervalSymbol > sym = std::make_unique< QwtIntervalSymbol >();
    if (ui->radioButtonBarStyle->isChecked()) {
        sym->setStyle(QwtIntervalSymbol::Bar);
        sym->setWidth(ui->spinBoxErrorBarPenWidth->value());
        sym->setPen(ui->penEditWidgetToErrorBar->getCurrentPen());
    } else if (ui->radioButtonBoxStyle->isChecked()) {
        sym->setStyle(QwtIntervalSymbol::Box);
        sym->setPen(ui->penEditWidgetToErrorBar->getCurrentPen());
        sym->setBrush(ui->brushEditWidgetToErrorBar->getCurrentBrush());
    }
    return sym.release();
}

/**
 * @brief 显示error bar
 * @param checked
 */
void DAChartErrorBarItemSettingWidget::onGroupBoxErrorBarEnable(bool checked)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    if (checked) {
        QwtIntervalSymbol* symbol = createIntervalSymbolFromUI();
        c->setSymbol(symbol);
    } else {
        c->setSymbol(nullptr);
    }
}

void DAChartErrorBarItemSettingWidget::onBrushChanged(const QBrush& b)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    c->setBrush(b);
}

void DAChartErrorBarItemSettingWidget::onGroupBoxFillEnable(bool checked)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    if (checked) {
        c->setBrush(getFillBrush());
    } else {
        c->setBrush(Qt::NoBrush);
    }
}

void DAChartErrorBarItemSettingWidget::onGroupBoxPenEnable(bool checked)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    if (checked) {
        c->setPen(getCurvePen());
    } else {
        c->setPen(QPen(Qt::NoPen));
    }
}

void DAChartErrorBarItemSettingWidget::onCurvePenChanged(const QPen& p)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    c->setPen(p);
}

void DAChartErrorBarItemSettingWidget::onErrorBarPenWidthChanged(int v)
{
    Q_UNUSED(v);
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    applySymbolSetting(c);
}

void DAChartErrorBarItemSettingWidget::onPenEditWidgetToErrorBarChanged(const QPen& p)
{
    Q_UNUSED(p);
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    applySymbolSetting(c);
}

void DAChartErrorBarItemSettingWidget::onBrushEditWidgetToErrorBarChanged(const QBrush& b)
{
    Q_UNUSED(b);
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    applySymbolSetting(c);
}

void DAChartErrorBarItemSettingWidget::onBarStyleButtonClicked(QAbstractButton* btn)
{
    Q_UNUSED(btn);
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* c = s_cast< QwtPlotIntervalCurve* >();
    applySymbolSetting(c);
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

}
