#include "DAAgentToolAddCurve.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#include <QPen>
#include <QColor>

namespace DA
{
static Qt::PenStyle parsePenStyle(const QString& s)
{
    QString l = s.toLower();
    if (l == "dash") return Qt::DashLine;
    if (l == "dot") return Qt::DotLine;
    if (l == "dashdot") return Qt::DashDotLine;
    if (l == "dashdotdot") return Qt::DashDotDotLine;
    return Qt::SolidLine;  // default "solid"
}

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolAddCurve::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("add_curve"),
                         QStringLiteral("Add a curve to an existing chart. Use figure_name to target a specific figure.")};
    spec.addParam({QStringLiteral("chart_id"),
                   QStringLiteral("Chart identifier (title or index). Empty or 'current' for active chart."),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_name"),
                   QStringLiteral("Figure name to target a specific figure. Empty for current active figure."),
                   {Type::String}});
    spec.addParam({QStringLiteral("data_name"), QStringLiteral("Dataset name"), {Type::String}, true});
    spec.addParam({QStringLiteral("x_column"), QStringLiteral("X-axis column name"), {Type::String}, true});
    spec.addParam({QStringLiteral("y_column"), QStringLiteral("Y-axis column name"), {Type::String}, true});
    spec.addParam({QStringLiteral("name"), QStringLiteral("Curve display name"), {Type::String}});
    spec.addParam({QStringLiteral("color"),
                   QStringLiteral("Curve color (hex or name, e.g. '#FF0000' or 'red')"),
                   {Type::String}});
    spec.addParam({QStringLiteral("width"), QStringLiteral("Line width"), {Type::Number}});
    spec.addParam({QStringLiteral("style"),
                   QStringLiteral("Line style: solid, dash, dot, dashdot"),
                   {Type::String}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolAddCurve::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString dataName   = params["data_name"].toString();
    QString xCol       = params["x_column"].toString();
    QString yCol       = params["y_column"].toString();
    QString name       = params["name"].toString();
    QString colorStr   = params["color"].toString();
    double  width      = params["width"].toDouble(0.0);
    QString styleStr   = params["style"].toString();

    if (dataName.isEmpty() || xCol.isEmpty() || yCol.isEmpty()) {
        return errorResponse("data_name, x_column, and y_column are required");
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();
    // Use operator[] (df["col"]) NOT df.loc("col") which accesses rows by label
    DAPySeries xs = df[ xCol ];
    DAPySeries ys = df[ yCol ];
    QVector< double > xData = toQVectorDouble(xs);
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
    if (n == 0) {
        return errorResponse("No data points to plot (empty columns)");
    }

    // Use the QVector<QPointF> overload for better compatibility
    QVector< QPointF > pts;
    pts.reserve(n);
    for (int i = 0; i < n; ++i) {
        pts.append(QPointF(xData[ i ], yData[ i ]));
    }

    QwtPlotCurve* curve = chart->addCurve(pts, name.isEmpty() ? yCol : name);
    if (curve) {
        QPen pen = curve->pen();
        if (!colorStr.isEmpty()) {
            pen.setColor(QColor(colorStr));
        }
        if (width > 0) {
            pen.setWidthF(width);
        }
        if (!styleStr.isEmpty()) {
            pen.setStyle(parsePenStyle(styleStr));
        }
        curve->setPen(pen);
    }

    // Re-enable auto-scaling so new data is visible alongside existing curves
    enableAutoScale(chart);
    chart->replot();

    return successResponse(QString("Added curve '%1' with %2 points").arg(name.isEmpty() ? yCol : name).arg(n));
}
}  // namespace DA
