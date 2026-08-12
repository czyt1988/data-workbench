// DAFigurePythonBinding.cpp - pybind11 binding for the da_figure embedded module
//
// This module exposes DAFigure's chart operations (DAChartDataInterface + DAChartStyleInterface)
// to Python. Python statistical plotting functions call through this module to use Qwt rendering.
//
// Architecture:
//   - ChartHandle: wraps a non-owning DAChartWidget*, delegates all calls to it
//   - PlotItem: opaque pybind11 wrapper for QwtPlotItem* (returned by addXxx methods)
//   - Module-level functions: getCurrentChart(), setPen(), setBrush(), setSymbol(), setCurveStyle()

// DAPybind11QtCaster.hpp includes DAPybind11InQt.h as its first header,
// which resolves the Qt "slots" vs pybind11 conflict.
#include "DAPybind11QtCaster.hpp"

// DAFigure headers
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartDataInterface.h"
#include "DAChartStyleInterface.h"
#include "DAFigurePythonBinding.h"

// Qwt headers
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_boxchart.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_symbol.h"
#include "qwt_samples.h"
#include "qwt_point_3d.h"
#include "qwt_interval.h"
#include "qwt_colormap_preset.h"
#include "qwt_grid_raster_data.h"

// Qt headers
#include <QPainterPath>
#include <QBrush>
#include <QPen>

namespace da_figure
{

// ==================== Callback mechanism ====================

static GetCurrentChartFn g_currentChartGetter = nullptr;

/**
 * @brief 设置获取当前图表控件的回调函数
 * @param fn 获取当前图表控件的回调函数指针
 * @sa getCurrentChartWidget()
 */
void setCurrentChartGetter(GetCurrentChartFn fn)
{
    g_currentChartGetter = fn;
}

/**
 * @brief 获取当前图表控件
 * @return 返回当前图表控件指针，若未设置回调则返回nullptr
 * @sa setCurrentChartGetter()
 */
DA::DAChartWidget* getCurrentChartWidget()
{
    if (!g_currentChartGetter) return nullptr;
    return g_currentChartGetter();
}

// ==================== ChartHandle class ====================
//
// Wraps a non-owning DA::DAChartWidget* pointer. All methods delegate to the
// underlying chart widget via the DAChartDataInterface and DAChartStyleInterface.

class ChartHandle
{
protected:
    DA::DAChartWidget* mChart;

public:
    ChartHandle(DA::DAChartWidget* chart = nullptr) : mChart(chart) {}

    bool isValid() const { return mChart != nullptr; }
    DA::DAChartWidget* chart() const { return mChart; }

    // ==================== DAChartDataInterface methods ====================

    QwtPlotItem* addCurveXY(const QVector< double >& x, const QVector< double >& y, const QString& title)
    {
        if (!isValid()) return nullptr;
        return static_cast< QwtPlotItem* >(mChart->addCurve(x, y, title));
    }

    QwtPlotItem* addCurvePoints(const QVector< QPointF >& points, const QString& title)
    {
        if (!isValid()) return nullptr;
        return static_cast< QwtPlotItem* >(mChart->addCurve(points, title));
    }

    QwtPlotItem* addScatter(const QVector< QPointF >& points, const QString& title)
    {
        if (!isValid()) return nullptr;
        return static_cast< QwtPlotItem* >(mChart->addScatter(points, title));
    }

    QwtPlotItem* addBarChart(const QVector< double >& values, const QString& title)
    {
        if (!isValid()) return nullptr;
        return static_cast< QwtPlotItem* >(mChart->addBarChart(values, title));
    }

    QwtPlotItem* addIntervalCurve(const QVector< double >& values,
                                   const QVector< double >& mins,
                                   const QVector< double >& maxs,
                                   const QString& title)
    {
        if (!isValid()) return nullptr;
        return static_cast< QwtPlotItem* >(mChart->addIntervalCurve(values, mins, maxs, title));
    }

    QwtPlotItem* addBoxChart(const pybind11::list& samples, const QString& title)
    {
        if (!isValid()) return nullptr;
        QVector< QwtBoxSample > boxSamples = convertToBoxSamples(samples);
        return static_cast< QwtPlotItem* >(mChart->addBoxChart(boxSamples, title));
    }

    QwtPlotItem* addHistogram(const pybind11::list& samples, const QString& title)
    {
        if (!isValid()) return nullptr;
        QVector< QwtIntervalSample > histSamples = convertToIntervalSamples(samples);
        return static_cast< QwtPlotItem* >(mChart->addHistogram(histSamples, title));
    }

    QwtPlotItem* addMultiBarChart(const QVector< double >& positions,
                                   const pybind11::list& values,
                                   const pybind11::list& titles)
    {
        if (!isValid()) return nullptr;
        QVector< QVector< double > > cppValues = convertToMultiBarValues(values);
        QStringList cppTitles;
        for (auto item : titles) {
            cppTitles.append(QString::fromStdString(item.cast<std::string>()));
        }
        return static_cast< QwtPlotItem* >(mChart->addMultiBarChart(positions, cppValues, cppTitles));
    }

    QwtPlotItem* addContour(const pybind11::list& points,
                            const QVector< double >& levels,
                            const QString& title)
    {
        if (!isValid()) return nullptr;
        QVector< QwtPoint3D > contourPoints = convertToPoint3D(points);
        return static_cast< QwtPlotItem* >(mChart->addContour(contourPoints, levels, title));
    }

    QwtPlotItem* addShapeItem(const pybind11::list& polygon, const QString& title)
    {
        if (!isValid()) return nullptr;
        QPainterPath path = convertToPainterPath(polygon);
        return static_cast< QwtPlotItem* >(mChart->addShapeItem(path, title));
    }

    QwtPlotItem* addVerticalLine(double x, const QString& title)
    {
        if (!isValid()) return nullptr;
        return static_cast< QwtPlotItem* >(mChart->addVerticalLine(x, title));
    }

    QwtPlotItem* addHorizontalLine(double y, const QString& title)
    {
        if (!isValid()) return nullptr;
        return static_cast< QwtPlotItem* >(mChart->addHorizontalLine(y, title));
    }

    QwtPlotItem* addSpectrogram(const pybind11::dict& gridData, const QString& title)
    {
        if (!isValid()) return nullptr;
        QwtGridRasterData* rasterData = convertToRasterData(gridData);
        if (!rasterData) return nullptr;
        QwtPlotSpectrogram* spectro = mChart->addSpectrogram(rasterData, title);
        // Set color map if specified
        if (gridData.contains("cmap")) {
            pybind11::object cmapObj = gridData["cmap"];
            if (pybind11::isinstance< pybind11::str >(cmapObj)) {
                QString cmapName = cmapObj.cast< QString >();
                auto cmap = QwtColorMapPreset::create(cmapName);
                if (cmap) {
                    spectro->setColorMap(cmap.release());
                }
            }
        }
        return static_cast< QwtPlotItem* >(spectro);
    }

    void removePlotItem(QwtPlotItem* item)
    {
        if (!isValid() || !item) return;
        mChart->removePlotItem(item);
    }

    void clearAllData()
    {
        if (!isValid()) return;
        mChart->clearAllData();
    }

    // ==================== DAChartStyleInterface methods ====================

    void setChartTitle(const QString& title)
    {
        if (!isValid()) return;
        mChart->setChartTitle(title);
    }

    void setAxisLabel(const QString& axis, const QString& label)
    {
        if (!isValid()) return;
        int axisId = axisNameToId(axis);
        if (axisId < 0) return;
        mChart->setAxisLabel(axisId, label);
    }

    void enableGrid(bool on)
    {
        if (!isValid()) return;
        mChart->enableGrid(on);
    }

    void enableLegend(bool on)
    {
        if (!isValid()) return;
        mChart->enableLegend(on);
    }

    void setBackgroundBrush(const QColor& color)
    {
        if (!isValid()) return;
        mChart->setBackgroundBrush(QBrush(color));
    }

    // ==================== Refresh ====================

    void replot()
    {
        if (!isValid()) return;
        mChart->replot();
    }

    void autoScale()
    {
        if (!isValid()) return;
        mChart->setAxisAutoScale(QwtPlot::xBottom);
        mChart->setAxisAutoScale(QwtPlot::yLeft);
        mChart->setAxisAutoScale(QwtPlot::xTop);
        mChart->setAxisAutoScale(QwtPlot::yRight);
    }

private:
    // ==================== Conversion helpers ====================

    static int axisNameToId(const QString& axis)
    {
        QString lower = axis.toLower();
        if (lower == "yleft" || lower == "y_left") return 0;    // QwtPlot::yLeft
        if (lower == "yright" || lower == "y_right") return 1;   // QwtPlot::yRight
        if (lower == "xbottom" || lower == "x_bottom") return 2; // QwtPlot::xBottom
        if (lower == "xtop" || lower == "x_top") return 3;       // QwtPlot::xTop
        return -1;
    }

    // Convert pybind11::list of dicts to QVector<QwtBoxSample>
    // Each dict has keys: position, whiskerLower, q1, median, q3, whiskerUpper
    static QVector< QwtBoxSample > convertToBoxSamples(const pybind11::list& samples)
    {
        QVector< QwtBoxSample > result;
        result.reserve(static_cast< int >(samples.size()));
        for (auto item : samples) {
            if (!pybind11::isinstance< pybind11::dict >(item)) continue;
            pybind11::dict d = item.cast< pybind11::dict >();
            double position     = dictGetDouble(d, "position", 0.0);
            double whiskerLower = dictGetDouble(d, "whiskerLower", 0.0);
            double q1           = dictGetDouble(d, "q1", 0.0);
            double median       = dictGetDouble(d, "median", 0.0);
            double q3           = dictGetDouble(d, "q3", 0.0);
            double whiskerUpper = dictGetDouble(d, "whiskerUpper", 0.0);
            result.append(QwtBoxSample(position, whiskerLower, q1, median, q3, whiskerUpper));
        }
        return result;
    }

    // Convert pybind11::list of dicts to QVector<QwtIntervalSample>
    // Each dict has keys: value, interval=[lo,hi]
    static QVector< QwtIntervalSample > convertToIntervalSamples(const pybind11::list& samples)
    {
        QVector< QwtIntervalSample > result;
        result.reserve(static_cast< int >(samples.size()));
        for (auto item : samples) {
            if (!pybind11::isinstance< pybind11::dict >(item)) continue;
            pybind11::dict d = item.cast< pybind11::dict >();
            double value = dictGetDouble(d, "value", 0.0);
            // interval can be a list [lo, hi] or a dict with min/max
            double lo = 0.0, hi = 0.0;
            pybind11::object intervalObj = d["interval"];
            if (intervalObj.is_none()) continue;
            if (pybind11::isinstance< pybind11::list >(intervalObj)) {
                pybind11::list intervalList = intervalObj.cast< pybind11::list >();
                if (intervalList.size() >= 2) {
                    lo = intervalList[0].cast< double >();
                    hi = intervalList[1].cast< double >();
                }
            } else if (pybind11::isinstance< pybind11::tuple >(intervalObj)) {
                pybind11::tuple intervalTuple = intervalObj.cast< pybind11::tuple >();
                if (intervalTuple.size() >= 2) {
                    lo = intervalTuple[0].cast< double >();
                    hi = intervalTuple[1].cast< double >();
                }
            }
            result.append(QwtIntervalSample(value, lo, hi));
        }
        return result;
    }

    // Convert pybind11::list of lists to QVector<QVector<double>>
    // Python layout: values[group][category] (outer=groups, inner=per-category values)
    // C++ layout for QwtPlotMultiBarChart::setSamples: values[category] = set of all group values
    // So we transpose: cppValues[category][group] = values[group][category]
    static QVector< QVector< double > > convertToMultiBarValues(const pybind11::list& values)
    {
        int numGroups = static_cast< int >(values.size());
        if (numGroups == 0) return {};

        int numCategories = 0;
        for (int g = 0; g < numGroups; ++g) {
            if (pybind11::isinstance< pybind11::list >(values[g])) {
                numCategories = std::max(numCategories, static_cast< int >(values[g].cast< pybind11::list >().size()));
            }
        }
        if (numCategories == 0) return {};

        QVector< QVector< double > > result(numCategories);
        for (int cat = 0; cat < numCategories; ++cat) {
            result[cat].resize(numGroups);
            for (int grp = 0; grp < numGroups; ++grp) {
                if (pybind11::isinstance< pybind11::list >(values[grp])) {
                    pybind11::list groupValues = values[grp].cast< pybind11::list >();
                    if (cat < static_cast< int >(groupValues.size())) {
                        result[cat][grp] = groupValues[cat].cast< double >();
                    }
                }
            }
        }
        return result;
    }

    // Convert pybind11::list of [x, y, z] triples to QVector<QwtPoint3D>
    static QVector< QwtPoint3D > convertToPoint3D(const pybind11::list& points)
    {
        QVector< QwtPoint3D > result;
        result.reserve(static_cast< int >(points.size()));
        for (auto item : points) {
            if (pybind11::isinstance< pybind11::list >(item)) {
                pybind11::list triple = item.cast< pybind11::list >();
                if (triple.size() >= 3) {
                    double x = triple[0].cast< double >();
                    double y = triple[1].cast< double >();
                    double z = triple[2].cast< double >();
                    result.append(QwtPoint3D(x, y, z));
                }
            } else if (pybind11::isinstance< pybind11::tuple >(item)) {
                pybind11::tuple triple = item.cast< pybind11::tuple >();
                if (triple.size() >= 3) {
                    double x = triple[0].cast< double >();
                    double y = triple[1].cast< double >();
                    double z = triple[2].cast< double >();
                    result.append(QwtPoint3D(x, y, z));
                }
            }
        }
        return result;
    }

    // Convert pybind11::list of [x, y] pairs to QPainterPath
    static QPainterPath convertToPainterPath(const pybind11::list& polygon)
    {
        QPainterPath path;
        bool first = true;
        for (auto item : polygon) {
            double x = 0.0, y = 0.0;
            bool ok = false;
            if (pybind11::isinstance< pybind11::list >(item)) {
                pybind11::list pair = item.cast< pybind11::list >();
                if (pair.size() >= 2) {
                    x = pair[0].cast< double >();
                    y = pair[1].cast< double >();
                    ok = true;
                }
            } else if (pybind11::isinstance< pybind11::tuple >(item)) {
                pybind11::tuple pair = item.cast< pybind11::tuple >();
                if (pair.size() >= 2) {
                    x = pair[0].cast< double >();
                    y = pair[1].cast< double >();
                    ok = true;
                }
            }
            if (!ok) continue;
            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
        }
        if (!first) {
            path.closeSubpath();
        }
        return path;
    }

    // Convert pybind11::dict to QwtGridRasterData*
    // Dict keys: z (2D list), x_interval [min,max], y_interval [min,max], cmap (string)
    //
    // QwtGridRasterData::setValue documentation says:
    //   data matrix.size = xAxis.size
    //   data matrix.at(n).size = yAxis.size
    // So v is indexed as v[x][y] (column-major).
    // Python z matrix is z[y][x] (row-major, standard numpy layout).
    // We must transpose: v[x][y] = z[y][x]
    static QwtGridRasterData* convertToRasterData(const pybind11::dict& gridData)
    {
        // Extract z matrix (2D list)
        pybind11::object zObj = gridData["z"];
        if (zObj.is_none() || !pybind11::isinstance< pybind11::list >(zObj)) return nullptr;
        pybind11::list zRows = zObj.cast< pybind11::list >();
        int numRows = static_cast< int >(zRows.size());  // y size
        if (numRows == 0) return nullptr;

        int numCols = 0;
        for (int i = 0; i < numRows; ++i) {
            if (pybind11::isinstance< pybind11::list >(zRows[i])) {
                numCols = std::max(numCols, static_cast< int >(zRows[i].cast< pybind11::list >().size()));
            }
        }
        if (numCols == 0) return nullptr;

        // Extract intervals
        double xMin = 0.0, xMax = static_cast< double >(numCols - 1);
        double yMin = 0.0, yMax = static_cast< double >(numRows - 1);

        if (gridData.contains("x_interval")) {
            pybind11::object xIntervalObj = gridData["x_interval"];
            if (!xIntervalObj.is_none()) {
                extractInterval(xIntervalObj, xMin, xMax);
            }
        }
        if (gridData.contains("y_interval")) {
            pybind11::object yIntervalObj = gridData["y_interval"];
            if (!yIntervalObj.is_none()) {
                extractInterval(yIntervalObj, yMin, yMax);
            }
        }

        // Generate x and y axis vectors (evenly spaced)
        QVector< double > xVec(numCols), yVec(numRows);
        for (int i = 0; i < numCols; ++i) {
            if (numCols > 1)
                xVec[i] = xMin + (xMax - xMin) * i / (numCols - 1);
            else
                xVec[i] = xMin;
        }
        for (int i = 0; i < numRows; ++i) {
            if (numRows > 1)
                yVec[i] = yMin + (yMax - yMin) * i / (numRows - 1);
            else
                yVec[i] = yMin;
        }

        // Build value matrix (transposed: v[x][y] = z[y][x])
        QVector< QVector< double > > v(numCols);
        for (int xi = 0; xi < numCols; ++xi) {
            v[xi].resize(numRows);
            for (int yi = 0; yi < numRows; ++yi) {
                if (xi < static_cast< int >(zRows[yi].cast< pybind11::list >().size())) {
                    v[xi][yi] = zRows[yi].cast< pybind11::list >()[xi].cast< double >();
                } else {
                    v[xi][yi] = 0.0;
                }
            }
        }

        // Create and populate the raster data
        QwtGridRasterData* rasterData = new QwtGridRasterData();
        rasterData->setValue(xVec, yVec, v);
        return rasterData;
    }

    // ==================== Utility functions ====================

    static double dictGetDouble(const pybind11::dict& d, const char* key, double defaultValue)
    {
        if (!d.contains(key)) return defaultValue;
        pybind11::object val = d[pybind11::str(key)];
        if (val.is_none()) return defaultValue;
        try {
            return val.cast< double >();
        } catch (...) {
            return defaultValue;
        }
    }

    static void extractInterval(const pybind11::object& obj, double& lo, double& hi)
    {
        if (pybind11::isinstance< pybind11::list >(obj)) {
            pybind11::list l = obj.cast< pybind11::list >();
            if (l.size() >= 2) {
                lo = l[0].cast< double >();
                hi = l[1].cast< double >();
            }
        } else if (pybind11::isinstance< pybind11::tuple >(obj)) {
            pybind11::tuple t = obj.cast< pybind11::tuple >();
            if (t.size() >= 2) {
                lo = t[0].cast< double >();
                hi = t[1].cast< double >();
            }
        }
    }
};

// ==================== Module-level style functions ====================

/**
 * @brief 将字符串转换为Qwt符号样式
 * @param style 符号样式名称字符串
 * @return 对应的QwtSymbol::Style枚举值，无法匹配时返回QwtSymbol::NoSymbol
 */
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

/**
 * @brief 将字符串转换为Qwt曲线样式
 * @param style 曲线样式名称字符串
 * @return 对应的QwtPlotCurve::CurveStyle枚举值，无法匹配时返回QwtPlotCurve::Lines
 */
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

/**
 * @brief 设置图表项的画笔（颜色和线宽）
 * @param item 图表项指针
 * @param color 画笔颜色
 * @param width 画笔线宽
 * @sa setBrushOnItem(), setSymbolOnItem()
 */
void setPenOnItem(QwtPlotItem* item, const QColor& color, double width)
{
    if (!item) return;
    QPen pen(color, width);
    // Try each chart type that supports setPen
    if (auto* curve = dynamic_cast< QwtPlotCurve* >(item)) {
        curve->setPen(pen);
    } else if (auto* bar = dynamic_cast< QwtPlotBarChart* >(item)) {
        bar->setPen(pen);
    } else if (auto* box = dynamic_cast< QwtPlotBoxChart* >(item)) {
        box->setPen(pen);
    } else if (auto* hist = dynamic_cast< QwtPlotHistogram* >(item)) {
        hist->setPen(pen);
    } else if (auto* interval = dynamic_cast< QwtPlotIntervalCurve* >(item)) {
        interval->setPen(pen);
    } else if (auto* marker = dynamic_cast< QwtPlotMarker* >(item)) {
        marker->setLinePen(pen);
    } else if (auto* shape = dynamic_cast< QwtPlotShapeItem* >(item)) {
        shape->setPen(pen);
    }
}

/**
 * @brief 设置图表项的画刷（填充颜色）
 * @param item 图表项指针
 * @param color 画刷颜色
 * @sa setPenOnItem(), setSymbolOnItem()
 */
void setBrushOnItem(QwtPlotItem* item, const QColor& color)
{
    if (!item) return;
    QBrush brush(color);
    if (auto* curve = dynamic_cast< QwtPlotCurve* >(item)) {
        curve->setBrush(brush);
    } else if (auto* bar = dynamic_cast< QwtPlotBarChart* >(item)) {
        bar->setBrush(brush);
    } else if (auto* hist = dynamic_cast< QwtPlotHistogram* >(item)) {
        hist->setBrush(brush);
    } else if (auto* box = dynamic_cast< QwtPlotBoxChart* >(item)) {
        box->setBrush(brush);
    } else if (auto* shape = dynamic_cast< QwtPlotShapeItem* >(item)) {
        shape->setBrush(brush);
    }
}

/**
 * @brief 设置曲线图表项的符号样式
 * @param item 图表项指针，仅对QwtPlotCurve类型生效
 * @param style 符号样式名称字符串
 * @param size 符号大小（像素）
 * @param color 符号颜色
 * @sa setPenOnItem(), setBrushOnItem()
 */
void setSymbolOnItem(QwtPlotItem* item, const QString& style, int size, const QColor& color)
{
    if (!item) return;
    QwtSymbol::Style symStyle = stringToSymbolStyle(style);
    if (symStyle == QwtSymbol::NoSymbol) return;
    if (auto* curve = dynamic_cast< QwtPlotCurve* >(item)) {
        QwtSymbol* symbol = new QwtSymbol(symStyle, QBrush(color), QPen(color), QSize(size, size));
        curve->setSymbol(symbol);
    }
}

/**
 * @brief 设置曲线图表项的绘制样式
 * @param item 图表项指针，仅对QwtPlotCurve类型生效
 * @param style 曲线样式名称字符串
 * @sa stringToCurveStyle()
 */
void setCurveStyleOnItem(QwtPlotItem* item, const QString& style)
{
    if (!item) return;
    if (auto* curve = dynamic_cast< QwtPlotCurve* >(item)) {
        curve->setStyle(stringToCurveStyle(style));
    }
}

}  // namespace da_figure

// ==================== pybind11 module ====================

PYBIND11_EMBEDDED_MODULE(da_figure, m)
{
    m.doc() = "DA Figure plotting interface for Python";

    // PlotItem - opaque pointer holder for QwtPlotItem
    // All addXxx() methods return QwtPlotItem* (upcast from specific types).
    // Python sees these as PlotItem objects.
    pybind11::class_< QwtPlotItem >(m, "PlotItem")
        .def("isValid", [](QwtPlotItem* item) { return item != nullptr; });

    // FigureWidget - minimal binding for DAFigureWidget
    pybind11::class_< DA::DAFigureWidget >(m, "FigureWidget")
        .def("getCurrentChart", [](DA::DAFigureWidget& self) -> da_figure::ChartHandle {
            return da_figure::ChartHandle(self.getCurrentChart());
        })
        .def("getCharts", [](DA::DAFigureWidget& self) -> pybind11::list {
            pybind11::list result;
            for (auto* chart : self.getCharts()) {
                result.append(da_figure::ChartHandle(chart));
            }
            return result;
        })
        .def("getChartCount", &DA::DAFigureWidget::getChartCount)
        .def("createChart", [](DA::DAFigureWidget& self) -> da_figure::ChartHandle {
            return da_figure::ChartHandle(self.createChart());
        });

    // ChartHandle - wraps DAChartWidget for chart data and style operations
    pybind11::class_< da_figure::ChartHandle >(m, "ChartHandle")
        .def(pybind11::init<>())
        .def("isValid", &da_figure::ChartHandle::isValid)

        // DAChartDataInterface methods - return PlotItem
        // All addXxx return QwtPlotItem* with reference policy (no ownership transfer).
        // The chart (DAChartWidget) owns the items; Python must not delete them.
        .def("addCurve",
             [](da_figure::ChartHandle& self, const QVector< double >& x, const QVector< double >& y, const QString& title) -> QwtPlotItem* {
                 return self.addCurveXY(x, y, title);
             },
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a curve from x and y arrays. Returns a PlotItem.")
        .def("addCurve",
             [](da_figure::ChartHandle& self, const QVector< QPointF >& points, const QString& title) -> QwtPlotItem* {
                 return self.addCurvePoints(points, title);
             },
             pybind11::arg("points"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a curve from a list of (x, y) point tuples. Returns a PlotItem.")
        .def("addScatter",
             [](da_figure::ChartHandle& self, const QVector< QPointF >& points, const QString& title) -> QwtPlotItem* {
                 return self.addScatter(points, title);
             },
             pybind11::arg("points"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a scatter plot. Returns a PlotItem.")
        .def("addBarChart",
             [](da_figure::ChartHandle& self, const QVector< double >& values, const QString& title) -> QwtPlotItem* {
                 return self.addBarChart(values, title);
             },
             pybind11::arg("values"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a bar chart. Returns a PlotItem.")
        .def("addIntervalCurve",
             [](da_figure::ChartHandle& self, const QVector< double >& values, const QVector< double >& mins, const QVector< double >& maxs, const QString& title) -> QwtPlotItem* {
                 return self.addIntervalCurve(values, mins, maxs, title);
             },
             pybind11::arg("values"), pybind11::arg("mins"), pybind11::arg("maxs"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add an interval curve (error bars). Returns a PlotItem.")
        .def("addBoxChart",
             [](da_figure::ChartHandle& self, const pybind11::list& samples, const QString& title) -> QwtPlotItem* {
                 return self.addBoxChart(samples, title);
             },
             pybind11::arg("samples"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a box chart. samples is a list of dicts with keys: position, whiskerLower, q1, median, q3, whiskerUpper. Returns a PlotItem.")
        .def("addHistogram",
             [](da_figure::ChartHandle& self, const pybind11::list& samples, const QString& title) -> QwtPlotItem* {
                 return self.addHistogram(samples, title);
             },
             pybind11::arg("samples"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a histogram. samples is a list of dicts with keys: value, interval=[lo,hi]. Returns a PlotItem.")
        .def("addMultiBarChart",
             [](da_figure::ChartHandle& self, const QVector< double >& positions, const pybind11::list& values, const pybind11::list& titles) -> QwtPlotItem* {
                 return self.addMultiBarChart(positions, values, titles);
             },
             pybind11::arg("positions"), pybind11::arg("values"), pybind11::arg("titles") = pybind11::list(),
             pybind11::return_value_policy::reference,
             "Add a grouped bar chart. positions is x-axis positions, values is a list of lists (outer=groups, inner=per-category). Returns a PlotItem.")
        .def("addContour",
             [](da_figure::ChartHandle& self, const pybind11::list& points, const QVector< double >& levels, const QString& title) -> QwtPlotItem* {
                 return self.addContour(points, levels, title);
             },
             pybind11::arg("points"), pybind11::arg("levels"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a contour plot. points is a list of [x,y,z] triples. Returns a PlotItem.")
        .def("addShapeItem",
             [](da_figure::ChartHandle& self, const pybind11::list& polygon, const QString& title) -> QwtPlotItem* {
                 return self.addShapeItem(polygon, title);
             },
             pybind11::arg("polygon"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a shape item (filled polygon). polygon is a list of [x,y] pairs. Returns a PlotItem.")
        .def("addVerticalLine",
             [](da_figure::ChartHandle& self, double x, const QString& title) -> QwtPlotItem* {
                 return self.addVerticalLine(x, title);
             },
             pybind11::arg("x"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a vertical line marker. Returns a PlotItem.")
        .def("addHorizontalLine",
             [](da_figure::ChartHandle& self, double y, const QString& title) -> QwtPlotItem* {
                 return self.addHorizontalLine(y, title);
             },
             pybind11::arg("y"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a horizontal line marker. Returns a PlotItem.")
        .def("addSpectrogram",
             [](da_figure::ChartHandle& self, const pybind11::dict& gridData, const QString& title) -> QwtPlotItem* {
                 return self.addSpectrogram(gridData, title);
             },
             pybind11::arg("grid_data"), pybind11::arg("title") = QString(),
             pybind11::return_value_policy::reference,
             "Add a spectrogram/heatmap. grid_data is a dict with keys: z (2D list), x_interval [min,max], y_interval [min,max], cmap (str). Returns a PlotItem.")
        .def("removePlotItem",
             [](da_figure::ChartHandle& self, QwtPlotItem* item) {
                 self.removePlotItem(item);
             },
             pybind11::arg("item"),
             "Remove a plot item from the chart.")
        .def("clearAllData", &da_figure::ChartHandle::clearAllData, "Clear all data items from the chart.")

        // DAChartStyleInterface methods
        .def("setChartTitle", &da_figure::ChartHandle::setChartTitle, pybind11::arg("title"), "Set the chart title.")
        .def("setAxisLabel",
             [](da_figure::ChartHandle& self, const QString& axis, const QString& label) {
                 self.setAxisLabel(axis, label);
             },
             pybind11::arg("axis"), pybind11::arg("label"),
             "Set axis label. axis is one of: 'yLeft', 'yRight', 'xBottom', 'xTop'.")
        .def("enableGrid", &da_figure::ChartHandle::enableGrid, pybind11::arg("on") = true, "Enable or disable the grid.")
        .def("enableLegend", &da_figure::ChartHandle::enableLegend, pybind11::arg("on") = true, "Enable or disable the legend.")
        .def("setBackgroundBrush",
             [](da_figure::ChartHandle& self, const QColor& color) {
                 self.setBackgroundBrush(color);
             },
             pybind11::arg("color"),
             "Set the chart background color.")
        .def("replot", &da_figure::ChartHandle::replot, "Refresh the chart display.")
        .def("autoScale", &da_figure::ChartHandle::autoScale, "Enable auto-scaling for all axes so the chart fits all attached items.")
        ;

    // ==================== Module-level functions ====================

    m.def("getCurrentChart",
          []() -> da_figure::ChartHandle {
              if (!da_figure::g_currentChartGetter) {
                  return da_figure::ChartHandle(nullptr);
              }
              return da_figure::ChartHandle(da_figure::g_currentChartGetter());
          },
          "Get the current active chart as a ChartHandle. Returns an invalid ChartHandle if no active chart.");

    m.def("getCurrentFigure",
          []() -> DA::DAFigureWidget* {
              if (!da_figure::g_currentChartGetter) return nullptr;
              auto chart = da_figure::g_currentChartGetter();
              if (!chart) return nullptr;
              return chart->figureWidget();
          },
          pybind11::return_value_policy::reference,
          "Get the current active figure widget. Returns None if no active figure.");

    m.def("setPen",
          [](QwtPlotItem* item, const QColor& color, double width) {
              da_figure::setPenOnItem(item, color, width);
          },
          pybind11::arg("item"), pybind11::arg("color"), pybind11::arg("width") = 1.0,
          "Set the pen color and width on a PlotItem.");

    m.def("setBrush",
          [](QwtPlotItem* item, const QColor& color) {
              da_figure::setBrushOnItem(item, color);
          },
          pybind11::arg("item"), pybind11::arg("color"),
          "Set the brush color on a PlotItem.");

    m.def("setSymbol",
          [](QwtPlotItem* item, const QString& style, int size, const QColor& color) {
              da_figure::setSymbolOnItem(item, style, size, color);
          },
          pybind11::arg("item"), pybind11::arg("style"), pybind11::arg("size") = 8, pybind11::arg("color") = QColor(Qt::black),
          "Set the symbol on a PlotItem. style is one of: 'Ellipse', 'Rect', 'Diamond', 'Triangle', 'Cross', 'XCross', 'Star1'.");

    m.def("setCurveStyle",
          [](QwtPlotItem* item, const QString& style) {
              da_figure::setCurveStyleOnItem(item, style);
          },
          pybind11::arg("item"), pybind11::arg("style"),
          "Set the curve style. style is one of: 'Lines', 'Steps', 'Dots', 'Sticks', 'NoCurve'.");

    // Factory: wrap a DAChartWidget* (registered in da_interface) in a ChartHandle
    // so that C++ controller code can pass a chart to Python plotting functions.
    m.def("getChartHandle",
          [](DA::DAChartWidget* chart) -> da_figure::ChartHandle {
              return da_figure::ChartHandle(chart);
          },
          pybind11::arg("chart"),
          "Create a ChartHandle wrapping a DAChartWidget. "
          "The DAChartWidget type is registered in the da_interface module; "
          "import da_interface before calling this function if the type is not yet known.");
}
