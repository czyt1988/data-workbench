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

QJsonObject DAAgentToolAddCurve::getToolSpec() const
{
    return QJsonObject{
        {"name", "add_curve"},
        {"description", "Add a curve to an existing chart."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier (title or index). Empty or 'current' for active chart."}}},
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"x_column", QJsonObject{{"type", "string"}, {"description", "X-axis column name"}}},
                {"y_column", QJsonObject{{"type", "string"}, {"description", "Y-axis column name"}}},
                {"name", QJsonObject{{"type", "string"}, {"description", "Curve display name"}}},
                {"color", QJsonObject{{"type", "string"}, {"description", "Curve color (hex or name, e.g. '#FF0000' or 'red')"}}},
                {"width", QJsonObject{{"type", "number"}, {"description", "Line width"}}},
                {"style", QJsonObject{{"type", "string"}, {"description", "Line style: solid, dash, dot, dashdot"}}}
            }},
            {"required", QJsonArray{"data_name", "x_column", "y_column"}}
        }}
    };
}

QJsonObject DAAgentToolAddCurve::execute(const QJsonObject& params)
{
    QString chartId   = params["chart_id"].toString();
    QString dataName  = params["data_name"].toString();
    QString xCol      = params["x_column"].toString();
    QString yCol      = params["y_column"].toString();
    QString name      = params["name"].toString();
    QString colorStr  = params["color"].toString();
    double  width     = params["width"].toDouble(0.0);
    QString styleStr  = params["style"].toString();

    if (dataName.isEmpty() || xCol.isEmpty() || yCol.isEmpty()) {
        return errorResponse("data_name, x_column, and y_column are required");
    }

    DAChartWidget* chart = findChart(chartId);
    if (!chart) {
        return errorResponse(QString("Chart '%1' not found").arg(chartId.isEmpty() ? "current" : chartId));
    }

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();
    DAPySeries xs = df.loc(xCol);
    DAPySeries ys = df.loc(yCol);
    QVector< double > xData = toQVectorDouble(xs);
    QVector< double > yData = toQVectorDouble(ys);

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

    chart->replot();

    return successResponse(QString("Added curve '%1' with %2 points").arg(name.isEmpty() ? yCol : name).arg(n));
}
}  // namespace DA
