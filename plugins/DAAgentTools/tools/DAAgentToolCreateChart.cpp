#include "DAAgentToolCreateChart.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#include <QVector>
#include <QPointF>
#include <algorithm>
#include <cmath>

namespace DA
{
// ---- local helpers ----

/// Compute a histogram from a vector of doubles.
static QVector< QwtIntervalSample > computeHistogram(const QVector< double >& data, int bins)
{
    QVector< QwtIntervalSample > samples;
    if (data.isEmpty() || bins <= 0) return samples;

    double minVal = data[ 0 ];
    double maxVal = data[ 0 ];
    for (double v : data) {
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
    }
    if (qFuzzyCompare(minVal, maxVal)) {
        maxVal = minVal + 1.0;
    }
    double range    = maxVal - minVal;
    double binWidth = range / bins;

    samples.reserve(bins);
    for (int i = 0; i < bins; ++i) {
        double left  = minVal + i * binWidth;
        double right = (i == bins - 1) ? maxVal : (left + binWidth);
        samples.append(QwtIntervalSample(0.0, QwtInterval(left, right)));
    }
    for (double v : data) {
        int bin = static_cast< int >((v - minVal) / binWidth);
        if (bin < 0) bin = 0;
        if (bin >= bins) bin = bins - 1;
        samples[ bin ].value += 1.0;
    }
    return samples;
}

/// Compute a QwtBoxSample (min, Q1, median, Q3, max) from sorted data.
static QwtBoxSample computeBoxSample(QVector< double > data, double position = 0.0)
{
    if (data.isEmpty()) return QwtBoxSample();
    std::sort(data.begin(), data.end());
    int n = data.size();
    auto quantile = [&](double q) -> double {
        double pos = q * (n - 1);
        int lo = static_cast< int >(pos);
        int hi = lo + 1;
        if (hi >= n) return data[ lo ];
        double frac = pos - lo;
        return data[ lo ] + frac * (data[ hi ] - data[ lo ]);
    };
    return QwtBoxSample(position, data.first(), quantile(0.25), quantile(0.5), quantile(0.75), data.last());
}

// ---- tool spec ----

QJsonObject DAAgentToolCreateChart::getToolSpec() const
{
    return QJsonObject{
        {"name", "create_chart"},
        {"description", "Create a new figure with a chart (line/scatter/bar/hist/box) from dataset columns. "
         "Each call creates a NEW figure — use figure_name to give it a searchable name (shown as the tab title). "
         "Other tools (add_curve, set_chart_style, etc.) can target this figure via figure_name."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"type", QJsonObject{{"type", "string"}, {"description", "Chart type: line, scatter, bar, hist, box"}}},
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"x", QJsonObject{{"type", "string"}, {"description", "X-axis column name"}}},
                {"y", QJsonObject{{"type", "string"}, {"description", "Y-axis column name(s)"}}},
                {"title", QJsonObject{{"type", "string"}, {"description", "Chart title (also used as figure_name if figure_name is empty)"}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name shown as tab title. If empty, uses title or auto-generates. Use this name with other tools' figure_name parameter to target this figure."}}},
                {"x_label", QJsonObject{{"type", "string"}, {"description", "X-axis label"}}},
                {"y_label", QJsonObject{{"type", "string"}, {"description", "Y-axis label"}}}
            }},
            {"required", QJsonArray{"type", "data_name", "x", "y"}}
        }}
    };
}

QJsonObject DAAgentToolCreateChart::execute(const QJsonObject& params)
{
    QString type       = params["type"].toString().toLower();
    QString dataName  = params["data_name"].toString();
    QString xCol       = params["x"].toString();
    QString yCol       = params["y"].toString();
    QString title      = params["title"].toString();
    QString figureName = params["figure_name"].toString();
    QString xLabel    = params["x_label"].toString();
    QString yLabel    = params["y_label"].toString();

    if (type.isEmpty() || dataName.isEmpty() || xCol.isEmpty() || yCol.isEmpty()) {
        return errorResponse("type, data_name, x, and y are all required");
    }

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    // Determine figure name: use figure_name if provided, else title, else auto-generate
    if (figureName.isEmpty()) {
        figureName = title.isEmpty() ? QString("Chart - %1").arg(yCol) : title;
    }

    // Create a new figure and set it as current — each create_chart call gets its own figure
    DAFigureWidget* fig = createFigure(figureName);
    if (!fig) {
        return errorResponse("Failed to create figure");
    }

    // Create a new chart in the figure (NOT currentChart() which reuses the existing chart)
    DAChartWidget* chart = fig->createChart();
    if (!chart) {
        return errorResponse("Failed to create chart in figure");
    }
    fig->setCurrentChart(chart);

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();

    // Extract x data — use operator[] (df["col"]) NOT df.loc("col") which accesses rows by label
    DAPySeries xs = df[ xCol ];
    QVector< double > xData = toQVectorDouble(xs);

    // Extract y data
    DAPySeries ys = df[ yCol ];
    QVector< double > yData = toQVectorDouble(ys);

    // Validate data extraction — toQVectorDouble returns empty for non-numeric string columns
    if (xData.isEmpty()) {
        return errorResponse(QString("Column '%1' could not be converted to numeric values. "
            "It may contain non-numeric data (text/categories). "
            "Please use a numeric column, or pre-aggregate the data using query_data.").arg(xCol));
    }
    if (yData.isEmpty()) {
        return errorResponse(QString("Column '%1' could not be converted to numeric values. "
            "It may contain non-numeric data (text/categories). "
            "Please use a numeric column, or pre-aggregate the data using query_data.").arg(yCol));
    }

    int n = qMin(xData.size(), yData.size());

    if (type == "line") {
        QVector< QPointF > pts;
        pts.reserve(n);
        for (int i = 0; i < n; ++i) {
            pts.append(QPointF(xData[ i ], yData[ i ]));
        }
        chart->addCurve(pts, yCol);
    } else if (type == "scatter") {
        QVector< QPointF > pts;
        pts.reserve(n);
        for (int i = 0; i < n; ++i) {
            pts.append(QPointF(xData[ i ], yData[ i ]));
        }
        chart->addScatter(pts, yCol);
    } else if (type == "bar") {
        QVector< QPointF > pts;
        pts.reserve(n);
        for (int i = 0; i < n; ++i) {
            pts.append(QPointF(xData[ i ], yData[ i ]));
        }
        chart->addBarChart(pts, title.isEmpty() ? yCol : title);
    } else if (type == "hist") {
        // Compute histogram of y column
        int bins = 20;
        QVector< QwtIntervalSample > samples = computeHistogram(yData, bins);
        chart->addHistogram(samples, title.isEmpty() ? yCol : title);
    } else if (type == "box") {
        // Compute box plot of y column
        QwtBoxSample sample = computeBoxSample(yData, 0.0);
        QVector< QwtBoxSample > samples;
        samples.append(sample);
        chart->addBoxChart(samples, title.isEmpty() ? yCol : title);
    } else {
        return errorResponse(QString("Unsupported chart type: %1").arg(type));
    }

    // Apply styling
    if (!title.isEmpty()) {
        chart->setChartTitle(title);
    }
    if (!xLabel.isEmpty()) {
        chart->setAxisLabel(QwtPlot::xBottom, xLabel);
    }
    if (!yLabel.isEmpty()) {
        chart->setAxisLabel(QwtPlot::yLeft, yLabel);
    }

    // Re-enable auto-scaling so data is visible.
    // DAFigureWidget::createChart locks axes to [0,800]×[0,500] via setAxisScale,
    // which disables Qwt auto-scaling — data outside that range would be invisible.
    enableAutoScale(chart);
    chart->replot();

    // Return structured data so the agent can reference this figure/chart later
    QJsonObject respData;
    respData["figure_name"] = figureName;
    respData["chart_title"] = title.isEmpty() ? yCol : title;
    respData["chart_type"]  = type;
    respData["data_points"] = n;
    return successResponse(respData);
}
}  // namespace DA
