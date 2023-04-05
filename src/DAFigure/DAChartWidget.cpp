#include "DAChartWidget.h"
#include <algorithm>
#include <QDebug>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>

#include "qwt_interval.h"
#include "qwt_picker_machine.h"
#include "qwt_legend_label.h"
#include "qwt_date_scale_draw.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_column_symbol.h"
#include "DAChartYValueMarker.h"
#include "DAChartPointMarker.h"
#include "DAChartYDataPicker.h"
#include "DAChartXYDataPicker.h"
#include "DAChartCrossTracker.h"
#include "DAChartCanvas.h"

#include "DAChartUtil.h"

// unsigned int ChartWave_qwt::staticValue_nAutoLineID = 0;//静态变量初始化
namespace DA
{
class DAChartWidgetPrivate
{
    DA_IMPL_PUBLIC(DAChartWidget)
public:
    QScopedPointer< QwtPlotZoomer > _zoomer;
    QScopedPointer< QwtPlotZoomer > _zoomerSecond;
    QwtPlotGrid* _grid;
    QwtPlotPicker* _picker;
    QwtPlotPanner* _panner;
    QwtPlotLegendItem* _legend;
    QwtLegend* _legendPanel;
    DAChartYDataPicker* _yDataPicker;
    DAChartXYDataPicker* _xyDataPicker;
    QColor _borderColor;
    DAChartWidgetPrivate(DAChartWidget* p)
        : q_ptr(p)
        , _grid(nullptr)
        , _picker(nullptr)
        , _panner(nullptr)
        , _legend(nullptr)
        , _legendPanel(nullptr)
        , _yDataPicker(nullptr)
        , _xyDataPicker(nullptr)
    {
    }
};

DAChartWidget::DAChartWidget(QWidget* parent) : QwtPlot(parent), d_ptr(new DAChartWidgetPrivate(this))
{
    setAutoReplot(false);
    setAutoFillBackground(true);

    QwtPlotLayout* pLayout = plotLayout();

    pLayout->setCanvasMargin(0);
    pLayout->setAlignCanvasToScales(true);

    DAChartCanvas* pCanvas = new DAChartCanvas();

    pCanvas->setCursor(Qt::ArrowCursor);
    setCanvas(pCanvas);
    pCanvas->setFocusPolicy(Qt::ClickFocus);
    //设置点击Canvas，plot获得焦点
    pCanvas->setFocusProxy(this);
    // Qt::NoBrush无法透明，一直有一个灰色的背景
    setCanvasBackground(Qt::white);
    setChartBackgroundBrush(Qt::NoBrush);
    setChartBorderColor(QColor());
    setFocusPolicy(Qt::ClickFocus);
    setLineWidth(0);

    setAutoReplot(true);
    setAllAxisMargin(0);

    //这个例子来着qwt-example-refreshtest
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
    return (d_ptr->_zoomer.data());
}
/**
 * @brief 获取缩放器2
 * @return 如果没有返回nullptr
 */
QwtPlotZoomer* DAChartWidget::getZoomerSecond()
{
    DA_D(DAChartWidget, d);
    return (d->_zoomerSecond.data());
}

/**
 * @brief 获取grid
 * @return
 */
QwtPlotGrid* DAChartWidget::getGrid() const
{
    return (d_ptr->_grid);
}

void DAChartWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    if (d_ptr->_borderColor.isValid()) {
        QPen pen(d_ptr->_borderColor);
        int lw = lineWidth();
        pen.setWidth(lw);
        painter.setPen(pen);
        painter.drawRect(rect());
    }
    QwtPlot::paintEvent(e);
}

bool DAChartWidget::isEnableGrid() const
{
    DA_DC(DAChartWidget, d);
    if (d->_grid) {
        return (d->_grid->isVisible());
    }
    return (false);
}

bool DAChartWidget::isEnableGridX() const
{
    DA_DC(DAChartWidget, d);
    if (d->_grid) {
        if (d->_grid->isVisible()) {
            return (d->_grid->xEnabled());
        }
    }
    return (false);
}

bool DAChartWidget::isEnableGridY() const
{
    DA_DC(DAChartWidget, d);
    if (d->_grid) {
        if (d->_grid->isVisible()) {
            return (d->_grid->yEnabled());
        }
    }
    return (false);
}

bool DAChartWidget::isEnableGridXMin() const
{
    DA_DC(DAChartWidget, d);
    if (d->_grid) {
        if (d->_grid->isVisible()) {
            return (d->_grid->xMinEnabled());
        }
    }
    return (false);
}

bool DAChartWidget::isEnableGridYMin() const
{
    DA_DC(DAChartWidget, d);
    if (d->_grid) {
        if (d->_grid->isVisible()) {
            return (d->_grid->yMinEnabled());
        }
    }
    return (false);
}

bool DAChartWidget::isEnablePanner() const
{
    DA_DC(DAChartWidget, d);
    if (d->_panner) {
        return (d->_panner->isEnabled());
    }
    return (false);
}

bool DAChartWidget::isEnableLegend() const
{
    DA_DC(DAChartWidget, d);
    if (d->_legend) {
        return (d->_legend->isVisible());
    }
    return (false);
}

bool DAChartWidget::isEnableLegendPanel() const
{
    DA_DC(DAChartWidget, d);
    if (d->_legendPanel) {
        return (d->_legendPanel->isVisible());
    }
    return (false);
}

bool DAChartWidget::isEnableYDataPicker() const
{
    DA_DC(DAChartWidget, d);
    if (d->_yDataPicker) {
        return (d->_yDataPicker->isEnabled());
    }
    return (false);
}

bool DAChartWidget::isEnableXYDataPicker() const
{
    DA_DC(DAChartWidget, d);
    if (d->_xyDataPicker) {
        return (d->_xyDataPicker->isEnabled());
    }
    return (false);
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

//========================================================================================
//网格 grid 操作
//========================================================================================

/**
 * @brief 设置网格
 * @param color 网格的颜色
 * @param width 网格线条的宽度
 * @param style 网格的样式
 * @return
 */
QwtPlotGrid* DAChartWidget::setupGrid(const QColor& color, qreal width, Qt::PenStyle style)
{
    QwtPlotGrid* g = new QwtPlotGrid;
    g->setMajorPen(color, width, style);
    g->setMinorPen(color, 0, Qt::DotLine);  //小刻度的样式
    setupGrid(g);
    return g;
}

/**
 * @brief 添加grid
 * @param g
 */
void DAChartWidget::setupGrid(QwtPlotGrid* g)
{
    if (d_ptr->_grid && d_ptr->_grid != g) {
        d_ptr->_grid->detach();
        delete d_ptr->_grid;
    }
    d_ptr->_grid = g;
    d_ptr->_grid->attach(this);
}

/**
 * @brief 获取图例
 * @return
 */
QwtPlotLegendItem* DAChartWidget::getLegend() const
{
    return d_ptr->_legend;
}

/**
 * @brief 删除网格
 */
void DAChartWidget::deleteGrid()
{
    DA_D(DAChartWidget, d);
    if (nullptr == d->_grid) {
        return;
    }
    d->_grid->detach();
    delete d->_grid;
    d->_grid = nullptr;
    replot();  //刷新，否则不显示
}

/**
 * @brief 显示网格
 * @param enable
 */
void DAChartWidget::enableGrid(bool enable)
{
    if (enable == isEnableGrid()) {
        return;  //状态一致不动作
    }
    DA_D(DAChartWidget, d);
    if (enable) {
        if (nullptr == d->_grid) {
            setupGrid();
        }
        d->_grid->enableX(true);
        d->_grid->enableY(true);
        d->_grid->show();
        emit enableGridXChanged(enable);
        emit enableGridYChanged(enable);
        emit enableGridChanged(enable);
        return;
    } else {
        if (nullptr == d->_grid) {
            return;
        }
        d->_grid->hide();
    }
    replot();
    emit enableGridChanged(enable);
}

void DAChartWidget::enableGridX(bool enable)
{
    if (isEnableGridX() == enable) {
        return;  //状态一致不动作
    }
    DA_D(DAChartWidget, d);
    if (nullptr == d->_grid) {
        return;
    }
    d->_grid->enableX(enable);
    emit enableGridXChanged(enable);

    if (!enable) {
        emit enableGridXMinChanged(false);
    }
    // _grid->show();//刷新
}

void DAChartWidget::enableGridY(bool enable)
{
    if (isEnableGridY() == enable) {
        return;  //状态一致不动作
    }
    DA_D(DAChartWidget, d);
    if (nullptr == d->_grid) {
        return;
    }
    d->_grid->enableY(enable);
    emit enableGridYChanged(enable);

    if (!enable) {
        emit enableGridYMinChanged(false);
    }
}

void DAChartWidget::enableGridXMin(bool enable)
{
    if (isEnableGridXMin() == enable) {
        return;  //状态一致不动作
    }
    DA_D(DAChartWidget, d);
    if (nullptr == d->_grid) {
        return;
    }
    d->_grid->enableXMin(enable);
    emit enableGridXMinChanged(enable);
}

void DAChartWidget::enableGridYMin(bool enable)
{
    if (isEnableGridYMin() == enable) {
        return;  //状态一致不动作
    }
    DA_D(DAChartWidget, d);
    if (nullptr == d->_grid) {
        return;
    }
    d->_grid->enableYMin(enable);
    emit enableGridYMinChanged(enable);
}

//========================================================================================
//画线和数据 操作
//========================================================================================

///
/// \brief 设置y值横线标记
/// \param data 值
/// \param strLabel 描述
/// \param clr 颜色
/// \todo type show be use
///
void DAChartWidget::markYValue(double data, const QString& strLabel, QColor clr)
{
    double x                       = axisXmax();
    DAChartYValueMarker* valueMark = new DAChartYValueMarker(data);

    valueMark->setXValue(x);
    valueMark->setLinePen(clr, 1);
    valueMark->setLabel(strLabel);
    valueMark->setLabelAlignment(Qt::AlignTop | Qt::AlignRight);
    valueMark->setSpacing(1);  //设置文字和mark的间隔
    valueMark->attach(this);
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
    if (isEnableZoomer() != enable) {
        enableZoomer(enable);
    }
    if (isEnableCrossPicker() != enable) {
        enableCrossPicker(enable);
    }
    if (isEnablePanner() != enable) {
        enablePan(enable);
    }
    if (isEnableYDataPicker() != enable) {
        enableYDataPicker(enable);
    }
    if (isEnableXYDataPicker() != enable) {
        enableXYDataPicker(enable);
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
    return d_ptr->_borderColor;
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
QwtPlotBarChart* DAChartWidget::addBar(const QVector< QPointF >& xyDatas, const QColor& color)
{
    QwtPlotBarChart* ser = new QwtPlotBarChart();
    ser->setSamples(xyDatas);
    ser->setLegendMode(QwtPlotBarChart::LegendBarTitles);
    ser->setLegendIconSize(QSize(10, 14));
    ser->setLayoutPolicy(QwtPlotBarChart::AutoAdjustSamples);
    ser->setLayoutHint(4.0);  // minimum width for a single bar
    ser->setSpacing(10);      // spacing between bars
    QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    QPalette p              = symbol->palette();
    p.setColor(QPalette::Window, color);
    symbol->setPalette(p);
    ser->setSymbol(symbol);
    ser->attach(this);
    return ser;
}

/**
 * @brief 添加一个bar
 *
 * x值为0~n均匀分布
 * @param yDatas
 * @return
 */
QwtPlotBarChart* DAChartWidget::addBar(const QVector< double >& yDatas, const QColor& color)
{
    QwtPlotBarChart* ser = new QwtPlotBarChart();
    ser->setSamples(yDatas);
    ser->setLegendMode(QwtPlotBarChart::LegendBarTitles);
    ser->setLegendIconSize(QSize(10, 14));
    ser->setLayoutPolicy(QwtPlotBarChart::AutoAdjustSamples);
    ser->setLayoutHint(4.0);  // minimum width for a single bar
    ser->setSpacing(10);      // spacing between bars
    QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    QPalette p              = symbol->palette();
    p.setColor(QPalette::Window, color);
    symbol->setPalette(p);
    ser->setSymbol(symbol);
    ser->attach(this);
    return ser;
}

/**
 * @brief 设置所有坐标轴的Margin
 */
void DAChartWidget::setAllAxisMargin(int m)
{
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        QwtScaleWidget* scaleWidget = axisWidget(axisPos);
        if (scaleWidget) {
            scaleWidget->setMargin(m);
        }
    }
}

/**
 * @brief 设置边框
 * @param c
 */
void DAChartWidget::setChartBorderColor(const QColor& c)
{
    d_ptr->_borderColor = c;
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
    DA_D(DAChartWidget, d);
    if (nullptr == d->_picker) {
        d->_picker = new DAChartCrossTracker(this->canvas());
    }
}

void DAChartWidget::enableCrossPicker(bool enable)
{
    DA_D(DAChartWidget, d);
    if (!enable && (nullptr == d->_picker)) {
        return;
    }
    if (nullptr == d->_picker) {
        setupCrossPicker();
    }
    d->_picker->setEnabled(enable);
    if (d->_zoomer.isNull()) {
        if (isEnableZoomer()) {
            //如果缩放开启，缩放的TrackerMode和picker重复，这里就需要把TrackerMode取消
            d->_zoomer->setTrackerMode((enable ? QwtPicker::AlwaysOff : QwtPicker::AlwaysOn));
        }
    }
    emit enableCrossPickerChanged(enable);
}

/**
 * @brief 允许拖放
 * @note 此操作会和zoomer互斥，如果enablePan(true),会调用enableZoomer(false)
 * @param enable
 */
void DAChartWidget::enablePan(bool enable)
{
    DA_D(DAChartWidget, d);
    if (isEnablePanner() == enable) {
        return;  //状态一致跳出
    }
    if (nullptr == d->_panner) {
        if (!enable) {
            return;
        }
        setupPanner();
    }
    d->_panner->setEnabled(enable);
    if (enable) {
        // pan和zoom互斥
        if (isEnableZoomer()) {
            enableZoomer(false);
        }
    }
    emit enablePannerChanged(enable);
}

/**
 * @brief 建立一个内置的Panner(拖动)，默认使用鼠标中键
 */
void DAChartWidget::setupPanner()
{
    //设置拖动
    DA_D(DAChartWidget, d);
    if (nullptr == d->_panner) {
        d->_panner = new QwtPlotPanner(canvas());
        d->_panner->setCursor(QCursor(Qt::ClosedHandCursor));
        d->_panner->setMouseButton(Qt::LeftButton);
    }
}

void DAChartWidget::deletePanner()
{
    DA_D(DAChartWidget, d);
    if (nullptr != d->_panner) {
        d->_panner->setParent(nullptr);  //断开和canvas()的父子连接
        delete d->_panner;
        d->_panner = nullptr;
    }
}

/**
 * @brief 允许缩放
 * @param enable
 * @note 此函数会构建缩放器，如果要设置自定义的缩放器，可以使用@sa setupZoomer 函数
 */
void DAChartWidget::enableZoomer(bool enable)
{
    if (isEnableZoomer() == enable) {
        return;  //状态一致不动作
    }
    DA_D(DAChartWidget, d);
    if (!enable) {
        if (d->_zoomer.isNull()) {
            return;
        }
    }
    if (d->_zoomer.isNull() /*|| nullptr == _zoomerSecond*/) {
        setupZoomer();
    }
    enableZoomer(d->_zoomer.data(), enable);
    enableZoomer(d->_zoomerSecond.data(), enable);
    if (isEnableCrossPicker()) {
        d->_zoomer->setTrackerMode((enable ? QwtPicker::AlwaysOff : QwtPicker::ActiveOnly));
    }
    if (enable) {
        if (isEnablePanner()) {
            enablePan(false);
        }
    }
    emit enableZoomerChanged(enable);
}

void DAChartWidget::enableZoomer(QwtPlotZoomer* zoomer, bool enable)
{
    if (nullptr == zoomer) {
        return;
    }
    if (enable) {
        zoomer->setEnabled(true);
        zoomer->setZoomBase(true);
        zoomer->setRubberBand(QwtPicker::RectRubberBand);
        zoomer->setTrackerMode((isEnableCrossPicker() ? QwtPicker::AlwaysOff : QwtPicker::ActiveOnly));
    } else {
        zoomer->setEnabled(false);
        zoomer->setRubberBand(QwtPicker::NoRubberBand);
        zoomer->setTrackerMode(QwtPicker::AlwaysOff);
    }
    if (isEnableCrossPicker()) {
        zoomer->setTrackerMode((enable ? QwtPicker::AlwaysOff : QwtPicker::ActiveOnly));
    }
}

///
/// \brief 回到放大的最底栈
///
void DAChartWidget::setZoomBase()
{
    DA_D(DAChartWidget, d);
    if (!d->_zoomer.isNull()) {
        d->_zoomer->setZoomBase(true);
    }
    if (!d->_zoomerSecond.isNull()) {
        d->_zoomerSecond->setZoomBase(true);
    }
}

/**
 * @brief 构建默认的缩放器
 */
void DAChartWidget::setupZoomer()
{
    DA_D(DAChartWidget, d);
    if (d->_zoomer.isNull()) {
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
        d->_zoomer.reset(new QwtPlotZoomer(xBottom, yLeft, canvas()));
        d->_zoomer->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_I, Qt::ShiftModifier);
        d->_zoomer->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_O, Qt::ShiftModifier);
        d->_zoomer->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Home);
        d->_zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
        d->_zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
        d->_zoomer->setTrackerMode(QwtPicker::AlwaysOff);
        d->_zoomer->setZoomBase(false);
        d->_zoomer->setMaxStackDepth(20);
#endif
    }
    if (nullptr == d->_zoomerSecond) {
        d->_zoomerSecond.reset(new QwtPlotZoomer(xTop, yRight, canvas()));
        d->_zoomerSecond->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_I, Qt::ShiftModifier);
        d->_zoomerSecond->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_O, Qt::ShiftModifier);
        d->_zoomerSecond->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Home);
        d->_zoomerSecond->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
        d->_zoomerSecond->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
        d->_zoomerSecond->setTrackerMode(QwtPicker::AlwaysOff);
        d->_zoomerSecond->setZoomBase(false);
        d->_zoomerSecond->setMaxStackDepth(20);
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
        d_ptr->_zoomerSecond.reset(z);
    } else {
        d_ptr->_zoomer.reset(z);
    }
}

void DAChartWidget::deleteZoomer()
{
    DA_D(DAChartWidget, d);
    if (!d->_zoomer.isNull()) {
        d->_zoomer.reset();
    }
    if (!d->_zoomerSecond.isNull()) {
        d->_zoomerSecond.reset();
    }
}

///
/// \brief 设置缩放重置
///
void DAChartWidget::setZoomReset()
{
    DA_D(DAChartWidget, d);
    if (!d->_zoomer.isNull()) {
        d->_zoomer->zoom(0);
    }
    if (!d->_zoomerSecond.isNull()) {
        d->_zoomerSecond->zoom(0);
    }
}

/**
 * @brief 放大
 */
void DAChartWidget::zoomIn()
{
    DA_D(DAChartWidget, d);
    if (d->_zoomer.isNull()) {
        qWarning() << tr("Before zoom in, the chart must setup a zoomer");  // cn:在放大图表之前需要先建立缩放器
        return;
    }

    QRectF rect = d->_zoomer->zoomRect();
    double w    = rect.width() * 0.625;
    double h    = rect.height() * 0.625;
    double x    = rect.x() + (rect.width() - w) / 2.0;
    double y    = rect.y() + (rect.height() - h) / 2.0;

    rect.setX(x);
    rect.setY(y);
    rect.setWidth(w);
    rect.setHeight(h);
    if (rect.isValid()) {
        qDebug() << "zoom in from " << d->_zoomer->zoomRect() << " to " << rect;
        d->_zoomer->zoom(rect);
    } else {
        qDebug() << "zoom in get invalid zoom rect,current zoom rect is " << d->_zoomer->zoomRect() << ",will zoom rect is " << rect;
    }
}

/**
 * @brief 缩小
 */
void DAChartWidget::zoomOut()
{
    DA_D(DAChartWidget, d);
    if (d->_zoomer.isNull()) {
        qWarning() << tr("Before zoom out, the chart must setup a zoomer");  // cn:在缩小图表之前需要先建立缩放器
        return;
    }

    QRectF rect = d->_zoomer->zoomRect();
    double w    = rect.width() * 1.6;
    double h    = rect.height() * 1.6;
    double x    = rect.x() - (w - rect.width()) / 2.0;
    double y    = rect.y() - (h - rect.height()) / 2.0;

    rect.setX(x);
    rect.setY(y);
    rect.setWidth(w);
    rect.setHeight(h);
    if (rect.isValid()) {
        qDebug() << "zoom out from " << d->_zoomer->zoomRect() << " to " << rect;
        d->_zoomer->zoom(rect);
    } else {
        qDebug() << "zoom out get invalid zoom rect,current zoom rect is " << d->_zoomer->zoomRect()
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
    QwtPlotZoomer* zoomer = d_ptr->_zoomer.data();
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

    zoomer = d_ptr->_zoomerSecond.data();
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

void DAChartWidget::setupLegend()
{
    if (nullptr == d_ptr->_legend) {
        d_ptr->_legend = new QwtPlotLegendItem();
        d_ptr->_legend->setRenderHint(QwtPlotItem::RenderAntialiased);
        QColor color(Qt::white);
        d_ptr->_legend->setTextPen(color);
        d_ptr->_legend->setBorderPen(color);
        QColor c(Qt::gray);
        c.setAlpha(200);
        d_ptr->_legend->setBackgroundBrush(c);
        d_ptr->_legend->attach(this);
    }
}

/**
 * @brief 显示图例
 * @param enable
 */
void DAChartWidget::enableLegend(bool enable)
{
    if (isEnableLegend() == enable) {
        return;  //状态一致不动作
    }
    if (enable) {
        if (d_ptr->_legend) {
            d_ptr->_legend->setVisible(true);
        } else {
            setupLegend();
        }
    } else {
        if (d_ptr->_legend) {
            d_ptr->_legend->setVisible(false);
        }
    }
    emit enableLegendChanged(enable);
}

void DAChartWidget::enableLegendPanel(bool enable)
{
    if (isEnableLegendPanel() == enable) {
        return;  //状态一致不动作
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
    if (d_ptr->_legendPanel) {
        return;
    }
    d_ptr->_legendPanel = new QwtLegend;
    d_ptr->_legendPanel->setDefaultItemMode(QwtLegendData::Checkable);
    insertLegend(d_ptr->_legendPanel, QwtPlot::RightLegend);
    //	connect( _legendPanel, &QwtLegend::checked,&ChartWave_qwt::showItem);
    connect(d_ptr->_legendPanel, SIGNAL(checked(const QVariant&, bool, int)), SLOT(showItem(const QVariant&, bool)));

    QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);

    for (int i = 0; i < items.size(); i++) {
        const QVariant itemInfo     = itemToInfo(items[ i ]);
        QwtLegendLabel* legendLabel = qobject_cast< QwtLegendLabel* >(d_ptr->_legendPanel->legendWidget(itemInfo));
        if (legendLabel) {
            legendLabel->setChecked(items[ i ]->isVisible());
        }
    }
}

void DAChartWidget::deletelegendPanel()
{
    DA_D(DAChartWidget, d);
    insertLegend(nullptr);  //内部会销毁
    d->_legendPanel = nullptr;
}

void DAChartWidget::setupYDataPicker()
{
    if (nullptr == d_ptr->_yDataPicker) {
        d_ptr->_yDataPicker = new DAChartYDataPicker(this->canvas());
        d_ptr->_yDataPicker->setRubberBandPen(QPen("MediumOrchid"));
    }
}

void DAChartWidget::deleteYDataPicker()
{
    if (nullptr != d_ptr->_yDataPicker) {
        d_ptr->_yDataPicker->setParent(nullptr);  //断开和canvas()的父子连接
        delete d_ptr->_yDataPicker;
        d_ptr->_yDataPicker = nullptr;
    }
}

void DAChartWidget::setupXYDataPicker()
{
    if (nullptr == d_ptr->_xyDataPicker) {
        d_ptr->_xyDataPicker = new DAChartXYDataPicker(this->canvas());
        d_ptr->_xyDataPicker->setRubberBandPen(QPen("MediumOrchid"));
    }
}

void DAChartWidget::deleteXYDataPicker()
{
    DA_D(DAChartWidget, d);
    if (nullptr != d->_xyDataPicker) {
        d->_xyDataPicker->setParent(nullptr);  //断开和canvas()的父子连接
        delete d->_xyDataPicker;
        d->_xyDataPicker = nullptr;
    }
}

void DAChartWidget::enableYDataPicker(bool enable)
{
    if (isEnableYDataPicker() == enable) {
        return;  //状态一致不动作
    }
    if (enable) {
        setupYDataPicker();
    } else {
        deleteYDataPicker();
    }
    emit enableYDataPickerChanged(enable);
}

void DAChartWidget::enableXYDataPicker(bool enable)
{
    if (isEnableXYDataPicker() == enable) {
        return;  //状态一致不动作
    }
    if (enable) {
        setupXYDataPicker();
    } else {
        deleteXYDataPicker();
    }
    emit enableXYDataPickerChanged(enable);
}

/**
 * @brief plt.xlabel 设置xbottom轴标题
 * @param label
 */
void DAChartWidget::setXLabel(const QString& label)
{
    setAxisTitle(QwtPlot::xBottom, label);
}

/**
 * @brief plt.ylabel 设置yLeft轴标题
 * @param label
 */
void DAChartWidget::setYLabel(const QString& label)
{
    setAxisTitle(QwtPlot::yLeft, label);
}

///
/// \brief 是否允许缩放
/// \return
///
bool DAChartWidget::isEnableZoomer() const
{
    DA_DC(DAChartWidget, d);
    if (d->_zoomer) {
        return (d->_zoomer->isEnabled());
    }
    if (d->_zoomerSecond) {
        return (d->_zoomerSecond->isEnabled());
    }
    return (false);
}

///
/// \brief 是否允许十字光标
/// \return
///
bool DAChartWidget::isEnableCrossPicker() const
{
    DA_DC(DAChartWidget, d);
    if (d->_picker) {
        return (d->_picker->isEnabled());
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

QList< QwtPlotCurve* > DAChartWidget::getCurveList()
{
    QList< QwtPlotCurve* > curves;
    QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);

    for (int i(0); i < items.size(); ++i) {
        curves.append(static_cast< QwtPlotCurve* >(items[ i ]));
    }
    return (curves);
}

///
/// \brief getMakerList 获取所有标记
/// \return
///
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

    dateScale = new QwtDateScaleDraw;  //原来的scaleDraw会再qwt自动delete
    setAxisScaleDraw(axisID, dateScale);
    return (dateScale);
}
}  // End Of Namespace DA
