// DAAgentChartToolBase.cpp —— 图表方法实现（plan-04 从 DAAgentToolBase.h 搬来，
// 实现体逐字照抄不改；依赖 mCore->getUiInterface()->getDockingArea()->
// getChartOperateWidget() 链路）。
#include "DAAgentChartToolBase.h"

namespace DA
{

/**
 * @brief 获取图表操作窗口
 * @return 图表操作窗口指针，若核心接口或停靠区不可用则返回 nullptr
 */
DAChartOperateWidget* DAAgentChartToolBase::chartOperateWidget() const
{
    if (!mCore) return nullptr;
    auto* ui = mCore->getUiInterface();
    if (!ui) return nullptr;
    auto* dock = ui->getDockingArea();
    if (!dock) return nullptr;
    return dock->getChartOperateWidget();
}

/**
 * @brief 获取当前活动的 Figure 窗口
 * @return 当前 Figure 指针，若图表操作窗口不可用则返回 nullptr
 */
DAFigureWidget* DAAgentChartToolBase::currentFigure() const
{
    auto* oper = chartOperateWidget();
    return oper ? oper->getCurrentFigure() : nullptr;
}

/**
 * @brief 获取当前活动的图表
 * @return 当前图表指针，若图表操作窗口不可用则返回 nullptr
 */
DAChartWidget* DAAgentChartToolBase::currentChart() const
{
    auto* oper = chartOperateWidget();
    return oper ? oper->getCurrentChart() : nullptr;
}

/**
 * @brief 按名称查找 Figure 窗口
 * @param name Figure 名称
 * @return 匹配的 Figure 指针，若未找到或名称为空则返回 nullptr
 */
DAFigureWidget* DAAgentChartToolBase::findFigureByName(const QString& name) const
{
    auto* oper = chartOperateWidget();
    if (!oper || name.isEmpty()) {
        return nullptr;
    }
    const QList< DAFigureWidget* > figs = oper->getFigureList();
    for (DAFigureWidget* fig : figs) {
        if (fig && oper->getFigureName(fig) == name) {
            return fig;
        }
    }
    return nullptr;
}

/**
 * @brief 创建新的 Figure 窗口并设为当前活动
 * @param name Figure 名称
 * @return 新创建的 Figure 指针，若图表操作窗口不可用则返回 nullptr
 */
DAFigureWidget* DAAgentChartToolBase::createFigure(const QString& name) const
{
    auto* oper = chartOperateWidget();
    if (!oper) {
        return nullptr;
    }
    DAFigureWidget* fig = oper->createFigure(name);
    if (fig) {
        oper->setCurrentFigure(fig);
    }
    return fig;
}

/**
 * @brief 查找指定图表
 * @param chartId 图表标识（标题或索引），为空或 "current" 表示当前活动图表
 * @param figureName 所属 Figure 名称，为空则在当前活动 Figure 中查找
 * @return 匹配的图表指针，若未找到则返回 nullptr
 */
DAChartWidget* DAAgentChartToolBase::findChart(const QString& chartId, const QString& figureName) const
{
    DAFigureWidget* fig = nullptr;
    if (!figureName.isEmpty()) {
        fig = findFigureByName(figureName);
    } else {
        fig = currentFigure();
    }
    if (!fig) {
        return nullptr;
    }
    if (chartId.isEmpty() || chartId == "current") {
        return fig->getCurrentChart();
    }
    // 优先按标题匹配
    const QList< DAChartWidget* > charts = fig->getCharts();
    for (DAChartWidget* c : charts) {
        if (c && c->getChartTitle() == chartId) {
            return c;
        }
    }
    // 其次尝试作为整数索引解析
    bool ok = false;
    int idx = chartId.toInt(&ok);
    if (ok) {
        if (idx >= 0 && idx < charts.size()) {
            return charts[ idx ];
        }
    }
    return nullptr;
}

/**
 * @brief 启用图表坐标轴的自动缩放
 * @param chart 目标图表指针
 */
void DAAgentChartToolBase::enableAutoScale(DAChartWidget* chart) const
{
    if (!chart) {
        return;
    }
    chart->setAxisAutoScale(QwtPlot::xBottom, true);
    chart->setAxisAutoScale(QwtPlot::yLeft, true);
}

/**
 * @brief 按 item_type 语义过滤 item 列表
 * @param items 待过滤的 item 列表（QwtPlot::itemList 的 z 序）
 * @param itemType 类型过滤词：'curve' / 'annotation' / 'region' / 'any'（空或未知值均按 any）
 * @return 过滤后的 item 列表，保持原 z 序；索引口径与 remove_chart_item 的 item_name 索引匹配一致
 */
QwtPlotItemList DAAgentChartToolBase::filterChartItems(const QwtPlotItemList& items, const QString& itemType)
{
    QString type = itemType.toLower();
    if (type.isEmpty()) {
        type = "any";
    }
    QwtPlotItemList candidates;
    for (QwtPlotItem* item : items) {
        if (!item) continue;
        int rtti = item->rtti();
        if (type == "curve") {
            if (rtti == QwtPlotItem::Rtti_PlotCurve) candidates.append(item);
        } else if (type == "annotation") {
            if (rtti == QwtPlotItem::Rtti_PlotMarker) candidates.append(item);
        } else if (type == "region") {
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
    return candidates;
}

/**
 * @brief 获取 item 的语义类型名
 * @param item 图表元素
 * @return 'curve'（曲线）/ 'annotation'（标记）/ 'region'（形状等其余元素）；
 *         grid/legend 等被 filterChartItems 排除的类型返回 "other"
 */
QString DAAgentChartToolBase::chartItemTypeName(const QwtPlotItem* item)
{
    if (!item) {
        return QStringLiteral("other");
    }
    const int rtti = item->rtti();
    if (rtti == QwtPlotItem::Rtti_PlotCurve) {
        return QStringLiteral("curve");
    }
    if (rtti == QwtPlotItem::Rtti_PlotMarker) {
        return QStringLiteral("annotation");
    }
    if (rtti == QwtPlotItem::Rtti_PlotGrid || rtti == QwtPlotItem::Rtti_PlotLegend) {
        return QStringLiteral("other");
    }
    return QStringLiteral("region");
}
}  // namespace DA
