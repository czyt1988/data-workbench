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
        {"description", "Create a chart (line/scatter/bar/hist/box) from dataset columns."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"type", QJsonObject{{"type", "string"}, {"description", "Chart type: line, scatter, bar, hist, box"}}},
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"x", QJsonObject{{"type", "string"}, {"description", "X-axis column name"}}},
                {"y", QJsonObject{{"type", "string"}, {"description", "Y-axis column name(s)"}}},
                {"title", QJsonObject{{"type", "string"}, {"description", "Chart title"}}},
                {"x_label", QJsonObject{{"type", "string"}, {"description", "X-axis label"}}},
                {"y_label", QJsonObject{{"type", "string"}, {"description", "Y-axis label"}}}
            }},
            {"required", QJsonArray{"type", "data_name", "x", "y"}}
        }}
    };
}

QJsonObject DAAgentToolCreateChart::execute(const QJsonObject& params)
{
    QString type     = params["type"].toString().toLower();
    QString dataName = params["data_name"].toString();
    QString xCol     = params["x"].toString();
    QString yCol     = params["y"].toString();
    QString title    = params["title"].toString();
    QString xLabel   = params["x_label"].toString();
    QString yLabel   = params["y_label"].toString();

    if (type.isEmpty() || dataName.isEmpty() || xCol.isEmpty() || yCol.isEmpty()) {
        return errorResponse("type, data_name, x, and y are all required");
    }

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAFigureWidget* fig = currentFigure();
    if (!fig) {
        return errorResponse("No active figure. Please create or open a figure first.");
    }
    DAChartWidget* chart = fig->currentChart();  // creates one if none exists
    if (!chart) {
        return errorResponse("Failed to create chart");
    }

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();

    // Extract x data
    DAPySeries xs = df.loc(xCol);
    QVector< double > xData = toQVectorDouble(xs);

    // Extract y data
    DAPySeries ys = df.loc(yCol);
    QVector< double > yData = toQVectorDouble(ys);

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
    chart->replot();

    return successResponse(QString("Created %1 chart with %2 data points").arg(type).arg(n));
}
}  // namespace DA
