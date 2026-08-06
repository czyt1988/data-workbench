#include "DAAgentToolListFigures.h"

namespace DA
{
QJsonObject DAAgentToolListFigures::getToolSpec() const
{
    return QJsonObject{
        {"name", "list_figures"},
        {"description", "List all figures and their charts. Returns figure names, chart titles, and chart indices. "
         "Use this to discover existing figures before modifying them with figure_name + chart_id."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{}},
            {"required", QJsonArray{}}
        }}
    };
}

QJsonObject DAAgentToolListFigures::execute(const QJsonObject& params)
{
    auto* oper = chartOperateWidget();
    if (!oper) {
        return errorResponse("Chart operate widget not available");
    }

    const QList< DAFigureWidget* > figs = oper->getFigureList();
    if (figs.isEmpty()) {
        return successResponse("No figures available. Use create_chart to create one.");
    }

    DAFigureWidget* currentFig = oper->getCurrentFigure();

    QJsonArray figureArray;
    for (DAFigureWidget* fig : figs) {
        if (!fig) {
            continue;
        }
        QJsonObject figObj;
        figObj["name"]      = oper->getFigureName(fig);
        figObj["id"]        = fig->getFigureId();
        figObj["is_current"] = (fig == currentFig);

        QJsonArray chartArray;
        const QList< DAChartWidget* > charts = fig->getCharts();
        for (int i = 0; i < charts.size(); ++i) {
            if (!charts[ i ]) {
                continue;
            }
            QJsonObject chartObj;
            chartObj["index"] = i;
            chartObj["title"] = charts[ i ]->getChartTitle();
            chartArray.append(chartObj);
        }
        figObj["charts"]      = chartArray;
        figObj["chart_count"] = charts.size();
        figureArray.append(figObj);
    }

    QJsonObject data;
    data["figures"]      = figureArray;
    data["figure_count"] = figs.size();
    return successResponse(data);
}
}  // namespace DA
