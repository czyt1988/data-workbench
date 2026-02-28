#include "DAChartWidget.h"

#if 0
#include <algorithm>
#include <QDebug>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <qwt_grid_raster_data.h>

#include "qwt_interval.h"
#include "qwt_picker_machine.h"
#include "qwt_legend_label.h"
#include "qwt_date_scale_draw.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_column_symbol.h"
#include "DAChartPointMarker.h"
#include "DAChartCrossTracker.h"
#include "DAChartCanvas.h"

#include "DAChartUtil.h"
#include "DAFigureWidget.h"
// unsigned int ChartWave_qwt::staticValue_nAutoLineID = 0;//静态变量初始化
namespace DA
{
class DAChartWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartWidget)
public:
    // 确保data picker构建
    void ensureDataPicker();
    //
    bool isEnableDataPicker(QwtPlotSeriesDataPicker::PickSeriesMode mode) const;
    // 开启data picker模式
    void enabelDataPicker(bool on, QwtPlotSeriesDataPicker::PickSeriesMode mode);

public:
    DAChartWidget::FpCreatePanner mPannerFactory { nullptr };
    DAChartWidget::FpCreatePicker mPickerFactory { nullptr };
    DAChartWidget::FpCreateSeriesDataPicker mSeriesDataPickerFactory { nullptr };
    QwtPlotCanvasZoomer* mZoomer { nullptr };
    QwtPlotMagnifier* mMagnifier { nullptr };
    QwtPlotPicker* mPicker { nullptr };
    QwtPlotPanner* mPanner { nullptr };
    QwtLegend* mLegendPanel { nullptr };
    QwtPlotSeriesDataPicker* mDataPicker { nullptr };

    QColor mBorderColor;
    PrivateData(DAChartWidget* p) : q_ptr(p)
    {
    }
};

void DAChartWidget::PrivateData::ensureDataPicker()
{
    if (!mDataPicker) {
        q_ptr->setupSeriesDataPicker();
    }
}

bool DAChartWidget::PrivateData::isEnableDataPicker(QwtPlotSeriesDataPicker::PickSeriesMode mode) const
{
    if (mDataPicker) {
        bool on = (mDataPicker->pickMode() == mode);
        if (!on) {
            return false;
        }
        return mDataPicker->isEnabled();
    }
    return false;
}

void DAChartWidget::PrivateData::enabelDataPicker(bool on, QwtPlotSeriesDataPicker::PickSeriesMode mode)
{
    if (isEnableDataPicker(mode) == on) {
        return;  // 状态一致不动作
    }
    ensureDataPicker();
    if (on) {
        mDataPicker->setEnabled(true);
        mDataPicker->setPickMode(mode);
    } else {
        mDataPicker->setEnabled(false);
    }
}
//===============================================================
// DAChartWidget
//===============================================================

DAChartWidget::DAChartWidget(QWidget* parent) : QwtPlot(parent), DA_PIMPL_CONSTRUCT
{
    setAutoReplot(false);
    setAutoFillBackground(true);

    QwtPlotLayout* pLayout = plotLayout();

    pLayout->setCanvasMargin(0);

    DAChartCanvas* pCanvas = new DAChartCanvas();

    pCanvas->setCursor(Qt::ArrowCursor);
    setCanvas(pCanvas);
    pCanvas->setFocusPolicy(Qt::ClickFocus);
    // 设置点击Canvas，plot获得焦点
    pCanvas->setFocusProxy(this);
    // Qt::NoBrush无法透明，一直有一个灰色的背景
    setCanvasBackground(Qt::white);
    setChartBackgroundBrush(Qt::NoBrush);
    setChartBorderColor(QColor());
    setFocusPolicy(Qt::ClickFocus);
    setLineWidth(0);

    setAutoReplot(true);
    setAllAxisWidgetMargin(0);

    // 这个例子来着qwt-example-refreshtest
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        QwtScaleDraw* scaleDraw = axisScaleDraw(axisPos);
        if (scaleDraw) {
            scaleDraw->enableComponent(QwtAbstractScaleDraw::Backbone, false);
        }
    }
}

DAChartWidget::~DAChartWidget()
{
}

void DAChartWidget::resizeEvent(QResizeEvent* event)
{
    QwtPlot::resizeEvent(event);

    // Qt 4.7.1: QGradient::StretchToDeviceMode is buggy on X11
    // updateGradient();
}

/**
 * @brief 获取缩放器
 * @return 如果没有返回nullptr
 */
QwtPlotCanvasZoomer* DAChartWidget::getZoomer() const
{
    return (d_ptr->mZoomer);
}

void DAChartWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    if (d_ptr->mBorderColor.isValid()) {
        QPen pen(d_ptr->mBorderColor);
        int lw = lineWidth();
        pen.setWidth(lw);
        painter.setPen(pen);
        painter.drawRect(rect());
    }
    QwtPlot::paintEvent(e);
}

/**
 * @brief 在当前plot中查找第一个QwtPlotGrid对象。
 * @return 如果找到，返回指向该对象的指针；否则返回nullptr。
 */
QwtPlotGrid* DAChartWidget::getGrid() const
{
    const QList< QwtPlotItem* > items = itemList(QwtPlotItem::Rtti_PlotGrid);
    if (!items.isEmpty()) {
        return static_cast< QwtPlotGrid* >(items.first());
    }
    return nullptr;
}

/**
 * @brief 设置gird的线条
 * @param color 颜色
 * @param width 宽度
 * @param style 样式
 * @param isMajor true代表设置主刻度，false代表设置次刻度
 */
void DAChartWidget::setGridLine(const QColor& color, qreal width, Qt::PenStyle style, bool isMajor)
{
    QwtPlotGrid* grid = getOrCreateGrid();
    if (isMajor) {
        grid->setMajorPen(color, width, style);
    } else {
        grid->setMinorPen(color, width, style);
    }
}

/**
 * @brief 检查网格是否可用。
 * @return 如果网格存在且可见，返回true；否则返回false。
 */
bool DAChartWidget::isGridEnable() const
{
    QwtPlotGrid* grid = getGrid();
    return grid && grid->isVisible();
}

/**
 * @brief 检查X轴主网格线是否启用。
 * @return 如果网格存在且X轴主网格线启用，返回true；否则返回false。
 * @note 此状态独立于网格的整体可见性（isGridVisible）。
 */
bool DAChartWidget::isGridXEnable() const
{
    QwtPlotGrid* grid = getGrid();
    return grid && grid->xEnabled();
}

/**
 * @brief 检查Y轴主网格线是否启用。
 * @return 如果网格存在且Y轴主网格线启用，返回true；否则返回false。
 * @note 此状态独立于网格的整体可见性（isGridVisible）。
 */
bool DAChartWidget::isGridYEnable() const
{
    QwtPlotGrid* grid = getGrid();
    return grid && grid->yEnabled();
}

/**
 * @brief 检查X轴次要网格线是否启用。
 * @return 如果网格存在且X轴次要网格线启用，返回true；否则返回false。
 * @note 此状态独立于网格的整体可见性（isGridVisible）。
 */
bool DAChartWidget::isGridXMinEnable() const
{
    QwtPlotGrid* grid = getGrid();
    return grid && grid->xMinEnabled();
}

/**
 * @brief 检查Y轴次要网格线是否启用。
 * @return 如果网格存在且Y轴次要网格线启用，返回true；否则返回false。
 * @note 此状态独立于网格的整体可见性（isGridVisible）。
 */
bool DAChartWidget::isGridYMinEnable() const
{
    QwtPlotGrid* grid = getGrid();
    return grid && grid->yMinEnabled();
}

/**
 * @brief 设置网格的整体可见性。
 *
 *  如果网格不存在，此函数会自动创建一个默认样式的网格，然后根据参数设置其可见性。
 *  @param enabled true表示显示网格，false表示隐藏网格。
 */
void DAChartWidget::setGridEnable(bool enabled)
{
    QwtPlotGrid* g = getOrCreateGrid();
    if (g->isVisible() != enabled) {
        g->setVisible(enabled);
        replot();
        emit gridSettingsChanged(g);
    }
}

/**
 * @brief 启用或禁用X轴主网格线。
 *
 * 如果网格不存在，此函数会自动创建一个默认样式的网格。
 * @param enabled true表示启用X轴主网格线，false表示禁用。
 */
void DAChartWidget::setGridXEnable(bool enabled)
{
    QwtPlotGrid* g = getOrCreateGrid();
    if (g->xEnabled() != enabled) {
        g->enableX(enabled);
        replot();
        emit gridSettingsChanged(g);
    }
}

/**
 * @brief 启用或禁用Y轴主网格线。
 *
 * 如果网格不存在，此函数会自动创建一个默认样式的网格。
 * @param enabled true表示启用Y轴主网格线，false表示禁用。
 */
void DAChartWidget::setGridYEnable(bool enabled)
{
    QwtPlotGrid* g = getOrCreateGrid();
    if (g->yEnabled() != enabled) {
        g->enableY(enabled);
        replot();
        emit gridSettingsChanged(g);
    }
}

/**
 * @brief 启用或禁用X轴次要网格线。
 *
 * 如果网格不存在，此函数会自动创建一个默认样式的网格。
 * @param enabled true表示启用X轴次要网格线，false表示禁用。
 */
void DAChartWidget::setGridXMinEnable(bool enabled)
{
    QwtPlotGrid* g = getOrCreateGrid();
    if (g->xMinEnabled() != enabled) {
        g->enableXMin(enabled);
        replot();
        emit gridSettingsChanged(g);
    }
}

/**
 * @brief 启用或禁用Y轴次要网格线。
 *
 * 如果网格不存在，此函数会自动创建一个默认样式的网格。
 * @param enabled true表示启用Y轴次要网格线，false表示禁用。
 */
void DAChartWidget::setGridYMinEnable(bool enabled)
{
    QwtPlotGrid* g = getOrCreateGrid();
    if (g->yMinEnabled() != enabled) {
        g->enableYMin(enabled);
        replot();
        emit gridSettingsChanged(g);
    }
}

/**
 * @brief 是否允许拖动
 * @return
 */
bool DAChartWidget::isPannerEnable() const
{
    DA_DC(d);
    if (d->mPanner) {
        return (d->mPanner->isEnabled());
    }
    return (false);
}

/**
 * @brief 获取panner
 * @return
 */
QwtPlotPanner* DAChartWidget::getPanner() const
{
    return d_ptr->mPanner;
}

bool DAChartWidget::isEnableLegendPanel() const
{
    if (d_ptr->mLegendPanel) {
        return (d_ptr->mLegendPanel->isVisible());
    }
    return (false);
}

/**
 * @brief 是否开启了Y值捕获
 * @return
 */
bool DAChartWidget::isYDataPickerEnable() const
{
    DA_DC(d);
    return d->isEnableDataPicker(QwtPlotSeriesDataPicker::PickYValue);
}

/**
 * @brief 是否开启了XY值捕获
 * @return
 */
bool DAChartWidget::isXYDataPickerEnable() const
{
    DA_DC(d);
    return d->isEnableDataPicker(QwtPlotSeriesDataPicker::PickNearestPoint);
}

/**
 * @brief 是否运行滚轮缩放
 * @return
 */
bool DAChartWidget::isMagnifierEnable() const
{
    DA_DC(d);
    if (d->mMagnifier) {
        return d->mMagnifier->isEnabled();
    }
    return false;
}

/**
 * @brief 把min,max,value的数组转换为QwtIntervalSample数组
 * @param min
 * @param max
 * @param value
 * @param invDatas
 */
void DAChartWidget::makeIntervalSample(const QVector< double >& value,
                                       const QVector< double >& min,
                                       const QVector< double >& max,
                                       QVector< QwtIntervalSample >& invDatas)
{
    int len = std::min({ min.size(), max.size(), value.size() });
    invDatas.clear();
    invDatas.reserve(len);
    for (int i = 0; i < len; ++i) {
        invDatas.push_back(QwtIntervalSample(value[ i ], min[ i ], max[ i ]));
    }
}

/**
 * @brief 注册panner工厂
 * @param fp 可传入nullptr，代表使用默认的panner
 */
void DAChartWidget::registerPannerFactory(const FpCreatePanner& fp)
{
    d_ptr->mPannerFactory = fp;
}

/**
 * @brief 注册picker工厂
 * @param fp
 */
void DAChartWidget::registerPickerFactory(const FpCreatePicker& fp)
{
    d_ptr->mPickerFactory = fp;
}

void DAChartWidget::registerSeriesDataPickerFactory(const DAChartWidget::FpCreateSeriesDataPicker& fp)
{
    d_ptr->mSeriesDataPickerFactory = fp;
}

/**
 * @brief 获取图例
 * @return
 */
QwtPlotLegendItem* DAChartWidget::getLegend() const
{
    const QList< QwtPlotItem* > items = itemList(QwtPlotItem::Rtti_PlotLegend);
    if (!items.isEmpty()) {
        return static_cast< QwtPlotLegendItem* >(items.first());
    }
    return nullptr;
}

/**
 * @brief 显示图例
 * @param enable
 */
void DAChartWidget::setLegendEnable(bool enable)
{
    QwtPlotLegendItem* legend = getOrCreateLegend();
    if (legend->isVisible() != enable) {
        legend->setVisible(enable);
        replot();
        emit legendSettingChanged(legend);
    }
}

/**
 * @brief 图例是否显示
 * @return
 */
bool DAChartWidget::isLegendEnable() const
{
    QwtPlotLegendItem* legend = getLegend();
    return legend && legend->isVisible();
}

/**
 * @brief 删除网格
 */
void DAChartWidget::deleteGrid()
{
    QwtPlotGrid* grid = getGrid();
    if (grid) {
        grid->detach();
        delete (grid);
    }
    replot();  // 刷新，否则不显示
}

double DAChartWidget::axisXmin(int axisId) const
{
    QwtInterval inl = axisInterval(axisId);

    if (inl.isValid()) {
        return (inl.minValue());
    }
    axisId = ((axisId == QwtPlot::xBottom) ? QwtPlot::xTop : QwtPlot::xBottom);
    inl    = axisInterval(axisId);
    if (inl.isValid()) {
        return (inl.minValue());
    }
    return (double());
}

double DAChartWidget::axisXmax(int axisId) const
{
    QwtInterval inl = axisInterval(axisId);

    if (inl.isValid()) {
        return (inl.maxValue());
    }
    axisId = ((axisId == QwtPlot::xBottom) ? QwtPlot::xTop : QwtPlot::xBottom);
    inl    = axisInterval(axisId);
    if (inl.isValid()) {
        return (inl.maxValue());
    }
    return (double());
}

double DAChartWidget::axisYmin(int axisId) const
{
    QwtInterval inl = axisInterval(axisId);

    if (inl.isValid()) {
        return (inl.minValue());
    }
    axisId = ((axisId == QwtPlot::yLeft) ? QwtPlot::yRight : QwtPlot::yLeft);
    inl    = axisInterval(axisId);
    if (inl.isValid()) {
        return (inl.minValue());
    }
    return (double());
}

double DAChartWidget::axisYmax(int axisId) const
{
    QwtInterval inl = axisInterval(axisId);

    if (inl.isValid()) {
        return (inl.maxValue());
    }
    axisId = ((axisId == QwtPlot::yLeft) ? QwtPlot::yRight : QwtPlot::yLeft);
    inl    = axisInterval(axisId);
    if (inl.isValid()) {
        return (inl.maxValue());
    }
    return (double());
}

///
/// \brief 此功能用于禁止所有活动的editor，如Zoomer，Picker，Panner，DataPicker等
///
void DAChartWidget::setEnableAllEditor(bool enable)
{
    if (isZoomerEnabled() != enable) {
        setZoomerEnable(enable);
    }
    if (isCrossPickerEnable() != enable) {
        setCrossPickerEnable(enable);
    }
    if (isPannerEnable() != enable) {
        setPanEnable(enable);
    }
    if (isYDataPickerEnable() != enable) {
        setYDataPickerEnable(enable);
    }
    if (isXYDataPickerEnable() != enable) {
        setXYDataPickerEnable(enable);
    }
}

/**
 * @brief 获取背景
 * @return
 */
QBrush DAChartWidget::getChartBackBrush() const
{
    return palette().brush(QPalette::Window);
}

/**
 * @brief 获取边框颜色
 * @return
 */
QColor DAChartWidget::getChartBorderColor() const
{
    return d_ptr->mBorderColor;
}

/**
 * @brief 添加一条竖直线
 * @param val
 * @param representedOnLegend 是否在legend上显示这个竖直线
 * @return
 */
QwtPlotMarker* DAChartWidget::addVLine(double val, bool representedOnLegend)
{
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setXValue(val);
    marker->setLineStyle(QwtPlotMarker::VLine);
    marker->setItemAttribute(QwtPlotItem::Legend, representedOnLegend);
    marker->attach(this);
    return (marker);
}

/**
 * @brief 添加一条水平线
 * @param val
 * @param representedOnLegend 是否在legend上显示这个竖直线
 * @return
 */
QwtPlotMarker* DAChartWidget::addHLine(double val, bool representedOnLegend)
{
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setYValue(val);
    marker->setLineStyle(QwtPlotMarker::HLine);
    marker->setItemAttribute(QwtPlotItem::Legend, representedOnLegend);
    marker->attach(this);
    return (marker);
}

/**
 * @brief 添加一个十字标记
 * @param x
 * @param y
 * @param representedOnLegend 是否在legend上显示这个标记
 * @return
 */
QwtPlotMarker* DAChartWidget::addCrossLine(double x, double y, bool representedOnLegend)
{
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setXValue(x);
    marker->setYValue(y);
    marker->setLineStyle(QwtPlotMarker::Cross);
    marker->setItemAttribute(QwtPlotItem::Legend, representedOnLegend);
    marker->attach(this);
    return (marker);
}

/**
 * @brief 添加曲线
 * @param xData
 * @param yData
 * @param size
 * @return
 */
QwtPlotCurve* DAChartWidget::addCurve(const double* xData, const double* yData, int size)
{
    if (size <= 0) {
        return (nullptr);
    }
    QwtPlotCurve* ser = new QwtPlotCurve();
    ser->setYAxis(yLeft);
    ser->setXAxis(xBottom);
    ser->setStyle(QwtPlotCurve::Lines);
    ser->setSamples(xData, yData, size);
    ser->attach(this);
    return (ser);
}

/**
 * @brief 添加曲线
 * @param xyDatas
 * @return
 */
QwtPlotCurve* DAChartWidget::addCurve(const QVector< QPointF >& xyDatas)
{
    QwtPlotCurve* series = new QwtPlotCurve();
    series->setYAxis(yLeft);
    series->setXAxis(xBottom);
    series->setStyle(QwtPlotCurve::Lines);
    series->setSamples(xyDatas);
    series->attach(this);
    return (series);
}

/**
 * @brief 添加曲线
 * @param xData
 * @param yData
 * @return
 */
QwtPlotCurve* DAChartWidget::addCurve(const QVector< double >& xData, const QVector< double >& yData)
{
    QwtPlotCurve* series = new QwtPlotCurve();
    series->setYAxis(yLeft);
    series->setXAxis(xBottom);
    series->setStyle(QwtPlotCurve::Lines);
    series->setSamples(xData, yData);
    series->attach(this);
    return (series);
}

/**
 * @brief 绘制散点图(dot)
 * @param xData
 * @param yData
 * @param size
 * @return
 */
QwtPlotCurve* DAChartWidget::addScatter(const double* xData, const double* yData, int size)
{
    QwtPlotCurve* p = addCurve(xData, yData, size);
    if (p) {
        p->setStyle(QwtPlotCurve::Dots);
    }
    return (p);
}

/**
 * @brief 绘制散点图(dot)
 * @param xyDatas
 * @return
 */
QwtPlotCurve* DAChartWidget::addScatter(const QVector< QPointF >& xyDatas)
{
    QwtPlotCurve* p = addCurve(xyDatas);
    if (p) {
        p->setStyle(QwtPlotCurve::Dots);
    }
    return (p);
}

/**
 * @brief 绘制散点图(dot)
 * @param xData
 * @param yData
 * @return
 */
QwtPlotCurve* DAChartWidget::addScatter(const QVector< double >& xData, const QVector< double >& yData)
{
    QwtPlotCurve* p = addCurve(xData, yData);
    if (p) {
        p->setStyle(QwtPlotCurve::Dots);
    }
    return (p);
}

/**
 * @brief 绘制误差图
 * @param invDatas
 * @return
 */
QwtPlotIntervalCurve* DAChartWidget::addIntervalCurve(const QVector< QwtIntervalSample >& invDatas)
{
    QwtPlotIntervalCurve* series = new QwtPlotIntervalCurve();
    series->setSamples(invDatas);
    series->attach(this);
    return (series);
}

/**
 * @brief 绘制误差图
 * @param min
 * @param max
 * @param value
 * @return
 */
QwtPlotIntervalCurve* DAChartWidget::addIntervalCurve(const QVector< double >& value,
                                                      const QVector< double >& min,
                                                      const QVector< double >& max)
{
    QVector< QwtIntervalSample > sample;
    makeIntervalSample(value, min, max, sample);
    return addIntervalCurve(sample);
}

/**
 * @brief 添加一个bar
 * @param xyDatas
 * @return
 */
QwtPlotBarChart* DAChartWidget::addBar(const QVector< QPointF >& xyDatas)
{
    QwtPlotBarChart* bar = new QwtPlotBarChart();
    bar->setSamples(xyDatas);
    //! LegendChartTitle:
    //! 图例中只显示​​整个图表​​的标题（通过title()设置）和一个默认颜色块
    //! 适用于所有柱状体使用同一种颜色或不需要区分单个柱子的场景
    //!
    //! LegendBarTitles:
    //! 为​​每个柱子​​生成图例项，显示specialSymbol()返回的符号和barTitle()设置的标题
    //!
    // ser->setLegendMode(QwtPlotBarChart::LegendBarTitles);
    // ser->setLegendIconSize(QSize(10, 14));
    // ser->setLayoutPolicy(QwtPlotBarChart::AutoAdjustSamples);
    // ser->setLayoutHint(4.0);  // minimum width for a single bar
    // ser->setSpacing(10);      // spacing between bars
    QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    bar->setSymbol(symbol);
    bar->attach(this);
    return bar;
}

/**
 * @brief 添加一个bar
 *
 * x值为0~n均匀分布
 * @param yDatas
 * @return
 */
QwtPlotBarChart* DAChartWidget::addBar(const QVector< double >& yDatas)
{
    QwtPlotBarChart* bar = new QwtPlotBarChart();
    bar->setSamples(yDatas);
    QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    bar->setSymbol(symbol);
    bar->attach(this);
    return bar;
}

QwtPlotSpectrogram* DAChartWidget::addSpectroGram(QwtGridRasterData* gridData)
{
    QwtPlotSpectrogram* spectrogram = new QwtPlotSpectrogram();
    spectrogram->setData(gridData);
    return spectrogram;
}

/**
 * @brief 设置所有坐标轴的Margin
 */
void DAChartWidget::setAllAxisWidgetMargin(int m)
{
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        QwtScaleWidget* scaleWidget = axisWidget(axisPos);
        if (scaleWidget) {
            scaleWidget->setMargin(m);
        }
    }
}

/**
 * @brief 获取figure，有可能返回nullptr
 * @return
 */
DAFigureWidget* DAChartWidget::getFigure() const
{
    // 一直遍历parent，获取fig
    QWidget* par = const_cast< DAChartWidget* >(this);
    while (QWidget* pp = par->parentWidget()) {
        DAFigureWidget* f = qobject_cast< DAFigureWidget* >(pp);
        if (f) {
            return f;
        }
        par = pp;
    }
    return nullptr;
}

/**
 * @brief title的另外一种方式
 * @return
 */
QString DAChartWidget::getChartTitle() const
{
    return title().text();
}

/**
 * @brief 设置边框
 * @param c
 */
void DAChartWidget::setChartBorderColor(const QColor& c)
{
    d_ptr->mBorderColor = c;
    repaint();
}
/**
 * @brief 设置背景
 * @param b
 */
void DAChartWidget::setChartBackgroundBrush(const QBrush& b)
{
    QPalette pl = palette();
    pl.setBrush(QPalette::Window, b);
    setPalette(pl);
    repaint();
}

/**
 * @brief  建立一个内置的picker(十字)
 */
void DAChartWidget::setupCrossPicker()
{
    DA_D(d);
    if (d->mPicker) {
        d->mPicker->setEnabled(false);
        d->mPicker->deleteLater();
        d->mPicker = nullptr;
    }
    if (d->mPickerFactory) {
        d->mPicker = d->mPickerFactory(this->canvas());
    } else {
        d->mPicker = new DAChartCrossTracker(this->canvas());
    }
}

void DAChartWidget::setCrossPickerEnable(bool enable)
{
    if (isCrossPickerEnable() == enable) {
        return;
    }
    if (!d_ptr->mPicker) {
        if (!enable) {
            return;
        }
        setupCrossPicker();
    }
    d_ptr->mPicker->setEnabled(enable);
    emit enableCrossPickerChanged(enable);
}

/**
 * @brief 允许拖放
 * @note 此操作会和zoomer互斥，如果enablePan(true),会调用enableZoomer(false)
 * @param enable
 */
void DAChartWidget::setPanEnable(bool enable)
{
    if (isPannerEnable() == enable) {
        return;  // 状态一致跳出
    }
    if (!d_ptr->mPanner) {
        if (!enable) {
            return;
        }
        setupPanner();
    }
    d_ptr->mPanner->setEnabled(enable);
    if (enable) {
        // pan和zoom互斥
        if (isZoomerEnabled()) {
            setZoomerEnable(false);
        }
    }
    emit enablePannerChanged(enable);
}

/**
 * @brief 建立一个内置的Panner(拖动)，默认使用鼠标中键
 */
void DAChartWidget::setupPanner()
{
    DA_D(d);
    // 设置拖动
    if (d->mPanner) {
        d->mPanner->setEnabled(false);
        d->mPanner->deleteLater();
        d->mPanner = nullptr;
    }
    if (d->mPannerFactory) {
        d->mPanner = d->mPannerFactory(canvas());
    } else {
        d->mPanner = new QwtPlotPanner(canvas());
    }
    d->mPanner->setMouseButton(Qt::MiddleButton);
}

void DAChartWidget::setupMagnifier()
{
    DA_D(d);
    if (d->mMagnifier) {
        d->mMagnifier->setEnabled(false);
        d->mMagnifier->deleteLater();
        d->mMagnifier = nullptr;
    }
    d->mMagnifier = new QwtPlotMagnifier(canvas());
}

/**
 * @brief 允许缩放
 * @param enable
 * @note 此函数会构建缩放器，如果要设置自定义的缩放器，可以使用@sa setupZoomer 函数
 */
void DAChartWidget::setZoomerEnable(bool enable)
{
    if (isZoomerEnabled() == enable) {
        return;  // 状态一致不动作
    }
    DA_D(d);
    if (!d->mZoomer) {
        setupZoomer();
    }
    d->mZoomer->setEnabled(enable);
    emit enableZoomerChanged(enable);
}

void DAChartWidget::setZoomerEnable(QwtPlotCanvasZoomer* zoomer, bool enable)
{
    if (nullptr == zoomer) {
        return;
    }
    if (enable) {
        zoomer->setEnabled(true);
        zoomer->setZoomBase(true);
        zoomer->setRubberBand(QwtPicker::RectRubberBand);
        zoomer->setTrackerMode((isCrossPickerEnable() ? QwtPicker::AlwaysOff : QwtPicker::ActiveOnly));
    } else {
        zoomer->setEnabled(false);
        zoomer->setRubberBand(QwtPicker::NoRubberBand);
        zoomer->setTrackerMode(QwtPicker::AlwaysOff);
    }
    if (isCrossPickerEnable()) {
        zoomer->setTrackerMode((enable ? QwtPicker::AlwaysOff : QwtPicker::ActiveOnly));
    }
}

/**
 * @brief 构建默认的缩放器
 */
void DAChartWidget::setupZoomer()
{
    DA_D(d);
    if (d->mZoomer) {
        d->mZoomer->setEnabled(false);
        d->mZoomer->deleteLater();
        d->mZoomer = nullptr;
    }
    d->mZoomer = new QwtPlotCanvasZoomer(canvas());
    d->mZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    d->mZoomer->setMaxStackDepth(30);

    QwtPlotMagnifier* magnifier = new QwtPlotMagnifier(canvas());
    magnifier->setMouseButton(Qt::NoButton);
}

/**
 * @brief DAChartWidget::setZoomReset
 */
void DAChartWidget::setZoomBase()
{
    DA_D(d);
    if (!d->mZoomer) {
        return;
    }
    d->mZoomer->setZoomBase();
}

/**
 * @brief 放大
 */
void DAChartWidget::zoomIn()
{
    DA_D(d);
    if (!d->mMagnifier) {
        setupMagnifier();
    }
    if (!d->mMagnifier->isEnabled()) {
        d->mMagnifier->setEnabled(true);
    }
    d->mMagnifier->rescale(1.2);
}

/**
 * @brief 缩小
 */
void DAChartWidget::zoomOut()
{
    DA_D(d);
    if (!d->mMagnifier) {
        setupMagnifier();
    }
    if (!d->mMagnifier->isEnabled()) {
        d->mMagnifier->setEnabled(true);
    }
    d->mMagnifier->rescale(0.8);
}

void DAChartWidget::setLegendPanelEnable(bool enable)
{
    if (isEnableLegendPanel() == enable) {
        return;  // 状态一致不动作
    }
    if (!d_ptr->mLegendPanel) {
        setuplegendPanel();
    }
    d_ptr->mLegendPanel->setVisible(enable);
    emit enableLegendPanelChanged(enable);
}

void DAChartWidget::setuplegendPanel()
{
    if (d_ptr->mLegendPanel) {
        return;
    }
    d_ptr->mLegendPanel = new QwtLegend;
    d_ptr->mLegendPanel->setDefaultItemMode(QwtLegendData::Checkable);
    insertLegend(d_ptr->mLegendPanel, QwtPlot::RightLegend);
    //	connect( _legendPanel, &QwtLegend::checked,&ChartWave_qwt::showItem);
    connect(d_ptr->mLegendPanel, SIGNAL(checked(const QVariant&, bool, int)), SLOT(showItem(const QVariant&, bool)));

    QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);

    for (int i = 0; i < items.size(); i++) {
        const QVariant itemInfo     = itemToInfo(items[ i ]);
        QwtLegendLabel* legendLabel = qobject_cast< QwtLegendLabel* >(d_ptr->mLegendPanel->legendWidget(itemInfo));
        if (legendLabel) {
            legendLabel->setChecked(items[ i ]->isVisible());
        }
    }
}

void DAChartWidget::setupSeriesDataPicker()
{
    DA_D(d);
    if (d->mDataPicker) {
        d->mDataPicker->setEnabled(false);
        d->mDataPicker->deleteLater();
        d->mDataPicker = nullptr;
    }
    if (d->mSeriesDataPickerFactory) {
        d->mDataPicker = d->mSeriesDataPickerFactory(canvas());
    } else {
        d->mDataPicker = new QwtPlotSeriesDataPicker(canvas());
    }
}

QwtPlotSeriesDataPicker* DAChartWidget::getOrCreateSeriesDataPicker()
{
    DA_D(d);
    if (!d->mDataPicker) {
        setupSeriesDataPicker();
    }
    return d->mDataPicker;
}

/**
 * @brief 开启y值捕获
 * @param enable
 */
void DAChartWidget::setYDataPickerEnable(bool enable)
{
    DA_D(d);
    d->enabelDataPicker(enable, QwtPlotSeriesDataPicker::PickYValue);
    Q_EMIT enableYDataPickerChanged(enable);
}

/**
 * @brief 开启xy值捕获
 * @param enable
 */
void DAChartWidget::setXYDataPickerEnable(bool enable)
{
    DA_D(d);
    d->enabelDataPicker(enable, QwtPlotSeriesDataPicker::PickNearestPoint);
    Q_EMIT enableXYDataPickerChanged(enable);
}

/**
 * @brief plt.xlabel 设置xbottom轴标题
 * @param label
 */
void DAChartWidget::setXLabelText(const QString& label)
{
    setAxisTitle(QwtPlot::xBottom, label);
}

/**
 * @brief plt.ylabel 设置yLeft轴标题
 * @param label
 */
void DAChartWidget::setYLabelText(const QString& label)
{
    setAxisTitle(QwtPlot::yLeft, label);
}

/**
 * @brief 此函数激活chartPropertyHasChanged信号
 */
void DAChartWidget::notifyChartPropertyHasChanged()
{
    emit chartPropertyHasChanged(this);
}

///
/// \brief 是否允许缩放
/// \return
///
bool DAChartWidget::isZoomerEnabled() const
{
    if (d_ptr->mZoomer) {
        return (d_ptr->mZoomer->isEnabled());
    }
    return (false);
}

///
/// \brief 是否允许十字光标
/// \return
///
bool DAChartWidget::isCrossPickerEnable() const
{
    if (d_ptr->mPicker) {
        return (d_ptr->mPicker->isEnabled());
    }
    return (false);
}

void DAChartWidget::showItem(const QVariant& itemInfo, bool on)
{
    QwtPlotItem* plotItem = infoToItem(itemInfo);

    if (plotItem) {
        plotItem->setVisible(on);
    }
}

/**
 * @brief getCureList 获取所有曲线
 * @return
 */
QList< QwtPlotCurve* > DAChartWidget::getCurveList()
{
    QList< QwtPlotCurve* > curves;
    QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);

    for (int i(0); i < items.size(); ++i) {
        curves.append(static_cast< QwtPlotCurve* >(items[ i ]));
    }
    return (curves);
}

/**
 * @brief getMakerList 获取所有标记
 * @return
 */
QList< QwtPlotMarker* > DAChartWidget::getMakerList()
{
    QList< QwtPlotMarker* > list;
    QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotMarker);

    for (int i(0); i < items.size(); ++i) {
        list.append(static_cast< QwtPlotMarker* >(items[ i ]));
    }
    return (list);
}

QwtDateScaleDraw* DAChartWidget::setAxisDateTimeScale(const QString& format, int axisID, QwtDate::IntervalType intType)
{
    QwtDateScaleDraw* dateScale = setAxisDateTimeScale(axisID);

    dateScale->setDateFormat(intType, format);
    return (dateScale);
}

QwtDateScaleDraw* DAChartWidget::setAxisDateTimeScale(int axisID)
{
    QwtDateScaleDraw* dateScale;

    dateScale = new QwtDateScaleDraw;  // 原来的scaleDraw会再qwt自动delete
    setAxisScaleDraw(axisID, dateScale);
    return (dateScale);
}

/**
 * @brief 一个辅助函数，用于安全地获取现有网格或创建一个新的网格。
 * @return 指向QwtPlotGrid对象的指针，该对象已被附着到当前plot。
 * @note 此函数不会返回nullptr
 */
QwtPlotGrid* DAChartWidget::getOrCreateGrid()
{
    QwtPlotGrid* grid = getGrid();
    if (!grid) {
        grid = new QwtPlotGrid();
        grid->setMajorPen(Qt::gray, 1, Qt::DotLine);  // 大刻度的样式
        grid->setMinorPen(Qt::gray, 0, Qt::DotLine);  // 小刻度的样式
        grid->setVisible(false);                      // 默认为false
        grid->attach(this);
    }
    return grid;
}

/**
 * @brief 一个辅助函数，用于安全地获取现有legend或创建一个新的legend。
 * @return 指向QwtPlotLegendItem对象的指针，该对象已被附着到当前plot。
 * @note 此函数不会返回nullptr
 */
QwtPlotLegendItem* DAChartWidget::getOrCreateLegend()
{
    QwtPlotLegendItem* legend = getLegend();
    if (!legend) {
        legend = new QwtPlotLegendItem();
        legend->setRenderHint(QwtPlotItem::RenderAntialiased);
        QColor color(Qt::white);
        legend->setTextPen(color);
        legend->setBorderPen(color);
        QColor c(Qt::gray);
        c.setAlpha(200);
        legend->setBackgroundBrush(c);
        legend->setVisible(false);  // 默认为false
        legend->attach(this);
    }
    return legend;
}

}  // End Of Namespace DA

#else

// DAChartWidget.cpp
#include "DAChartWidget.h"
#include "DAChartUtil.h"
#include "DAFigureWidget.h"
#include "DAChartCanvas.h"
#include "DAChartCrossTracker.h"
// QWT 相关头文件
#include "qwt_plot_canvas.h"
#include "qwt_plot_layout.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_legenditem.h"
#include "qwt_plot_panner.h"
#include "qwt_plot_magnifier.h"
#include "qwt_plot_canvas_zoomer.h"
#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_picker.h"
#include "qwt_legend.h"
#include "qwt_date_scale_draw.h"
#include "qwt_column_symbol.h"
#include "qwt_grid_raster_data.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_widget.h"
// Qt 头文件
#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QDebug>

namespace DA
{

// ==================== 私有实现类 ====================

class DAChartWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartWidget)
public:
    PrivateData(DAChartWidget* q) : q_ptr(q)
    {
    }

    void initialize();
    void setupCanvas();
    void setupInteractions();

    // 交互组件
    QwtPlotCanvasZoomer* zoomer { nullptr };
    QwtPlotMagnifier* magnifier { nullptr };
    QwtPlotPicker* crosshair { nullptr };
    QwtPlotPanner* panner { nullptr };
    QwtPlotSeriesDataPicker* dataPicker { nullptr };
    QwtLegend* legendPanel { nullptr };

    // 显示组件
    QwtPlotGrid* grid { nullptr };
    QwtPlotLegendItem* legend { nullptr };

    // 样式属性
    QColor borderColor;
    QBrush backgroundBrush { Qt::white };
    QBrush canvasBackground { Qt::white };
    // 工厂函数
    DAChartWidget::PannerFactory pannerFactory;
    DAChartWidget::PickerFactory pickerFactory;
    DAChartWidget::DataPickerFactory dataPickerFactory;
};

void DAChartWidget::PrivateData::initialize()
{
    setupCanvas();

    // 设置绘图布局
    QwtPlotLayout* layout = q_ptr->plotLayout();
    layout->setCanvasMargin(0);

    // 初始化样式
    q_ptr->setAutoFillBackground(true);
    q_ptr->setPalette(QPalette(backgroundBrush.color()));
    q_ptr->setCanvasBackground(canvasBackground);
    q_ptr->setLineWidth(0);

    //
    pickerFactory = [](QWidget* canvas) -> QwtPlotPicker* { return new DAChartCrossTracker(canvas); };
}

void DAChartWidget::PrivateData::setupCanvas()
{
    // 创建自定义画布
    DAChartCanvas* canvas = new DAChartCanvas();
    canvas->setCursor(Qt::ArrowCursor);
    canvas->setFocusPolicy(Qt::ClickFocus);
    canvas->setFocusProxy(q_ptr);
    q_ptr->setCanvas(canvas);
}

void DAChartWidget::PrivateData::setupInteractions()
{
    // 交互组件在需要时延迟创建
}

// ==================== DAChartWidget 实现 ====================

DAChartWidget::DAChartWidget(QWidget* parent) : QwtPlot(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->initialize();
    enableMouseWheelZoom(true);  // 默认开启滚轮缩放
}

DAChartWidget::~DAChartWidget()
{
    // 智能指针自动清理
}

// ==================== DAChartDataInterface 实现 ====================

/**
 * @brief 获取所有数据相关的rtti,此函数会影响clearAllData函数
 *
 * 如果你有自定义的数据绘图，你要重写此函数，把新增的rtti加入到返回的rtti列表中
 * @return 涉及绘图相关的rtti，网格等不会返回
 * @sa clearAllData
 */
QList< int > DAChartWidget::dataRttis() const
{
    QList< int > rttis;
    rttis << QwtPlotItem::Rtti_PlotMarker << QwtPlotItem::Rtti_PlotCurve << QwtPlotItem::Rtti_PlotSpectroCurve
          << QwtPlotItem::Rtti_PlotIntervalCurve << QwtPlotItem::Rtti_PlotHistogram << QwtPlotItem::Rtti_PlotSpectrogram
          << QwtPlotItem::Rtti_PlotTradingCurve << QwtPlotItem::Rtti_PlotBarChart << QwtPlotItem::Rtti_PlotMultiBarChart
          << QwtPlotItem::Rtti_PlotZone << QwtPlotItem::Rtti_PlotVectorField;
    return rttis;
}

QwtPlotCurve* DAChartWidget::addCurve(const QVector< double >& xData, const QVector< double >& yData, const QString& title)
{
    if (xData.isEmpty() || yData.isEmpty() || xData.size() != yData.size()) {
        qWarning() << "Invalid data for curve:" << title;
        return nullptr;
    }
    QwtPlotCurve* curve = new QwtPlotCurve(title);
    curve->setSamples(xData, yData);
    curve->setPen(QPen(Qt::blue, 2.0));
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->attach(this);
    return curve;
}

QwtPlotCurve* DAChartWidget::addCurve(const QVector< QPointF >& points, const QString& title)
{
    if (points.isEmpty()) {
        qWarning() << "Empty points for curve:" << title;
        return nullptr;
    }

    QwtPlotCurve* curve = new QwtPlotCurve(title);
    curve->setSamples(points);
    curve->setPen(QPen(Qt::blue, 2.0));
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->attach(this);
    return curve;
}

QList< QwtPlotCurve* > DAChartWidget::getCurves() const
{
    QList< QwtPlotCurve* > curves;
    const QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);

    for (QwtPlotItem* item : items) {
        if (QwtPlotCurve* curve = dynamic_cast< QwtPlotCurve* >(item)) {
            curves.append(curve);
        }
    }

    return curves;
}

void DAChartWidget::removeCurve(QwtPlotCurve* curve)
{
    if (!curve)
        return;

    curve->detach();
    delete curve;
}

QwtPlotCurve* DAChartWidget::addScatter(const QVector< QPointF >& points, const QString& title)
{
    QwtPlotCurve* curve = addCurve(points, title);
    if (curve) {
        curve->setStyle(QwtPlotCurve::Dots);
        QwtSymbol* symbol = new QwtSymbol(QwtSymbol::Ellipse, QBrush(Qt::red), QPen(Qt::black, 1), QSize(6, 6));
        curve->setSymbol(symbol);
    }
    return curve;
}

QwtPlotBarChart* DAChartWidget::addBarChart(const QVector< double >& values, const QString& title)
{
    if (values.isEmpty()) {
        qWarning() << "Empty values for bar chart:" << title;
        return nullptr;
    }

    QwtPlotBarChart* barChart = new QwtPlotBarChart(title);

    // 转换为点数据 (x从0开始)
    QVector< QPointF > points;
    points.reserve(values.size());
    for (int i = 0; i < values.size(); ++i) {
        points.append(QPointF(i, values[ i ]));
    }

    barChart->setSamples(points);

    // 设置柱状图样式
    QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    symbol->setFrameStyle(QwtColumnSymbol::Raised);
    symbol->setBrush(QBrush(QColor(65, 105, 225)));  // 皇家蓝色
    barChart->setSymbol(symbol);

    barChart->attach(this);
    return barChart;
}

QwtPlotBarChart* DAChartWidget::addBarChart(const QVector< QPointF >& points, const QString& title)
{
    if (points.isEmpty()) {
        qWarning() << "Empty points for bar chart:" << title;
        return nullptr;
    }

    QwtPlotBarChart* barChart = new QwtPlotBarChart(title);
    barChart->setSamples(points);

    // 设置柱状图样式
    QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    symbol->setFrameStyle(QwtColumnSymbol::Raised);
    symbol->setBrush(QBrush(QColor(65, 105, 225)));
    barChart->setSymbol(symbol);
    barChart->attach(this);
    return barChart;
}

QwtPlotIntervalCurve* DAChartWidget::addIntervalCurve(const QVector< double >& values,
                                                      const QVector< double >& mins,
                                                      const QVector< double >& maxs,
                                                      const QString& title)
{
    int minSize = qMin(qMin(values.size(), mins.size()), maxs.size());
    if (minSize == 0) {
        qWarning() << "Invalid data for interval curve:" << title;
        return nullptr;
    }

    QVector< QwtIntervalSample > samples;
    samples.reserve(minSize);

    for (int i = 0; i < minSize; ++i) {
        samples.append(QwtIntervalSample(values[ i ], mins[ i ], maxs[ i ]));
    }

    QwtPlotIntervalCurve* intervalCurve = new QwtPlotIntervalCurve(title);
    intervalCurve->setSamples(samples);
    intervalCurve->setPen(QPen(Qt::darkGreen, 1));
    intervalCurve->setBrush(QBrush(QColor(144, 238, 144, 128)));  // 浅绿色，半透明
    intervalCurve->attach(this);
    return intervalCurve;
}

QwtPlotMarker* DAChartWidget::addVerticalLine(double x, const QString& title)
{
    QwtPlotMarker* marker = new QwtPlotMarker(title);
    marker->setXValue(x);
    marker->setLineStyle(QwtPlotMarker::VLine);
    marker->setLinePen(QPen(Qt::black, 1, Qt::DashLine));
    marker->attach(this);
    return marker;
}

QwtPlotMarker* DAChartWidget::addHorizontalLine(double y, const QString& title)
{
    QwtPlotMarker* marker = new QwtPlotMarker(title);
    marker->setYValue(y);
    marker->setLineStyle(QwtPlotMarker::HLine);
    marker->setLinePen(QPen(Qt::black, 1, Qt::DashLine));
    marker->attach(this);
    return marker;
}

QwtPlotMarker* DAChartWidget::addCrossLine(double x, double y, const QString& title)
{
    QwtPlotMarker* marker = new QwtPlotMarker(title);
    marker->setXValue(x);
    marker->setYValue(y);
    marker->setLineStyle(QwtPlotMarker::Cross);
    marker->setLinePen(QPen(Qt::black, 1, Qt::DotLine));
    marker->attach(this);
    return marker;
}

QwtPlotSpectrogram* DAChartWidget::addSpectrogram(QwtGridRasterData* gridData, const QString& title)
{
    if (!gridData) {
        qWarning() << "Null grid data for spectrogram:" << title;
        return nullptr;
    }

    QwtPlotSpectrogram* spectrogram = new QwtPlotSpectrogram(title);
    spectrogram->setData(gridData);
    spectrogram->attach(this);
    return spectrogram;
}

void DAChartWidget::removePlotItem(QwtPlotItem* item)
{
    if (!item) {
        return;
    }

    item->detach();
    delete item;
}

void DAChartWidget::clearAllData()
{
    const QwtPlotItemList items = itemList();

    // 删除所有数据相关的项目（保留网格、图例等显示元素）
    const QList< int > rttis = dataRttis();
    for (QwtPlotItem* item : items) {
        if (rttis.contains(item->rtti())) {
            item->detach();
            delete item;
        }
    }
}

QRectF DAChartWidget::getDataBounds() const
{
    return DAChartUtil::getVisibleRegionRang(const_cast< DAChartWidget* >(this));
}

bool DAChartWidget::hasData() const
{
    const QList< int > rttis    = dataRttis();
    const QwtPlotItemList items = itemList();
    for (const QwtPlotItem* item : items) {
        if (rttis.contains(item->rtti())) {
            return true;
        }
    }
    return false;
}

// ==================== DAChartStyleInterface 实现 ====================

void DAChartWidget::setChartTitle(const QString& title)
{
    setTitle(title);
    notifyPropertiesChanged(ChartTitleChanged);
}

QString DAChartWidget::getChartTitle() const
{
    return title().text();
}

void DAChartWidget::setBackgroundBrush(const QBrush& brush)
{
    d_ptr->backgroundBrush = brush;

    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, brush);
    setPalette(palette);
    notifyPropertiesChanged(BackgroundChanged);
}

QBrush DAChartWidget::getBackgroundBrush() const
{
    return d_ptr->backgroundBrush;
}

void DAChartWidget::setBorderColor(const QColor& color)
{
    if (d_ptr->borderColor != color) {
        d_ptr->borderColor = color;
        update();
        notifyPropertiesChanged(BorderColorChanged);
    }
}

QColor DAChartWidget::getBorderColor() const
{
    return d_ptr->borderColor;
}

void DAChartWidget::setAxisLabel(int axisId, const QString& label)
{
    setAxisTitle(axisId, label);
    notifyPropertiesChanged(AxisLabelChanged);
}

QString DAChartWidget::getAxisLabel(int axisId) const
{
    return axisTitle(axisId).text();
}

void DAChartWidget::setAxisColor(int axisId, const QColor& color)
{
    QwtScaleWidget* aw = this->axisWidget(axisId);
    if (aw) {
        aw->setScaleColor(color);
        notifyPropertiesChanged(AxisColorChanged);
    }
}

QColor DAChartWidget::getAxisColor(int axisId) const
{
    const QwtScaleWidget* aw = this->axisWidget(axisId);
    return aw ? aw->scaleColor() : QColor();
}

void DAChartWidget::enableGrid(bool enable)
{
    if (!d_ptr->grid && enable) {
        d_ptr->grid = new QwtPlotGrid();
        d_ptr->grid->setMajorPen(QPen(Qt::gray, 1, Qt::DotLine));
        d_ptr->grid->setMinorPen(QPen(Qt::lightGray, 0.5, Qt::DotLine));
        d_ptr->grid->attach(this);
    }

    if (d_ptr->grid) {
        d_ptr->grid->setVisible(enable);
        notifyPropertiesChanged(GridEnabledChanged);
    }
}

void DAChartWidget::enableGridX(bool enable)
{
    if (!d_ptr->grid && enable) {
        // 如果网格不存在但需要启用，先创建网格
        enableGrid(true);
    }

    if (d_ptr->grid) {
        bool currentState = d_ptr->grid->xEnabled();
        if (currentState != enable) {
            d_ptr->grid->enableX(enable);

            // 确保网格整体可见
            if (enable && !d_ptr->grid->isVisible()) {
                d_ptr->grid->setVisible(true);
            }

            notifyPropertiesChanged(GridStyleChanged);
        }
    }
}

void DAChartWidget::enableGridY(bool enable)
{
    if (!d_ptr->grid && enable) {
        // 如果网格不存在但需要启用，先创建网格
        enableGrid(true);
    }

    if (d_ptr->grid) {
        bool currentState = d_ptr->grid->yEnabled();
        if (currentState != enable) {
            d_ptr->grid->enableY(enable);

            // 确保网格整体可见
            if (enable && !d_ptr->grid->isVisible()) {
                d_ptr->grid->setVisible(true);
            }

            notifyPropertiesChanged(GridStyleChanged);
        }
    }
}

void DAChartWidget::enableGridXMin(bool enable)
{
    if (!d_ptr->grid && enable) {
        // 如果网格不存在但需要启用，先创建网格
        enableGrid(true);
    }

    if (d_ptr->grid) {
        bool currentState = d_ptr->grid->xMinEnabled();
        if (currentState != enable) {
            d_ptr->grid->enableXMin(enable);

            // 确保网格整体可见
            if (enable && !d_ptr->grid->isVisible()) {
                d_ptr->grid->setVisible(true);
            }
            notifyPropertiesChanged(GridStyleChanged);
        }
    }
}

void DAChartWidget::enableGridYMin(bool enable)
{
    if (!d_ptr->grid && enable) {
        // 如果网格不存在但需要启用，先创建网格
        enableGrid(true);
    }

    if (d_ptr->grid) {
        bool currentState = d_ptr->grid->yMinEnabled();
        if (currentState != enable) {
            d_ptr->grid->enableYMin(enable);

            // 确保网格整体可见
            if (enable && !d_ptr->grid->isVisible()) {
                d_ptr->grid->setVisible(true);
            }
            notifyPropertiesChanged(GridStyleChanged);
        }
    }
}

bool DAChartWidget::isGridEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible();
}

bool DAChartWidget::isGridXEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->xEnabled();
}

bool DAChartWidget::isGridYEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->yEnabled();
}

bool DAChartWidget::isGridXMinEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->xMinEnabled();
}

bool DAChartWidget::isGridYMinEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->yMinEnabled();
}

void DAChartWidget::setGridStyle(const QColor& color, qreal width, Qt::PenStyle style, bool isMajor)
{
    if (!d_ptr->grid) {
        enableGrid(true);
    }

    if (d_ptr->grid) {
        if (isMajor) {
            d_ptr->grid->setMajorPen(QPen(color, width, style));
        } else {
            d_ptr->grid->setMinorPen(QPen(color, width, style));
        }
        notifyPropertiesChanged(GridStyleChanged);
    }
}

void DAChartWidget::setGridMajorStyle(const QColor& color, qreal width, Qt::PenStyle style)
{
    setGridStyle(color, width, style, true);
}

void DAChartWidget::setGridMinorStyle(const QColor& color, qreal width, Qt::PenStyle style)
{
    setGridStyle(color, width, style, false);
}

void DAChartWidget::enableLegend(bool enable)
{
    if (!d_ptr->legend && enable) {
        d_ptr->legend = new QwtPlotLegendItem();
        d_ptr->legend->setRenderHint(QwtPlotItem::RenderAntialiased);
        d_ptr->legend->setBackgroundBrush(QBrush(QColor(255, 255, 255, 200)));
        d_ptr->legend->setBorderPen(QPen(Qt::gray));
        d_ptr->legend->setTextPen(QPen(Qt::black));
        d_ptr->legend->attach(this);
    }

    if (d_ptr->legend) {
        d_ptr->legend->setVisible(enable);
        notifyPropertiesChanged(LegendEnabledChanged);
    }
}

bool DAChartWidget::isLegendEnabled() const
{
    return d_ptr->legend && d_ptr->legend->isVisible();
}

void DAChartWidget::setLegendPosition(Qt::Alignment alignment)
{
    if (d_ptr->legend) {
        d_ptr->legend->setAlignmentInCanvas(alignment);
        notifyPropertiesChanged(LegendPositionChanged);
    }
}

Qt::Alignment DAChartWidget::getLegendPosition() const
{
    // QwtPlotLegendItem 没有直接的位置获取方法
    if (d_ptr->legend) {
        return d_ptr->legend->alignmentInCanvas();
    }
    return Qt::AlignRight | Qt::AlignTop;
}

void DAChartWidget::setLegendBackground(const QBrush& brush)
{
    if (d_ptr->legend) {
        d_ptr->legend->setBackgroundBrush(brush);
        notifyPropertiesChanged(LegendBackgroundChanged);
    }
}

QBrush DAChartWidget::getLegendBackground() const
{
    return d_ptr->legend ? d_ptr->legend->backgroundBrush() : QBrush();
}

void DAChartWidget::setLegendTextColor(const QColor& color)
{
    if (d_ptr->legend) {
        d_ptr->legend->setTextPen(QPen(color));
        notifyPropertiesChanged(LegendTextColorChanged);
    }
}

QColor DAChartWidget::getLegendTextColor() const
{
    return d_ptr->legend ? d_ptr->legend->textPen().color() : QColor();
}

void DAChartWidget::setupDateTimeAxis(int axisId, const QString& format)
{
    QwtDateScaleDraw* dateScale = new QwtDateScaleDraw(Qt::LocalTime);

    if (!format.isEmpty()) {
        // 设置智能日期格式
        DAChartUtil::setupSmartDateFormat(dateScale, format);
    }
    setAxisScaleDraw(axisId, dateScale);

    notifyPropertiesChanged(DateTimeScaleSetup);
}

bool DAChartWidget::isDateTimeAxis(int axisId) const
{
    return dynamic_cast< const QwtDateScaleDraw* >(axisScaleDraw(axisId)) != nullptr;
}

// ==================== DAChartInteractionInterface 实现 ====================

void DAChartWidget::enableZoom(bool enable)
{
    if (!d_ptr->zoomer && enable) {
        setupZoomer();
    }

    if (d_ptr->zoomer) {
        d_ptr->zoomer->setEnabled(enable);

        if (enable) {
            // 缩放启用时禁用平移
            enablePan(false);
        }
        notifyPropertiesChanged(ZoomStateChanged);
    }
}

bool DAChartWidget::isZoomEnabled() const
{
    return d_ptr->zoomer && d_ptr->zoomer->isEnabled();
}

void DAChartWidget::zoomToOriginal()
{
    if (d_ptr->zoomer) {
        d_ptr->zoomer->setZoomBase();
        replot();
    }
}

void DAChartWidget::zoomIn()
{
    if (!d_ptr->magnifier) {
        setupMagnifier();
    }

    if (d_ptr->magnifier) {
        d_ptr->magnifier->rescale(0.8);
    }
}

void DAChartWidget::zoomOut()
{
    if (!d_ptr->magnifier) {
        setupMagnifier();
    }

    if (d_ptr->magnifier) {
        d_ptr->magnifier->rescale(1.2);
    }
}

QwtPlotCanvasZoomer* DAChartWidget::getZoomer() const
{
    return d_ptr->zoomer;
}

void DAChartWidget::enablePan(bool enable)
{
    if (!d_ptr->panner && enable) {
        setupPanner();
    }

    if (d_ptr->panner) {
        d_ptr->panner->setEnabled(enable);

        if (enable) {
            // 平移启用时禁用缩放
            enableZoom(false);
        }
        notifyPropertiesChanged(PanStateChanged);
    }
}

bool DAChartWidget::isPanEnabled() const
{
    return d_ptr->panner && d_ptr->panner->isEnabled();
}

QwtPlotPanner* DAChartWidget::getPanner() const
{
    return d_ptr->panner;
}

void DAChartWidget::enableCrosshair(bool enable)
{
    if (!d_ptr->crosshair && enable) {
        setupCrosshair();
    }

    if (d_ptr->crosshair) {
        d_ptr->crosshair->setEnabled(enable);
        notifyPropertiesChanged(CrosshairStateChanged);
    }
}

bool DAChartWidget::isCrosshairEnabled() const
{
    return d_ptr->crosshair && d_ptr->crosshair->isEnabled();
}

QwtPlotPicker* DAChartWidget::getCrosshair() const
{
    return d_ptr->crosshair;
}

bool DAChartWidget::isDataPickingEnabled() const
{
    return isYValuePickingEnabled() || isXYValuePickingEnabled();
}

void DAChartWidget::enableYValuePicking(bool enable)
{
    if (!d_ptr->dataPicker && enable) {
        setupDataPicker();
    }

    if (d_ptr->dataPicker) {
        if (enable) {
            d_ptr->dataPicker->setEnabled(true);
            d_ptr->dataPicker->setPickMode(QwtPlotSeriesDataPicker::PickYValue);
        } else if (!isXYValuePickingEnabled()) {
            d_ptr->dataPicker->setEnabled(false);
        }
        notifyPropertiesChanged(DataPickingStateChanged);
    }
}

bool DAChartWidget::isYValuePickingEnabled() const
{
    return d_ptr->dataPicker && d_ptr->dataPicker->isEnabled()
           && d_ptr->dataPicker->pickMode() == QwtPlotSeriesDataPicker::PickYValue;
}

void DAChartWidget::enableXYValuePicking(bool enable)
{
    if (!d_ptr->dataPicker && enable) {
        setupDataPicker();
    }

    if (d_ptr->dataPicker) {
        if (enable) {
            d_ptr->dataPicker->setEnabled(true);
            d_ptr->dataPicker->setPickMode(QwtPlotSeriesDataPicker::PickNearestPoint);
        } else if (!isYValuePickingEnabled()) {
            d_ptr->dataPicker->setEnabled(false);
        }
        notifyPropertiesChanged(DataPickingStateChanged);
    }
}

bool DAChartWidget::isXYValuePickingEnabled() const
{
    return d_ptr->dataPicker && d_ptr->dataPicker->isEnabled()
           && d_ptr->dataPicker->pickMode() == QwtPlotSeriesDataPicker::PickNearestPoint;
}

QwtPlotSeriesDataPicker* DAChartWidget::getDataPicker() const
{
    return d_ptr->dataPicker;
}

void DAChartWidget::enableMouseWheelZoom(bool enable)
{
    if (!d_ptr->magnifier && enable) {
        setupMagnifier();
    }

    if (d_ptr->magnifier) {
        d_ptr->magnifier->setEnabled(enable);
        notifyPropertiesChanged(MouseWheelZoomStateChanged);
    }
}

bool DAChartWidget::isMouseWheelZoomEnabled() const
{
    return d_ptr->magnifier && d_ptr->magnifier->isEnabled();
}

QwtPlotMagnifier* DAChartWidget::getMagnifier() const
{
    return d_ptr->magnifier;
}

void DAChartWidget::enableLegendPanel(bool enable)
{
    if (!d_ptr->legendPanel && enable) {
        setupLegendPanel();
    }

    if (d_ptr->legendPanel) {
        d_ptr->legendPanel->setVisible(enable);
        notifyPropertiesChanged(LegendPanelEnabledChanged);
    }
}

bool DAChartWidget::isLegendPanelEnabled() const
{
    return d_ptr->legendPanel && d_ptr->legendPanel->isVisible();
}

QwtLegend* DAChartWidget::getLegendPanel() const
{
    return d_ptr->legendPanel;
}

void DAChartWidget::registerPannerFactory(const PannerFactory& factory)
{
    d_ptr->pannerFactory = factory;
}

void DAChartWidget::registerPickerFactory(const PickerFactory& factory)
{
    d_ptr->pickerFactory = factory;
}

void DAChartWidget::registerDataPickerFactory(const DataPickerFactory& factory)
{
    d_ptr->dataPickerFactory = factory;
}

// ==================== 工具函数 ====================

DAFigureWidget* DAChartWidget::getFigure() const
{
    QWidget* parent = const_cast< DAChartWidget* >(this);
    while (QWidget* grandParent = parent->parentWidget()) {
        if (DAFigureWidget* figure = qobject_cast< DAFigureWidget* >(grandParent)) {
            return figure;
        }
        parent = grandParent;
    }
    return nullptr;
}

void DAChartWidget::notifyPropertiesChanged(ChartPropertyChangeFlags flag)
{
    Q_EMIT chartPropertiesChanged(this, flag);
}

// ==================== 事件处理 ====================

void DAChartWidget::paintEvent(QPaintEvent* event)
{

    // 然后调用基类绘制
    QwtPlot::paintEvent(event);
    // 绘制边框
    if (d_ptr->borderColor.isValid()) {
        QPainter painter(this);
        painter.setPen(QPen(d_ptr->borderColor, lineWidth()));
        painter.drawRect(rect());
    }
}

// ==================== 私有方法实现 ====================

void DAChartWidget::setupZoomer()
{
    if (d_ptr->zoomer) {
        d_ptr->zoomer->setEnabled(false);
        d_ptr->zoomer->deleteLater();
    }

    d_ptr->zoomer = new QwtPlotCanvasZoomer(canvas());
    d_ptr->zoomer->setRubberBand(QwtPicker::RectRubberBand);
    d_ptr->zoomer->setTrackerMode(QwtPicker::ActiveOnly);
    d_ptr->zoomer->setMaxStackDepth(30);
}

void DAChartWidget::setupPanner()
{
    if (d_ptr->panner) {
        d_ptr->panner->setEnabled(false);
        d_ptr->panner->deleteLater();
    }

    if (d_ptr->pannerFactory) {
        d_ptr->panner = d_ptr->pannerFactory(canvas());
    } else {
        d_ptr->panner = new QwtPlotPanner(canvas());
    }

    d_ptr->panner->setMouseButton(Qt::MiddleButton);
}

void DAChartWidget::setupMagnifier()
{
    if (d_ptr->magnifier) {
        d_ptr->magnifier->setEnabled(false);
        d_ptr->magnifier->deleteLater();
    }

    d_ptr->magnifier = new QwtPlotMagnifier(canvas());
}

void DAChartWidget::setupCrosshair()
{
    if (d_ptr->crosshair) {
        d_ptr->crosshair->setEnabled(false);
        d_ptr->crosshair->deleteLater();
    }

    if (d_ptr->pickerFactory) {
        d_ptr->crosshair = d_ptr->pickerFactory(canvas());
    } else {
        // 使用默认的十字线
        d_ptr->crosshair = new QwtPlotPicker(canvas());
        d_ptr->crosshair->setTrackerMode(QwtPicker::AlwaysOn);
        d_ptr->crosshair->setRubberBand(QwtPicker::CrossRubberBand);
        d_ptr->crosshair->setRubberBandPen(QPen(Qt::gray, 1));
    }
}

void DAChartWidget::setupDataPicker()
{
    if (d_ptr->dataPicker) {
        d_ptr->dataPicker->setEnabled(false);
        d_ptr->dataPicker->deleteLater();
    }

    if (d_ptr->dataPickerFactory) {
        d_ptr->dataPicker = d_ptr->dataPickerFactory(canvas());
    } else {
        d_ptr->dataPicker = new QwtPlotSeriesDataPicker(canvas());
    }
}

void DAChartWidget::setupLegendPanel()
{
    if (d_ptr->legendPanel) {
        d_ptr->legendPanel->setVisible(false);
        d_ptr->legendPanel->deleteLater();
    }

    d_ptr->legendPanel = new QwtLegend();
    d_ptr->legendPanel->setDefaultItemMode(QwtLegendData::Checkable);
    insertLegend(d_ptr->legendPanel, QwtPlot::RightLegend);

    connect(d_ptr->legendPanel, &QwtLegend::checked, this, &DAChartWidget::onLegendItemToggled);
}

void DAChartWidget::onLegendItemToggled(const QVariant& itemInfo, bool checked)
{
    QwtPlotItem* item = infoToItem(itemInfo);
    if (item) {
        item->setVisible(checked);
    }
}

}  // namespace DA

#endif
