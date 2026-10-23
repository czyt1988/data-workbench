#include "DAAppConfigApplier.h"
#include <QObject>
#include "DAAppCore.h"
#include "DAAppUI.h"
#include "AppMainWindow.h"
#include "SARibbonBar.h"
#include "DAAppActions.h"
#include "DARecentFilesManager.h"
#include "DAMessageLogQueue.h"
#include "DALogger.h"
#include "DADumpCapture.h"
#include "DAPyScriptRunner.h"
#include "DAAppDockingArea.h"
#include "DAUndoHistoryWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include "DAPyWorkFlowOperateWidget.h"
#include "Chart/DAChartOperateWidget.h"
#include "DADataOperateWidget.h"

namespace DA
{
namespace
{

AppMainWindow* mainWindowOf(DAAppCore* core)
{
    if (nullptr == core) {
        return nullptr;
    }
    DAAppUI* ui = core->getAppUi();
    if (nullptr == ui) {
        return nullptr;
    }
    return qobject_cast< AppMainWindow* >(ui->getMainWindow());
}

/**
 * @brief 应用通用/界面配置（Ribbon 样式与主题、内部标签页方位、撤销栈深度、退出保存界面状态）
 */
void applyUISettings(const DAAppConfig& cfg, DAAppCore* core)
{
    const DAUISettings& s = cfg.ui();
    AppMainWindow* mainWindow = mainWindowOf(core);
    if (nullptr == mainWindow) {
        return;
    }
    if (SARibbonBar* bar = mainWindow->ribbonBar()) {
        bar->setRibbonStyle(static_cast< SARibbonBar::RibbonStyles >(s.ribbonStyle));
    }
    // ribbon 主题（-1 表示不覆盖框架默认主题）
    if (s.ribbonTheme >= 0) {
        mainWindow->setRibbonTheme(static_cast< SARibbonTheme >(s.ribbonTheme));
    }
    mainWindow->setSaveUIStateOnClose(s.saveUiStateOnClose);

    DAAppUI* ui = core->getAppUi();
    if (nullptr == ui) {
        return;
    }
    DAAppDockingArea* dockArea = ui->getAppDockingArea();
    if (nullptr == dockArea) {
        return;
    }
    // 操作窗口内部标签页（工作流页/图表/数据表）方位（仅影响嵌套停靠区，外层 dock 区域保持默认）
    const bool tabsAtBottom = (s.dockTabPosition == DADockTabPosition::Bottom);
    if (DAPyWorkFlowOperateWidget* wfo = dockArea->getWorkFlowOperateWidget()) {
        wfo->setInnerDockTabsAtBottom(tabsAtBottom);
    }
    if (DAChartOperateWidget* chartOw = dockArea->getChartOperateWidget()) {
        chartOw->setInnerDockTabsAtBottom(tabsAtBottom);
    }
    if (DADataOperateWidget* dataOw = dockArea->getDataOperateWidget()) {
        dataOw->setInnerDockTabsAtBottom(tabsAtBottom);
    }
    // 撤销栈深度：施加到已打开的数据页 + 同步历史面板 SpinBox（0 = 无限制，面板显示回退 20）
    if (DADataOperateWidget* dataOw = dockArea->getDataOperateWidget()) {
        const auto pages = dataOw->getAllDataFrameWidgets();
        for (DADataOperateOfDataFrameWidget* page : pages) {
            page->setUndoLimit(s.undoStackLimit);
        }
    }
    if (DAUndoHistoryWidget* undoHistory = dockArea->getUndoHistoryWidget()) {
        undoHistory->setUndoLimit(s.undoStackLimit > 0 ? s.undoStackLimit : 20);
    }
}

/**
 * @brief 应用日志分级与界面显示条数（轮转参数只在启动时生效，需重启）
 */
void applyLogSettings(const DAAppConfig& cfg)
{
    const DALogSettings& s = cfg.log();
    DALogger::instance().setLevel(s.level);
    DALogger::instance().setQueueLevel(s.queueLevel);
    DAMessageLogQueue::instance().setCapacity(s.showLogNum);
}

/**
 * @brief 应用高级配置（最近文件条数、崩溃转储清理、自动保存定时器）
 */
void applyAdvancedSettings(const DAAppConfig& cfg, DAAppCore* core)
{
    const DAAdvancedSettings& s = cfg.advanced();
    if (DAAppUI* ui = core ? core->getAppUi() : nullptr) {
        if (DAAppActions* actions = ui->getAppActions()) {
            if (actions->recentFilesManager) {
                actions->recentFilesManager->setMaxEntries(s.recentFilesMax);
            }
        }
    }
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
    DADumpCapture::cleanupOldDumps(s.dumpRetentionDays);
#else
    Q_UNUSED(s.dumpRetentionDays);
#endif
    if (AppMainWindow* mainWindow = mainWindowOf(core)) {
        mainWindow->setupAutosaveTimer(s.autosaveIntervalMin);
    }
}

/**
 * @brief 应用脚本执行引擎参数（workspace-dir 在下次打开工程时生效）
 */
void applyScriptSettings(const DAAppConfig& cfg)
{
    const DAScriptSettings& s = cfg.script();
    DAPyScriptRunner::setScriptTimeout(s.scriptTimeoutSec);
    DAPyScriptRunner::setResultMaxChars(s.scriptResultMaxChars);
}

}  // namespace

void applyAppSettings(const DAAppConfig& cfg, DAAppCore* core, DAAppConfig::Section section)
{
    switch (section) {
    case DAAppConfig::Section::UI:
        applyUISettings(cfg, core);
        break;
    case DAAppConfig::Section::Log:
        applyLogSettings(cfg);
        break;
    case DAAppConfig::Section::Advanced:
        applyAdvancedSettings(cfg, core);
        break;
    case DAAppConfig::Section::Script:
        applyScriptSettings(cfg);
        break;
    case DAAppConfig::Section::Python:
        // Python 解释器路径与额外搜索路径只在启动时生效，运行期无需处理
        break;
    }
}

void applyAllAppSettings(const DAAppConfig& cfg, DAAppCore* core)
{
    applyUISettings(cfg, core);
    applyLogSettings(cfg);
    applyAdvancedSettings(cfg, core);
    applyScriptSettings(cfg);
}

}  // namespace DA
