#include "DAChartWidget.h"
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
#include "qwt_figure.h"
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

QwtPlotIntervalCurve* DAChartWidget::addIntervalCurve(
    const QVector< double >& values, const QVector< double >& mins, const QVector< double >& maxs, const QString& title
)
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

QwtFigure* DAChartWidget::figure() const
{
    QWidget* parent = const_cast< DAChartWidget* >(this);
    while (QWidget* grandParent = parent->parentWidget()) {
        if (QwtFigure* figure = qobject_cast< QwtFigure* >(grandParent)) {
            return figure;
        }
        parent = grandParent;
    }
    return nullptr;
}

DAFigureWidget* DA::DAChartWidget::figureWidget() const
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