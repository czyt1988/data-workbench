// DAAgentChartToolBase.cpp —— 图表方法实现（plan-04 从 DAAgentToolBase.h 搬来，
// 实现体逐字照抄不改；依赖 mCore->getUiInterface()->getDockingArea()->
// getChartOperateWidget() 链路）。
#include "DAAgentChartToolBase.h"

namespace DA
{
DAChartOperateWidget* DAAgentChartToolBase::chartOperateWidget() const
{
    if (!mCore) return nullptr;
    auto* ui = mCore->getUiInterface();
    if (!ui) return nullptr;
    auto* dock = ui->getDockingArea();
    if (!dock) return nullptr;
    return dock->getChartOperateWidget();
}

DAFigureWidget* DAAgentChartToolBase::currentFigure() const
{
    auto* oper = chartOperateWidget();
    return oper ? oper->getCurrentFigure() : nullptr;
}

DAChartWidget* DAAgentChartToolBase::currentChart() const
{
    auto* oper = chartOperateWidget();
    return oper ? oper->getCurrentChart() : nullptr;
}

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

void DAAgentChartToolBase::enableAutoScale(DAChartWidget* chart) const
{
    if (!chart) {
        return;
    }
    chart->setAxisAutoScale(QwtPlot::xBottom, true);
    chart->setAxisAutoScale(QwtPlot::yLeft, true);
}
}  // namespace DA
