#include "DAAgentToolRemoveChartItem.h"
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_marker.h"
#include "qwt_text.h"

namespace DA
{
QJsonObject DAAgentToolRemoveChartItem::getToolSpec() const
{
    return QJsonObject{
        {"name", "remove_chart_item"},
        {"description", "Remove a curve, annotation, or region from a chart. "
         "Identify the item by its title or index. Use list_figures to discover item names."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier (title or index). Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"item_name", QJsonObject{{"type", "string"}, {"description", "Item title or index (0-based). Use list_figures to find item names."}}},
                {"item_type", QJsonObject{{"type", "string"}, {"description", "Filter item type: 'curve', 'annotation', 'region', or 'any' (default)"}}}
            }},
            {"required", QJsonArray{"item_name"}}
        }}
    };
}

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

    // Get all plot items (QwtPlot::itemList returns items in z-order)
    const QwtPlotItemList items = chart->itemList();

    // Filter by item_type
    QwtPlotItemList candidates;
    for (QwtPlotItem* item : items) {
        if (!item) continue;
        int rtti = item->rtti();
        if (itemType == "curve") {
            if (rtti == QwtPlotItem::Rtti_PlotCurve) candidates.append(item);
        } else if (itemType == "annotation") {
            if (rtti == QwtPlotItem::Rtti_PlotMarker) candidates.append(item);
        } else if (itemType == "region") {
            // Regions are QwtPlotShapeItem — exclude known non-data items
            if (rtti != QwtPlotItem::Rtti_PlotCurve
                && rtti != QwtPlotItem::Rtti_PlotMarker
                && rtti != QwtPlotItem::Rtti_PlotGrid
                && rtti != QwtPlotItem::Rtti_PlotLegend) {
                candidates.append(item);
            }
        } else {
            // "any" — match all items except grid and legend
            if (rtti != QwtPlotItem::Rtti_PlotGrid
                && rtti != QwtPlotItem::Rtti_PlotLegend) {
                candidates.append(item);
            }
        }
    }

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
        for (QwtPlotItem* item : candidates) {
            if (item && item->title().text() == itemName) {
                target = item;
                break;
            }
        }
    }

    if (!target) {
        return errorResponse(QString("Item '%1' not found. Use list_figures to see available items.").arg(itemName));
    }

    QString removedName = target->title().text();
    chart->removePlotItem(target);
    chart->replot();

    return successResponse(QString("Removed item '%1'").arg(removedName));
}
}  // namespace DA
