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
#include "qwt_colormap_preset.h"
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

/**
 * @brief 初始化私有数据，设置画布、布局和样式
 */
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

/**
 * @brief 设置自定义画布，配置光标和焦点策略
 */
void DAChartWidget::PrivateData::setupCanvas()
{
    // 创建自定义画布
    DAChartCanvas* canvas = new DAChartCanvas();
    canvas->setCursor(Qt::ArrowCursor);
    canvas->setFocusPolicy(Qt::ClickFocus);
    canvas->setFocusProxy(q_ptr);
    q_ptr->setCanvas(canvas);
}

/**
 * @brief 设置交互组件（延迟创建）
 */
void DAChartWidget::PrivateData::setupInteractions()
{
    // 交互组件在需要时延迟创建
}

// ==================== DAChartWidget 实现 ====================

/**
 * @brief 构造函数
 * @param parent 父窗口部件
 */
DAChartWidget::DAChartWidget(QWidget* parent) : QwtPlot(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->initialize();
    enableMouseWheelZoom(true);  // 默认开启滚轮缩放
}

/**
 * @brief 析构函数
 */
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
          << QwtPlotItem::Rtti_PlotBoxChart << QwtPlotItem::Rtti_PlotShape << QwtPlotItem::Rtti_PlotZone
          << QwtPlotItem::Rtti_PlotVectorField;
    return rttis;
}

/**
 * @brief 添加曲线
 * @param xData x轴数据
 * @param yData y轴数据
 * @param title 曲线标题
 * @return 创建的曲线指针，如果数据无效返回nullptr
 */
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

/**
 * @brief 添加曲线
 * @param points 点集合
 * @param title 曲线标题
 * @return 创建的曲线指针，如果点集合为空返回nullptr
 */
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

/**
 * @brief 获取所有曲线
 * @return 曲线列表
 */
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

/**
 * @brief 移除曲线
 * @param curve 要移除的曲线指针
 */
void DAChartWidget::removeCurve(QwtPlotCurve* curve)
{
    if (!curve)
        return;

    curve->detach();
    delete curve;
}

/**
 * @brief 添加散点图
 * @param points 点集合
 * @param title 散点图标题
 * @return 创建的曲线指针（散点样式），如果点集合为空返回nullptr
 */
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

/**
 * @brief 添加柱状图
 * @param values 值数组
 * @param title 柱状图标题
 * @return 创建的柱状图指针，如果值为空返回nullptr
 */
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

/**
 * @brief 添加柱状图
 * @param points 点集合
 * @param title 柱状图标题
 * @return 创建的柱状图指针，如果点集合为空返回nullptr
 */
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

/**
 * @brief 添加区间曲线
 * @param values 值数组
 * @param mins 最小值数组
 * @param maxs 最大值数组
 * @param title 区间曲线标题
 * @return 创建的区间曲线指针，如果数据无效返回nullptr
 */
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

/**
 * @brief 添加垂直线标记
 * @param x x轴位置
 * @param title 标记标题
 * @return 创建的标记指针
 */
QwtPlotMarker* DAChartWidget::addVerticalLine(double x, const QString& title)
{
    QwtPlotMarker* marker = new QwtPlotMarker(title);
    marker->setXValue(x);
    marker->setLineStyle(QwtPlotMarker::VLine);
    marker->setLinePen(QPen(Qt::black, 1, Qt::DashLine));
    marker->attach(this);
    return marker;
}

/**
 * @brief 添加水平线标记
 * @param y y轴位置
 * @param title 标记标题
 * @return 创建的标记指针
 */
QwtPlotMarker* DAChartWidget::addHorizontalLine(double y, const QString& title)
{
    QwtPlotMarker* marker = new QwtPlotMarker(title);
    marker->setYValue(y);
    marker->setLineStyle(QwtPlotMarker::HLine);
    marker->setLinePen(QPen(Qt::black, 1, Qt::DashLine));
    marker->attach(this);
    return marker;
}

/**
 * @brief 添加十字线标记
 * @param x x轴位置
 * @param y y轴位置
 * @param title 标记标题
 * @return 创建的标记指针
 */
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

/**
 * @brief 添加光谱图
 * @param gridData 网格栅格数据
 * @param title 光谱图标题
 * @return 创建的光谱图指针，如果网格数据为空返回nullptr
 */
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

/**
 * @brief 添加箱线图
 * @param samples 箱线图样本集合
 * @param title 箱线图标题
 * @return 创建的箱线图指针，如果样本为空返回nullptr
 */
QwtPlotBoxChart* DAChartWidget::addBoxChart(const QVector< QwtBoxSample >& samples, const QString& title)
{
    if (samples.isEmpty()) {
        qWarning() << "Empty samples for box chart:" << title;
        return nullptr;
    }

    QwtPlotBoxChart* boxChart = new QwtPlotBoxChart(title);
    boxChart->setSamples(samples);
    boxChart->setPen(QPen(Qt::blue, 1.0));
    boxChart->setBrush(QBrush(QColor(100, 149, 237, 128)));
    boxChart->setOrientation(Qt::Vertical);
    boxChart->setRenderHint(QwtPlotItem::RenderAntialiased);
    boxChart->attach(this);
    return boxChart;
}

/**
 * @brief 添加直方图
 * @param samples 区间样本集合
 * @param title 直方图标题
 * @return 创建的直方图指针，如果样本为空返回nullptr
 */
QwtPlotHistogram* DAChartWidget::addHistogram(const QVector< QwtIntervalSample >& samples, const QString& title)
{
    if (samples.isEmpty()) {
        qWarning() << "Empty samples for histogram:" << title;
        return nullptr;
    }

    QwtPlotHistogram* histogram = new QwtPlotHistogram(title);
    histogram->setSamples(samples);
    histogram->setPen(QPen(QColor(65, 105, 225)));
    histogram->setBrush(QBrush(QColor(65, 105, 225, 128)));
    histogram->setRenderHint(QwtPlotItem::RenderAntialiased);
    histogram->attach(this);
    return histogram;
}

/**
 * @brief 添加多柱状图
 * @param positions 位置数组
 * @param values 每组柱子的值集合
 * @param titles 每组柱子的标题列表
 * @return 创建的多柱状图指针，如果数据无效返回nullptr
 */
QwtPlotMultiBarChart* DAChartWidget::addMultiBarChart(
    const QVector< double >& positions, const QVector< QVector< double > >& values, const QStringList& titles)
{
    if (positions.isEmpty() || values.isEmpty()) {
        qWarning() << "Invalid data for multi bar chart:" << titles;
        return nullptr;
    }

    int numPositions = positions.size();
    int numSeries    = values.size();

    QVector< QwtSetSample > setSamples;
    setSamples.reserve(numPositions);
    for (int i = 0; i < numPositions; ++i) {
        QVector< double > set;
        set.reserve(numSeries);
        for (int j = 0; j < numSeries; ++j) {
            if (i < values[ j ].size()) {
                set.append(values[ j ][ i ]);
            } else {
                set.append(0.0);
            }
        }
        setSamples.append(QwtSetSample(positions[ i ], set));
    }

    QwtPlotMultiBarChart* multiBarChart = new QwtPlotMultiBarChart();
    multiBarChart->setSamples(setSamples);

    if (!titles.isEmpty()) {
        QList< QwtText > barTitles;
        for (const QString& t : titles) {
            barTitles.append(QwtText(t));
        }
        multiBarChart->setBarTitles(barTitles);
    }

    multiBarChart->setRenderHint(QwtPlotItem::RenderAntialiased);
    multiBarChart->attach(this);
    return multiBarChart;
}

/**
 * @brief 添加等高线图
 * @param points 三维点集合
 * @param levels 等高线级别数组
 * @param title 等高线图标题
 * @return 创建的等高线图指针，如果点集合为空返回nullptr
 */
QwtPlotSpectroCurve* DAChartWidget::addContour(
    const QVector< QwtPoint3D >& points, const QVector< double >& levels, const QString& title)
{
    if (points.isEmpty()) {
        qWarning() << "Empty points for contour:" << title;
        return nullptr;
    }

    QwtPlotSpectroCurve* curve = new QwtPlotSpectroCurve(title);
    curve->setSamples(points);
    curve->setPenWidth(1.0);
    curve->setColorMap(QwtColorMapPreset::create(QwtColorMapPreset::Viridis).release());
    if (!levels.isEmpty()) {
        curve->setColorRange(QwtInterval(levels.first(), levels.last()));
    }
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->attach(this);
    return curve;
}

/**
 * @brief 添加图形项
 * @param path 绘图路径
 * @param title 图形项标题
 * @return 创建的图形项指针
 */
QwtPlotShapeItem* DAChartWidget::addShapeItem(const QPainterPath& path, const QString& title)
{
    QwtPlotShapeItem* item = new QwtPlotShapeItem(title);
    item->setShape(path);
    item->setPen(QPen(Qt::NoPen));
    item->setBrush(QBrush(QColor(100, 149, 237, 128)));
    item->setRenderHint(QwtPlotItem::RenderAntialiased);
    item->attach(this);
    return item;
}

/**
 * @brief 移除绘图项
 * @param item 要移除的绘图项指针
 */
void DAChartWidget::removePlotItem(QwtPlotItem* item)
{
    if (!item) {
        return;
    }

    item->detach();
    delete item;
}

/**
 * @brief 清除所有数据相关的绘图项（保留网格、图例等显示元素）
 * @sa dataRttis
 */
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

/**
 * @brief 获取数据边界范围
 * @return 数据可见区域的矩形范围
 */
QRectF DAChartWidget::getDataBounds() const
{
    return DAChartUtil::getVisibleRegionRang(const_cast< DAChartWidget* >(this));
}

/**
 * @brief 判断图表中是否有数据
 * @return 如果有数据返回true，否则返回false
 */
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

/**
 * @brief 设置图表标题
 * @param title 图表标题文本
 */
void DAChartWidget::setChartTitle(const QString& title)
{
    setTitle(title);
    notifyPropertiesChanged(ChartTitleChanged);
}

/**
 * @brief 获取图表标题
 * @return 图表标题文本
 */
QString DAChartWidget::getChartTitle() const
{
    return title().text();
}

/**
 * @brief 设置背景画刷
 * @param brush 背景画刷
 */
void DAChartWidget::setBackgroundBrush(const QBrush& brush)
{
    d_ptr->backgroundBrush = brush;

    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, brush);
    setPalette(palette);
    notifyPropertiesChanged(BackgroundChanged);
}

/**
 * @brief 获取背景画刷
 * @return 背景画刷
 */
QBrush DAChartWidget::getBackgroundBrush() const
{
    return d_ptr->backgroundBrush;
}

/**
 * @brief 设置边框颜色
 * @param color 边框颜色
 */
void DAChartWidget::setBorderColor(const QColor& color)
{
    if (d_ptr->borderColor != color) {
        d_ptr->borderColor = color;
        update();
        notifyPropertiesChanged(BorderColorChanged);
    }
}

/**
 * @brief 获取边框颜色
 * @return 边框颜色
 */
QColor DAChartWidget::getBorderColor() const
{
    return d_ptr->borderColor;
}

/**
 * @brief 设置坐标轴标签
 * @param axisId 坐标轴ID
 * @param label 标签文本
 */
void DAChartWidget::setAxisLabel(int axisId, const QString& label)
{
    setAxisTitle(axisId, label);
    notifyPropertiesChanged(AxisLabelChanged);
}

/**
 * @brief 获取坐标轴标签
 * @param axisId 坐标轴ID
 * @return 标签文本
 */
QString DAChartWidget::getAxisLabel(int axisId) const
{
    return axisTitle(axisId).text();
}

/**
 * @brief 设置坐标轴颜色
 * @param axisId 坐标轴ID
 * @param color 颜色
 */
void DAChartWidget::setAxisColor(int axisId, const QColor& color)
{
    QwtScaleWidget* aw = this->axisWidget(axisId);
    if (aw) {
        aw->setScaleColor(color);
        notifyPropertiesChanged(AxisColorChanged);
    }
}

/**
 * @brief 获取坐标轴颜色
 * @param axisId 坐标轴ID
 * @return 坐标轴颜色，如果坐标轴不存在返回无效颜色
 */
QColor DAChartWidget::getAxisColor(int axisId) const
{
    const QwtScaleWidget* aw = this->axisWidget(axisId);
    return aw ? aw->scaleColor() : QColor();
}

/**
 * @brief 启用或禁用网格
 * @param enable 是否启用
 */
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

/**
 * @brief 设置网格轴启用状态的辅助函数
 * @param enable 是否启用
 * @param getCurrent 获取当前状态的回调函数
 * @param setEnabled 设置状态的回调函数
 */
void DAChartWidget::setGridAxisEnabled(bool enable,
                                       std::function< bool() > getCurrent,
                                       std::function< void(bool) > setEnabled)
{
    if (!d_ptr->grid && enable) {
        // 如果网格不存在但需要启用，先创建网格
        enableGrid(true);
    }

    if (d_ptr->grid) {
        bool currentState = getCurrent();
        if (currentState != enable) {
            setEnabled(enable);

            // 确保网格整体可见
            if (enable && !d_ptr->grid->isVisible()) {
                d_ptr->grid->setVisible(true);
            }

            notifyPropertiesChanged(GridStyleChanged);
        }
    }
}

/**
 * @brief 启用或禁用X轴网格
 * @param enable 是否启用
 */
void DAChartWidget::enableGridX(bool enable)
{
    setGridAxisEnabled(enable,
                       [ this ]() { return d_ptr->grid->xEnabled(); },
                       [ this ](bool e) { d_ptr->grid->enableX(e); });
}

/**
 * @brief 启用或禁用Y轴网格
 * @param enable 是否启用
 */
void DAChartWidget::enableGridY(bool enable)
{
    setGridAxisEnabled(enable,
                       [ this ]() { return d_ptr->grid->yEnabled(); },
                       [ this ](bool e) { d_ptr->grid->enableY(e); });
}

/**
 * @brief 启用或禁用X轴次网格
 * @param enable 是否启用
 */
void DAChartWidget::enableGridXMin(bool enable)
{
    setGridAxisEnabled(enable,
                       [ this ]() { return d_ptr->grid->xMinEnabled(); },
                       [ this ](bool e) { d_ptr->grid->enableXMin(e); });
}

/**
 * @brief 启用或禁用Y轴次网格
 * @param enable 是否启用
 */
void DAChartWidget::enableGridYMin(bool enable)
{
    setGridAxisEnabled(enable,
                       [ this ]() { return d_ptr->grid->yMinEnabled(); },
                       [ this ](bool e) { d_ptr->grid->enableYMin(e); });
}

/**
 * @brief 判断网格是否启用
 * @return 如果网格启用返回true，否则返回false
 */
bool DAChartWidget::isGridEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible();
}

/**
 * @brief 判断X轴网格是否启用
 * @return 如果X轴网格启用返回true，否则返回false
 */
bool DAChartWidget::isGridXEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->xEnabled();
}

/**
 * @brief 判断Y轴网格是否启用
 * @return 如果Y轴网格启用返回true，否则返回false
 */
bool DAChartWidget::isGridYEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->yEnabled();
}

/**
 * @brief 判断X轴次网格是否启用
 * @return 如果X轴次网格启用返回true，否则返回false
 */
bool DAChartWidget::isGridXMinEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->xMinEnabled();
}

/**
 * @brief 判断Y轴次网格是否启用
 * @return 如果Y轴次网格启用返回true，否则返回false
 */
bool DAChartWidget::isGridYMinEnabled() const
{
    return d_ptr->grid && d_ptr->grid->isVisible() && d_ptr->grid->yMinEnabled();
}

/**
 * @brief 设置网格样式
 * @param color 颜色
 * @param width 线宽
 * @param style 画笔样式
 * @param isMajor 是否为主网格线
 */
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

/**
 * @brief 设置主网格线样式
 * @param color 颜色
 * @param width 线宽
 * @param style 画笔样式
 */
void DAChartWidget::setGridMajorStyle(const QColor& color, qreal width, Qt::PenStyle style)
{
    setGridStyle(color, width, style, true);
}

/**
 * @brief 设置次网格线样式
 * @param color 颜色
 * @param width 线宽
 * @param style 画笔样式
 */
void DAChartWidget::setGridMinorStyle(const QColor& color, qreal width, Qt::PenStyle style)
{
    setGridStyle(color, width, style, false);
}

/**
 * @brief 启用或禁用图例
 * @param enable 是否启用
 */
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

/**
 * @brief 判断图例是否启用
 * @return 如果图例启用返回true，否则返回false
 */
bool DAChartWidget::isLegendEnabled() const
{
    return d_ptr->legend && d_ptr->legend->isVisible();
}

/**
 * @brief 设置图例位置
 * @param alignment 对齐方式
 */
void DAChartWidget::setLegendPosition(Qt::Alignment alignment)
{
    if (d_ptr->legend) {
        d_ptr->legend->setAlignmentInCanvas(alignment);
        notifyPropertiesChanged(LegendPositionChanged);
    }
}

/**
 * @brief 获取图例位置
 * @return 图例对齐方式，如果图例不存在返回右上角对齐
 */
Qt::Alignment DAChartWidget::getLegendPosition() const
{
    // QwtPlotLegendItem 没有直接的位置获取方法
    if (d_ptr->legend) {
        return d_ptr->legend->alignmentInCanvas();
    }
    return Qt::AlignRight | Qt::AlignTop;
}

/**
 * @brief 设置图例背景画刷
 * @param brush 背景画刷
 */
void DAChartWidget::setLegendBackground(const QBrush& brush)
{
    if (d_ptr->legend) {
        d_ptr->legend->setBackgroundBrush(brush);
        notifyPropertiesChanged(LegendBackgroundChanged);
    }
}

/**
 * @brief 获取图例背景画刷
 * @return 背景画刷，如果图例不存在返回空画刷
 */
QBrush DAChartWidget::getLegendBackground() const
{
    return d_ptr->legend ? d_ptr->legend->backgroundBrush() : QBrush();
}

/**
 * @brief 设置图例文字颜色
 * @param color 文字颜色
 */
void DAChartWidget::setLegendTextColor(const QColor& color)
{
    if (d_ptr->legend) {
        d_ptr->legend->setTextPen(QPen(color));
        notifyPropertiesChanged(LegendTextColorChanged);
    }
}

/**
 * @brief 获取图例文字颜色
 * @return 文字颜色，如果图例不存在返回无效颜色
 */
QColor DAChartWidget::getLegendTextColor() const
{
    return d_ptr->legend ? d_ptr->legend->textPen().color() : QColor();
}

/**
 * @brief 设置日期时间坐标轴
 * @param axisId 坐标轴ID
 * @param format 日期时间格式字符串
 */
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

/**
 * @brief 判断坐标轴是否为日期时间轴
 * @param axisId 坐标轴ID
 * @return 如果是日期时间轴返回true，否则返回false
 */
bool DAChartWidget::isDateTimeAxis(int axisId) const
{
    return dynamic_cast< const QwtDateScaleDraw* >(axisScaleDraw(axisId)) != nullptr;
}

// ==================== DAChartInteractionInterface 实现 ====================

/**
 * @brief 启用或禁用缩放
 * @param enable 是否启用
 */
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

/**
 * @brief 判断缩放是否启用
 * @return 如果缩放启用返回true，否则返回false
 */
bool DAChartWidget::isZoomEnabled() const
{
    return d_ptr->zoomer && d_ptr->zoomer->isEnabled();
}

/**
 * @brief 重置缩放到原始状态
 */
void DAChartWidget::zoomToOriginal()
{
    if (d_ptr->zoomer) {
        d_ptr->zoomer->setZoomBase();
        replot();
    }
}

/**
 * @brief 放大
 */
void DAChartWidget::zoomIn()
{
    if (!d_ptr->magnifier) {
        setupMagnifier();
    }

    if (d_ptr->magnifier) {
        d_ptr->magnifier->rescale(0.8);
    }
}

/**
 * @brief 缩小
 */
void DAChartWidget::zoomOut()
{
    if (!d_ptr->magnifier) {
        setupMagnifier();
    }

    if (d_ptr->magnifier) {
        d_ptr->magnifier->rescale(1.2);
    }
}

/**
 * @brief 获取缩放器
 * @return 缩放器指针
 */
QwtPlotCanvasZoomer* DAChartWidget::getZoomer() const
{
    return d_ptr->zoomer;
}

/**
 * @brief 启用或禁用平移
 * @param enable 是否启用
 */
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

/**
 * @brief 判断平移是否启用
 * @return 如果平移启用返回true，否则返回false
 */
bool DAChartWidget::isPanEnabled() const
{
    return d_ptr->panner && d_ptr->panner->isEnabled();
}

/**
 * @brief 获取平移器
 * @return 平移器指针
 */
QwtPlotPanner* DAChartWidget::getPanner() const
{
    return d_ptr->panner;
}

/**
 * @brief 启用或禁用十字光标
 * @param enable 是否启用
 */
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

/**
 * @brief 判断十字光标是否启用
 * @return 如果十字光标启用返回true，否则返回false
 */
bool DAChartWidget::isCrosshairEnabled() const
{
    return d_ptr->crosshair && d_ptr->crosshair->isEnabled();
}

/**
 * @brief 获取十字光标选择器
 * @return 十字光标选择器指针
 */
QwtPlotPicker* DAChartWidget::getCrosshair() const
{
    return d_ptr->crosshair;
}

/**
 * @brief 判断数据选取是否启用
 * @return 如果Y值选取或XY值选取启用返回true，否则返回false
 */
bool DAChartWidget::isDataPickingEnabled() const
{
    return isYValuePickingEnabled() || isXYValuePickingEnabled();
}

/**
 * @brief 启用或禁用Y值选取
 * @param enable 是否启用
 */
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

/**
 * @brief 判断Y值选取是否启用
 * @return 如果Y值选取启用返回true，否则返回false
 */
bool DAChartWidget::isYValuePickingEnabled() const
{
    return d_ptr->dataPicker && d_ptr->dataPicker->isEnabled()
           && d_ptr->dataPicker->pickMode() == QwtPlotSeriesDataPicker::PickYValue;
}

/**
 * @brief 启用或禁用XY值选取
 * @param enable 是否启用
 */
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

/**
 * @brief 判断XY值选取是否启用
 * @return 如果XY值选取启用返回true，否则返回false
 */
bool DAChartWidget::isXYValuePickingEnabled() const
{
    return d_ptr->dataPicker && d_ptr->dataPicker->isEnabled()
           && d_ptr->dataPicker->pickMode() == QwtPlotSeriesDataPicker::PickNearestPoint;
}

/**
 * @brief 获取数据选择器
 * @return 数据选择器指针
 */
QwtPlotSeriesDataPicker* DAChartWidget::getDataPicker() const
{
    return d_ptr->dataPicker;
}

/**
 * @brief 启用或禁用滚轮缩放
 * @param enable 是否启用
 */
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

/**
 * @brief 判断滚轮缩放是否启用
 * @return 如果滚轮缩放启用返回true，否则返回false
 */
bool DAChartWidget::isMouseWheelZoomEnabled() const
{
    return d_ptr->magnifier && d_ptr->magnifier->isEnabled();
}

/**
 * @brief 获取放大器
 * @return 放大器指针
 */
QwtPlotMagnifier* DAChartWidget::getMagnifier() const
{
    return d_ptr->magnifier;
}

/**
 * @brief 启用或禁用图例面板
 * @param enable 是否启用
 */
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

/**
 * @brief 判断图例面板是否启用
 * @return 如果图例面板启用返回true，否则返回false
 */
bool DAChartWidget::isLegendPanelEnabled() const
{
    return d_ptr->legendPanel && d_ptr->legendPanel->isVisible();
}

/**
 * @brief 获取图例面板
 * @return 图例面板指针
 */
QwtLegend* DAChartWidget::getLegendPanel() const
{
    return d_ptr->legendPanel;
}

/**
 * @brief 注册平移器工厂函数
 * @param factory 平移器工厂
 */
void DAChartWidget::registerPannerFactory(const PannerFactory& factory)
{
    d_ptr->pannerFactory = factory;
}

/**
 * @brief 注册选择器工厂函数
 * @param factory 选择器工厂
 */
void DAChartWidget::registerPickerFactory(const PickerFactory& factory)
{
    d_ptr->pickerFactory = factory;
}

/**
 * @brief 注册数据选择器工厂函数
 * @param factory 数据选择器工厂
 */
void DAChartWidget::registerDataPickerFactory(const DataPickerFactory& factory)
{
    d_ptr->dataPickerFactory = factory;
}

// ==================== 工具函数 ====================

/**
 * @brief 获取所属的Figure对象
 * @return Figure指针，如果不存在返回nullptr
 */
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

/**
 * @brief 获取所属的DAFigureWidget对象
 * @return DAFigureWidget指针，如果不存在返回nullptr
 */
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

/**
 * @brief 通知属性变更
 * @param flag 变更标志位
 */
void DAChartWidget::notifyPropertiesChanged(ChartPropertyChangeFlags flag)
{
    Q_EMIT chartPropertiesChanged(this, flag);
}

// ==================== 事件处理 ====================

/**
 * @brief 绘制事件处理，绘制边框并调用基类绘制
 * @param event 绘制事件
 */
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

/**
 * @brief 设置缩放器
 */
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

/**
 * @brief 设置平移器
 */
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

/**
 * @brief 设置放大器
 */
void DAChartWidget::setupMagnifier()
{
    if (d_ptr->magnifier) {
        d_ptr->magnifier->setEnabled(false);
        d_ptr->magnifier->deleteLater();
    }

    d_ptr->magnifier = new QwtPlotMagnifier(canvas());
}

/**
 * @brief 设置十字光标
 */
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

/**
 * @brief 设置数据选择器
 */
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

/**
 * @brief 设置图例面板
 */
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

/**
 * @brief 图例项切换状态回调
 * @param itemInfo 图例项信息
 * @param checked 是否选中
 */
void DAChartWidget::onLegendItemToggled(const QVariant& itemInfo, bool checked)
{
    QwtPlotItem* item = infoToItem(itemInfo);
    if (item) {
        item->setVisible(checked);
        replot();
    }
}

}  // namespace DA
