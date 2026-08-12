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
}  // namespace DA
