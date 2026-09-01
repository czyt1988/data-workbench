#include "DAAgentToolListChartItems.h"
#include "DAChartUtil.h"
#include "qwt_text.h"

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolListChartItems::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("list_chart_items"),
                         QStringLiteral("List all items (curves, annotations, regions) inside a chart. For each item "
                                        "returns its index, legend label ('title' — the name shown in the chart "
                                        "legend), display name, type, visibility, color, and data point count. Pass "
                                        "the legend label as item_name to remove_chart_item / update_curve_style, or "
                                        "use the index (indices are relative to the same item_type filter used by "
                                        "remove_chart_item). Use list_figures first to discover figure names and "
                                        "chart ids.")};
    spec.addParam({QStringLiteral("chart_id"),
                   QStringLiteral("Chart identifier (title or index). Empty or 'current' for active chart."),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_name"),
                   QStringLiteral("Figure name to target a specific figure. Empty for current active figure."),
                   {Type::String}});
    spec.addParam({QStringLiteral("item_type"),
                   QStringLiteral("Filter item type: 'curve', 'annotation', 'region', or 'any' (default). Indices "
                                  "match remove_chart_item behavior for the same item_type."),
                   {Type::String}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolListChartItems::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString itemType   = params["item_type"].toString().toLower();
    if (itemType.isEmpty()) {
        itemType = "any";
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId)
                                           : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    // 与 remove_chart_item 同一口径过滤，保证索引一致
    const QwtPlotItemList items = filterChartItems(chart->itemList(), itemType);
    if (items.isEmpty()) {
        return successResponse(QString("No items of type '%1' found in chart '%2'. Use create_chart or add_curve to "
                                       "add items first.")
                                   .arg(itemType, chart->getChartTitle()));
    }

    QJsonArray itemArray;
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotItem* item = items[ i ];
        if (!item) continue;
        QJsonObject itemObj;
        itemObj[ "index" ] = i;
        // 图例名（legend label），与 remove_chart_item / update_curve_style 的名字匹配键同源
        itemObj[ "title" ] = item->title().text();
        // 大纲显示名，title 为空时自动生成带序号名（如 curve-2），作为识别兜底
        itemObj[ "name" ]        = DAChartUtil::plotItemName(item);
        itemObj[ "type" ]        = chartItemTypeName(item);
        itemObj[ "visible" ]     = item->isVisible();
        itemObj[ "data_points" ] = DAChartUtil::dynamicGetPlotChartItemDataCount(item);
        const QColor color       = DAChartUtil::getPlotItemColor(item);
        if (color.isValid()) {
            itemObj[ "color" ] = color.name();
        }
        itemArray.append(itemObj);
    }

    DAFigureWidget* fig = figureName.isEmpty() ? currentFigure() : findFigureByName(figureName);
    QString figName;
    if (auto* oper = chartOperateWidget(); oper && fig) {
        figName = oper->getFigureName(fig);
    }

    QJsonObject data;
    data[ "figure_name" ] = figName;
    data[ "chart_title" ] = chart->getChartTitle();
    data[ "item_type" ]   = itemType;
    data[ "item_count" ]  = items.size();
    data[ "items" ]       = itemArray;
    return successResponse(data);
}
}  // namespace DA
