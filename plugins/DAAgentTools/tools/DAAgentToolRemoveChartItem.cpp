#include "DAAgentToolRemoveChartItem.h"
#include "qwt_plot_item.h"
#include "qwt_text.h"

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolRemoveChartItem::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("remove_chart_item"),
                         QStringLiteral("Remove a curve, annotation, or region from a chart. Identify the item by its "
                                        "title (legend label) or index. Use list_chart_items to discover item names "
                                        "and indices.")};
    spec.addParam({QStringLiteral("chart_id"),
                   QStringLiteral("Chart identifier (title or index). Empty or 'current' for active chart."),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_name"),
                   QStringLiteral("Figure name to target a specific figure. Empty for current active figure."),
                   {Type::String}});
    spec.addParam({QStringLiteral("item_name"),
                   QStringLiteral("Item title (legend label) or index (0-based). Use list_chart_items to find item "
                                  "names and indices."),
                   {Type::String},
                   true});
    spec.addParam({QStringLiteral("item_type"),
                   QStringLiteral("Filter item type: 'curve', 'annotation', 'region', or 'any' (default)"),
                   {Type::String}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolRemoveChartItem::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString itemName   = params["item_name"].toString();
    QString itemType   = params["item_type"].toString().toLower();

    if (itemName.isEmpty()) {
        return errorResponse("item_name is required");
    }

    if (itemType.isEmpty()) {
        itemType = "any";
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    // 按 item_type 过滤（共享口径，与 list_chart_items 的索引一致）
    const QwtPlotItemList candidates = filterChartItems(chart->itemList(), itemType);

    if (candidates.isEmpty()) {
        return errorResponse(QString("No items of type '%1' found in this chart").arg(itemType));
    }

    // Match by title or index
    QwtPlotItem* target = nullptr;

    // Try integer index
    bool ok = false;
    int idx = itemName.toInt(&ok);
    if (ok && idx >= 0 && idx < candidates.size()) {
        target = candidates[ idx ];
    } else {
        // Match by title
        for (QwtPlotItem* item : std::as_const(candidates)) {
            if (item && item->title().text() == itemName) {
                target = item;
                break;
            }
        }
    }

    if (!target) {
        return errorResponse(QString("Item '%1' not found. Use list_chart_items to see available items.").arg(itemName));
    }

    QString removedName = target->title().text();
    chart->removePlotItem(target);
    chart->replot();

    return successResponse(QString("Removed item '%1'").arg(removedName));
}
}  // namespace DA
