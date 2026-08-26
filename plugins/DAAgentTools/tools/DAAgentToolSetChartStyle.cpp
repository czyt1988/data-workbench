#include "DAAgentToolSetChartStyle.h"
#include "DAChartUtil.h"

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

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolSetChartStyle::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("set_chart_style"),
                         QStringLiteral("Set chart title, axis labels, legend, grid, background, and other style "
                                        "options. Use figure_name to target a specific figure.")};
    spec.addParam({QStringLiteral("chart_id"),
                   QStringLiteral("Chart identifier. Empty or 'current' for active chart."),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_name"),
                   QStringLiteral("Figure name to target a specific figure. Empty for current active figure."),
                   {Type::String}});
    spec.addParam({QStringLiteral("title"), QStringLiteral("Chart title"), {Type::String}});
    spec.addParam({QStringLiteral("x_label"), QStringLiteral("X-axis label"), {Type::String}});
    spec.addParam({QStringLiteral("y_label"), QStringLiteral("Y-axis label"), {Type::String}});
    spec.addParam({QStringLiteral("legend"), QStringLiteral("Show legend (true/false)"), {Type::Boolean}});
    spec.addParam({QStringLiteral("grid"), QStringLiteral("Show grid (true/false)"), {Type::Boolean}});
    spec.addParam({QStringLiteral("x_axis_type"),
                   QStringLiteral("X-axis scale type: 'datetime' for time-series data (formats tick labels as dates), "
                                  "'normal' for linear scale. If omitted, axis type is unchanged."),
                   {Type::String}});
    spec.addParam({QStringLiteral("x_date_format"),
                   QStringLiteral("Date format for datetime axis, e.g. 'yyyy-MM-dd', 'yyyy-MM-dd hh:mm'. Default "
                                  "'yyyy-MM-dd hh:mm:ss'. Only used when x_axis_type is 'datetime'."),
                   {Type::String}});
    spec.addParam({QStringLiteral("background_color"),
                   QStringLiteral("Plot background color (hex or name, e.g. '#F5F5F5' or 'white')"),
                   {Type::String}});
    spec.addParam({QStringLiteral("border_color"),
                   QStringLiteral("Plot border color (hex or name)"),
                   {Type::String}});
    spec.addParam({QStringLiteral("grid_major_color"),
                   QStringLiteral("Major grid line color (hex or name)"),
                   {Type::String}});
    spec.addParam({QStringLiteral("grid_major_style"),
                   QStringLiteral("Major grid line style: solid, dash, dot"),
                   {Type::String}});
    spec.addParam({QStringLiteral("legend_position"),
                   QStringLiteral("Legend position: top, bottom, left, right"),
                   {Type::String}});
    spec.addParam({QStringLiteral("legend_background_color"),
                   QStringLiteral("Legend background color (hex or name)"),
                   {Type::String}});
    spec.addParam({QStringLiteral("legend_text_color"),
                   QStringLiteral("Legend text color (hex or name)"),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_background_color"),
                   QStringLiteral("Figure (outer) background color (hex or name). Sets the figure background, not the "
                                  "chart canvas."),
                   {Type::String}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
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

    // X-axis scale type (datetime / normal)
    if (params.contains("x_axis_type")) {
        QString axisType = params["x_axis_type"].toString().toLower();
        if (axisType == "datetime") {
            QString fmt = params["x_date_format"].toString();
            if (fmt.isEmpty()) {
                fmt = QStringLiteral("yyyy-MM-dd hh:mm:ss");
            }
            chart->setupDateTimeAxis(QwtPlot::xBottom, fmt);
            changed = true;
        } else if (axisType == "normal") {
            DAChartUtil::setAxisNormalScale(chart, QwtPlot::xBottom);
            changed = true;
        }
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
