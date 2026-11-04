/**
 * @file DAAppRibbonAreaStats.cpp
 * @brief DAAppRibbonArea 的「统计」上下文标签页实现
 *
 * 统计页不走 showContextCategory/hideContextCategory 的互斥逻辑：它只由统计窗口
 * dock 的可见性驱动（见 setStatsContextCategoryVisible），与表格/工作流/绘图上下文
 * 可以同时出现。
 *
 * 动作全部由 DADataStatsWidget 持有（勾选状态即统计参数状态，窗口关闭后保留），
 * 本文件只做布局，不复制任何状态。
 */
#include "DAAppRibbonArea.h"
#include "DAUiObjectNames.h"
// Qt
#include <QDebug>
#include <QToolButton>
#include <utility>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonContextCategory.h"
#include "SARibbonTabBar.h"
// 统计窗口（动作由窗口持有，Ribbon 只负责布局）
#include "Chart/DADataStatsWidget.h"

using namespace DA;

/**
 * @brief 构建「统计」上下文标签页
 *
 * 上下文标题「统计」，分类页「统计操作」，三面板结构：
 * - 统计参数：每个统计项一个动作（勾选即出现在统计表里），分位数收在一个下拉里。
 *   大/中按钮交错排布（2 大 -> 2 个双列中按钮 -> 2 大 -> 4 个双列中按钮 -> 1 大），
 *   中型按钮在 3 行/2 行模式下都是一列两个，切换行模式不会出现半列。
 * - 检验：正态性检验、均值 95% 置信区间、离群点分析。
 * - 绘图：箱线图/帕累托图互斥切换、帕累托的分布率/累计分布选项、转入绘图区。
 *
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 * @param stats 统计窗口，动作由它持有；为 nullptr 时说明 dock 尚未建好，直接跳过
 */
void DAAppRibbonArea::buildContextCategoryStats(SARibbonBar* ribbon, DADataStatsWidget* stats)
{
    if (!stats) {
        qWarning() << "DAAppRibbonArea::buildContextCategoryStats: statistics widget is not ready";
        return;
    }
    // 初始不显示：由统计窗口 dock 的可见性驱动（见 setStatsContextCategoryVisible）
    mContextStats = ribbon->addContextCategory(tr("Statistics"));  // cn:统计
    mContextStats->setObjectName(QString::fromUtf8(UiNames::Ribbon::StatsContextCategory));
    mCategoryStats = mContextStats->addCategoryPage(tr("Statistics Operations"));  // cn:统计操作
    mCategoryStats->setObjectName(QString::fromUtf8(UiNames::Ribbon::StatsOperateCategory));

    // ---- 统计参数面板：大/中按钮交错，中型按钮一列两个 ----
    mPannelStatsParams = mCategoryStats->addPanel(tr("Statistics Parameters"));  // cn:统计参数
    mPannelStatsParams->setObjectName(QString::fromUtf8(UiNames::Ribbon::StatsParamsPanel));
    mPannelStatsParams->addLargeAction(stats->command(DADataStatsWidget::Command::Count));
    mPannelStatsParams->addLargeAction(stats->command(DADataStatsWidget::Command::Mean));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Std));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Min));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Max));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Percentile),
                                        QToolButton::InstantPopup);
    mPannelStatsParams->addLargeAction(stats->command(DADataStatsWidget::Command::Missing));
    mPannelStatsParams->addLargeAction(stats->command(DADataStatsWidget::Command::Unique));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Sum));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Var));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Sem));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Range));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Iqr));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Mad));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Mode));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Cv));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Skew));
    mPannelStatsParams->addMediumAction(stats->command(DADataStatsWidget::Command::Kurt));
    mPannelStatsParams->addLargeAction(stats->command(DADataStatsWidget::Command::Refresh));

    // ---- 检验面板：正态性检验（大）+ 置信区间/离群点（双列中）----
    mPannelStatsTests = mCategoryStats->addPanel(tr("Tests"));  // cn:检验
    mPannelStatsTests->setObjectName(QString::fromUtf8(UiNames::Ribbon::StatsTestsPanel));
    mPannelStatsTests->addLargeAction(stats->command(DADataStatsWidget::Command::TestNormality));
    mPannelStatsTests->addMediumAction(stats->command(DADataStatsWidget::Command::TestConfidence));
    mPannelStatsTests->addMediumAction(stats->command(DADataStatsWidget::Command::TestOutliers));

    // ---- 绘图面板：箱线图/帕累托图（大，互斥）+ 分布率/累计分布（双列中）+ 转入绘图区（大）----
    mPannelContextStatsPlot = mCategoryStats->addPanel(tr("Plots"));  // cn:绘图
    mPannelContextStatsPlot->setObjectName(QString::fromUtf8(UiNames::Ribbon::StatsPlotPanel));
    mPannelContextStatsPlot->addLargeAction(stats->command(DADataStatsWidget::Command::PlotBoxplot));
    mPannelContextStatsPlot->addLargeAction(stats->command(DADataStatsWidget::Command::PlotPareto));
    mPannelContextStatsPlot->addMediumAction(stats->command(DADataStatsWidget::Command::ParetoRate));
    mPannelContextStatsPlot->addMediumAction(stats->command(DADataStatsWidget::Command::ParetoCumulative));
    mPannelContextStatsPlot->addSeparator();
    mPannelContextStatsPlot->addLargeAction(stats->command(DADataStatsWidget::Command::TransferToFigure));
}

/**
 * @brief 显示/隐藏「统计」上下文标签页
 *
 * 只由统计窗口 dock 的可见性驱动：dock 显示时出现并切到「统计操作」页，
 * 关闭后隐藏。刻意不绑定焦点变化——点击 Ribbon 上的按钮会让焦点离开浮动
 * dock，绑焦点会导致统计页一点就消失、动作根本点不到。
 * @param on true 显示，false 隐藏
 */
void DAAppRibbonArea::setStatsContextCategoryVisible(bool on)
{
    if (!mContextStats) {
        return;
    }
    SARibbonBar* ribbon = ribbonBar();
    if (!on) {
        ribbon->hideContextCategory(mContextStats);
        return;
    }
    if (ribbon->isContextCategoryVisible(mContextStats)) {
        return;
    }
    // 记录显示前的上下文页索引：showContextCategory 会把新页追加到末尾
    const QList< int > before = ribbon->currentVisibleContextCategoryTabIndexs();
    ribbon->showContextCategory(mContextStats);
    const QList< int > after = ribbon->currentVisibleContextCategoryTabIndexs();
    for (int index : std::as_const(after)) {
        if (!before.contains(index)) {
            // 让「统计操作」页成为当前页（showContextCategory 只加 tab，不切换当前页）
            ribbon->ribbonTabBar()->setCurrentIndex(index);
            break;
        }
    }
}
