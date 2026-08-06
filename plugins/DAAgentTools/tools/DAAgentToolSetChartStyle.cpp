#include "DAAgentToolSetChartStyle.h"

namespace DA
{
QJsonObject DAAgentToolSetChartStyle::getToolSpec() const
{
    return QJsonObject{
        {"name", "set_chart_style"},
        {"description", "Set chart title, axis labels, legend, and grid visibility. Use figure_name to target a specific figure."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier. Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"title", QJsonObject{{"type", "string"}, {"description", "Chart title"}}},
                {"x_label", QJsonObject{{"type", "string"}, {"description", "X-axis label"}}},
                {"y_label", QJsonObject{{"type", "string"}, {"description", "Y-axis label"}}},
                {"legend", QJsonObject{{"type", "boolean"}, {"description", "Show legend (true/false)"}}},
                {"grid", QJsonObject{{"type", "boolean"}, {"description", "Show grid (true/false)"}}}
            }},
            {"required", QJsonArray{}}
        }}
    };
}

QJsonObject DAAgentToolSetChartStyle::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString title       = params["title"].toString();
    QString xLabel      = params["x_label"].toString();
    QString yLabel      = params["y_label"].toString();

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    bool changed = false;
    if (!title.isEmpty()) {
        chart->setChartTitle(title);
        changed = true;
    }
    if (!xLabel.isEmpty()) {
        chart->setAxisLabel(QwtPlot::xBottom, xLabel);
        changed = true;
    }
    if (!yLabel.isEmpty()) {
        chart->setAxisLabel(QwtPlot::yLeft, yLabel);
        changed = true;
    }
    if (params.contains("legend")) {
        chart->enableLegend(params["legend"].toBool());
        changed = true;
    }
    if (params.contains("grid")) {
        chart->enableGrid(params["grid"].toBool());
        changed = true;
    }

    if (changed) {
        chart->replot();
    }

    return successResponse("Chart style updated");
}
}  // namespace DA
