// DAChartPlotRenderer.cpp - Pure C++ renderer that operates DAChartWidget to render
// statistical plot items from Qwt native types and DAUtils composite structs.
//
// This class is the C++ counterpart of the Python ChartHandle binding in
// DAFigurePythonBinding.cpp. Instead of converting Python objects to Qwt types,
// it receives Qwt native types and DAPlotDataTypes structs directly.
//
// Key design:
//   - No pybind11 dependency — pure C++ only
//   - Each render method calls the corresponding DAChartWidget::addXxx method
//   - Style is a QVariantMap with optional keys; missing keys use hardcoded defaults
//   - Every render method returns QwtPlotItem* (may be nullptr on failure)
//   - The renderer does NOT call Python, does NOT do statistics — it only renders

#include "DAChartPlotRenderer.h"
#include "DAChartWidget.h"
#include "DAChartUtil.h"

// Qwt headers
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_boxchart.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_symbol.h"
#include "qwt_samples.h"
#include "qwt_point_3d.h"
#include "qwt_interval.h"
#include "qwt_grid_raster_data.h"
#include "qwt_colormap_preset.h"

// Qt headers
#include <QPainterPath>
#include <QBrush>
#include <QPen>

// ==================== Static style conversion helpers ====================

static QwtSymbol::Style stringToSymbolStyle(const QString& style)
{
    QString s = style.toLower();
    if (s == "ellipse") return QwtSymbol::Ellipse;
    if (s == "rect") return QwtSymbol::Rect;
    if (s == "diamond") return QwtSymbol::Diamond;
    if (s == "triangle") return QwtSymbol::Triangle;
    if (s == "dtriangle") return QwtSymbol::DTriangle;
    if (s == "utriangle") return QwtSymbol::UTriangle;
    if (s == "ltriangle") return QwtSymbol::LTriangle;
    if (s == "rtriangle") return QwtSymbol::RTriangle;
    if (s == "cross") return QwtSymbol::Cross;
    if (s == "xcross") return QwtSymbol::XCross;
    if (s == "star1") return QwtSymbol::Star1;
    if (s == "star2") return QwtSymbol::Star2;
    if (s == "hexagon") return QwtSymbol::Hexagon;
    if (s == "hline") return QwtSymbol::HLine;
    if (s == "vline") return QwtSymbol::VLine;
    return QwtSymbol::NoSymbol;
}

static QwtPlotCurve::CurveStyle stringToCurveStyle(const QString& style)
{
    QString s = style.toLower();
    if (s == "lines") return QwtPlotCurve::Lines;
    if (s == "steps") return QwtPlotCurve::Steps;
    if (s == "dots") return QwtPlotCurve::Dots;
    if (s == "sticks") return QwtPlotCurve::Sticks;
    if (s == "nocurve") return QwtPlotCurve::NoCurve;
    return QwtPlotCurve::Lines;
}

// ==================== Constructor / Destructor ====================

/**
 * @brief 构造函数，绑定目标图表控件
 * @param chart 关联的DAChartWidget指针
 */
DAChartPlotRenderer::DAChartPlotRenderer(DA::DAChartWidget* chart) : mChart(chart) {}

/**
 * @brief 析构函数
 */
DAChartPlotRenderer::~DAChartPlotRenderer() = default;

// ==================== Style fallback helpers ====================

/**
 * @brief 从样式映射中获取颜色值，不存在时返回默认值
 * @param style 样式映射表
 * @param key 键名
 * @param fallback 默认颜色
 * @return 样式中的颜色或默认值
 */
QColor DAChartPlotRenderer::defaultColor(const QVariantMap& style, const QString& key, const QColor& fallback) const
{
    auto it = style.constFind(key);
    if (it == style.constEnd()) return fallback;
    if (it->canConvert<QColor>()) return it->value<QColor>();
    return fallback;
}

/**
 * @brief 从样式映射中获取浮点数，不存在时返回默认值
 * @param style 样式映射表
 * @param key 键名
 * @param fallback 默认值
 * @return 样式中的浮点数或默认值
 */
double DAChartPlotRenderer::defaultDouble(const QVariantMap& style, const QString& key, double fallback) const
{
    auto it = style.constFind(key);
    if (it == style.constEnd()) return fallback;
    bool ok = false;
    double val = it->toDouble(&ok);
    return ok ? val : fallback;
}

/**
 * @brief 从样式映射中获取字符串，不存在时返回默认值
 * @param style 样式映射表
 * @param key 键名
 * @param fallback 默认值
 * @return 样式中的字符串或默认值
 */
QString DAChartPlotRenderer::defaultString(const QVariantMap& style, const QString& key, const QString& fallback) const
{
    auto it = style.constFind(key);
    if (it == style.constEnd()) return fallback;
    return it->toString();
}

/**
 * @brief 从样式映射中获取整数，不存在时返回默认值
 * @param style 样式映射表
 * @param key 键名
 * @param fallback 默认值
 * @return 样式中的整数或默认值
 */
int DAChartPlotRenderer::defaultInt(const QVariantMap& style, const QString& key, int fallback) const
{
    auto it = style.constFind(key);
    if (it == style.constEnd()) return fallback;
    bool ok = false;
    int val = it->toInt(&ok);
    return ok ? val : fallback;
}

// ==================== Style application helpers ====================

/**
 * @brief 为绘图项设置画笔
 * @param item 目标绘图项
 * @param color 画笔颜色
 * @param width 画笔宽度
 */
void DAChartPlotRenderer::applyPen(QwtPlotItem* item, const QColor& color, double width)
{
    if (!item) return;
    QPen pen(color, width);
    if (auto* curve = dynamic_cast<QwtPlotCurve*>(item)) {
        curve->setPen(pen);
    } else if (auto* bar = dynamic_cast<QwtPlotBarChart*>(item)) {
        bar->setPen(pen);
    } else if (auto* box = dynamic_cast<QwtPlotBoxChart*>(item)) {
        box->setPen(pen);
    } else if (auto* hist = dynamic_cast<QwtPlotHistogram*>(item)) {
        hist->setPen(pen);
    } else if (auto* interval = dynamic_cast<QwtPlotIntervalCurve*>(item)) {
        interval->setPen(pen);
    } else if (auto* shape = dynamic_cast<QwtPlotShapeItem*>(item)) {
        shape->setPen(pen);
    }
}

/**
 * @brief 为绘图项设置画刷
 * @param item 目标绘图项
 * @param color 画刷颜色
 */
void DAChartPlotRenderer::applyBrush(QwtPlotItem* item, const QColor& color)
{
    if (!item) return;
    QBrush brush(color);
    if (auto* curve = dynamic_cast<QwtPlotCurve*>(item)) {
        curve->setBrush(brush);
    } else if (auto* bar = dynamic_cast<QwtPlotBarChart*>(item)) {
        bar->setBrush(brush);
    } else if (auto* hist = dynamic_cast<QwtPlotHistogram*>(item)) {
        hist->setBrush(brush);
    } else if (auto* box = dynamic_cast<QwtPlotBoxChart*>(item)) {
        box->setBrush(brush);
    } else if (auto* shape = dynamic_cast<QwtPlotShapeItem*>(item)) {
        shape->setBrush(brush);
    }
}

/**
 * @brief 为绘图项设置符号样式
 * @param item 目标绘图项
 * @param symbolStyle 符号样式名称
 * @param size 符号大小
 * @param color 符号颜色
 */
void DAChartPlotRenderer::applySymbol(QwtPlotItem* item, const QString& symbolStyle, int size, const QColor& color)
{
    if (!item) return;
    QwtSymbol::Style symStyle = stringToSymbolStyle(symbolStyle);
    if (symStyle == QwtSymbol::NoSymbol) return;
    if (auto* curve = dynamic_cast<QwtPlotCurve*>(item)) {
        QwtSymbol* symbol = new QwtSymbol(symStyle, QBrush(color), QPen(color), QSize(size, size));
        curve->setSymbol(symbol);
    }
}

// ==================== Basic primitives ====================

/**
 * @brief 渲染曲线
 * @param points 曲线数据点
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderCurve(const QPolygonF& points, const QVariantMap& style)
{
    if (!mChart || points.isEmpty()) return nullptr;

    QString title = defaultString(style, "title", QString());

    // Convert QPolygonF to QVector<QPointF> for DAChartWidget::addCurve
    QVector<QPointF> pts;
    pts.reserve(points.size());
    for (const auto& p : points) {
        pts.append(p);
    }

    QwtPlotCurve* curve = mChart->addCurve(pts, title);
    if (!curve) return nullptr;

    QColor color     = defaultColor(style, "color", QColor(0, 100, 200));
    double width     = defaultDouble(style, "width", 1.5);
    applyPen(curve, color, width);

    // Optional fill
    if (style.contains("fillColor")) {
        QColor fillColor = defaultColor(style, "fillColor", QColor(0, 100, 200, 80));
        applyBrush(curve, fillColor);
    }

    // Optional symbol
    QString symStyle = defaultString(style, "symbol", QString());
    if (!symStyle.isEmpty()) {
        int symSize = defaultInt(style, "symbolSize", 8);
        applySymbol(curve, symStyle, symSize, color);
    }

    // Optional curve style (default is Lines)
    QString curveStyle = defaultString(style, "curveStyle", QString());
    if (!curveStyle.isEmpty()) {
        curve->setStyle(stringToCurveStyle(curveStyle));
    }

    return static_cast<QwtPlotItem*>(curve);
}

/**
 * @brief 渲染散点图
 * @param points 散点数据
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderScatter(const QVector<QPointF>& points, const QVariantMap& style)
{
    if (!mChart || points.isEmpty()) return nullptr;

    QString title = defaultString(style, "title", QString());

    QwtPlotCurve* curve = mChart->addScatter(points, title);
    if (!curve) return nullptr;

    QColor color = defaultColor(style, "color", QColor(200, 50, 50));
    double width = defaultDouble(style, "width", 1.0);
    applyPen(curve, color, width);

    // Scatter uses symbols by default
    QString symStyle = defaultString(style, "symbol", "Ellipse");
    int symSize      = defaultInt(style, "symbolSize", 6);
    applySymbol(curve, symStyle, symSize, color);

    return static_cast<QwtPlotItem*>(curve);
}

/**
 * @brief 渲染直方图
 * @param bins 区间边界数组
 * @param counts 每个区间的计数值
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderHistogram(const QVector<double>& bins,
                                                   const QVector<double>& counts,
                                                   const QVariantMap& style)
{
    if (!mChart || bins.size() < 2 || counts.size() != bins.size() - 1) return nullptr;

    QString title = defaultString(style, "title", QString());

    // Convert bin edges + counts to QwtIntervalSample
    // Each sample: QwtIntervalSample(value=count[i], interval=[bins[i], bins[i+1]])
    QVector<QwtIntervalSample> samples;
    samples.reserve(counts.size());
    for (int i = 0; i < counts.size(); ++i) {
        samples.append(QwtIntervalSample(counts[i], bins[i], bins[i + 1]));
    }

    QwtPlotHistogram* hist = mChart->addHistogram(samples, title);
    if (!hist) return nullptr;

    QColor color = defaultColor(style, "color", QColor(100, 150, 200));
    double width = defaultDouble(style, "width", 1.0);
    applyPen(hist, color, width);

    QColor fillColor = defaultColor(style, "fillColor", QColor(100, 150, 200, 120));
    applyBrush(hist, fillColor);

    return static_cast<QwtPlotItem*>(hist);
}

/**
 * @brief 渲染区间曲线
 * @param data 区间曲线数据
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderIntervalCurve(const DA::DAIntervalCurveData& data, const QVariantMap& style)
{
    if (!mChart || data.x.isEmpty()) return nullptr;
    if (data.yLower.size() != data.x.size() || data.yUpper.size() != data.x.size()) return nullptr;

    QString title = defaultString(style, "title", QString());

    // DAChartWidget::addIntervalCurve takes (values, mins, maxs)
    //   values = x coordinates, mins = lower bounds, maxs = upper bounds
    QwtPlotIntervalCurve* curve = mChart->addIntervalCurve(data.x, data.yLower, data.yUpper, title);
    if (!curve) return nullptr;

    QColor color = defaultColor(style, "color", QColor(150, 150, 150, 180));
    double width = defaultDouble(style, "width", 1.0);
    applyPen(curve, color, width);

    // Optional fill for the interval area
    if (style.contains("fillColor")) {
        QColor fillColor = defaultColor(style, "fillColor", QColor(150, 150, 150, 80));
        curve->setBrush(QBrush(fillColor));
    }

    return static_cast<QwtPlotItem*>(curve);
}

/**
 * @brief 渲染多边形形状
 * @param polygon 多边形顶点
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderShape(const QPolygonF& polygon, const QVariantMap& style)
{
    if (!mChart || polygon.isEmpty()) return nullptr;

    QString title = defaultString(style, "title", QString());

    // Convert QPolygonF to QPainterPath
    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();

    QwtPlotShapeItem* shape = mChart->addShapeItem(path, title);
    if (!shape) return nullptr;

    QColor penColor = defaultColor(style, "color", QColor(0, 0, 0));
    double width    = defaultDouble(style, "width", 1.0);
    applyPen(shape, penColor, width);

    QColor fillColor = defaultColor(style, "fillColor", QColor(200, 200, 200, 100));
    applyBrush(shape, fillColor);

    return static_cast<QwtPlotItem*>(shape);
}

// ==================== Composite types ====================

/**
 * @brief 渲染箱线图
 * @param data 箱线图数据
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderBoxChart(const DA::DABoxPlotData& data, const QVariantMap& style)
{
    if (!mChart || data.positions.isEmpty()) return nullptr;

    QString title = defaultString(style, "title", QString());
    int n        = data.positions.size();

    // Convert DABoxPlotData to QVector<QwtBoxSample>
    // QwtBoxSample(position, whiskerLower, q1, median, q3, whiskerUpper)
    QVector<QwtBoxSample> samples;
    samples.reserve(n);
    for (int i = 0; i < n; ++i) {
        samples.append(QwtBoxSample(
            data.positions[i],
            (i < data.whiskerLower.size()) ? data.whiskerLower[i] : 0.0,
            (i < data.q1.size()) ? data.q1[i] : 0.0,
            (i < data.median.size()) ? data.median[i] : 0.0,
            (i < data.q3.size()) ? data.q3[i] : 0.0,
            (i < data.whiskerUpper.size()) ? data.whiskerUpper[i] : 0.0
        ));
    }

    QwtPlotBoxChart* box = mChart->addBoxChart(samples, title);
    if (!box) return nullptr;

    QColor color = defaultColor(style, "color", QColor(100, 100, 200));
    double width = defaultDouble(style, "width", 1.0);
    applyPen(box, color, width);

    QColor fillColor = defaultColor(style, "fillColor", QColor(200, 200, 230, 150));
    applyBrush(box, fillColor);

    // Handle outliers: render as scatter points (attached to chart, not returned)
    for (const auto& outlierGroup : data.outliers) {
        if (!outlierGroup.isEmpty()) {
            mChart->addScatter(outlierGroup, QString());
        }
    }

    return static_cast<QwtPlotItem*>(box);
}

/**
 * @brief 渲染柱状图
 * @param data 柱状图数据
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderBarChart(const DA::DABarChartData& data, const QVariantMap& style)
{
    if (!mChart || data.values.isEmpty()) return nullptr;

    QString title = defaultString(style, "title", QString());

    QwtPlotBarChart* bar = mChart->addBarChart(data.values, title);
    if (!bar) return nullptr;

    QColor color = defaultColor(style, "color", QColor(70, 130, 180));
    double width = defaultDouble(style, "width", 1.0);
    applyPen(bar, color, width);

    QColor fillColor = defaultColor(style, "fillColor", QColor(70, 130, 180, 150));
    applyBrush(bar, fillColor);

    // If CI bounds are present, add error bars as interval curve
    if (!data.ciLower.isEmpty() && !data.ciUpper.isEmpty() &&
        data.ciLower.size() == data.values.size() &&
        data.ciUpper.size() == data.values.size()) {
        // Build x positions (0, 1, 2, ...) for the interval curve
        QVector<double> positions;
        positions.reserve(data.values.size());
        for (int i = 0; i < data.values.size(); ++i) {
            positions.append(static_cast<double>(i));
        }
        mChart->addIntervalCurve(positions, data.ciLower, data.ciUpper, title.isEmpty() ? QString() : (title + " (CI)"));
    }

    return static_cast<QwtPlotItem*>(bar);
}

/**
 * @brief 渲染光谱图
 * @param data 光谱图数据
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderSpectrogram(const DA::DASpectrogramData& data, const QVariantMap& style)
{
    if (!mChart || data.values.isEmpty() || data.nrows <= 0 || data.ncols <= 0) return nullptr;

    QString title = defaultString(style, "title", QString());

    // Build axis vectors (evenly spaced between min and max)
    QVector<double> xVec(data.ncols), yVec(data.nrows);
    for (int i = 0; i < data.ncols; ++i) {
        if (data.ncols > 1)
            xVec[i] = data.xmin + (data.xmax - data.xmin) * i / (data.ncols - 1);
        else
            xVec[i] = data.xmin;
    }
    for (int i = 0; i < data.nrows; ++i) {
        if (data.nrows > 1)
            yVec[i] = data.ymin + (data.ymax - data.ymin) * i / (data.nrows - 1);
        else
            yVec[i] = data.ymin;
    }

    // Build value matrix: QwtGridRasterData expects v[x][y] = z
    // DASpectrogramData.values is flat, assumed row-major: values[row * ncols + col]
    // Transpose: v[col][row] = values[row * ncols + col]
    QVector<QVector<double>> v(data.ncols);
    for (int col = 0; col < data.ncols; ++col) {
        v[col].resize(data.nrows);
        for (int row = 0; row < data.nrows; ++row) {
            int idx = row * data.ncols + col;
            v[col][row] = (idx < data.values.size()) ? data.values[idx] : 0.0;
        }
    }

    QwtGridRasterData* rasterData = new QwtGridRasterData();
    rasterData->setValue(xVec, yVec, v);

    QwtPlotSpectrogram* spectro = mChart->addSpectrogram(rasterData, title);
    if (!spectro) {
        delete rasterData;
        return nullptr;
    }

    // Set color map if specified
    QString cmapName = defaultString(style, "cmap", QString());
    if (!cmapName.isEmpty()) {
        auto cmap = QwtColorMapPreset::create(cmapName);
        if (cmap) {
            spectro->setColorMap(cmap.release());
        }
    }

    return static_cast<QwtPlotItem*>(spectro);
}

/**
 * @brief 渲染等高线
 * @param data 等高线数据
 * @param style 样式映射表
 * @return 创建的QwtPlotItem指针，失败返回nullptr
 */
QwtPlotItem* DAChartPlotRenderer::renderContours(const DA::DAContourData& data, const QVariantMap& style)
{
    if (!mChart || data.polygons.isEmpty()) return nullptr;

    QString title = defaultString(style, "title", QString());

    // DAContourData contains pre-computed contour polygons with levels.
    // Combine all polygons into a single QPainterPath and render as a shape item.
    QPainterPath path;
    for (int i = 0; i < data.polygons.size(); ++i) {
        const QPolygonF& poly = data.polygons[i];
        if (poly.isEmpty()) continue;
        path.moveTo(poly[0]);
        for (int j = 1; j < poly.size(); ++j) {
            path.lineTo(poly[j]);
        }
        path.closeSubpath();
    }

    QwtPlotShapeItem* shape = mChart->addShapeItem(path, title);
    if (!shape) return nullptr;

    QColor penColor = defaultColor(style, "color", QColor(0, 0, 200));
    double width    = defaultDouble(style, "width", 1.0);
    applyPen(shape, penColor, width);

    // Contours are typically unfilled lines, but can be filled if requested
    if (style.contains("fillColor")) {
        QColor fillColor = defaultColor(style, "fillColor", QColor(200, 200, 200, 100));
        applyBrush(shape, fillColor);
    }

    return static_cast<QwtPlotItem*>(shape);
}

// ==================== Chart metadata ====================

/**
 * @brief 设置图表标题
 * @param title 标题文本
 */
void DAChartPlotRenderer::setChartTitle(const QString& title)
{
    if (!mChart) return;
    mChart->setChartTitle(title);
}

/**
 * @brief 设置坐标轴标签
 * @param axis 坐标轴编号
 * @param label 标签文本
 */
void DAChartPlotRenderer::setAxisLabel(int axis, const QString& label)
{
    if (!mChart) return;
    mChart->setAxisLabel(axis, label);
}

/**
 * @brief 设置坐标轴为分类刻度
 * @param axis 坐标轴编号
 * @param tickPositions 刻度位置数组
 * @param labels 刻度标签列表
 * @param dataLower 数据下界
 * @param dataUpper 数据上界
 */
void DAChartPlotRenderer::setAxisCategoryScale(int axis, const QVector<double>& tickPositions,
                                                const QStringList& labels, double dataLower, double dataUpper)
{
    if (!mChart) return;
    DA::DAChartUtil::setAxisCategoryScale(mChart, axis, tickPositions, labels, dataLower, dataUpper);
}

/**
 * @brief 设置底部X轴为分类刻度
 * @param tickPositions 刻度位置数组
 * @param labels 刻度标签列表
 * @param dataLower 数据下界
 * @param dataUpper 数据上界
 */
void DAChartPlotRenderer::setXBottomCategoryScale(const QVector<double>& tickPositions, const QStringList& labels,
                                                  double dataLower, double dataUpper)
{
    if (!mChart) return;
    DA::DAChartUtil::setAxisCategoryScale(mChart, QwtPlot::xBottom, tickPositions, labels, dataLower, dataUpper);
}

/**
 * @brief 启用或禁用网格
 * @param on 是否启用
 */
void DAChartPlotRenderer::enableGrid(bool on)
{
    if (!mChart) return;
    mChart->enableGrid(on);
}

/**
 * @brief 启用或禁用图例
 * @param on 是否启用
 */
void DAChartPlotRenderer::enableLegend(bool on)
{
    if (!mChart) return;
    mChart->enableLegend(on);
}

/**
 * @brief 刷新图表重绘
 */
void DAChartPlotRenderer::replot()
{
    if (!mChart) return;
    mChart->replot();
}
