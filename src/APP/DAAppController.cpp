#include "DAAppController.h"
// Qt
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTimer>
#include <QFontComboBox>
#include <QComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QApplication>
#include <QActionGroup>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QClipboard>
// qwt
#include "qwt_figure.h"
#include "qwt_plot_series_data_picker.h"

// API
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "DAAppRibbonArea.h"
#include "DAAppDockingArea.h"
#include "DAAppCommand.h"
#include "DAAppActions.h"
#include "DAAppDataManager.h"
#include "DAProjectInterface.h"
// plugin
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// command
// Widget
#include "DAWaitCursorScoped.h"
#include "DADataOperateWidget.h"
#include "DADataOperatePageWidget.h"
#include "DADataManageWidget.h"
#include "DAAppChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartAxisSettingPanel.h"
#include "DAAppChartManageWidget.h"
#include "SettingPages/DASettingPageGeneral.h"
#include "DASettingContainerWidget.h"
#include "DARecentFilesManager.h"
#include "Chart/DAChartSettingWidget.h"
#include "DAColorTheme.h"
#include "DAGui/ChartSetting/DAFigureWidgetSettingPanel.h"
#include "DAGui/Chart3DSetting/DAChart3DSettingWidget.h"
// Dialog
#include "DAPluginManagerDialog.h"
#include "DAAppSettingDialog.h"
#include "Dialog/DAExportToPngSettingDialog.h"
#include "Dialog/DAWorkbenchAboutDialog.h"
// DAWidgets
#include "DAFontEditPannelWidget.h"
#include "DAShapeEditPannelWidget.h"
// Table style
#include "DATableCellStyle.h"
// Workflow
#include "DAPyWorkFlowOperateWidget.h"
#include "DAPyWorkFlowGraphicsView.h"
#include "DAGraphicsDrawRectSceneAction.h"
#include "DAGraphicsDrawTextItemSceneAction.h"
// project
#include "DAAppProject.h"
#include "DAAppProjectActionPolicy.h"
// Py
#include "DAPybind11QtCaster.hpp"
#include "Dialog/DATxtFileImportDialog.h"
#include "DAPyDTypeComboBox.h"
#include "DAPyScripts.h"
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"
// Widget
#include "DADataOperateOfDataFrameWidget.h"
#include "DATableDisplayFormatComboBox.h"
#include "Dialog/DADialogTableDisplayFormat.h"
#include "DADataTableView.h"
// Python workflow
#include "DAPyWorkFlowScene.h"
// Stats plot widgets
#include "Dialog/DADialogStatsChartGuide.h"
#include "ChartAddItem/DAChartAddStatsHistplotWidget.h"
#include "ChartAddItem/DAChartAddStatsKdeplot1dWidget.h"
#include "ChartAddItem/DAChartAddStatsKdeplot2dWidget.h"
#include "ChartAddItem/DAChartAddStatsBoxplotWidget.h"
#include "ChartAddItem/DAChartAddStatsHeatmapWidget.h"
#include "ChartAddItem/DAChartAddStatsScatterplotWidget.h"
#include "ChartAddItem/DAChartAddStatsBarplotWidget.h"
#include "ChartAddItem/DAChartAddStatsRegplotWidget.h"
#include "ChartAddItem/DAChartAddStatsEcdfplotWidget.h"
#include "ChartAddItem/DAChartSeriesSelectWidget.h"
#include "ChartAddItem/DAAbstractStatsChartAddWidget.h"
#include "DAStatsPlotCoordinator.h"
// Agent
#include "DAAgentModule.h"
#include "DAAgentInterface.h"        // PMF connect 到接口信号/方法需完整类型
#include "DAAgentDockWidget.h"       // DAAppDockingArea 仅前向声明；PMF connect 需完整类型（plan-02）
//
#include "SettingPages/DAAppConfig.h"
#include "DALogCategory.h"

#ifndef DAAPPRIBBONAREA_WINDOW_NAME
#define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppController", "DA", nullptr)
#endif

// #ifndef DAAPPRIBBONAREA_WINDOW_NAME
// #define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppController", u8"试验数据分析系统", nullptr)
// #endif

// 未实现的功能标记
#define DAAPPCONTROLLER_PASS()                                                                                         \
    QMessageBox::warning(                                                                                              \
        app(),                                                                                                         \
        QCoreApplication::translate("DAAppRibbonArea", "warning", nullptr),                                            \
        QCoreApplication::translate("DAAppRibbonArea",                                                                 \
                                    "The current function is not implemented, only the UI is reserved, "               \
                                    "please pay attention: https://gitee.com/czyt1988/data-work-flow",                 \
                                    nullptr))

// 快速链接信号槽
#define DAAPPCONTROLLER_ACTION_BIND(actionname, functionname)                                                          \
    connect(actionname, &QAction::triggered, this, &DAAppController::functionname)

namespace DA
{

DAAppController::DAAppController(QObject* par) : QObject(par)
{
}

DAAppController::~DAAppController()
{
}
/**
 * @brief 设置AppMainWindow
 * @param mainWindow
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppMainWindow(AppMainWindow* mainWindow)
{
    mMainWindow = mainWindow;
    return (*this);
}

/**
 * @brief 设置core
 * @param core
 * @return
 */
DAAppController& DAAppController::setAppCore(DAAppCore* core)
{
    mCore    = core;
    mProject = mCore->getProjectInterface();
    return (*this);
}
/**
 * @brief 设置ribbon
 * @param ribbon
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppRibbonArea(DAAppRibbonArea* ribbon)
{
    mRibbon = ribbon;
    return (*this);
}

/**
 * @brief 设置dock
 * @param dock
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppDockingArea(DAAppDockingArea* dock)
{
    mDock = dock;

    return (*this);
}

/**
 * @brief 设置AppCommand
 * @param cmd
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppCommand(DAAppCommand* cmd)
{
    mCommand = cmd;
    return (*this);
}

/**
 * @brief 设置AppActions
 * @param act
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppActions(DAAppActions* act)
{
    mActions = act;
    return (*this);
}

/**
 * @brief 设置AppDataManager
 * @param d
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppDataManager(DAAppDataManager* d)
{
    mDatas = d;
    return (*this);
}

/**
 * @brief 获取app
 * @return
 */
AppMainWindow* DAAppController::app() const
{
    return mMainWindow;
}

/**
 * @brief 控制层初始化
 */
void DAAppController::initialize()
{
    mDock->getChartOperateDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getChartOperateDock()->setToggleViewAction(mActions->actionShowChartArea);
    mDock->getChartManageDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getChartManageDock()->setToggleViewAction(mActions->actionShowChartManagerArea);
    mDock->getWorkFlowOperateDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getWorkFlowOperateDock()->setToggleViewAction(mActions->actionShowWorkFlowArea);
    mDock->getWorkflowNodeListDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getWorkflowNodeListDock()->setToggleViewAction(mActions->actionShowWorkFlowManagerArea);
    mDock->getDataOperateDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getDataOperateDock()->setToggleViewAction(mActions->actionShowDataArea);
    mDock->getDataManageDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getDataManageDock()->setToggleViewAction(mActions->actionShowDataManagerArea);
    // Agent 助手 Dock 的显示/隐藏由 actionShowAgentArea 驱动
    mDock->getAgentDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getAgentDock()->setToggleViewAction(mActions->actionShowAgentArea);
    // Agent 信号链：DAGui(Dock) 与 DAAgent 互不依赖，由 APP 层 connect（决策 D3b）。
    // 接口信号 → Dock 槽（13 条）—— DAAgentModule 不再持有 Dock，亦不再在 connectSignals
    // 中连 Bridge→Dock / this→Dock，唯一渲染路径经接口信号。
    auto* agent = mCore->getAgentInterface();
    auto* dock  = mDock->getAgentDockWidget();
    if (agent && dock) {
        connect(agent, &DAAgentInterface::agentToken, dock, &DAAgentDockWidget::onAgentToken);
        connect(agent, &DAAgentInterface::agentMessageComplete, dock, &DAAgentDockWidget::onAgentMessageComplete);
        connect(agent, &DAAgentInterface::agentToolCall, dock, &DAAgentDockWidget::onAgentToolCall);
        connect(agent, &DAAgentInterface::agentToolResult, dock, &DAAgentDockWidget::onAgentToolResult);
        connect(agent, &DAAgentInterface::agentQuestion, dock, &DAAgentDockWidget::onAgentQuestion);
        connect(agent, &DAAgentInterface::agentError, dock, &DAAgentDockWidget::onAgentError);
        connect(agent, &DAAgentInterface::agentRetrying, dock, &DAAgentDockWidget::onAgentRetrying);
        connect(agent, &DAAgentInterface::agentReady, dock, &DAAgentDockWidget::onAgentReady);
        connect(agent, &DAAgentInterface::agentStarting, dock, &DAAgentDockWidget::onAgentStarting);
        connect(agent, &DAAgentInterface::agentBusy, dock, &DAAgentDockWidget::onAgentBusy);
        connect(agent, &DAAgentInterface::agentSessionLoaded, dock, &DAAgentDockWidget::onAgentSessionLoaded);
        connect(agent, &DAAgentInterface::tokenUsageUpdated, dock, &DAAgentDockWidget::onAgentUsage);
        connect(agent, &DAAgentInterface::sessionSwitched, dock, &DAAgentDockWidget::onSessionSwitched);
        connect(agent, &DAAgentInterface::sessionListChanged, dock, &DAAgentDockWidget::onSessionListChanged);
        connect(agent, &DAAgentInterface::sessionCreated, dock, &DAAgentDockWidget::onSessionCreated);
        connect(agent, &DAAgentInterface::sessionCleared, dock, &DAAgentDockWidget::onSessionCleared);
        connect(agent, &DAAgentInterface::systemMessage, dock, &DAAgentDockWidget::onSystemMessage);
        // 供应商/多模型选择：接口信号 → Dock 槽（2 条），Dock 信号 → 接口方法（1 条）
        connect(agent, &DAAgentInterface::availableModelsChanged, dock, &DAAgentDockWidget::onAvailableModelsChanged);
        connect(agent, &DAAgentInterface::activeModelChanged, dock, &DAAgentDockWidget::onActiveModelChanged);
        connect(dock, &DAAgentDockWidget::activeModelChangeRequested, agent, &DAAgentInterface::setActiveModel);
        // Dock 信号 → 接口方法（7 条；均为信号→方法 PMF 连接，emit 源信号即调用方法体，
        // 含各自持久化/启动逻辑，无需 lambda。agentStopRequested 暂无对接，略）。
        // 注意 sessionCreateRequested 连 &DAAgentInterface::newSession（非 createSession）：
        // newSession emit sessionCreated → onSessionCreated → clearChat；createSession 不 emit
        // sessionCreated，连错会导致点"+"后聊天区不清空。
        connect(dock, &DAAgentDockWidget::sendMessageRequested, agent, &DAAgentInterface::sendMessage);
        connect(dock, &DAAgentDockWidget::stopRequested, agent, &DAAgentInterface::stop);
        connect(dock, &DAAgentDockWidget::userAnswerSelected, agent, &DAAgentInterface::sendUserAnswer);
        connect(dock, &DAAgentDockWidget::sessionSwitchRequested, agent, &DAAgentInterface::switchSession);
        connect(dock, &DAAgentDockWidget::sessionDeleteRequested, agent, &DAAgentInterface::deleteSession);
        connect(dock, &DAAgentDockWidget::sessionRenameRequested, agent, &DAAgentInterface::renameSession);
        connect(dock, &DAAgentDockWidget::sessionCreateRequested, agent, &DAAgentInterface::newSession);
        // Agent 绘图引用超链接：da-figure: 协议链接点击 → raise 绘图区并定位 figure
        connect(dock, &DAAgentDockWidget::figureLinkRequested, this, &DAAppController::onFigureLinkRequested);
    }
    initConnection();
    initScripts();
    initPyWorkflowConnections();
    // 数据管理树的 series 节点右键菜单（与 DataFrame 表头右键共用 action）
    if (DADataManageWidget* dmw = getDataManageWidget()) {
        if (DADataManagerTreeWidget* tree = dmw->getTreeWidget()) {
            setupDataManagerTreeSeriesContextMenu(tree);
        }
    }

    // Agent 会话清理调度（plan-05 步骤6）：启动即清，避免积压；
    // cleanupSessions 读 QSettings agent/max_sessions(默认20)、session_retention_days(默认30)
    // → cleanupOldSessions(max, days, m_currentSessionId)，skip 活跃会话 + lastActive 保护，
    // 清理在任何时机都安全（非时序保护所必需）。
    if (auto* agentMod = qobject_cast< DAAgentModule* >(mCore->getAgentInterface())) {
        agentMod->cleanupSessions();
    }

    // 启动后初始化会话 UI + 预启动 agent——改由 AppMainWindow::init() 末尾调用
    // postPluginInit()，确保在 initPlugins() 加载所有插件工具之后再执行。
    // 此前用 QTimer::singleShot(0,...) 延迟，但 AppMainWindow 构造函数中的
    // updateSplash() → processEvents() 会在 init() → initPlugins() 之前触发
    // 0ms 定时器，导致 prestartAgent 的 init 消息携带空工具列表。
}

void DAAppController::postPluginInit()
{
    // 插件加载后调用：推送模型选择 + 预启动 agent。
    // 由 AppMainWindow::init()（在 initPlugins() 之后）经 singleShot(0) 调用，
    // 确保所有插件工具已注册，prestartAgent 的 init 消息将携带完整工具列表。
    if (auto* agentMod = qobject_cast< DAAgentModule* >(mCore->getAgentInterface())) {
        agentMod->setCurrentProjectPath(QString());  // 启动无工程，projectPath=null
        agentMod->restoreLastActiveSession();         // 填充下拉 + 全新对话
        agentMod->pushModelSelection();  // 推送供应商/模型列表 + 激活选择到 Dock 下拉
        agentMod->prestartAgent();  // 预启动 agent 子进程（受 auto_prestart 开关 + LLM 配置控制）
    }
}

/**
 * @brief 基本绑定
 * @note 在setDockAreaInterface函数中还有很多绑定操作
 */
void DAAppController::initConnection()
{
    // Main Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionOpen, open);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionSave, save);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionSaveAs, saveAs);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAppendProject, onActionAppendProjectTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionOpenMarkdown, onActionOpenMarkdownTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionSetting, onActionSettingTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionPluginManager, onActionPluginManagerTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAbout, onActionAboutTriggered);
    // Data Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAddData, onActionAddDataTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRemoveData, onActionRemoveDataTriggered);
    // Chart Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAddFigure, onActionAddFigureTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionFigureNewXYAxis, onActionFigureNewXYAxisTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddCurve, onActionChartAddCurveTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddScatter2D, onActionChartAddScatterTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddBar, onActionChartAddBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddErrorBar, onActionactionChartAddErrorBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddBoxPlot, onActionChartAddBoxPlotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddCloudMap, onActionChartAddCloudMapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddMultiBar, onActionChartAddMultiBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddHistogramBar, onActionChartAddHistogramTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddContourMap, onActionChartAddContourMapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddVectorfield, onActionChartAddVectorfieldTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAdd3DSurface, onActionChartAdd3DSurfaceTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAdd3DBar, onActionChartAdd3DBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAdd3DLine, onActionChartAdd3DLineTriggered);

    // Stats Plot
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsHistplot, onActionStatsHistplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsKdeplot1d, onActionStatsKdeplot1dTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsKdeplot2d, onActionStatsKdeplot2dTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsBoxplot, onActionStatsBoxplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsHeatmap, onActionStatsHeatmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsScatterplot, onActionStatsScatterplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsBarplot, onActionStatsBarplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsRegplot, onActionStatsRegplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsECDFplot, onActionStatsECDFplotTriggered);

    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGrid, onActionChartEnableGridTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridX, onActionChartEnableGridXTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridY, onActionChartEnableGridYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridXMin, onActionChartEnableGridXMinEnableTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridYMin, onActionChartEnableGridYMinTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableZoom, onActionChartEnableZoomTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartZoomIn, onActionChartZoomInTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartZoomOut, onActionChartZoomOutTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartZoomAll, onActionChartZoomAllTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePan, onActionChartEnablePanTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePickerCross, onActionChartEnablePickerCrossTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePickerY, onActionChartEnablePickerYTriggered);
    connect(mActions->actionGroupChartPickerTextRegion,
            &QActionGroup::triggered,
            this,
            &DAAppController::onActionGroupChartPickerTextRegionTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartYPickerShowXValueEnabled,
                                onActionChartYPickerShowXValueEnabledTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePickerXY, onActionChartEnablePickerXYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartLinkAllPickerEnabled, onActionChartLinkAllPickerEnabledTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableLegend, onActionChartEnableLegendTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCopyFigureInClipboard, onActionCopyFigureToClipboardTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartDataPickerSetting, onActionChartDataPickerSettingTriggered);
    for (QAction* act : std::as_const(mActions->actionListOfColorTheme)) {
        connect(act, &QAction::triggered, this, [ this, act ]() { onActionGroupFigureThemeTriggered(act); });
    }
    connect(mActions->actionGroupChartEditor, &QActionGroup::triggered, this, &DAAppController::onActionGroupChartEditorTriggered);
    // 数据操作的上下文标签 Data Operate Context Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionInsertRow, onActionInsertRowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionInsertRowAbove, onActionInsertRowAboveTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionInsertColumnRight, onActionInsertColumnRightTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionInsertColumnLeft, onActionInsertColumnLeftTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRemoveRow, onActionRemoveRowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRemoveColumn, onActionRemoveColumnTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRemoveCell, onActionRemoveCellTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRenameColumns, onActionRenameColumnsTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRenameColumn, onActionRenameColumnTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCopyColumnName, onActionCopyColumnNameTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionGotoMax, onActionGotoMaxTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionGotoMin, onActionGotoMinTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowColumnDescribe, onActionShowColumnDescribeTriggered);

    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCastToNum, onActionCastToNumTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCastToString, onActionCastToStringTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCastToDatetime, onActionCastToDatetimeTriggered);
    // 不知为何使用函数指针无法关联信号和槽
    //  connect(mComboxColumnTypes, &DAPyDTypeComboBox::currentDTypeChanged, this,&DAAppRibbonArea::onComboxColumnTypesCurrentDTypeChanged);
    //  QObject::connect: signal not found in DAPyDTypeComboBox
    connect(mRibbon->mComboxColumnTypes,
            &DAPyDTypeComboBox::currentDTypeChanged,
            this,
            &DAAppController::onComboxColumnTypesCurrentDTypeChanged);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChangeToIndex, onActionChangeToIndexTriggered);
    // 表格样式
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionClearStyleSelected, onActionClearStyleSelectedTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionClearStyleAll, onActionClearStyleAllTriggered);
    connect(mRibbon->mBtnTableFillColor,
            &SARibbonColorToolButton::colorChanged,
            this,
            &DAAppController::onTableStyleFillColorChanged);
    connect(mRibbon->mWidgetTableFont,
            &DAFontEditPannelWidget::currentFontChanged,
            this,
            &DAAppController::onTableStyleFontChanged);
    connect(mRibbon->mWidgetTableFont,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAAppController::onTableStyleFontColorChanged);
    // 表格列显示格式
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionTableFormatCells, onActionTableFormatCellsTriggered);
    connect(mRibbon->mComboxDisplayFormat,
            &DATableDisplayFormatComboBox::currentFormatCategoryChanged,
            this,
            &DAAppController::onTableDisplayFormatCategoryChanged);
    // View Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowWorkFlowArea, onActionShowWorkFlowAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowWorkFlowManagerArea, onActionShowWorkFlowManagerAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowChartArea, onActionShowChartAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowChartManagerArea, onActionShowChartManagerAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowDataArea, onActionShowDataAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowDataManagerArea, onActionShowDataManagerAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowMessageLogView, onActionShowMessageLogViewTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowSettingWidget, onActionSettingWidgetTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowRightSideBar, onActionShowRightSideBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowLeftSideBar, onActionShowLeftSideBarTriggered);
    // workflow view 工作流视图
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowViewReadOnly, onActionWorkflowViewReadOnlyTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowViewMarker, onActionWorkflowViewMarkerTriggered);
    // workflow edit 工作流编辑
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowNew, onActionNewWorkflowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowEnableItemLinkageMove,
                                onActionWorkflowEnableItemLinkageMoveTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowLinkEnable, onActionWorkflowLinkEnableTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionItemGrouping, onActionItemGroupingTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionItemUngroup, onActionItemUngroupTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowRun, onActionRunCurrentWorkflowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowTerminate, onActionTerminateCurrentWorkflowTriggered);
    // workflow edit 工作流编辑/data edit 绘图编辑
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowStartDrawRect, onActionStartDrawRectTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowStartDrawText, onActionStartDrawTextTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowAddBackgroundPixmap, onActionAddBackgroundPixmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowLockBackgroundPixmap, onActionLockBackgroundPixmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowEnableItemMoveWithBackground,
                                onActionEnableItemMoveWithBackgroundTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportWorkflowSceneToPNG, onActionExportWorkflowScenePNGTriggered);
    // other
    connect(mActions->actionGroupRibbonTheme, &QActionGroup::triggered, this, &DAAppController::onActionGroupRibbonThemeTriggered);
    //===================================================
    // setDockAreaInterface 有其他的绑定
    //===================================================
    //! 注意！！
    //! 在setDockAreaInterface函数中还有很多绑定操作
    //
    DAAppProject* p = mCore->getAppProject();
    if (p) {
        connect(p, &DAAppProject::projectSaved, this, &DAAppController::onProjectSaved);
        connect(p, &DAAppProject::projectLoaded, this, &DAAppController::onProjectLoaded);
        connect(p, &DAAppProject::dirtyStateChanged, this, &DAAppController::onProjectDirtyStateChanged);
    }
    //===================================================
    // Edit标签字体相关信号槽
    //===================================================
    connect(mRibbon, &DAAppRibbonArea::selectedFont, this, &DAAppController::onEditFontChanged);
    connect(mRibbon, &DAAppRibbonArea::selectedFontColor, this, &DAAppController::onEditFontColorChanged);
    connect(mRibbon, &DAAppRibbonArea::selectedBrush, this, &DAAppController::onEditBrushChanged);
    connect(mRibbon, &DAAppRibbonArea::selectedPen, this, &DAAppController::onEditPenChanged);

    //===================================================
    // workflow窗口字体相关信号槽
    //===================================================

    connect(mRibbon, &DAAppRibbonArea::selectedWorkflowItemFont, this, &DAAppController::onCurrentWorkflowFontChanged);
    connect(mRibbon, &DAAppRibbonArea::selectedWorkflowItemFontColor, this, &DAAppController::onCurrentWorkflowFontColorChanged);
    connect(mRibbon,
            &DAAppRibbonArea::selectedWorkflowItemBrush,
            this,
            &DAAppController::onCurrentWorkflowShapeBackgroundBrushChanged);
    connect(mRibbon, &DAAppRibbonArea::selectedWorkflowItemPen, this, &DAAppController::onCurrentWorkflowShapeBorderPenChanged);

    //===================================================
    // name
    //===================================================
    connect(mDock->dockManager(), &ads::CDockManager::focusedDockWidgetChanged, this, &DAAppController::onFocusedDockWidgetChanged);
    // 程序化raise dock窗口时也激活context category
    connect(mDock, &DADockingAreaInterface::dockWidgetRaised, this, &DAAppController::onDockWidgetRaised);
    // DADataManageWidget 数据操作
    // DADataOperateWidget
    DADataOperateWidget* dow = mDock->getDataOperateWidget();
    connect(dow, &DADataOperateWidget::dataTableCreated, this, &DAAppController::onDataOperatePageCreated);
    // DAChartManager
    DAChartManageWidget* cmw = mDock->getChartManageWidget();
    connect(cmw, &DAChartManageWidget::figureElementClicked, this, &DAAppController::onFigureElementClicked);
    connect(cmw, &DAChartManageWidget::figureElementClicked, this, &DAAppController::onFigureElementDbClicked);
    // figure 窗口设置按钮信号
    connect(cmw, &DAChartManageWidget::requestFigureSetting, this, [ this ](DA::DAFigureWidget* fig) {
        DASettingContainerWidget* setting = getSettingContainerWidget();
        if (setting) {
            DAFigureWidgetSettingPanel* panel = setting->getFigureWidgetSettingWidget();
            if (panel) {
                panel->setTarget(fig);
                setting->showFigureWidgetSettingWidget();
            }
        }
        // 显示设置窗口 dock
        mDock->raiseDockByWidget((QWidget*)(mDock->getSettingContainerWidget()));
    });
    // 坐标轴设置面板的可见性变化 -> 刷新DAChartManageWidget树形控件的可见性列
    if (DASettingContainerWidget* setting = getSettingContainerWidget()) {
        if (DAChartSettingWidget* chartSetting = setting->getChartSettingWidget()) {
            connect(chartSetting, &DAChartSettingWidget::axisVisibilityChanged, cmw, &DAChartManageWidget::refreshAxisVisibility);
        }
    }
    // DAChartOperateWidget
    DAChartOperateWidget* cow = mDock->getChartOperateWidget();
    connect(cow, &DAChartOperateWidget::figureCreated, this, &DAAppController::onFigureCreated);
    connect(cow, &DAChartOperateWidget::currentFigureChanged, this, &DAAppController::onCurrentFigureChanged);
    // 绘图项创建完成时提升绘图 dock，让用户能看到新绘图
    if (DAAppChartOperateWidget* appCow = qobject_cast< DAAppChartOperateWidget* >(cow)) {
        connect(appCow, &DAAppChartOperateWidget::plotItemCreated, this, &DAAppController::onPlotItemCreated);
        connect(appCow, &DAAppChartOperateWidget::plot3DItemCreated, this, &DAAppController::onPlot3DItemCreated);
    }
    //
    DAPyWorkFlowOperateWidget* workflowOpt = mDock->getWorkFlowOperateWidget();
    // 鼠标动作完成的触发
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::sceneActionDeactived,
            this,
            &DAAppController::onWorkFlowGraphicsSceneActionDeactive);
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::selectionItemChanged,
            this,
            &DAAppController::onWorkflowSceneSelectionItemChanged);
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
            this,
            &DAAppController::onCurrentWorkflowWidgetChanged);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowStartExecute, this, &DAAppController::onWorkflowStartExecute);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowFinished, this, &DAAppController::onWorkflowFinished);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::itemsAdded, this, &DAAppController::onWorkflowSceneitemsAdded);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::itemsRemoved, this, &DAAppController::onWorkflowSceneitemsRemoved);
    connect(mActions->actionWorkflowShowGrid,
            &QAction::triggered,
            workflowOpt,
            &DAPyWorkFlowOperateWidget::setCurrentWorkflowShowGrid);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowCreated, this, &DAAppController::onWorkflowCreated);

    connect(mActions->recentFilesManager, &DARecentFilesManager::fileSelected, this, &DAAppController::onRecentFileSelected);
}

/**
 * @brief 设置工程为脏
 *
 * @note 如果工程状态已经是脏，此函数不会做任何动作也不会触发任何信号
 * @param on
 */
void DAAppController::setDirty(bool on)
{
    if (mProject) {
        mProject->setModified(on);
    }
}

/**
 * @brief 工程是否为脏
 * @return
 */
bool DAAppController::isDirty() const
{
    if (mProject) {
        return mProject->isDirty();
    }
    return false;
}

/**
 * @brief 导入数据
 * @param filePath
 * @param args
 * @return
 */
bool DAAppController::importData(const QString& filePath, const QVariantMap& args, QString* err)
{
    bool r = mDatas->importFromFile(filePath, args, err);
    if (r) {
        mDock->raiseDockByWidget((QWidget*)(mDock->getDataManageWidget()));
        setDirty();
    }
    return r;
}

/**
 * @brief 更新窗口标题
 */
void DAAppController::updateWindowTitle()
{
    DAAppProject* project = DA_APP_CORE.getAppProject();
    if (!project || project->isEmpty()) {
        app()->setWindowTitle(makeWindowTitle());
        return;
    }
    app()->setWindowTitle(makeWindowTitle(project));
}

/**
 * @brief 生成窗口标题
 * @return
 */
QString DAAppController::makeWindowTitle()
{
    return QString("%1 [*]").arg(DAAPPRIBBONAREA_WINDOW_NAME);
}

/**
 * @brief 生成当前项目下的窗口标题
 * @return
 */
QString DAAppController::makeWindowTitle(DAProjectInterface* proj)
{
    return QString("%1[*]-%2").arg(DAAPPRIBBONAREA_WINDOW_NAME, proj->getProjectBaseName());
}

/**
 * @brief 是否应用到所有绘图
 * @return
 */
bool DAAppController::isApplyToAllCharts() const
{
    return mActions->actionFigureSettingApplyAllChart->isChecked();
}

bool DAAppController::save()
{
    DAAppProject* project = DA_APP_CORE.getAppProject();
    return project->requestSave();
}

/**
 * @brief 另存为
 */
void DAAppController::saveAs()
{
    QFileDialog dialog(app(),
                       tr("Save Project"),  // cn:保存工程
                       QString(),
                       tr("Project File") + QString(" (*.%1)").arg(DAAppProject::getProjectFileSuffix())  // cn:工程文件
    );
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(DAAppProject::getProjectFileSuffix());
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList files = dialog.selectedFiles();
    if (files.isEmpty()) {
        return;
    }
    QString projectPath = files.first();
    QFileInfo fi(projectPath);
    if (fi.exists()) {
        // 说明是目录
        QMessageBox::StandardButton btn = QMessageBox::question(
            app(),
            tr("Warning"),                                                      // cn:警告
            tr("Whether to overwrite the file: %1").arg(fi.absoluteFilePath())  // cn:是否覆盖文件:%1
        );
        if (btn != QMessageBox::Yes) {
            return;
        }
    }
    // 另存为
    DA_WAIT_CURSOR_SCOPED();
    DAAppProject* project = DA_APP_CORE.getAppProject();
    if (!project->save(projectPath)) {
        daCritical << tr("Failed to save project! Path: %1").arg(projectPath);  // cn:工程保存失败！路径为:%1
        return;
    }
    daInfo << tr("Project saved successfully, path: %1").arg(projectPath);  // cn:工程保存成功，路径为:%1
}
/**
 * @brief 获取当前dataframeOperateWidget,如果没有返回nullptr
 *
 * 此函数不返回nullptr的前提是:DataOperateWidget处于焦点，且是DataFrameOperateWidget
 * @param checkDataOperateAreaFocused 是否检测DataOperateWidget是否处于焦点，默认为true
 * @return
 */
DADataOperateOfDataFrameWidget* DAAppController::getCurrentDataFrameOperateWidget(bool checkDataOperateAreaFocused,
                                                                                  bool isShowMessage)
{
    if (nullptr == mDock) {
        return nullptr;
    }
    if (checkDataOperateAreaFocused) {
        if (!(mDock->isDockingAreaFocused(DAAppDockingArea::DockingAreaDataOperate))) {
            // 窗口未选中就退出
            if (isShowMessage) {
                daWarning << tr("Please select the data operation window");  // cn:请选中数据操作窗口
            }
            return nullptr;
        }
    }
    return mDock->getDataOperateWidget()->getCurrentDataFrameWidget();
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAPyWorkFlowOperateWidget* DAAppController::getWorkFlowOperateWidget() const
{
    return mDock->getWorkFlowOperateWidget();
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataOperateWidget* DAAppController::getDataOperateWidget() const
{
    return mDock->getDataOperateWidget();
}

/**
 * @brief 获取绘图操作窗口
 * @return
 */
DAAppChartOperateWidget* DAAppController::getChartOperateWidget() const
{
    return qobject_cast< DAAppChartOperateWidget* >(mDock->getChartOperateWidget());
}

/**
 * @brief 获取数据管理窗口
 * @return
 */
DADataManageWidget* DAAppController::getDataManageWidget() const
{
    return mDock->getDataManageWidget();
}
/**
 * @brief 获取当前的绘图
 * @return 如果没有回返回nullptr
 */
DAFigureWidget* DAAppController::getCurrentFigure()
{
    return getChartOperateWidget()->getCurrentFigure();
}

DAFigureWidget* DAAppController::gcf()
{
    return getCurrentFigure();
}

/**
 * @brief 获取当前的图表
 * @return
 */
DAChartWidget* DAAppController::getCurrentChart() const
{
    return getChartOperateWidget()->getCurrentChart();
}

/**
 * @brief 获取当前的所有图表
 * @return
 */
QList< DAChartWidget* > DAAppController::getAllCharts() const
{
    return getChartOperateWidget()->getAllCharts();
}

QList< DAChartWidget* > DAAppController::gcas() const
{
    return getAllCharts();
}

/**
 * @brief 给当前绘图应用方法
 * @param fp
 * @return 成功应用返回true,没有任何绘图返回false,执行过程出错也返回false
 */
bool DAAppController::applyToCharts(const DAAppController::FpChartWidgetApply& fp)
{
    const QList< DAChartWidget* > ws = needOperateCharts();
    for (DAChartWidget* w : ws) {
        if (!fp(w)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 获取需要操作的绘图
 * @return
 */
QList< DAChartWidget* > DAAppController::needOperateCharts() const
{
    QList< DAChartWidget* > ws;
    if (isApplyToAllCharts()) {
        ws = getAllCharts();
    } else {
        ws.append(getCurrentChart());
    }
    return ws;
}

DAChartWidget* DAAppController::gca() const
{
    return getCurrentChart();
}

/**
 * @brief 获取设置窗口
 * @return
 */
DASettingContainerWidget* DAAppController::getSettingContainerWidget() const
{
    return mDock->getSettingContainerWidget();
}

/**
 * @brief 判断当前是否是在绘图操作模式，就算绘图操作不在焦点，但绘图操作在前端，此函数也返回true
 * @return
 */
bool DAAppController::isLastFocusedOnChartOptWidget() const
{
    return mLastFocusedOpertateWidget.testFlag(LastFocusedOnChartOpt);
}

/**
 * @brief 判断当前是否是在工作流操作模式，就算工作流操作不在焦点，但工作流操作在前端，此函数也返回true
 * @return
 */
bool DAAppController::isLastFocusedOnWorkflowOptWidget() const
{
    return mLastFocusedOpertateWidget.testFlag(LastFocusedOnWorkflowOpt);
}

/**
 * @brief 判断当前是否是在数据操作模式，就算数据操作不在焦点，但工作流操作在前端，此函数也返回true
 * @return
 */
bool DAAppController::isLastFocusedOnDataOptWidget() const
{
    return mLastFocusedOpertateWidget.testFlag(LastFocusedOnDataOpt);
}

DAAppConfig* DAAppController::getConfig() const
{
    return mConfig;
}

void DAAppController::setConfig(DAAppConfig* config)
{
    mConfig = config;
}

/**
 * @brief GraphicsScene的鼠标动作执行完成，把action的选中标记清除
 * @param mf
 */
void DAAppController::onWorkFlowGraphicsSceneActionDeactive(DA::DAAbstractGraphicsSceneAction* scAction)
{
    if (DAGraphicsDrawRectSceneAction* d = dynamic_cast< DAGraphicsDrawRectSceneAction* >(scAction)) {
        mActions->actionWorkflowStartDrawRect->setChecked(false);
    } else if (DAGraphicsDrawTextItemSceneAction* d = dynamic_cast< DAGraphicsDrawTextItemSceneAction* >(scAction)) {
        mActions->actionWorkflowStartDrawText->setChecked(false);
    }
}

/**
 * @brief DAPyWorkFlowOperateWidget有新的工作流窗口创建会触发此槽
 * @param wfw
 */
void DAAppController::onWorkflowCreated(DAPyWorkFlowEditWidget* wfw)
{
    if (mCommand) {
        mCommand->addStack(wfw->getUndoStack());
    }
}

/**
 * @brief 最近打开文件有选择
 * @param filePath
 */
void DAAppController::onRecentFileSelected(const QString& filePath)
{
    if (!openCheck()) {
        return;
    }
    DA_WAIT_CURSOR_SCOPED();
    openProjectFile(filePath);
}

/**
 * @brief 绘图元素选中
 *
 * 信号由DAChartManageWidget发出
 * @param selection
 */
void DAAppController::onFigureElementClicked(const DAFigureElementSelection& selection)
{
    // 这里和设置页面同步
    DASettingContainerWidget* setting = getSettingContainerWidget();
    if (!setting) {
        return;
    }

    // === 3D 选择分支 ===
    if (selection.isSelectedPlot3D() || selection.isSelectedPlot3DItem()) {
        // 切换到 3D 设置面板
        DAChart3DSettingWidget* chart3DSetting = setting->getChart3DSettingWidget();
        if (!chart3DSetting) {
            return;
        }
        chart3DSetting->setSelection(selection);
        setting->showChart3DSettingWidget();
        // 如果选中了 3D plot，切换 figure 的 current 3D chart
        if (selection.isSelectedPlot3D()) {
            selection.figureWidget->setCurrent3DChart(selection.plot3D);
        }
        return;
    }

    // === 2D 选择分支 ===
    // 引入 3D 面板后，2D 分支必须显式切换回 2D 面板，
    // 否则用户从 3D 选中切换到 2D 时面板仍停留在 3D 设置。
    DAChartSettingWidget* chartSetting = setting->getChartSettingWidget();
    if (!chartSetting) {
        return;
    }
    setting->showChartSettingWidget();
    chartSetting->setSelection(selection);
    if (selection.isSelectedPlot()) {
        // 单独选中plot，那么把figure的current plot进行切换
        if (selection.plot->isHostPlot()) {
            selection.figureWidget->setCurrentChart(selection.plot);
        } else {
            selection.figureWidget->setCurrentChart(selection.plot->hostPlot());
        }
    }
    // setSelection 在 plot 未变时不会调用 updateUI，需显式刷新坐标轴面板的可见性 checkbox，
    // 确保右键菜单等途径修改可见性后面板状态同步
    if (selection.isSelectedScaleWidget()) {
        if (DAChartAxisSettingPanel* p = chartSetting->getChartAxisSetWidget(selection.axisId)) {
            p->updateUI();
        }
    }
}

/**
 * @brief 双击显示绘图和设置对话框
 * @param selection
 */
void DAAppController::onFigureElementDbClicked(const DAFigureElementSelection& selection)
{
    // === 3D 双击：与单击一致，同步设置面板并提升 dock ===
    if (selection.isSelectedPlot3D() || selection.isSelectedPlot3DItem()) {
        onFigureElementClicked(selection);
        DASettingContainerWidget* setting = getSettingContainerWidget();
        if (setting) {
            setting->showChart3DSettingWidget();
        }
        mDock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
        return;
    }

    // === 2D 逻辑 ===
    // 和单击事件一致
    onFigureElementClicked(selection);
    // 同步查看是否设置对话框在前台
    DASettingContainerWidget* setting = getSettingContainerWidget();
    if (setting) {
        setting->showChartSettingWidget();
    }
    // 把绘图界面显示在前台
    mDock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
    // 如果是visible双击，那么改变item的visible属性

    if (DAFigureElementSelection::ColumnVisible == selection.selectionColumn) {
        DAChartManageWidget* cmw = mDock->getChartManageWidget();
        if (selection.isSelectedPlotItem()) {
            selection.plotItem->setVisible(!selection.plotItem->isVisible());
            // plotitem可见性改变后，需要通知刷新
            selection.plot->replot();
            // 刷新树形控件的可见性列显示
            if (cmw) {
                cmw->refreshPlotItemVisibility(selection.plotItem);
            }
        } else if (selection.isSelectedScaleWidget()) {
            bool isAxisVisible = selection.plot->isAxisVisible(selection.axisId);
            selection.plot->setAxisVisible(selection.axisId, !isAxisVisible);
            // 对于设置窗口要进行更新
            if (setting) {
                if (DAChartSettingWidget* chartSetting = setting->getChartSettingWidget()) {
                    if (DAChartAxisSettingPanel* axisSettingWidget = chartSetting->getChartAxisSetWidget(selection.axisId)) {
                        axisSettingWidget->updateUI();
                    }
                }
            }
            // 刷新树形控件的可见性列显示
            if (cmw) {
                cmw->refreshAxisVisibility(selection.plot, selection.axisId);
            }
        }
    }
}

/**
 * @brief 插件管理 [config category] - [Plugin Manager]
 * @param on
 */
void DAAppController::onActionPluginManagerTriggered(bool on)
{
    Q_UNUSED(on);
    DAPluginManagerDialog dlg(mMainWindow->getPluginManager(), app());

    dlg.exec();
}

/**
 * @brief 关于
 */
void DAAppController::onActionAboutTriggered()
{
    DAWorkbenchAboutDialog dlg(mMainWindow);
    dlg.exec();
}

/**
 * @brief 设定界面
 */
void DAAppController::onActionSettingTriggered()
{
    if (mMainWindow) {
        mMainWindow->showSettingDialog();
    }
}

/**
 * @brief 根据widget激活对应的context category
 *
 * 此方法从onFocusedDockWidgetChanged提取，供焦点变化和程序化raise两条路径复用
 * @param widget dock内部维护的widget指针
 */
void DAAppController::activateContextCategoryForWidget(QWidget* widget)
{
    if (nullptr == widget) {
        mRibbon->hideContextCategory(DAAppRibbonArea::AllContextCategory);
        return;
    }
    // 数据操作窗口激活时，检查是否需要显示m_contextDataFrame
    if (widget == getDataOperateWidget()) {
        // 数据窗口激活
        mLastFocusedOpertateWidget = LastFocusedOnDataOpt;
        mRibbon->showContextCategory(DAAppRibbonArea::ContextCategoryData);
    } else if (widget == getWorkFlowOperateWidget()) {
        // 工作流窗口激活
        mLastFocusedOpertateWidget = LastFocusedOnWorkflowOpt;
        // 此函数会激活当前窗口的stack
        getWorkFlowOperateWidget()->setUndoStackActive();
        mRibbon->showContextCategory(DAAppRibbonArea::ContextCategoryWorkflow);
        getSettingContainerWidget()->showWorkFlowNodeItemSettingWidget();
    } else if (widget == getChartOperateWidget()) {
        // 绘图窗口激活
        mLastFocusedOpertateWidget = LastFocusedOnChartOpt;
        mRibbon->showContextCategory(DAAppRibbonArea::ContextCategoryChart);
        getSettingContainerWidget()->showChartSettingWidget();
    } else if (widget == getDataManageWidget()) {
        if (mCommand) {
            QUndoStack* stack = mCommand->getDataManagerStack();
            if (stack && !(stack->isActive())) {  // Data 相关的窗口 undostack激活
                stack->setActive();
            }
        }
    }
}

/**
 * @brief DockWidget的焦点变化
 * @param old
 * @param now
 */
void DAAppController::onFocusedDockWidgetChanged(ads::CDockWidget* old, ads::CDockWidget* now)
{
    Q_UNUSED(old);
    activateContextCategoryForWidget(now ? now->widget() : nullptr);
}

/**
 * @brief 程序化raise dock窗口时激活context category
 * @param w dock内部维护的widget
 */
void DAAppController::onDockWidgetRaised(QWidget* w)
{
    activateContextCategoryForWidget(w);
}

bool DAAppController::openCheck()
{
    DAAppProject* project        = DA_APP_CORE.getAppProject();
    const bool hasProjectContent = !project->isEmpty();
    if (project->isDirty()) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            app(),
            tr("Question"),  // cn:提示
            tr("The current project has unsaved changes. Do you want to save before opening another project?"),  // cn:当前工程有未保存的更改，是否在打开其他工程之前保存？
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel,
            QMessageBox::StandardButton::Yes);
        switch (resolveAppOpenPreparation(hasProjectContent, true, toAppSavePromptChoice(btn))) {
        case DAAppOpenPreparation::SaveThenOpen:
            return save();
        case DAAppOpenPreparation::OpenDirectly:
            return true;
        case DAAppOpenPreparation::CancelOpening:
        default:
            return false;
        }
    }

    if (resolveAppOpenPreparation(hasProjectContent, false, DAAppSavePromptChoice::Discard)
        == DAAppOpenPreparation::ConfirmReplaceThenOpen) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            app(),
            tr("Question"),                                                   // cn:提示
            tr("Another project already exists. Do you want to replace it?")  // cn:已存在其他工程，是否要替换？
        );
        return btn == QMessageBox::Yes;
    }

    return true;
}

/**
 * @brief 打开文件
 */
void DAAppController::open()
{
    if (!openCheck()) {
        return;
    }

    QFileDialog dialog(app());
    dialog.setFileMode(QFileDialog::ExistingFile);
    QStringList filters;
    filters << tr("Project File") + QString(" (*.%1)").arg(DAAppProject::getProjectFileSuffix());  // cn:工程文件
    dialog.setNameFilters(filters);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    DA_WAIT_CURSOR_SCOPED();
    openProjectFile(fileNames.first());
}

/**
 * @brief 打开 Markdown 文件，在中央区 dock 中显示
 *
 * 弹出文件对话框选择 .md/.markdown 文件，委托 @ref DAAppDockingArea::showMarkdownFile
 * 在中央区按需创建/复用 dock 标签页并渲染内容。
 */
void DAAppController::onActionOpenMarkdownTriggered()
{
    QFileDialog dialog(app());
    dialog.setFileMode(QFileDialog::ExistingFile);
    QStringList filters;
    filters << tr("Markdown files") + QStringLiteral(" (*.md *.markdown)")   // cn:Markdown 文件
            << tr("Any files") + QStringLiteral(" (*)");                      // cn:所有文件
    dialog.setNameFilters(filters);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    const QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    mDock->showMarkdownFile(fileNames.first());
}

/**
 * @brief 打开工程文件
 *
 * @param $PARAMS
 * @return 成功返回true
 */
bool DAAppController::openProjectFile(const QString& projectFilePath)
{
    DAAppProject* project = DA_APP_CORE.getAppProject();
    if (!project->load(projectFilePath)) {
        daCritical << tr("Failed to load project file: %1").arg(projectFilePath);  // cn:加载工程文件失败:%1
        return false;
    }
    // 加入最近打开的文件中
    mActions->recentFilesManager->addFile(projectFilePath);
    return true;
}

/**
 * @brief 工程的胀状态改变槽
 * @param isdirty
 */
void DAAppController::onProjectDirtyStateChanged(bool isdirty)
{
    app()->setWindowModified(isdirty);
}

/**
 * @brief 追加工作流
 * TODO: 此处应该调整到DAAPPProjectInterface
 */
void DAAppController::onActionAppendProjectTriggered()
{
    QFileDialog dialog(app());
    dialog.setFileMode(QFileDialog::ExistingFile);
    QStringList filters;
    filters << tr("Project File") + QString(" (*.%1)").arg(DAAppProject::getProjectFileSuffix());  // cn:工程文件
    dialog.setNameFilters(filters);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    DAAppProject* project = DA_APP_CORE.getAppProject();
    DA_WAIT_CURSOR_SCOPED();

    QFile file(fileNames.first());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    if (!project->appendWorkflowInProject(file.readAll(), true)) {
        daCritical << tr("Failed to load project file: %1").arg(fileNames.first());  // cn:加载工程文件失败:%1
        return;
    }
    updateWindowTitle();
}

/**
 * @brief 工程成功保存触发的信号
 * @param path
 */
void DAAppController::onProjectSaved(const QString& path)
{
    DAAppProject* project = DA_APP_CORE.getAppProject();
    updateWindowTitle();
    if (mDock) {
        DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
        if (wf) {
            wf->setCurrentWorkflowName(project->getProjectBaseName());
        }
    }
    // saveAs/save 路径更新（plan-05 MAJOR-7 + 契约5）：
    // projectSaved 信号在 save()/saveAs() 统一触发（onSaveFinish emit），
    // 此处同步当前工程路径 + 更新当前会话索引 projectPath 字段（plan-03 契约5），
    // 使下拉过滤与 restoreLastActiveSession 均可命中新路径。
    // 不在 saveAs 末尾调（避免漏 requestSave()→save() 首次保存从无路径到有路径的场景）。
    if (auto* agentMod = qobject_cast< DAAgentModule* >(mCore->getAgentInterface())) {
        agentMod->setCurrentProjectPath(path);             // 同步 m_currentProjectPath（MAJOR-7）
        agentMod->setSessionProjectPathForCurrent(path);   // 契约5：转发 store.setSessionProjectPath
                                                            // 其实现体已内含 setLastActive，无需另调
    }
    daInfo << tr("Project saved successfully, path: %1").arg(path);  // cn:工程保存成功，路径为:%1
}

/**
 * @brief 工程成功加载触发的信号
 * @param path
 */
void DAAppController::onProjectLoaded(const QString& path)
{
    DAAppProject* project = DA_APP_CORE.getAppProject();
    updateWindowTitle();
    if (mDock) {
        DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
        if (wf) {
            wf->setCurrentWorkflowName(project->getProjectBaseName());
        }
    }
    // 工程加载后初始化会话 UI（plan-05 步骤4）：
    // loadSessionsFromProject 已在 executeLoad 回调中调用（步骤2，先于本槽，时序见 MAJOR-5），
    // 此处接线当前工程路径，填充会话下拉但不自动恢复上次会话——始终以全新对话开始。
    // 统一恢复点（MAJOR-3）：restoreLastActiveSession 只在此调一次。
    if (auto* agentMod = qobject_cast< DAAgentModule* >(mCore->getAgentInterface())) {
        agentMod->setCurrentProjectPath(path);     // 先设置当前工程路径（步骤5 接线）
        agentMod->restoreLastActiveSession();       // 填充下拉 + 全新对话
    }
    daInfo << tr("Project loaded successfully, path: %1").arg(path);  // cn:工程加载成功，路径为:%1
}

/**
 * @brief 数据操作窗口添加，需要绑定相关信号槽到ribbon的页面
 * @param page
 */
void DAAppController::onDataOperatePageCreated(DADataOperatePageWidget* page)
{
    if (mCommand) {
        mCommand->addStack(page->getUndoStack());
    }
    switch (page->getDataOperatePageType()) {
    case DADataOperatePageWidget::DataOperateOfDataFrame: {
        DADataOperateOfDataFrameWidget* w = static_cast< DADataOperateOfDataFrameWidget* >(page);
        connect(w,
                &DADataOperateOfDataFrameWidget::selectTypeChanged,
                this,
                &DAAppController::onDataOperateDataFrameWidgetSelectTypeChanged);
        // 选中变化时反向同步 ribbon 样式控件
        connect(w, &DADataOperateOfDataFrameWidget::currentStyleChanged, this, &DAAppController::onTableStyleCurrentChanged);
        // 选中列变化时反向同步 ribbon 显示格式下拉框
        connect(w, &DADataOperateOfDataFrameWidget::currentDisplayFormatChanged, this, &DAAppController::onTableDisplayFormatCurrentChanged);
        // 表头右键菜单注入
        setupDataFrameHeaderContextMenu(w);
    } break;
    default:
        break;
    }
}

/**
 * @brief 脚本定义的内容初始化
 */
void DAAppController::initScripts()
{
    if (!DAPyScripts::isInitScripts()) {
        return;
    }

    mFileReadFilters = QStringList(DAPyScripts::getIO().getFileReadFilters());

    qDebug() << mFileReadFilters;
}

/**
 * @brief 初始化Python工作流信号槽连接
 *
 * 此函数在initialize()中调用，用于设置Python工作流相关的
 * 信号槽连接。当前为占位实现，待DAPyWorkFlowScene完整集成后添加具体连接逻辑。
 */
void DAAppController::initPyWorkflowConnections()
{
    // TODO: 在DAPyWorkFlowScene编辑器集成后完善信号槽连接
    // 当前Action的信号槽已在initConnection中通过DAAPPCONTROLLER_ACTION_BIND宏绑定
    qDebug() << "Python workflow connections initialized (placeholder)";
}
/**
 * @brief 选择的样式改变信号
 * @param column
 * @param dt
 */
void DAAppController::onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DAPyDType dt)
{
    Q_UNUSED(column);
    mRibbon->setDataframeOperateCurrentDType(dt);
    // 按 dtype 重建显示格式下拉框的可用类别
    if (mRibbon && mRibbon->mComboxDisplayFormat) {
        mRibbon->mComboxDisplayFormat->updateDType(dt);
    }
}

/**
 * @brief dataframe的列数据类型改变
 * @param index
 */
void DAAppController::onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt)
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->changeSelectColumnType(dt);
    }
}
/**
 * @brief 添加背景图
 */
void DAAppController::onActionAddBackgroundPixmapTriggered()
{
    QStringList filters;
    filters << tr("Image files") + QString(" (*.png *.jpg)")  // cn:图片文件
            << tr("Any files") + QString(" (*)")              // cn:任意文件
        ;

    QFileDialog dialog(app());
    dialog.setNameFilters(filters);

    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList f = dialog.selectedFiles();
    if (!f.isEmpty()) {
        DAPyWorkFlowOperateWidget* ow = mDock->getWorkFlowOperateWidget();
        ow->addBackgroundPixmap(f.first());
        mDock->raiseDockingArea(DAAppDockingArea::DockingAreaWorkFlowOperate);
    }
}

void DAAppController::onActionLockBackgroundPixmapTriggered(bool on)
{
    mDock->getWorkFlowOperateWidget()->setBackgroundPixmapLock(on);
}

void DAAppController::onActionEnableItemMoveWithBackgroundTriggered(bool on)
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->enableItemMoveWithBackground(on);
    }
}

/**
   @brief 允许移动图元时，其它和此图元链接起来的图元跟随移动
   @param a
 */
void DAAppController::onActionWorkflowEnableItemLinkageMoveTriggered(bool on)
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->setEnableItemLinkageMove(on);
    }
}

/**
   @brief 分组
 */
void DAAppController::onActionItemGroupingTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->groupingSelectItems_();
    }
}

/**
   @brief 取消分组
 */
void DAAppController::onActionItemUngroupTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->removeSelectItemGroup_();
    }
}

/**
 * @brief 导出png
 */
void DAAppController::onActionExportWorkflowScenePNGTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        DAExportToPngSettingDialog dlg(mMainWindow);
        if (QDialog::Accepted != dlg.exec()) {
            return;
        }
        auto image = s->toImage(dlg.getDPI());
        QString p  = dlg.getSelectSaveFilePath();
        if (image.save(p)) {
            daInfo << tr("Image saved successfully to %1").arg(p);  // cn:图片保存成功：%1
        } else {
            daCritical << tr("Failed to save image to %1").arg(p);  // cn:图片保存失败：%1
        }
    }
}

/**
 * @brief 工作流视图锁定
 * @param on
 */
void DAAppController::onActionWorkflowViewReadOnlyTriggered(bool on)
{
    if (DAPyWorkFlowOperateWidget* s = mDock->getWorkFlowOperateWidget()) {
        s->setCurrentWorkflowReadOnly(on);
    }
}

/**
   @brief 主题切换
   @param a
 */
void DAAppController::onActionGroupRibbonThemeTriggered(QAction* a)
{
    if (mActions->actionRibbonThemeOffice2013 == a) {
        mMainWindow->setRibbonTheme(SARibbonTheme::RibbonThemeOffice2013);
    } else if (mActions->actionRibbonThemeOffice2016Blue == a) {
        mMainWindow->setRibbonTheme(SARibbonTheme::RibbonThemeOffice2016Blue);
    } else if (mActions->actionRibbonThemeOffice2021Blue == a) {
        mMainWindow->setRibbonTheme(SARibbonTheme::RibbonThemeOffice2021Blue);
    } else if (mActions->actionRibbonThemeDark == a) {
        mMainWindow->setRibbonTheme(SARibbonTheme::RibbonThemeDark);
    }
}

void DAAppController::onActionRunCurrentWorkflowTriggered()
{
    qDebug() << "onActionRunCurrentWorkflowTriggered";
    // 先检查是否有工程
    DAAppProject* p = DA_APP_CORE.getAppProject();
    if (nullptr == p) {
        daCritical << tr("Received null project interface");  // cn:获取到空工程接口
        return;
    }
    QString bn = p->getProjectBaseName();
    if (bn.isEmpty()) {
        QMessageBox::warning(app(),
                             tr("Warning"),                                                   // cn:警告
                             tr("Before running the workflow, you need to save the project")  // cn:在运行工作流之前，需要先保存工程
        );
        return;
    }
    mDock->getWorkFlowOperateWidget()->runCurrentWorkFlow();
}

/**
 * @brief 终止当前的工作流
 */
void DAAppController::onActionTerminateCurrentWorkflowTriggered()
{
    qDebug() << "onActionTerminateCurrentWorkflowTriggered";
    mDock->getWorkFlowOperateWidget()->terminateCurrentWorkFlow();
}

void DAAppController::onEditFontChanged(const QFont& f)
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        onCurrentWorkflowFontChanged(f);
    } else if (isLastFocusedOnChartOptWidget()) {
    }
}

void DAAppController::onEditFontColorChanged(const QColor& c)
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        onCurrentWorkflowFontColorChanged(c);
    } else if (isLastFocusedOnChartOptWidget()) {
    }
}

void DAAppController::onEditBrushChanged(const QBrush& b)
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        onCurrentWorkflowShapeBackgroundBrushChanged(b);
    } else if (isLastFocusedOnChartOptWidget()) {
    }
}

void DAAppController::onEditPenChanged(const QPen& p)
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        onCurrentWorkflowShapeBorderPenChanged(p);
    } else if (isLastFocusedOnChartOptWidget()) {
    }
}

void DAAppController::onCurrentWorkflowFontChanged(const QFont& f)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setDefaultTextFont(f);
    wf->setSelectTextFont(f);
    // 同步
    mRibbon->setEditFont(f);
}

void DAAppController::onCurrentWorkflowFontColorChanged(const QColor& c)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setDefaultTextColor(c);
    wf->setSelectTextColor(c);
    // 同步
    mRibbon->setEditFontColor(c);
    setDirty();
}

void DAAppController::onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setSelectShapeBackgroundBrush(b);
    // 同步
    mRibbon->setEditBrush(b);
    setDirty();
}

void DAAppController::onCurrentWorkflowShapeBorderPenChanged(const QPen& p)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setSelectShapeBorderPen(p);
    // 同步
    mRibbon->setEditPen(p);
    setDirty();
}

void DAAppController::onWorkflowSceneSelectionItemChanged(QGraphicsItem* lastSelectItem)
{
    if (lastSelectItem == nullptr) {
        return;
    }
    if (DAGraphicsItem* daitem = dynamic_cast< DAGraphicsItem* >(lastSelectItem)) {
        // 属于DAGraphicsItem系列
        mRibbon->setWorkFlowEditBrush(daitem->getBackgroundBrush());
        mRibbon->setWorkFlowEditPen(daitem->getBorderPen());
        // 通用编辑同步
        mRibbon->setEditBrush(daitem->getBackgroundBrush());
        mRibbon->setEditPen(daitem->getBorderPen());
    } else if (DAGraphicsStandardTextItem* titem = dynamic_cast< DAGraphicsStandardTextItem* >(lastSelectItem)) {

        mRibbon->setWorkFlowEditFont(titem->font());
        mRibbon->setWorkFlowEditFontColor(titem->defaultTextColor());
        // 通用编辑同步
        mRibbon->setEditFont(titem->font());
        mRibbon->setEditFontColor(titem->defaultTextColor());
    }
}

/**
 * @brief 工作流开始执行的关联槽
 * @param wfw
 */
void DAAppController::onWorkflowStartExecute(DAPyWorkFlowEditWidget* wfw)
{
    Q_UNUSED(wfw);
    mActions->actionWorkflowRun->setEnabled(false);
    mActions->actionWorkflowTerminate->setEnabled(true);
}

/**
 * @brief 工作流执行结束的关联槽
 * @param wfw
 * @param success
 */
void DAAppController::onWorkflowFinished(DAPyWorkFlowEditWidget* wfw, bool success)
{
    mActions->actionWorkflowRun->setEnabled(true);
    mActions->actionWorkflowTerminate->setEnabled(false);
}

/**
 * @brief 场景有item添加
 * @param sc
 * @param its
 */
void DAAppController::onWorkflowSceneitemsAdded(DAGraphicsScene* sc, const QList< QGraphicsItem* >& its)
{
    setDirty(true);
}

/**
 * @brief 场景有item删除
 * @param sc
 * @param its
 */
void DAAppController::onWorkflowSceneitemsRemoved(DAGraphicsScene* sc, const QList< QGraphicsItem* >& its)
{
    setDirty(true);
}

/**
 * @brief 当前的wf切换
 * @param wfw
 */
void DAAppController::onCurrentWorkflowWidgetChanged(DAPyWorkFlowEditWidget* wfw)
{
    DAPyWorkFlowOperateWidget* workflowOpt = mDock->getWorkFlowOperateWidget();
    mRibbon->updateWorkflowAboutRibbon(workflowOpt);
}

/**
 * @brief 新fig创建
 * @param f
 */
void DAAppController::onFigureCreated(DAFigureWidget* f)
{
    if (nullptr == f) {
        return;
    }
    if (mCommand) {
        QUndoStack* stack = f->getUndoStack();
        mCommand->addStack(stack);
        stack->setActive();
    }
    // updateFigureAboutRibbon(f);//在onActionAddFigureTriggered中调用了
    setDirty();
    connect(f, &DAFigureWidget::currentChartChanged, this, &DAAppController::onCurrentChartChanged);
    connect(f, &DAFigureWidget::chartEditorStatusChanged, this, &DAAppController::onChartEditorStatusChanged);
}

/**
 * @brief 当前的fig变化
 * @param f
 * @param index
 */
void DAAppController::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
    Q_UNUSED(index);
    if (nullptr == f) {
        return;
    }
    qDebug() << "DAAppController::onCurrentFigureChanged";
    mRibbon->updateFigureAboutRibbon(f);
    QUndoStack* stack = f->getUndoStack();
    if (stack) {
        stack->setActive();
    }
}

/**
 * @brief 当前的chart改变
 * @param c
 */
void DAAppController::onCurrentChartChanged(DAChartWidget* c)
{
    if (nullptr == c) {
        return;
    }
    mRibbon->updateChartAboutRibbon(c);
}

/**
 * @brief 图表编辑器状态改变
 * @param status
 */
void DAAppController::onChartEditorStatusChanged(DAFigureWidget::ChartEditorStatus status)
{
    if (DAFigureWidget::ChartEditorStatus::EndEdit == status) {
        // 结束编辑
        // 触发结束编辑有可能是按键，或一些异常状态，为了避免和按钮的状态不同步，这里结束编辑后要把actiongroup管理的action都设置为unchecked
        const QList< QAction* > actions = mActions->actionGroupChartEditor->actions();
        for (QAction* a : actions) {
            if (a->isChecked()) {
                a->setChecked(false);
            }
        }
    }
}
/**
 * @brief 绘图项创建完成（DADialogChartGuide 确认后）
 *
 * 此时用户已在绘图引导对话框中确认添加绘图，提升绘图 dock 让用户看到新创建的绘图
 * @param fig 绘图窗口
 * @param plot 图表
 * @param item 创建的绘图项
 */
void DAAppController::onPlotItemCreated(DAFigureWidget* f, DAChartWidget* plot, QwtPlotItem* item)
{
    Q_UNUSED(f);
    Q_UNUSED(plot);
    Q_UNUSED(item);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 3D绘图项创建完成，提升绘图 dock 显示新3D绘图
 */
void DAAppController::onPlot3DItemCreated(DA::DAFigureWidget* f, DA::DAChart3DWidget* plot, Qwt3DPlotItem* item)
{
    Q_UNUSED(f);
    Q_UNUSED(plot);
    Q_UNUSED(item);
    // 提升绘图操作区域到前台
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 添加数据
 */
void DAAppController::onActionAddDataTriggered()
{
    QFileDialog dialog(app());
    dialog.setNameFilters(mFileReadFilters);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    const QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    QString fileName = fileNames.back();
    // 对txt要弹出对话框进行指引
    QVariantMap args;
    QString err;
    QFileInfo fi(fileName);
    if (fi.suffix().toLower() == "txt") {
        DATxtFileImportDialog dlg(mMainWindow);
        dlg.setTextFilePath(fileName);
        if (QDialog::Accepted != dlg.exec()) {
            return;
        }
        // 获取导入txt的配置
        args = dlg.getSetting();
        qDebug() << "da_read:args->" << args;
    } else {
    }
    DA_WAIT_CURSOR_SCOPED();
    importData(fileName, args, &err);
}

/**
 * @brief 移除数据
 */
void DAAppController::onActionRemoveDataTriggered()
{
    DADataManageWidget* dmw = mDock->getDataManageWidget();
    dmw->removeSelectData();
    setDirty();
}

/**
 * @brief 添加一个figure
 */
void DAAppController::onActionAddFigureTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    // 添加绘图
    DAFigureWidget* fig = chartopt->createFigure();
    // 这里不需要回退
    DAChartWidget* chart = fig->createChart();
    chart->setAxisLabel(QwtAxis::XBottom, "x");
    chart->setAxisLabel(QwtAxis::YLeft, "y");
    // 把fig的undostack添加
    mCommand->addStack(fig->getUndoStack());
    mRibbon->updateFigureAboutRibbon(fig);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    chartopt->setCurrentFigure(fig);
    setDirty();
}

/**
 * @brief Agent 绘图引用超链接点击处理
 *
 * 解析 da-figure: 协议超链接，定位目标 figure 并 raise 绘图区域。
 * 支持两种格式：
 *   - da-figure:&lt;figure_name&gt;  按 tab 文本定位（agent 默认，简单）
 *   - da-figure:id=&lt;uuid&gt;       按 figure_id 精确定位（抗重名/改名）
 * @param href 超链接 href
 */
void DAAppController::onFigureLinkRequested(const QString& href)
{
    static const QString kPrefix = QStringLiteral("da-figure:");
    if (!href.startsWith(kPrefix, Qt::CaseInsensitive)) {
        qWarning() << "[FigureLink] invalid href:" << href;
        return;
    }
    QString payload = href.mid(kPrefix.length());
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    if (!chartopt) {
        qWarning() << "[FigureLink] chart operate widget is null";
        return;
    }
    DAFigureWidget* fig = nullptr;
    if (payload.startsWith(QStringLiteral("id="), Qt::CaseInsensitive)) {
        // 精确格式：da-figure:id=<uuid>
        fig = chartopt->findFigure(payload.mid(3));
    } else if (!payload.isEmpty()) {
        // 简单格式：da-figure:<figure_name>，按 tab 文本遍历匹配
        const QList< DAFigureWidget* > figs = chartopt->getFigureList();
        for (DAFigureWidget* f : figs) {
            if (chartopt->getFigureName(f) == payload) {
                fig = f;
                break;
            }
        }
    }
    if (!fig) {
        daWarning << tr("Figure '%1' not found, it may have been closed or renamed").arg(payload);  // cn:未找到绘图"%1"，可能已关闭或被重命名
        return;
    }
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    chartopt->setCurrentFigure(fig);
}

/**
 * @brief 新建一个坐标系
 */
void DAAppController::onActionFigureNewXYAxisTriggered()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        daWarning << tr("Before creating a new coordinate, you need to create a figure");  // cn:在创建一个坐标系之前，需要先创建一个绘图窗口
        return;
    }
    DAChartWidget* w = fig->createChart_(QRectF(0.1, 0.1, 0.4, 0.4));
    w->enableGrid();
    w->enablePan();
    //    w->addCurve({ 1, 2, 3, 4, 5 }, { 3, 5, 8, 0, -3 })->setTitle("curve1");
    //    w->addCurve({ 1, 2, 3, 4, 5 }, { 5, 7, 0, -1, 1 })->setTitle("curve2");
    mRibbon->updateChartAboutRibbon(w);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    setDirty();
}

/**
 * @brief 创建一个曲线
 */
void DAAppController::onActionChartAddCurveTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Curve);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加散点图
 */
void DAAppController::onActionChartAddScatterTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Scatter);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加柱状图
 */
void DAAppController::onActionChartAddBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Bar);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加误差棒图
 */
void DAAppController::onActionactionChartAddErrorBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::ErrorBar);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加误差棒图
 */
void DAAppController::onActionChartAddBoxPlotTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Box);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加谱图
 */
void DAAppController::onActionChartAddCloudMapTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Spectrogram);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加多重柱状图
 */
void DAAppController::onActionChartAddMultiBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::MultiBar);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加直方图
 */
void DAAppController::onActionChartAddHistogramTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Histogram);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加等高线图
 */
void DAAppController::onActionChartAddContourMapTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Contour);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加向量场图
 */
void DAAppController::onActionChartAddVectorfieldTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::VectorField);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D曲面图
 */
void DAAppController::onActionChartAdd3DSurfaceTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Surface3D);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D柱状图
 */
void DAAppController::onActionChartAdd3DBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Bar3D);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D线图
 */
void DAAppController::onActionChartAdd3DLineTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Line3D);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 统计直方图
 */
void DAAppController::onActionStatsHistplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsHistplot);
}

/**
 * @brief 一维核密度图
 */
void DAAppController::onActionStatsKdeplot1dTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsKdeplot1d);
}

/**
 * @brief 二维核密度图
 */
void DAAppController::onActionStatsKdeplot2dTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsKdeplot2d);
}

/**
 * @brief 统计箱线图
 */
void DAAppController::onActionStatsBoxplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsBoxplot);
}

/**
 * @brief 热力图
 */
void DAAppController::onActionStatsHeatmapTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsHeatmap);
}

/**
 * @brief 统计散点图
 */
void DAAppController::onActionStatsScatterplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsScatterplot);
}

/**
 * @brief 统计柱状图
 */
void DAAppController::onActionStatsBarplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsBarplot);
}

/**
 * @brief 回归图
 */
void DAAppController::onActionStatsRegplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsRegplot);
}

/**
 * @brief 经验累积分布图
 */
void DAAppController::onActionStatsECDFplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsECDFplot);
}

/**
 * @brief 确保当前 Figure 和 Chart 存在（不存在则创建）
 * @param fig 输出 Figure 指针
 * @param chart 输出 Chart 指针
 * @return 如果 fig 和 chart 都有效返回 true
 */
bool DAAppController::ensureFigureChart(DAFigureWidget*& fig, DAChartWidget*& chart)
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    fig                               = chartopt->getCurrentFigure();
    if (!fig) {
        fig = chartopt->createFigure();
    }
    if (!fig) {
        return false;
    }
    chart = fig->getCurrentChart();
    if (!chart) {
        chart = fig->gca();
    }
    if (!chart) {
        chart = fig->createChart();
    }
    if (!chart) {
        return false;
    }
    return true;
}

/**
 * @brief 显示统计绘图引导对话框并预选指定类型
 * @param type 统计绘图类型
 */
void DAAppController::showStatsChartGuide(DA::DAChartTypes type)
{
    DAFigureWidget* fig  = nullptr;
    DAChartWidget* chart = nullptr;
    if (!ensureFigureChart(fig, chart)) {
        return;
    }
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);

    if (!mStatsChartGuideDlg) {
        mStatsChartGuideDlg = new DADialogStatsChartGuide(app());
        mStatsChartGuideDlg->setDataManager(mDatas->dataManager());
        connect(mStatsChartGuideDlg, &DADialogStatsChartGuide::plotRequested, this, &DAAppController::onStatsGuideAccepted);
    }
    mStatsChartGuideDlg->setFigureWidget(fig);
    mStatsChartGuideDlg->setChartWidget(chart);
    mStatsChartGuideDlg->setCurrentChartType(type);
    mStatsChartGuideDlg->show();
    mStatsChartGuideDlg->raise();
    mStatsChartGuideDlg->activateWindow();
}

/**
 * @brief 允许网格
 * @param on
 */
void DAAppController::onActionChartEnableGridTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGrid(on);
        w->replot();
        return true;
    });
    if (res) {
        // grid生效与否决定了X，Y以及Xmin,Ymin能否显示
        mRibbon->updateChartGridAboutRibbon(getCurrentChart());
        setDirty();
    }
}

/**
 * @brief 横向网格
 * @param on
 */
void DAAppController::onActionChartEnableGridXTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridX(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}
/**
 * @brief 纵向网格
 * @param on
 */
void DAAppController::onActionChartEnableGridYTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridY(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}
/**
 * @brief 横向密集网格
 * @param on
 */
void DAAppController::onActionChartEnableGridXMinEnableTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridXMin(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}
/**
 * @brief 纵向密集网格
 * @param on
 */
void DAAppController::onActionChartEnableGridYMinTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridYMin(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 当前图表允许缩放
 * @param on
 */
void DAAppController::onActionChartEnableZoomTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableZoom(on);
        return true;
    });
    if (res) {
        mRibbon->updateChartZoomPanAboutRibbon(getCurrentChart());
    }
}

/**
 * @brief 当前图表放大
 */
void DAAppController::onActionChartZoomInTriggered()
{
    applyToCharts([](DAChartWidget* w) -> bool {
        w->zoomIn();
        return true;
    });
}

/**
 * @brief 当前图表缩小
 */
void DAAppController::onActionChartZoomOutTriggered()
{
    applyToCharts([](DAChartWidget* w) -> bool {
        w->zoomOut();
        return true;
    });
}

/**
 * @brief 当前图表全部显示
 */
void DAAppController::onActionChartZoomAllTriggered()
{
    bool res = applyToCharts([](DAChartWidget* w) -> bool {
        w->rescaleAxes();
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 允许绘图拖动
 * @param on
 */
void DAAppController::onActionChartEnablePanTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enablePan(on);
        return true;
    });
    if (res) {
        mRibbon->updateChartZoomPanAboutRibbon(getCurrentChart());
    }
}

/**
 * @brief 允许绘图拾取
 * @param on
 */
void DAAppController::onActionChartEnablePickerCrossTriggered(bool on)
{
    applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableCrosshair(on);
        return true;
    });
}

/**
 * @brief 允许绘图拾取Y
 * @param on
 */
void DAAppController::onActionChartEnablePickerYTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableYValuePicking(on);
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 设置pick的区域
 * @param act
 */
void DAAppController::onActionGroupChartPickerTextRegionTriggered(QAction* act)
{
    applyToCharts([ act ](DAChartWidget* w) -> bool {
        QwtPlotSeriesDataPicker* picker = w->getDataPicker();
        QwtPlotSeriesDataPicker::TextPlacement tp =
            static_cast< QwtPlotSeriesDataPicker::TextPlacement >(act->data().toInt());
        if (picker) {
            picker->setTextArea(tp);
        }
        return true;
    });
}

/**
 * @brief 允许绘图拾取XY
 * @param on
 */
void DAAppController::onActionChartEnablePickerXYTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableXYValuePicking(on);
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 连接所有picker
 * @param on
 */
void DAAppController::onActionChartLinkAllPickerEnabledTriggered(bool on)
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        fig->setDataPickerGroupEnabled(on);
    }
}

/**
 * @brief
 * @param on
 */
void DAAppController::onActionChartYPickerShowXValueEnabledTriggered(bool on)
{
    applyToCharts([ on ](DAChartWidget* w) -> bool {
        QwtPlotSeriesDataPicker* picker = w->getDataPicker();
        if (picker) {
            picker->setEnableShowXValue(on);
        }
        return true;
    });
}

/**
 * @brief 允许图例
 * @param on
 */
void DAAppController::onActionChartEnableLegendTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableLegend(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 绘图样式选择
 * @param act
 */
void DAAppController::onActionGroupFigureThemeTriggered(QAction* act)
{
    DAColorTheme::ColorThemeStyle style = static_cast< DAColorTheme::ColorThemeStyle >(act->data().toInt());
    DAColorTheme theme(style);
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        return;
    }
    fig->setColorTheme(theme);
}

void DAAppController::onActionCopyFigureToClipboardTriggered()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        return;
    }
    fig->copyToClipboard();
}

void DAAppController::onActionChartDataPickerSettingTriggered()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        return;
    }
    DAChartWidget* chart = fig->getCurrentChart();
    if (!chart) {
        chart = fig->gca();
    }
    if (!chart) {
        return;
    }
    DASettingContainerWidget* setting = getSettingContainerWidget();
    if (!setting) {
        return;
    }
    DAChartSettingWidget* chartSetting = setting->getChartSettingWidget();
    if (!chartSetting) {
        return;
    }
    chartSetting->setPlot(chart);
    chartSetting->showDataPickerSetting();
    setting->showChartSettingWidget();
    mDock->raiseDockByWidget((QWidget*)(mDock->getSettingContainerWidget()));
}

void DAAppController::onActionGroupChartEditorTriggered(QAction* a)
{
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig || !a) {
        return;
    }
    if (!a->isChecked()) {
        // 都不选中
        fig->endChartEditor();
        return;
    }
    bool isok = false;
    int type  = a->data().toInt(&isok);
    if (!isok) {
        return;
    }
    fig->beginChartEditor(static_cast< DAFigureWidget::ChartEditorType >(type));
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief dataframe删除行
 */
void DAAppController::onActionRemoveRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectRow();
        setDirty();
    }
}

/**
 * @brief dataframe删除列
 */
void DAAppController::onActionRemoveColumnTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectColumn();
        setDirty();
    }
}

/**
 * @brief 移除单元格
 */
void DAAppController::onActionRemoveCellTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectCell();
        setDirty();
    }
}

/**
 * @brief 插入行
 */
void DAAppController::onActionInsertRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowBelowBySelect();
        setDirty();
    }
}

/**
 * @brief 在选中位置上面插入一行
 */
void DAAppController::onActionInsertRowAboveTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowAboveBySelect();
        setDirty();
    }
}
/**
 * @brief 在选中位置右边插入一列
 */
void DAAppController::onActionInsertColumnRightTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnRightBySelect();
        setDirty();
    }
}
/**
 * @brief 在选中位置左边插入一列
 */
void DAAppController::onActionInsertColumnLeftTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnLeftBySelect();
        setDirty();
    }
}

/**
 * @brief dataframe列重命名
 */
void DAAppController::onActionRenameColumnsTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->renameColumns();
        setDirty();
    }
}

/**
 * @brief dataframe单列重命名（表头右键）
 *
 * 弹出 QInputDialog 让用户输入新列名，调用 widget->renameColumn。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppController::onActionRenameColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    DADataTableView* tv = dfopt->getDataTableView();
    if (!tv) {
        return;
    }
    QString oldName = tv->actualColumnName(col);
    bool ok         = false;
    QString newName = QInputDialog::getText(dfopt,
                                            tr("Rename Column"),     // cn:重命名列
                                            tr("New column name:"),  // cn:新列名：
                                            QLineEdit::Normal,
                                            oldName,
                                            &ok);
    if (!ok) {
        return;
    }
    if (dfopt->renameColumn(col, newName.trimmed())) {
        setDirty();
    }
}

/**
 * @brief 复制列名到剪贴板（表头右键）
 *
 * 依赖表头右键菜单已先选中目标列（B-2-a），列名来源走 view 的 actualColumnName。
 */
void DAAppController::onActionCopyColumnNameTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    DADataTableView* tv = dfopt->getDataTableView();
    if (!tv) {
        return;
    }
    QString name = tv->actualColumnName(col);
    if (name.isEmpty()) {
        return;
    }
    QClipboard* cb = QApplication::clipboard();
    if (cb) {
        cb->setText(name);
    }
}

/**
 * @brief 跳转到最大值（表头右键）
 *
 * 取当前选中列的最大值位置索引，调用 selectActualCell 滚动并高亮。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppController::onActionGotoMaxTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    QString colNm = df.columnName(col);
    DAPySeries s  = df[ colNm ];
    long pos      = s.idxmaxPosition();
    if (pos < 0) {
        daWarning << tr("Cannot find the maximum value in this column (empty, all-NaN, or incomparable types)");  // cn:此列无法找到最大值（空列、全为NaN或类型不可比较）
        return;
    }
    // 选中并滚动到目标单元格
    DADataTableView* tv = dfopt->getDataTableView();
    if (tv) {
        tv->selectActualCell(static_cast< int >(pos), col);
    }
    // 打印结果到信息栏
    {
        pybind11::gil_scoped_acquire gil;
        try {
            pybind11::object val = s.iat(pos);
            QString valStr       = pybind11::str(val).cast< QString >();
            daInfo << tr("Column [%1] maximum value: %2, row: %3").arg(colNm, valStr).arg(pos);  // cn:列[%1] 最大值: %2, 第 %3 行
        } catch (const std::exception& e) {
            qCritical() << e.what();
            daInfo << tr("Column [%1] maximum value at row %2").arg(colNm).arg(pos);  // cn:列[%1] 最大值位于第 %2 行
        }
    }
}

/**
 * @brief 跳转到最小值（表头右键）
 *
 * 取当前选中列的最小值位置索引，调用 selectActualCell 滚动并高亮。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppController::onActionGotoMinTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    QString colNm = df.columnName(col);
    DAPySeries s  = df[ colNm ];
    long pos      = s.idxminPosition();
    if (pos < 0) {
        daWarning << tr("Cannot find the minimum value in this column (empty, all-NaN, or incomparable types)");  // cn:此列无法找到最小值（空列、全为NaN或类型不可比较）
        return;
    }
    // 选中并滚动到目标单元格
    DADataTableView* tv = dfopt->getDataTableView();
    if (tv) {
        tv->selectActualCell(static_cast< int >(pos), col);
    }
    // 打印结果到信息栏
    {
        pybind11::gil_scoped_acquire gil;
        try {
            pybind11::object val = s.iat(pos);
            QString valStr       = pybind11::str(val).cast< QString >();
            daInfo << tr("Column [%1] minimum value: %2, row: %3").arg(colNm, valStr).arg(pos);  // cn:列[%1] 最小值: %2, 第 %3 行
        } catch (...) {
            daInfo << tr("Column [%1] minimum value at row %2").arg(colNm).arg(pos);  // cn:列[%1] 最小值位于第 %2 行
        }
    }
}

/**
 * @brief 显示列统计信息（表头右键）
 *
 * 通过 DADataOperateOfDataFrameWidget::showColumnDescribe 调用 pandas describe
 * 获取当前列的统计信息并弹窗展示。
 */
void DAAppController::onActionShowColumnDescribeTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    dfopt->showColumnDescribe(col);
}

/**
 * @brief 为 DataFrame 操作窗口的表头注入右键菜单
 *
 * 方案 B-2：controller 直接操作 widget 的 horizontalHeader。
 * 右键时先选中该列（replace 语义），再弹出菜单。
 * 菜单项 enable 按 Q9-A 统一判定：走到这里 col 已有效，全部 enable。
 * 生命周期依赖 Qt 自动断连（header 是 widget 子对象，widget 销毁即断连）。
 * @param w 目标 DataFrame 操作窗口
 */
void DAAppController::populateColumnContextMenu(QMenu& menu)
{
    menu.addAction(mActions->actionRenameColumn);
    menu.addAction(mActions->actionRemoveColumn);
    menu.addSeparator();
    menu.addAction(mActions->actionCopyColumnName);
    menu.addSeparator();
    menu.addAction(mActions->actionGotoMax);
    menu.addAction(mActions->actionGotoMin);
    menu.addSeparator();
    menu.addAction(mActions->actionShowColumnDescribe);
}

void DAAppController::selectColumnInDataFrameWidget(DADataOperateOfDataFrameWidget* w, int col)
{
    if (!w || col < 0) {
        return;
    }
    DADataTableView* tv = w->getDataTableView();
    if (!tv) {
        return;
    }
    QItemSelectionModel* sel = tv->selectionModel();
    QAbstractItemModel* m    = tv->model();
    if (!sel || !m) {
        return;
    }
    sel->clearSelection();
    QItemSelection selRange(m->index(0, col), m->index(m->rowCount() - 1, col));
    sel->select(selRange, QItemSelectionModel::Select | QItemSelectionModel::Columns);
}

/**
 * @brief 为 DataFrame 操作窗口的表头注入右键菜单
 *
 * 方案 B-2：controller 直接操作 widget 的 horizontalHeader。
 * 右键时先选中该列（replace 语义），再弹出菜单。
 * 菜单项 enable 按 Q9-A 统一判定：走到这里 col 已有效，全部 enable。
 * 生命周期依赖 Qt 自动断连（header 是 widget 子对象，widget 销毁即断连）。
 * @param w 目标 DataFrame 操作窗口
 */
void DAAppController::setupDataFrameHeaderContextMenu(DADataOperateOfDataFrameWidget* w)
{
    if (!w || !w->getDataTableView()) {
        return;
    }
    QHeaderView* hv = w->getDataTableView()->horizontalHeader();
    if (!hv) {
        return;
    }
    hv->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(hv, &QWidget::customContextMenuRequested, this, [ this, w, hv ](const QPoint& pos) {
        int col = hv->logicalIndexAt(pos);
        if (col < 0) {
            return;
        }
        DAPyDataFrame df = w->getDataframe();
        if (df.isNone()) {
            return;
        }
        auto shape = df.shape();
        if (col >= (int)shape.second) {
            return;
        }
        // 先选中该列（replace 语义），槽函数通过 getSelectedOneDataframeColumn 取列
        selectColumnInDataFrameWidget(w, col);
        // 弹出共用菜单
        QMenu menu(w);
        populateColumnContextMenu(menu);
        menu.exec(hv->mapToGlobal(pos));
    });
}

/**
 * @brief 为数据管理树的 series 节点注入右键菜单（与表头右键共用 action）
 *
 * 右键 series 节点时：取 DAData + seriesName → 列名转列索引 →
 * 找到（或打开）对应的 DataFrame 操作窗口 → 选中该列 → 弹出与表头右键相同的菜单。
 * 槽函数无需改动，依赖 getCurrentDataFrameOperateWidget + getSelectedOneDataframeColumn。
 * @param w 数据管理树窗口
 */
void DAAppController::setupDataManagerTreeSeriesContextMenu(DADataManagerTreeWidget* w)
{
    if (!w) {
        return;
    }
    QTreeView* tv = w->getTreeView();
    if (!tv) {
        return;
    }
    tv->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tv, &QWidget::customContextMenuRequested, this, [ this, w, tv ](const QPoint& pos) {
        QModelIndex proxyIndex = tv->indexAt(pos);
        if (!proxyIndex.isValid()) {
            return;
        }
        // 映射到 source model
        DADataManagerTreeFilterProxyModel* proxy = w->getProxyModel();
        QModelIndex srcIndex                     = proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;
        QStandardItem* item                      = w->getModel()->itemFromIndex(srcIndex);
        if (!item || !DADataManagerTreeModel::isDataframeSeriesItem(item)) {
            return;
        }
        DAData data = DADataManagerTreeModel::itemToData(item);
        if (!data.isDataFrame()) {
            return;
        }
        QString seriesName = item->text();
        DAPyDataFrame df   = data.toDataFrame();
        if (df.isNone()) {
            return;
        }
        // 列名 → 列索引
        QList< QString > cols = df.columns();
        int col               = cols.indexOf(seriesName);
        if (col < 0) {
            return;
        }
        // 查找已打开的 DataFrame 窗口，没有则打开
        DADataOperateOfDataFrameWidget* dfopt = getDataOperateWidget()->findDataFrameWidget(data);
        if (!dfopt) {
            getDataOperateWidget()->showData(data);
            dfopt = getDataOperateWidget()->findDataFrameWidget(data);
        }
        if (!dfopt) {
            return;
        }
        // 选中该列，槽函数通过 getSelectedOneDataframeColumn 取列
        selectColumnInDataFrameWidget(dfopt, col);
        // 弹出共用菜单
        QMenu menu(w);
        populateColumnContextMenu(menu);
        menu.exec(tv->viewport()->mapToGlobal(pos));
    });
}

/**
 * @brief 选中列转换为数值
 */
void DAAppController::onActionCastToNumTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->castSelectToNum();
        setDirty();
    }
}

/**
 * @brief 选中列转换为文字
 */
void DAAppController::onActionCastToStringTriggered()
{
    DAAPPCONTROLLER_PASS();
}

/**
 * @brief 选中列转换为日期
 */
void DAAppController::onActionCastToDatetimeTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->castSelectToDatetime();
        setDirty();
    }
}

/**
 * @brief 选中列转换为索引
 */
void DAAppController::onActionChangeToIndexTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->changeSelectColumnToIndex();
        setDirty();
    }
}

/**
 * @brief 显示工作流区域
 */
void DAAppController::onActionShowWorkFlowAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getWorkFlowOperateWidget()));
}

void DAAppController::onActionShowWorkFlowManagerAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getWorkflowNodeListWidget()));
}

/**
 * @brief 显示绘图区域
 */
void DAAppController::onActionShowChartAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getChartOperateWidget()));
}

void DAAppController::onActionShowChartManagerAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getChartManageWidget()));
}

/**
 * @brief 显示数据区域
 */
void DAAppController::onActionShowDataAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getDataOperateWidget()));
}

/**
 * @brief 显示数据管理区域
 */
void DAAppController::onActionShowDataManagerAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getDataManageWidget()));
}

/**
 * @brief 显示信息区域
 */
void DAAppController::onActionShowMessageLogViewTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getMessageLogViewWidget()));
}

/**
 * @brief 显示设置区域
 */
void DAAppController::onActionSettingWidgetTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getSettingContainerWidget()));
}

/**
 * @brief 显示标记线
 *
 * 此action有个menu，menu的action选中会设置当前action的图标，具体实现放在@ref DAAppRibbonArea::buildContextCategoryWorkflowView_ 函数中
 * @param on
 * @sa DAAppRibbonArea::buildContextCategoryWorkflowView_
 */
void DAAppController::onActionWorkflowViewMarkerTriggered(bool on)
{
    auto wo = mDock->getWorkFlowOperateWidget();
    if (!wo) {
        return;
    }
    if (on) {
        // 激活marker
        auto actionCross = wo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCrossLineMarker);
        if (!actionCross) {
            return;
        }
        actionCross->trigger();
    } else {
        // 激活marker
        auto actionNone = wo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker);
        if (!actionNone) {
            return;
        }
        actionNone->trigger();
    }
}

void DAAppController::onActionShowRightSideBarTriggered(bool on)
{
    mDock->toggleRightSidebar(on);
}

void DAAppController::onActionShowLeftSideBarTriggered(bool on)
{
    mDock->toggleLeftSidebar(on);
}

/**
 * @brief 新建工作流
 */
void DAAppController::onActionNewWorkflowTriggered()
{
    bool ok      = false;
    QString text = QInputDialog::getText(app(),
                                         tr("New workflow name"),   // cn:新工作流名称
                                         tr("New workflow name:"),  // cn:新工作流名称
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (!ok || text.isEmpty()) {
        return;
    }
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->appendWorkflow(text);
    setDirty();
}

/**
 * @brief 绘制矩形
 *
 * * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppController::onActionStartDrawRectTriggered(bool on)
{
    if (on) {
        mDock->getWorkFlowOperateWidget()->setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::AddRectItemAction);
    }
}
/**
 * @brief 绘制文本
 *
 * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppController::onActionStartDrawTextTriggered(bool on)
{
    if (on) {
        mDock->getWorkFlowOperateWidget()->setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::AddTextItemAction);
    }
}

/**
 * @brief 允许连线
 * @param on
 */
void DAAppController::onActionWorkflowLinkEnableTriggered(bool on)
{
    if (auto wo = mDock->getWorkFlowOperateWidget()) {
        wo->setEnableWorkflowLink(on);
    }
}

/**
 * @brief 表格底色变更
 * @param c 颜色
 */
void DAAppController::onTableStyleFillColorChanged(const QColor& c)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setBackground(QBrush(c));
    w->mergeStyleToSelection(fragment);
    qDebug() << "DAAppController::onTableStyleFillColorChanged(" << c << ")";
}

/**
 * @brief 表格字体变更
 * @param f 字体
 */
void DAAppController::onTableStyleFontChanged(const QFont& f)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setFont(f);
    w->mergeStyleToSelection(fragment);
}

/**
 * @brief 表格字体颜色变更
 * @param c 颜色
 */
void DAAppController::onTableStyleFontColorChanged(const QColor& c)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setForeground(c);
    w->mergeStyleToSelection(fragment);
}

/**
 * @brief 清除选中区样式
 */
void DAAppController::onActionClearStyleSelectedTriggered()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->clearStyleSelection();
    }
}

/**
 * @brief 清除整表所有样式
 */
void DAAppController::onActionClearStyleAllTriggered()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->clearStyleAll();
    }
}

/**
 * @brief 选中区样式反向同步 ribbon 控件
 *
 * 根据选中区代表样式更新 ribbon 的底色按钮/字体面板。
 * 各属性独立判断：valid 的属性更新对应控件，invalid 的不调整。
 * 用 blockSignals 避免控件信号触发循环。
 * @param style 选中区代表样式
 */
void DAAppController::onTableStyleCurrentChanged(const DA::DATableCellStyle& style)
{
    // 底色按钮
    if (mRibbon && mRibbon->mBtnTableFillColor) {
        QSignalBlocker block(mRibbon->mBtnTableFillColor);
        if (style.backgroundValid()) {
            mRibbon->mBtnTableFillColor->setColor(style.background().color());
        }
    }
    // 字体面板
    if (mRibbon && mRibbon->mWidgetTableFont) {
        QSignalBlocker block(mRibbon->mWidgetTableFont);
        if (style.fontValid()) {
            mRibbon->mWidgetTableFont->setCurrentFont(style.font());
        }
        if (style.foregroundValid()) {
            mRibbon->mWidgetTableFont->setCurrentFontColor(style.foreground());
        }
    }
}

/**
 * @brief 显示格式下拉框类别变化：对选中列应用该类别（默认参数）
 * @param c 选中的格式类别
 */
void DAAppController::onTableDisplayFormatCategoryChanged(DA::DATableDisplayFormat::Category c)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    if (c == DA::DATableDisplayFormat::General) {
        w->clearDisplayFormatFromSelection();
    } else {
        DA::DATableDisplayFormat fmt(c);
        w->setDisplayFormatToSelection(fmt);
    }
    setDirty();
}

/**
 * @brief “设置单元格格式”按钮：打开对话框，OK 后应用结果
 */
void DAAppController::onActionTableFormatCellsTriggered()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    int col = w->getSelectedOneDataframeColumn(false);
    if (col < 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    DAPyDataFrame df = w->getDataframe();
    DA::DAPyDType dt;
    QVariant sample;
    try {
        if (!df.isNone()) {
            dt     = df.dtypeObject(static_cast< std::size_t >(col));
            sample = df.iat(0, static_cast< std::size_t >(col));
        }
    } catch (const std::exception& e) {
        qWarning() << "DADialogTableDisplayFormat prepare:" << e.what();
    }
    DA::DADialogTableDisplayFormat dlg(w->getCurrentColumnDisplayFormat(), dt, sample, app());
    if (dlg.exec() == QDialog::Accepted) {
        DA::DATableDisplayFormat fmt = dlg.getResult();
        if (fmt.isValid()) {
            w->setDisplayFormatToSelection(fmt);
        } else {
            w->clearDisplayFormatFromSelection();
        }
        setDirty();
    }
}

/**
 * @brief 选中列显示格式反向同步 ribbon 格式下拉框
 * @param fmt 当前代表格式（invalid 表示无格式）
 */
void DAAppController::onTableDisplayFormatCurrentChanged(const DA::DATableDisplayFormat& fmt)
{
    if (mRibbon && mRibbon->mComboxDisplayFormat) {
        QSignalBlocker block(mRibbon->mComboxDisplayFormat);
        mRibbon->mComboxDisplayFormat->setCurrentFormat(fmt);
    }
}

/**
 * @brief 统计绘图请求的统一处理槽
 *
 * DAAbstractStatsChartAddWidget::plotRequested 信号触发后，此函数：
 * 1. 从 params 中读取 column / hue 列名
 * 2. 从 widget 的数据选择组件获取 DAData/DAPyDataFrame
 * 3. 调用 executeStatsPlot，由 DAStatsPlotCoordinator 完成绘图
 *
 * @note 绘图类型通过 params["__plot_type__"] 指定，缺省为 "histplot"。
 */
void DAAppController::onStatsPlotRequested(const QJsonObject& params, DA::DAFigureWidget* fig, DA::DAChartWidget* chart)
{
    qDebug() << "[onStatsPlotRequested] called, fig=" << fig << "chart=" << chart;
    if (!fig || !chart) {
        daWarning << tr("No figure/chart available for statistical plot");
        return;
    }

    // Resolve the DAData from the sender widget's dynamic property
    DAData data;
    QObject* senderObj = sender();
    qDebug() << "[onStatsPlotRequested] sender=" << senderObj;
    auto* statsWidget = qobject_cast< DAAbstractStatsChartAddWidget* >(senderObj);
    if (statsWidget) {
        QVariant dataVar = statsWidget->property("__da_data__");
        if (dataVar.isValid() && dataVar.canConvert< DAData >()) {
            data = dataVar.value< DAData >();
        }
        qDebug() << "[onStatsPlotRequested] statsWidget found, data.isDataFrame=" << data.isDataFrame();
    } else {
        qDebug() << "[onStatsPlotRequested] sender is NOT a StatsChartAddWidget (lambda forwarding issue?)";
    }
    if (!data.isDataFrame()) {
        daWarning << tr("Cannot resolve the data source for statistical plot; "
                        "please ensure a dataframe is selected in the settings window");
        return;
    }

    executeStatsPlot(params, fig, chart, data);
}

/**
 * @brief 统计绘图引导对话框确认后的处理槽
 *
 * 直接从对话框获取当前 widget 的 DAData，不依赖 sender()（因为 lambda 转发会破坏 sender 链）。
 */
void DAAppController::onStatsGuideAccepted(const QJsonObject& params, DA::DAFigureWidget* fig, DA::DAChartWidget* chart)
{
    if (!fig || !chart) {
        daWarning << tr("No figure/chart available for statistical plot");
        return;
    }

    if (!mStatsChartGuideDlg) {
        return;
    }

    DAAbstractStatsChartAddWidget* w = mStatsChartGuideDlg->getCurrentStatsChartAddWidget();
    if (!w) {
        return;
    }

    // 从 widget 的动态属性获取 DAData
    QVariant dataVar = w->property("__da_data__");
    DAData data;
    if (dataVar.isValid() && dataVar.canConvert< DAData >()) {
        data = dataVar.value< DAData >();
    }
    if (!data.isDataFrame()) {
        daWarning << tr("Cannot resolve the data source for statistical plot; "
                        "please ensure a dataframe is selected in the settings window");
        return;
    }

    executeStatsPlot(params, fig, chart, data);
}

/**
 * @brief 执行统计绘图的公共逻辑
 * @param params 绘图参数 JSON
 * @param fig 目标 Figure 窗口
 * @param chart 目标 Chart 窗口
 * @param data 数据源 DAData
 */
void DAAppController::executeStatsPlot(const QJsonObject& params,
                                       DA::DAFigureWidget* fig,
                                       DA::DAChartWidget* chart,
                                       const DAData& data)
{
    QString columnName = params.value("column").toString();
    if (columnName.isEmpty()) {
        daWarning << tr("No data column selected");  // cn: 未选择数据列
        return;
    }

    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        daWarning << tr("The selected data source is empty");  // cn: 选中的数据源为空
        return;
    }

    QString plotType = params.value("__plot_type__").toString("histplot");
    qDebug() << "[executeStatsPlot] type=" << plotType << "column=" << columnName
             << "data.isDataFrame=" << data.isDataFrame();

    DAWaitCursorScoped wait;
    Q_UNUSED(wait);

    DAStatsPlotCoordinator coordinator;
    coordinator.execute(plotType, params, fig, chart, data);

    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    mRibbon->updateFigureAboutRibbon(fig);
    if (DAAppChartOperateWidget* chartopt = getChartOperateWidget()) {
        chartopt->setCurrentFigure(fig);
    }
    // 让坐标轴显示正常
    chart->rescaleAxes();
}

}  // end DA
