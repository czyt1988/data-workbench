#include "DAChartWidget.h"
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
    QScopedPointer< QwtPlotZoomer > mZoomer;
    QScopedPointer< QwtPlotZoomer > mZoomerSecond;
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
QwtPlotZoomer* DAChartWidget::getZoomer() const
{
    return (d_ptr->mZoomer.data());
}
/**
 * @brief 获取缩放器2
 * @return 如果没有返回nullptr
 */
QwtPlotZoomer* DAChartWidget::getZoomerSecond()
{
    return (d_ptr->mZoomerSecond.data());
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
    if (!enable && (nullptr == d_ptr->mPicker)) {
        return;
    }
    if (nullptr == d_ptr->mPicker) {
        setupCrossPicker();
    }
    d_ptr->mPicker->setEnabled(enable);
    if (d_ptr->mZoomer.isNull()) {
        if (isZoomerEnabled()) {
            // 如果缩放开启，缩放的TrackerMode和picker重复，这里就需要把TrackerMode取消
            d_ptr->mZoomer->setTrackerMode((enable ? QwtPicker::AlwaysOff : QwtPicker::AlwaysOn));
        }
    }
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
    if (nullptr == d_ptr->mPanner) {
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

void DAChartWidget::deletePanner()
{
    if (nullptr != d_ptr->mPanner) {
        d_ptr->mPanner->setParent(nullptr);  // 断开和canvas()的父子连接
        delete d_ptr->mPanner;
        d_ptr->mPanner = nullptr;
    }
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
    if (!enable) {
        if (d_ptr->mZoomer.isNull()) {
            return;
        }
    }
    if (d_ptr->mZoomer.isNull() /*|| nullptr == _zoomerSecond*/) {
        setupZoomer();
    }
    setZoomerEnable(d_ptr->mZoomer.data(), enable);
    setZoomerEnable(d_ptr->mZoomerSecond.data(), enable);
    if (isCrossPickerEnable()) {
        d_ptr->mZoomer->setTrackerMode((enable ? QwtPicker::AlwaysOff : QwtPicker::ActiveOnly));
    }
    if (enable) {
        if (isPannerEnable()) {
            setPanEnable(false);
        }
    }
    emit enableZoomerChanged(enable);
}

void DAChartWidget::setZoomerEnable(QwtPlotZoomer* zoomer, bool enable)
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

///
/// \brief 回到放大的最底栈
///
void DAChartWidget::setZoomBase()
{
    if (!d_ptr->mZoomer.isNull()) {
        d_ptr->mZoomer->setZoomBase(true);
    }
    if (!d_ptr->mZoomerSecond.isNull()) {
        d_ptr->mZoomerSecond->setZoomBase(true);
    }
}

/**
 * @brief 构建默认的缩放器
 */
void DAChartWidget::setupZoomer()
{
    if (d_ptr->mZoomer.isNull()) {
        qDebug() << "setup zoom";
#if 0
        Zoomer_qwt *zoom = new Zoomer_qwt(xBottom, yLeft, canvas());//Zoomer_qwt( QwtPlot::xBottom, QwtPlot::yLeft,canvas() );

        zoom->on_enable_scrollBar(isHaveScroll);
        zoom->setRubberBand(QwtPicker::RectRubberBand);
        zoom->setTrackerPen(QColor(Qt::black));
        zoom->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_I, Qt::ShiftModifier);
        zoom->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_O, Qt::ShiftModifier);
        zoom->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Home);
        zoom->setMousePattern(QwtEventPattern::MouseSelect2,
            Qt::RightButton, Qt::ControlModifier);
        zoom->setMousePattern(QwtEventPattern::MouseSelect3,
            Qt::RightButton);
        zoom->setTrackerMode(QwtPicker::AlwaysOff);
        _zoomer = zoom;
#else
        d_ptr->mZoomer.reset(new QwtPlotZoomer(xBottom, yLeft, canvas()));
        d_ptr->mZoomer->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_I, Qt::ShiftModifier);
        d_ptr->mZoomer->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_O, Qt::ShiftModifier);
        d_ptr->mZoomer->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Home);
        d_ptr->mZoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
        d_ptr->mZoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
        d_ptr->mZoomer->setTrackerMode(QwtPicker::AlwaysOff);
        d_ptr->mZoomer->setZoomBase(false);
        d_ptr->mZoomer->setMaxStackDepth(20);
#endif
    }
    if (nullptr == d_ptr->mZoomerSecond) {
        d_ptr->mZoomerSecond.reset(new QwtPlotZoomer(xTop, yRight, canvas()));
        d_ptr->mZoomerSecond->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_I, Qt::ShiftModifier);
        d_ptr->mZoomerSecond->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_O, Qt::ShiftModifier);
        d_ptr->mZoomerSecond->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Home);
        d_ptr->mZoomerSecond->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
        d_ptr->mZoomerSecond->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
        d_ptr->mZoomerSecond->setTrackerMode(QwtPicker::AlwaysOff);
        d_ptr->mZoomerSecond->setZoomBase(false);
        d_ptr->mZoomerSecond->setMaxStackDepth(20);
    }
    QwtPlotMagnifier* magnifier = new QwtPlotMagnifier(canvas());
    magnifier->setMouseButton(Qt::NoButton);
}

/**
 * @brief 构建自定义的缩放器
 * @param z
 */
void DAChartWidget::setupZoomer(QwtPlotZoomer* z, bool issecondZoom)
{
    if (issecondZoom) {
        d_ptr->mZoomerSecond.reset(z);
    } else {
        d_ptr->mZoomer.reset(z);
    }
}

void DAChartWidget::deleteZoomer()
{
    if (!d_ptr->mZoomer.isNull()) {
        d_ptr->mZoomer.reset();
    }
    if (!d_ptr->mZoomerSecond.isNull()) {
        d_ptr->mZoomerSecond.reset();
    }
}

///
/// \brief 设置缩放重置
///
void DAChartWidget::setZoomReset()
{
    if (!d_ptr->mZoomer.isNull()) {
        d_ptr->mZoomer->zoom(0);
    }
    if (!d_ptr->mZoomerSecond.isNull()) {
        d_ptr->mZoomerSecond->zoom(0);
    }
}

/**
 * @brief 放大
 */
void DAChartWidget::zoomIn()
{
    if (d_ptr->mZoomer.isNull()) {
        qWarning() << tr("Before zoom in, the chart must setup a zoomer");  // cn:在放大图表之前需要先建立缩放器
        return;
    }

    QRectF rect = d_ptr->mZoomer->zoomRect();
    double w    = rect.width() * 0.625;
    double h    = rect.height() * 0.625;
    double x    = rect.x() + (rect.width() - w) / 2.0;
    double y    = rect.y() + (rect.height() - h) / 2.0;

    rect.setX(x);
    rect.setY(y);
    rect.setWidth(w);
    rect.setHeight(h);
    if (rect.isValid()) {
        qDebug() << "zoom in from " << d_ptr->mZoomer->zoomRect() << " to " << rect;
        d_ptr->mZoomer->zoom(rect);
    } else {
        qDebug() << "zoom in get invalid zoom rect,current zoom rect is " << d_ptr->mZoomer->zoomRect()
                 << ",will zoom rect is " << rect;
    }
}

/**
 * @brief 缩小
 */
void DAChartWidget::zoomOut()
{
    if (d_ptr->mZoomer.isNull()) {
        qWarning() << tr("Before zoom out, the chart must setup a zoomer");  // cn:在缩小图表之前需要先建立缩放器
        return;
    }

    QRectF rect = d_ptr->mZoomer->zoomRect();
    double w    = rect.width() * 1.6;
    double h    = rect.height() * 1.6;
    double x    = rect.x() - (w - rect.width()) / 2.0;
    double y    = rect.y() - (h - rect.height()) / 2.0;

    rect.setX(x);
    rect.setY(y);
    rect.setWidth(w);
    rect.setHeight(h);
    if (rect.isValid()) {
        qDebug() << "zoom out from " << d_ptr->mZoomer->zoomRect() << " to " << rect;
        d_ptr->mZoomer->zoom(rect);
    } else {
        qDebug() << "zoom out get invalid zoom rect,current zoom rect is " << d_ptr->mZoomer->zoomRect()
                 << ",will zoom rect is " << rect;
    }
}

///
/// \brief 缩放到最适合比例，就是可以把所有图都能看清的比例
///
void DAChartWidget::zoomInCompatible()
{
    QwtInterval intv[ axisCnt ];
    bool needdelete       = false;
    QwtPlotZoomer* zoomer = d_ptr->mZoomer.data();
    if (nullptr == zoomer) {
        zoomer     = new QwtPlotZoomer(xBottom, yLeft, canvas());
        needdelete = true;
    }
    DAChartUtil::dataRange(this, &intv[ yLeft ], &intv[ yRight ], &intv[ xBottom ], &intv[ xTop ]);

    int axx = zoomer->xAxis();
    int axy = zoomer->yAxis();
    QRectF rect1;
    rect1.setRect(intv[ axx ].minValue(), intv[ axy ].minValue(), intv[ axx ].width(), intv[ axy ].width());
    if (rect1.isValid()) {
        qDebug() << "DAChartWidget::zoomInCompatible zoomer1 rect=" << rect1;
        zoomer->zoom(rect1);
    }
    if (needdelete) {
        delete zoomer;
    }

    zoomer = d_ptr->mZoomerSecond.data();
    if (zoomer) {

        int axx = zoomer->xAxis();
        int axy = zoomer->yAxis();
        QRectF rect1;
        rect1.setRect(intv[ axx ].minValue(), intv[ axy ].minValue(), intv[ axx ].width(), intv[ axy ].width());
        if (rect1.isValid()) {
            qDebug() << "DAChartWidget::zoomInCompatible zoomer2 rect=" << rect1;
            zoomer->zoom(rect1);
        }
    }

    /* !此方法不行
     * if(!d->_zoomer.isNull())
     * {
     *  int axx = d->_zoomer->xAxis();
     *  int axy = d->_zoomer->yAxis();
     *  double xmin = axisScaleEngine(axx)->lowerMargin();
     *  double xmax = axisScaleEngine(axx)->upperMargin();
     *  double ymin = axisScaleEngine(axy)->lowerMargin();
     *  double ymax = axisScaleEngine(axy)->upperMargin();
     *  QRectF rect1;
     *  rect1.setRect(xmin,ymin,xmax-xmin,ymax-ymin);
     *  qDebug()<<rect1;
     *  d->_zoomer->zoom(rect1);
     * }
     */
}

void DAChartWidget::setLegendPanelEnable(bool enable)
{
    if (isEnableLegendPanel() == enable) {
        return;  // 状态一致不动作
    }
    if (enable) {
        setuplegendPanel();
    } else {
        deletelegendPanel();
    }
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

void DAChartWidget::deletelegendPanel()
{
    insertLegend(nullptr);  // 内部会销毁
    d_ptr->mLegendPanel = nullptr;
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
    if (d_ptr->mZoomerSecond) {
        return (d_ptr->mZoomerSecond->isEnabled());
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
