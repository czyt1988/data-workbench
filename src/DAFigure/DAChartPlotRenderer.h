#ifndef DACHARTPLOTRENDERER_H
#define DACHARTPLOTRENDERER_H

#include "DAFigureAPI.h"

#include <QPolygonF>
#include <QVector>
#include <QVariantMap>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QPointF>

#include "DAUtils/DAPlotDataTypes.h"

class QwtPlotItem;

namespace DA {
class DAChartWidget;
}

/**
 * @brief Pure C++ chart rendering helper that operates DAChartWidget to render
 *        statistical plot items from Qwt native types and DAUtils composite structs.
 *
 * This class does NOT depend on pybind11 or Python. It receives ready-to-plot
 * data (Qwt native types or DAPlotDataTypes structs) and delegates to
 * DAChartWidget's add methods, applying style parameters from a QVariantMap.
 *
 * The QVariantMap style keys (all optional, with hardcoded defaults):
 *   - "title":       QString, item title
 *   - "color":       QColor, pen/line color
 *   - "fillColor":   QColor, brush/fill color (alpha supported)
 *   - "width":       double, pen width
 *   - "symbol":      QString, symbol style name (Ellipse, Rect, Diamond, Triangle,
 *                    DTriangle, UTriangle, LTriangle, RTriangle, Cross, XCross,
 *                    Star1, Star2, Hexagon, HLine, VLine)
 *   - "symbolSize":  int, symbol size in pixels
 *   - "curveStyle":  QString, curve style (Lines, Steps, Dots, Sticks, NoCurve)
 *   - "cmap":        QString, color map name for spectrogram (e.g. "Jet")
 */
class DAFIGURE_API DAChartPlotRenderer
{
public:
    explicit DAChartPlotRenderer(DA::DAChartWidget* chart);
    ~DAChartPlotRenderer();

    // ==================== Basic primitives ====================

    QwtPlotItem* renderCurve(const QPolygonF& points, const QVariantMap& style = {});
    QwtPlotItem* renderScatter(const QVector<QPointF>& points, const QVariantMap& style = {});
    QwtPlotItem* renderHistogram(const QVector<double>& bins, const QVector<double>& counts, const QVariantMap& style = {});
    QwtPlotItem* renderIntervalCurve(const DA::DAIntervalCurveData& data, const QVariantMap& style = {});
    QwtPlotItem* renderShape(const QPolygonF& polygon, const QVariantMap& style = {});

    // ==================== Composite types ====================

    QwtPlotItem* renderBoxChart(const DA::DABoxPlotData& data, const QVariantMap& style = {});
    QwtPlotItem* renderBarChart(const DA::DABarChartData& data, const QVariantMap& style = {});
    QwtPlotItem* renderSpectrogram(const DA::DASpectrogramData& data, const QVariantMap& style = {});
    QwtPlotItem* renderContours(const DA::DAContourData& data, const QVariantMap& style = {});

    // ==================== Chart metadata ====================

    void setChartTitle(const QString& title);
    void setAxisLabel(int axis, const QString& label);
    // 设置坐标轴为字符串类别刻度（在 tickPositions 处显示 labels 字符串）
    // dataLower/dataUpper 为该轴数据的实际范围（用于确定显示区间，与刻度位置解耦）
    void setAxisCategoryScale(int axis, const QVector<double>& tickPositions, const QStringList& labels,
                              double dataLower, double dataUpper);
    // 便捷方法：将 xBottom 轴设置为字符串类别刻度
    void setXBottomCategoryScale(const QVector<double>& tickPositions, const QStringList& labels,
                                 double dataLower, double dataUpper);
    void enableGrid(bool on);
    void enableLegend(bool on);
    void replot();

private:
    DA::DAChartWidget* mChart;

    // Style fallback helpers — extract typed values from QVariantMap with defaults
    QColor defaultColor(const QVariantMap& style, const QString& key, const QColor& fallback) const;
    double defaultDouble(const QVariantMap& style, const QString& key, double fallback) const;
    QString defaultString(const QVariantMap& style, const QString& key, const QString& fallback) const;
    int defaultInt(const QVariantMap& style, const QString& key, int fallback) const;

    // Style application helpers — set pen/brush/symbol on various QwtPlotItem subtypes
    void applyPen(QwtPlotItem* item, const QColor& color, double width);
    void applyBrush(QwtPlotItem* item, const QColor& color);
    void applySymbol(QwtPlotItem* item, const QString& symbolStyle, int size, const QColor& color);
};

#endif  // DACHARTPLOTRENDERER_H
