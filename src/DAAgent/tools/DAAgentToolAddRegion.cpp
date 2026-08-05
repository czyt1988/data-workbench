#include "DAAgentToolAddRegion.h"
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <qwt_scale_div.h>

namespace DA
{
QJsonObject DAAgentToolAddRegion::getToolSpec() const
{
    return QJsonObject{
        {"name", "add_region"},
        {"description", "Add a highlighted vertical region between start_x and end_x on a chart. Use figure_name to target a specific figure."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier. Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"start_x", QJsonObject{{"type", "number"}, {"description", "Start x value of the region"}}},
                {"end_x", QJsonObject{{"type", "number"}, {"description", "End x value of the region"}}},
                {"color", QJsonObject{{"type", "string"}, {"description", "Region color (hex or name)"}}},
                {"label", QJsonObject{{"type", "string"}, {"description", "Region label"}}}
            }},
            {"required", QJsonArray{"start_x", "end_x"}}
        }}
    };
}

QJsonObject DAAgentToolAddRegion::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    double startX       = params["start_x"].toDouble();
    double endX         = params["end_x"].toDouble();
    QString colorStr    = params["color"].toString();
    QString label       = params["label"].toString();

    if (startX > endX) {
        std::swap(startX, endX);
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    // Get y-axis range to cover the full vertical extent
    const QwtScaleDiv& yDiv = chart->axisScaleDiv(QwtPlot::yLeft);
    double yMin = yDiv.lowerBound();
    double yMax = yDiv.upperBound();
    if (qFuzzyCompare(yMin, yMax)) {
        yMin -= 1.0;
        yMax += 1.0;
    }

    // Build rectangle path
    QPainterPath path;
    path.addRect(QRectF(startX, yMin, endX - startX, yMax - yMin));

    QwtPlotShapeItem* item = chart->addShapeItem(path, label);
    if (item) {
        QColor fill = colorStr.isEmpty() ? QColor(255, 200, 0) : QColor(colorStr);
        fill.setAlpha(60);  // semi-transparent
        item->setBrush(QBrush(fill));
        QPen pen(fill.darker(150));
        pen.setStyle(Qt::DashLine);
        item->setPen(pen);
    }

    chart->replot();
    return successResponse(QString("Added region [%1, %2]").arg(startX).arg(endX));
}
}  // namespace DA
