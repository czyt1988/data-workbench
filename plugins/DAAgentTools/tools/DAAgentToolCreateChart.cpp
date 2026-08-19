#include "DAAgentToolCreateChart.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#include <QVector>
#include <QPointF>
#include <QJsonDocument>
#include <QStringList>
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

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolCreateChart::getToolSpec() const
{
    return QJsonObject{
        {"name", "create_chart"},
        {"description", "Create a new figure with a chart (line/scatter/bar/hist/box) from dataset columns. "
         "If figure_name matches an existing figure, the chart is added to that figure (useful for building subplots). "
         "Otherwise a new figure is created. "
         "The y parameter accepts an array of column names to draw multiple curves on the same chart in one call. "
         "When the X column is a datetime type, the X-axis is automatically set to a datetime scale. "
         "The returned figure_id/figure_name can be used in da-figure: hyperlinks so the user can open this figure from your reply."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"type", QJsonObject{{"type", "string"}, {"description", "Chart type: line, scatter, bar, hist, box"}}},
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"x", QJsonObject{{"type", "string"}, {"description", "X-axis column name"}}},
                {"y", QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}, {"description", "Y-axis column name(s). Pass a single-element array for one curve, or multiple column names for multiple curves on the same chart."}}},
                {"title", QJsonObject{{"type", "string"}, {"description", "Chart title (also used as figure_name if figure_name is empty)"}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name. If a figure with this name already exists, the chart is added to that figure (useful for building subplots). If no match, a new figure is created. If empty, a new figure is auto-created."}}},
                {"x_label", QJsonObject{{"type", "string"}, {"description", "X-axis label"}}},
                {"y_label", QJsonObject{{"type", "string"}, {"description", "Y-axis label"}}}
            }},
            {"required", QJsonArray{"type", "data_name", "x", "y"}}
        }}
    };
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolCreateChart::execute(const QJsonObject& params)
{
    QString type       = params["type"].toString().toLower();
    QString dataName  = params["data_name"].toString();
    QString xCol       = params["x"].toString();
    QString title      = params["title"].toString();
    QString figureName = params["figure_name"].toString();
    QString xLabel    = params["x_label"].toString();
    QString yLabel    = params["y_label"].toString();

    // Parse y parameter: accept JSON array, single string, or JSON-array-as-string
    // (some LLMs pass a JSON-encoded array string when the schema says array)
    QStringList yCols;
    QJsonValue yVal = params["y"];
    if (yVal.isArray()) {
        for (const auto& v : yVal.toArray()) {
            yCols.append(v.toString());
        }
    } else if (yVal.isString()) {
        QString yStr = yVal.toString();
        QJsonDocument doc = QJsonDocument::fromJson(yStr.toUtf8());
        if (doc.isArray()) {
            for (const auto& v : doc.array()) {
                yCols.append(v.toString());
            }
        } else {
            yCols.append(yStr);
        }
    }

    if (type.isEmpty() || dataName.isEmpty() || xCol.isEmpty() || yCols.isEmpty()) {
        return errorResponse("type, data_name, x, and y are all required");
    }
    // Filter out empty column names
    for (const QString& c : yCols) {
        if (c.isEmpty()) {
            return errorResponse("y contains an empty column name");
        }
    }

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    // Determine figure name: use figure_name if provided, else title, else auto-generate
    if (figureName.isEmpty()) {
        figureName = title.isEmpty() ? QString("Chart - %1").arg(yCols.first()) : title;
    }

    // If a figure with this name already exists, reuse it (enables subplot population).
    // Otherwise create a new figure.
    DAFigureWidget* fig = findFigureByName(figureName);
    if (!fig) {
        fig = createFigure(figureName);
    }
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

    // Validate x data — toQVectorDouble returns empty for non-numeric string columns
    if (xData.isEmpty()) {
        return errorResponse(QString("Column '%1' could not be converted to numeric values. "
            "It may contain non-numeric data (text/categories). "
            "Please use a numeric column, or pre-aggregate the data using query_data.").arg(xCol));
    }

    // Auto-detect: if X column is datetime64, set up a datetime axis so the X-axis
    // displays formatted dates instead of raw epoch milliseconds.
    bool xAxisDateTime = false;
    if (xs.isDateTime()) {
        chart->setupDateTimeAxis(QwtPlot::xBottom);
        xAxisDateTime = true;
    }

    // Draw each Y column as a separate curve/item on the same chart
    QJsonArray curvesArray;
    int dataPoints = 0;

    for (int yi = 0; yi < yCols.size(); ++yi) {
        const QString& yCol = yCols[ yi ];

        DAPySeries ys = df[ yCol ];
        QVector< double > yData = toQVectorDouble(ys);

        if (yData.isEmpty()) {
            return errorResponse(QString("Column '%1' could not be converted to numeric values. "
                "It may contain non-numeric data (text/categories). "
                "Please use a numeric column, or pre-aggregate the data using query_data.").arg(yCol));
        }

        int n = qMin(xData.size(), yData.size());
        if (yi == 0) {
            dataPoints = n;
        }

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
            chart->addBarChart(pts, yCol);
        } else if (type == "hist") {
            int bins = 20;
            QVector< QwtIntervalSample > samples = computeHistogram(yData, bins);
            chart->addHistogram(samples, yCol);
        } else if (type == "box") {
            // Use incremental x position so multiple box plots don't overlap
            QwtBoxSample sample = computeBoxSample(yData, static_cast< double >(yi));
            QVector< QwtBoxSample > samples;
            samples.append(sample);
            chart->addBoxChart(samples, yCol);
        } else {
            return errorResponse(QString("Unsupported chart type: %1").arg(type));
        }

        curvesArray.append(yCol);
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
    respData["figure_id"]   = fig->getFigureId();
    respData["figure_name"] = figureName;
    respData["chart_title"] = title.isEmpty() ? yCols.join(", ") : title;
    respData["chart_type"]  = type;
    respData["data_points"] = dataPoints;
    respData["curves"]      = curvesArray;
    respData["x_axis_datetime"] = xAxisDateTime;
    return successResponse(respData);
}
}  // namespace DA
