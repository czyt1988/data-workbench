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

    if (qFuzzyCompare(startX, endX)) {
        return errorResponse("start_x and end_x must be different");
    }
    if (startX > endX) {
        std::swap(startX, endX);
    }
    if (!colorStr.isEmpty() && !QColor(colorStr).isValid()) {
        return errorResponse(QString("Invalid color '%1'").arg(colorStr));
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    // Re-enable auto-scale and replot so the y-axis reflects the real data
    // range. createChart locks axes to [0,800]x[0,500] via setAxisScale, which
    // disables Qwt auto-scaling; without this, axisScaleDiv(yLeft) below
    // returns the stale locked range and the region spans an empty y area.
    enableAutoScale(chart);
    chart->replot();

    // Get y-axis range to cover the full vertical extent (now the data range)
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
        fill.setAlpha(100);  // semi-transparent, raised from 60 for visibility
        item->setBrush(QBrush(fill));
        QPen pen(fill.darker(150));
        pen.setStyle(Qt::DashLine);
        pen.setWidthF(1.5);
        item->setPen(pen);
    }

    chart->replot();

    // Off-screen x-range hint: warn if the region falls entirely outside the
    // visible axis range (so the caller knows it was added but not visible).
    QString warning;
    const QwtScaleDiv& xDiv = chart->axisScaleDiv(QwtPlot::xBottom);
    if (endX < xDiv.lowerBound() || startX > xDiv.upperBound()) {
        warning = " (warning: region x-range is outside the visible axis range)";
    }
    return successResponse(QString("Added region [%1, %2]%3").arg(startX).arg(endX).arg(warning));
}
}  // namespace DA
