#include "DAAgentToolSetChartStyle.h"

namespace DA
{
static Qt::PenStyle parsePenStyle(const QString& s)
{
    QString l = s.toLower();
    if (l == "dash") return Qt::DashLine;
    if (l == "dot") return Qt::DotLine;
    if (l == "dashdot") return Qt::DashDotLine;
    if (l == "dashdotdot") return Qt::DashDotDotLine;
    return Qt::SolidLine;
}

QJsonObject DAAgentToolSetChartStyle::getToolSpec() const
{
    return QJsonObject{
        {"name", "set_chart_style"},
        {"description", "Set chart title, axis labels, legend, grid, background, and other style options. "
         "Use figure_name to target a specific figure."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier. Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"title", QJsonObject{{"type", "string"}, {"description", "Chart title"}}},
                {"x_label", QJsonObject{{"type", "string"}, {"description", "X-axis label"}}},
                {"y_label", QJsonObject{{"type", "string"}, {"description", "Y-axis label"}}},
                {"legend", QJsonObject{{"type", "boolean"}, {"description", "Show legend (true/false)"}}},
                {"grid", QJsonObject{{"type", "boolean"}, {"description", "Show grid (true/false)"}}},
                {"background_color", QJsonObject{{"type", "string"}, {"description", "Plot background color (hex or name, e.g. '#F5F5F5' or 'white')"}}},
                {"border_color", QJsonObject{{"type", "string"}, {"description", "Plot border color (hex or name)"}}},
                {"grid_major_color", QJsonObject{{"type", "string"}, {"description", "Major grid line color (hex or name)"}}},
                {"grid_major_style", QJsonObject{{"type", "string"}, {"description", "Major grid line style: solid, dash, dot"}}},
                {"legend_position", QJsonObject{{"type", "string"}, {"description", "Legend position: top, bottom, left, right"}}},
                {"legend_background_color", QJsonObject{{"type", "string"}, {"description", "Legend background color (hex or name)"}}},
                {"legend_text_color", QJsonObject{{"type", "string"}, {"description", "Legend text color (hex or name)"}}},
                {"figure_background_color", QJsonObject{{"type", "string"}, {"description", "Figure (outer) background color (hex or name). Sets the figure background, not the chart canvas."}}}
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

    // Plot background color
    if (params.contains("background_color")) {
        chart->setBackgroundBrush(QBrush(QColor(params["background_color"].toString())));
        changed = true;
    }

    // Plot border color
    if (params.contains("border_color")) {
        chart->setBorderColor(QColor(params["border_color"].toString()));
        changed = true;
    }

    // Grid major style (color / pen style)
    if (params.contains("grid_major_color") || params.contains("grid_major_style")) {
        QColor gc = params.contains("grid_major_color")
                        ? QColor(params["grid_major_color"].toString()) : QColor(Qt::gray);
        qreal gw = 1.0;
        Qt::PenStyle gs = params.contains("grid_major_style")
                              ? parsePenStyle(params["grid_major_style"].toString()) : Qt::DotLine;
        chart->setGridMajorStyle(gc, gw, gs);
        changed = true;
    }

    // Legend position
    if (params.contains("legend_position")) {
        QString pos = params["legend_position"].toString().toLower();
        Qt::Alignment align = Qt::AlignRight | Qt::AlignTop;
        if (pos == "top") align = Qt::AlignTop;
        else if (pos == "bottom") align = Qt::AlignBottom;
        else if (pos == "left") align = Qt::AlignLeft;
        else if (pos == "right") align = Qt::AlignRight;
        chart->setLegendPosition(align);
        changed = true;
    }

    // Legend background color
    if (params.contains("legend_background_color")) {
        chart->setLegendBackground(QBrush(QColor(params["legend_background_color"].toString())));
        changed = true;
    }

    // Legend text color
    if (params.contains("legend_text_color")) {
        chart->setLegendTextColor(QColor(params["legend_text_color"].toString()));
        changed = true;
    }

    // Figure-level background color (operates on the DAFigureWidget, not the chart)
    if (params.contains("figure_background_color")) {
        DAFigureWidget* fig = nullptr;
        if (!figureName.isEmpty()) {
            fig = findFigureByName(figureName);
        } else {
            fig = currentFigure();
        }
        if (fig) {
            fig->setBackgroundColor(QColor(params["figure_background_color"].toString()));
        }
        changed = true;
    }

    if (changed) {
        chart->replot();
    }

    return successResponse("Chart style updated");
}
}  // namespace DA
