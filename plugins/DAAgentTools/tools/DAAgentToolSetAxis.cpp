#include "DAAgentToolSetAxis.h"
#include "DAChartUtil.h"

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolSetAxis::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("set_axis"),
                         QStringLiteral("Configure axis scale type, range, and appearance. Supports datetime axis "
                                        "(for time-series data), axis range setting, and axis color. Use "
                                        "figure_name/chart_id to target a specific chart.")};
    spec.addParam({QStringLiteral("chart_id"),
                   QStringLiteral("Chart identifier (title or index). Empty or 'current' for active chart."),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_name"),
                   QStringLiteral("Figure name to target a specific figure. Empty for current active figure."),
                   {Type::String}});
    spec.addParam({QStringLiteral("axis"), QStringLiteral("Axis to configure: 'x' or 'y'"), {Type::String}, true});
    spec.addParam({QStringLiteral("scale_type"),
                   QStringLiteral("Scale type: 'normal' (linear) or 'datetime'. If omitted, scale type is unchanged."),
                   {Type::String}});
    spec.addParam({QStringLiteral("date_format"),
                   QStringLiteral("Date format for datetime axis, e.g. 'yyyy-MM-dd', 'yyyy-MM-dd hh:mm'. Default "
                                  "'yyyy-MM-dd hh:mm:ss'. Only used when scale_type is 'datetime'."),
                   {Type::String}});
    spec.addParam({QStringLiteral("min"),
                   QStringLiteral("Axis minimum value. Only for normal (linear) scale."),
                   {Type::Number}});
    spec.addParam({QStringLiteral("max"),
                   QStringLiteral("Axis maximum value. Only for normal (linear) scale."),
                   {Type::Number}});
    spec.addParam({QStringLiteral("color"),
                   QStringLiteral("Axis color (hex or name, e.g. '#FF0000' or 'red')"),
                   {Type::String}});
    spec.addParam({QStringLiteral("label_rotation"),
                   QStringLiteral("Axis label rotation in degrees"),
                   {Type::Number}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolSetAxis::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString axisStr    = params["axis"].toString().toLower();
    QString scaleType  = params["scale_type"].toString().toLower();
    QString dateFormat = params["date_format"].toString();
    double  minVal     = params["min"].toDouble(0.0);
    double  maxVal     = params["max"].toDouble(0.0);
    QString colorStr   = params["color"].toString();
    double  rotation   = params["label_rotation"].toDouble(0.0);

    if (axisStr != "x" && axisStr != "y") {
        return errorResponse("axis must be 'x' or 'y'");
    }

    int axisId = (axisStr == "x") ? QwtPlot::xBottom : QwtPlot::yLeft;

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    bool changed = false;

    // Scale type
    if (!scaleType.isEmpty()) {
        if (scaleType == "datetime") {
            if (dateFormat.isEmpty()) {
                dateFormat = QStringLiteral("yyyy-MM-dd hh:mm:ss");
            }
            chart->setupDateTimeAxis(axisId, dateFormat);
            changed = true;
        } else if (scaleType == "normal") {
            DAChartUtil::setAxisNormalScale(chart, axisId);
            changed = true;
        }
    }

    // Axis range (only for normal scale — datetime uses auto-scale)
    if (params.contains("min")) {
        DAChartUtil::setAxisScaleMin(chart, axisId, minVal);
        changed = true;
    }
    if (params.contains("max")) {
        DAChartUtil::setAxisScaleMax(chart, axisId, maxVal);
        changed = true;
    }

    // Axis color
    if (!colorStr.isEmpty()) {
        chart->setAxisColor(axisId, QColor(colorStr));
        changed = true;
    }

    // Label rotation
    if (params.contains("label_rotation")) {
        DAChartUtil::setAxisLabelRotation(chart, axisId, rotation);
        changed = true;
    }

    if (changed) {
        chart->replot();
    }

    return successResponse(QString("Axis '%1' configured").arg(axisStr));
}
}  // namespace DA
