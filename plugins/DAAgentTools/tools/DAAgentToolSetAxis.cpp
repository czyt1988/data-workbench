#include "DAAgentToolSetAxis.h"
#include "DAChartUtil.h"

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolSetAxis::getToolSpec() const
{
    return QJsonObject{
        {"name", "set_axis"},
        {"description", "Configure axis scale type, range, and appearance. "
         "Supports datetime axis (for time-series data), axis range setting, and axis color. "
         "Use figure_name/chart_id to target a specific chart."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier (title or index). Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"axis", QJsonObject{{"type", "string"}, {"description", "Axis to configure: 'x' or 'y'"}}},
                {"scale_type", QJsonObject{{"type", "string"}, {"description", "Scale type: 'normal' (linear) or 'datetime'. If omitted, scale type is unchanged."}}},
                {"date_format", QJsonObject{{"type", "string"}, {"description", "Date format for datetime axis, e.g. 'yyyy-MM-dd', 'yyyy-MM-dd hh:mm'. Default 'yyyy-MM-dd hh:mm:ss'. Only used when scale_type is 'datetime'."}}},
                {"min", QJsonObject{{"type", "number"}, {"description", "Axis minimum value. Only for normal (linear) scale."}}},
                {"max", QJsonObject{{"type", "number"}, {"description", "Axis maximum value. Only for normal (linear) scale."}}},
                {"color", QJsonObject{{"type", "string"}, {"description", "Axis color (hex or name, e.g. '#FF0000' or 'red')"}}},
                {"label_rotation", QJsonObject{{"type", "number"}, {"description", "Axis label rotation in degrees"}}}
            }},
            {"required", QJsonArray{"axis"}}
        }}
    };
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
