#include "DAAppController.h"
#include "DAAppTableActionHandler.h"
#include "DAAppChartActionHandler.h"
#include "DAAppWorkflowActionHandler.h"
// Qt
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTimer>
#include <QFontComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QApplication>
#include <QActionGroup>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QClipboard>
#include <QFile>
#include <QMimeData>
#include <QUrl>
// qwt
#include "qwt_figure.h"
#include "qwt_plot_series_data_picker.h"
#include <QwtPlotMarker>
#include <QwtPlotTextLabel>
#include <QwtText>
// SARibbon
#include "SARibbonGlobal.h"
#include "SARibbonMainWindow.h"

// API
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "Ribbon/DAAppRibbonArea.h"
#include "DAAppDockingArea.h"
#include "DAChartOperateWidget.h"
#include "DAAppCommand.h"
#include "DAUndoHistoryWidget.h"
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
#include "Table/DADataOperateWidget.h"
#include "Table/DADataOperatePageWidget.h"
#include "Common/DAAbstractOperateWidget.h"
#include "DataManager/DADataManageWidget.h"
#include "DAAppChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "Chart/Setting/DAChartAxisSettingPanel.h"
#include "DAAppChartManageWidget.h"
#include "SettingPages/DASettingPageGeneral.h"
#include "Common/Settings/DASettingContainerWidget.h"
#include "Common/DARecentFilesManager.h"
#include "Chart/DAChartSettingWidget.h"
#include "Chart/DADataLinkTableWidget.h"
#include "DAColorTheme.h"
#include "Chart/Setting/DAFigureWidgetSettingPanel.h"
#include "Chart/Setting3D/DAChart3DSettingWidget.h"
// Dialog
#include "DAPluginManagerDialog.h"
#include "DAAppSettingDialog.h"
#include "Dialog/DAExportToPngSettingDialog.h"
#include "Dialog/DALayoutManagerDialog.h"
#include "Dialog/DAWorkbenchAboutDialog.h"
// DAWidgets
#include "DAFontEditPannelWidget.h"
#include "SARibbonColorToolButton.h"
#include "DAShapeEditPannelWidget.h"
// Table style
#include "Table/Metadata/DATableCellStyle.h"
// 表格元数据与数据撤销命令（004 T1.27 就地形状变换：Swap 命令 + 元数据束快照/清理）
#include "Table/Metadata/DATableMetadata.h"
#include "Table/Metadata/DATableMetadataRegistry.h"
#include "DADataUndoCommand.h"
// Workflow
#include "Workflow/DAPyWorkFlowOperateWidget.h"
#include "Workflow/DAPyWorkFlowGraphicsView.h"
#include "DAGraphicsDrawRectSceneAction.h"
#include "DAGraphicsDrawTextItemSceneAction.h"
// project
#include "DAAppProject.h"
#include "DAAppProjectActionPolicy.h"
// Py
#include "DAPybind11QtCaster.hpp"
#include "Table/Dialogs/DATxtFileImportDialog.h"
#include "DAPyDTypeComboBox.h"
#include "DAPyScripts.h"
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"
// Widget
#include "Table/DADataOperateOfDataFrameWidget.h"
#include "Table/Views/DADataHeaderView.h"
#include "Table/Views/DATableDisplayFormatComboBox.h"
#include "Table/Dialogs/DADialogTableDisplayFormat.h"
#include "Table/Views/DADataTableView.h"
// Python workflow
#include "DAPyWorkFlowScene.h"
// Stats plot widgets
#include "Chart/DADialogStatsChartGuide.h"
#include "Chart/AddItem/DAChartAddStatsHistplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsKdeplot1dWidget.h"
#include "Chart/AddItem/DAChartAddStatsKdeplot2dWidget.h"
#include "Chart/AddItem/DAChartAddStatsBoxplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsHeatmapWidget.h"
#include "Chart/AddItem/DAChartAddStatsScatterplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsBarplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsRegplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsEcdfplotWidget.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Chart/AddItem/DAAbstractStatsChartAddWidget.h"
#include "Chart/DAStatsPlotCoordinator.h"
// 类型化统计绘图请求（onStatsGuideAccepted 消费字段，需完整类型）
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
// 数据统计分析浮动 dock（PMF connect notifyDataChanged 需完整类型）
#include "Chart/DADataStatsWidget.h"
// 表内快速绘图（转入绘图区的编排器）
#include "Table/Chart/DAChartQuickPlot.h"
#include "Table/Chart/DATableChartTypes.h"
// Agent
#include "DAAgentModule.h"
#include "DAAgentInterface.h"        // PMF connect 到接口信号/方法需完整类型
#include "DAAgentDockWidget.h"       // DAAppDockingArea 仅前向声明；PMF connect 需完整类型（plan-02）
#include "DAAgentLinkDispatcher.h"   // Agent 本地跳转链接分发（da-<kind>: 协议注册表）
#include "DADataManagerInterface.h"  // da-data: 链接按名字/id 查找数据集
//
#include "SettingPages/DAAppConfig.h"
#include "DAAppLayoutManager.h"
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
        QCoreApplication::translate("DAAppRibbonArea", "Warning", nullptr),                                             \
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
 * @brief 组装动作分发器共享的应用上下文依赖
 *
 * 分发器只通过这份依赖拿到协作者，不反向持有其他分发器，
 * 依赖图保持以控制器为中心的星形。
 */
DAAppActionHandlerDeps DAAppController::makeActionHandlerDeps()
{
    DAAppActionHandlerDeps d;
    d.controller = this;
    d.actions    = mActions;
    d.ribbon     = mRibbon;
    d.dock       = mDock;
    d.command    = mCommand;
    d.datas      = mDatas;
    d.config     = mConfig;
    d.mainWindow = mMainWindow;
    return d;
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
    // 数据联动表 Dock：默认隐藏，视图开关驱动显示（数据探针产生数据时经 requestRaise 自动弹出）
    mDock->getDataLinkTableDock()->setToggleViewActionMode(ads::CDockWidget::ActionModeShow);
    mDock->getDataLinkTableDock()->setToggleViewAction(mActions->actionShowDataLinkTable);
    // Agent 信号链：DAGui(Dock) 与 DAAgent 互不依赖，由 APP 层 connect（决策 D3b）。
    // 接口信号 → Dock 槽（13 条）—— DAAgentModule 不再持有 Dock，亦不再在 connectSignals
    // 中连 Bridge→Dock / this→Dock，唯一渲染路径经接口信号。
    // session-tabs：会话级事件信号首参为 sessionId，Dock 宿主按会话路由到对应视图
    //（后台会话视图实时渲染）。
    auto* agent = mCore->getAgentInterface();
    auto* dock  = mDock->getAgentDockWidget();
    if (agent && dock) {
        // 接线约定（审计 L16 文档化）：凡 Dock 设有槽的接口信号均已接线；
        // agentDone 与 agentTurnPossiblyIncomplete 为**有意不接**——前者由
        // agentBusy(false) 统一驱动 UI 恢复（done 只覆盖正常完成路径），
        // 后者的用户提醒由 Module 同刻发射的 systemMessage 承载。详见
        // DAAgentInterface.h 两个信号的注释
        connect(agent, &DAAgentInterface::agentToken, dock, &DAAgentDockWidget::onAgentToken);
        connect(agent, &DAAgentInterface::agentMessageComplete, dock, &DAAgentDockWidget::onAgentMessageComplete);
        connect(agent, &DAAgentInterface::agentToolCall, dock, &DAAgentDockWidget::onAgentToolCall);
        // 工具排队状态（决策点 2 ③）：工具卡"排队中/运行中"标识
        connect(agent, &DAAgentInterface::agentToolQueued, dock, &DAAgentDockWidget::onAgentToolQueued);
        connect(agent, &DAAgentInterface::agentToolResult, dock, &DAAgentDockWidget::onAgentToolResult);
        connect(agent, &DAAgentInterface::agentQuestion, dock, &DAAgentDockWidget::onAgentQuestion);
        // 挂起问题卡作废（审计问题 17，镜像审批 dismissed 契约）：撤未回答问题卡
        connect(agent, &DAAgentInterface::agentQuestionDismissed, dock, &DAAgentDockWidget::onQuestionDismissed);
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
        // 决策点 5 方案 c（审计问题 18）：跨工程存活会话 → 提示条 + 全部工程视图
        connect(agent, &DAAgentInterface::foreignAgentSessionsRunning, dock, &DAAgentDockWidget::onForeignAgentSessionsRunning);
        // 供应商/多模型选择：接口信号 → Dock 槽（2 条），Dock 信号 → 接口方法（1 条）
        // availableModelsChanged 载荷为 DAAgentModelRef 结构体列表（DAGui 不依赖 DAAgent，
        // 经 APP 桥接层转换为 QVariantMap{provider,model,context_window,max_output_tokens}）
        connect(agent, &DAAgentInterface::availableModelsChanged, dock,
                [dock](const QList< DA::DAAgentModelRef >& models) {
                    QVariantList vl;
                    for (const DA::DAAgentModelRef& m : models) {
                        QVariantMap vm;
                        vm["provider"]         = m.provider;
                        vm["model"]            = m.model;
                        vm["context_window"]   = m.contextWindow;
                        vm["max_output_tokens"] = m.maxOutputTokens;
                        vl.append(vm);
                    }
                    dock->onAvailableModelsChanged(vl);
                });
        connect(agent, &DAAgentInterface::activeModelChanged, dock, &DAAgentDockWidget::onActiveModelChanged);
        connect(dock, &DAAgentDockWidget::activeModelChangeRequested, agent, &DAAgentInterface::setActiveModel);
        // 权限层（permission-layer P1）：接口信号 → Dock 槽（4 条），Dock 信号 → 接口方法（3 条）
        connect(agent, &DAAgentInterface::agentToolApprovalRequest, dock, &DAAgentDockWidget::onToolApprovalRequest);
        connect(agent, &DAAgentInterface::agentToolApprovalDismissed, dock, &DAAgentDockWidget::onToolApprovalDismissed);
        // 子 agent 进度（subagent-phase1 C）：接口信号 → Dock 槽（进度卡片渲染）
        connect(agent, &DAAgentInterface::agentSubagentProgress, dock, &DAAgentDockWidget::onAgentSubagentProgress);
        connect(agent, &DAAgentInterface::permissionModeChanged, dock, &DAAgentDockWidget::onPermissionModeChanged);
        // 模式"显式设置"状态（启动推送）：Dock 据此决定 A13 确认卡（默认全自动不弹卡）
        connect(agent, &DAAgentInterface::permissionModeExplicitChanged, dock, &DAAgentDockWidget::onPermissionModeExplicitChanged);
        connect(dock, &DAAgentDockWidget::permissionModeChangeRequested, agent, &DAAgentInterface::setPermissionMode);
        connect(dock, &DAAgentDockWidget::toolApprovalDecision, agent, &DAAgentInterface::sendToolApproval);
        // 启动 yolo 确认卡（A13）用户拒绝 → 降级 auto
        connect(dock, &DAAgentDockWidget::startupModeConfirmResponse, this,
                [agent](bool keepYolo) {
                    if (!keepYolo) {
                        agent->setPermissionMode(QStringLiteral("auto"));
                    }
                });
        // Dock 信号 → 接口方法（均为信号→方法 PMF 连接，emit 源信号即调用方法体，
        // 含各自持久化/启动逻辑，无需 lambda）。
        // session-tabs：「+」走 Dock 宿主的 unbound 视图懒创建路径（首条消息才落盘
        // 建会话），不再连接 newSession；发消息/回答前宿主已"交互即激活"确保
        // 来源视图会话为模块当前会话。
        connect(dock, &DAAgentDockWidget::sendMessageRequested, agent, &DAAgentInterface::sendMessage);
        connect(dock, &DAAgentDockWidget::stopRequested, agent, &DAAgentInterface::stop);
        connect(dock, &DAAgentDockWidget::userAnswerSelected, agent, &DAAgentInterface::sendUserAnswer);
        connect(dock, &DAAgentDockWidget::sessionSwitchRequested, agent, &DAAgentInterface::switchSession);
        connect(dock, &DAAgentDockWidget::sessionDeleteRequested, agent, &DAAgentInterface::deleteSession);
        connect(dock, &DAAgentDockWidget::sessionRenameRequested, agent, &DAAgentInterface::renameSession);
        // session-tabs 新增桥接：视图生命周期 / unbound 激活 / 按会话停止
        //（会话管理对话框/标签右键"停止"亦走 stopSessionRequested——审计 L14）
        connect(dock, &DAAgentDockWidget::sessionViewAttachedChanged, agent, &DAAgentInterface::setSessionViewAttached);
        connect(dock, &DAAgentDockWidget::currentSessionClearedRequested, agent, &DAAgentInterface::clearCurrentSession);
        connect(dock, &DAAgentDockWidget::stopSessionRequested, agent, &DAAgentInterface::stopSession);
        // Agent 本地跳转超链接：da-<kind>: 协议链接点击 → 分发器按协议处理
        connect(dock, &DAAgentDockWidget::linkActivated, this, &DAAppController::onLinkActivated);
    }
    // Agent 本地跳转链接处理器注册（da-figure:/da-data: 等协议）
    setupAgentLinkHandlers();
    // 动作分发器：按业务域装配，控制器只做装配与跨域协调
    mTableHandler = new DAAppTableActionHandler(makeActionHandlerDeps(), this);
    mChartHandler = new DAAppChartActionHandler(makeActionHandlerDeps(), this);
    mWorkflowHandler = new DAAppWorkflowActionHandler(makeActionHandlerDeps(), this);
    initConnection();
    mTableHandler->initConnections();
    mChartHandler->initConnections();
    mWorkflowHandler->initConnections();
    initScripts();
    // 数据管理树右键菜单：series 节点=列操作菜单（与表头右键共用 action）；
    // 数据集条目=重命名/移除/导出菜单（复用 ribbon 数据标签页 action）
    if (DADataManageWidget* dmw = getDataManageWidget()) {
        if (DADataManagerTreeWidget* tree = dmw->getTreeWidget()) {
            mTableHandler->setupDataManagerTreeContextMenu(tree);
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
        agentMod->setScriptWorkspaceDir(QString());  // 权限层：启动无工程 → ${workspace} 清空
        agentMod->restoreLastActiveSession();         // 填充下拉 + 全新对话
        agentMod->pushModelSelection();  // 推送供应商/模型列表 + 激活选择到 Dock 下拉
        agentMod->pushPermissionMode();  // 推送权限模式到 Dock 模式选择器（显式 yolo 时 Dock 弹启动确认，A13）
        agentMod->prestartAgent();  // 预启动 agent 子进程（受 auto_prestart 开关 + LLM 配置控制）
    }
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
    // 当前窗口切换后同步「快速绘图」按钮可用状态（无数据表页/无选区时置灰）
    if (mTableHandler) {
        mTableHandler->updateQuickPlotActionState();
        // 同步「排序」面板可用状态（无表格页/只读源时置灰）
        mTableHandler->updateSortActionState();
    }
    if (nullptr == widget) {
        mRibbon->hideContextCategory(DAAppRibbonArea::AllContextCategory);
        return;
    }
    // 数据操作窗口激活时，检查是否需要显示m_contextDataFrame
    if (widget == getDataOperateWidget()) {
        // 数据窗口激活
        mLastFocusedOpertateWidget = LastFocusedOnDataOpt;
        // 激活当前数据页的undo栈，保证全局undo/redo action状态同步
        activateUndoStackForWidget(getDataOperateWidget());
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
        // 激活当前figure的undo栈，保证全局undo/redo action状态同步
        activateUndoStackForWidget(getChartOperateWidget());
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
 * @brief 激活操作窗口当前的undo栈
 *
 * 顶层dock焦点切到数据操作/绘图窗口时，嵌套管理器的focusedDockWidgetChanged可能不触发
 * （焦点未落入嵌套dock内容区），此函数作为兜底把当前页/当前figure的栈设为active，
 * 使QUndoGroup的undo/redo action状态跟随当前窗口
 * @param w 操作窗口，其getUndoStack()返回当前激活页/figure的栈
 */
void DAAppController::activateUndoStackForWidget(DAAbstractOperateWidget* w)
{
    if (nullptr == w) {
        return;
    }
    QUndoStack* stack = w->getUndoStack();
    if (stack && !stack->isActive()) {
        stack->setActive();
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
 * @brief 主页剪贴板-剪切（按焦点路由）
 */
void DAAppController::onActionCutTriggered()
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        if (QAction* act = getWorkFlowOperateWidget()->getInnerAction(DAPyWorkFlowOperateWidget::ActionCut)) {
            act->trigger();
        }
    } else if (isLastFocusedOnDataOptWidget()) {
        if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
            if (dfopt->cutSelection() > 0) {
                setDirty();
            }
        }
    }
}

/**
 * @brief 主页剪贴板-复制（按焦点路由）
 */
void DAAppController::onActionCopyTriggered()
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        if (QAction* act = getWorkFlowOperateWidget()->getInnerAction(DAPyWorkFlowOperateWidget::ActionCopy)) {
            act->trigger();
        }
    } else if (isLastFocusedOnDataOptWidget()) {
        if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
            DADataTableView* tv = dfopt->getDataTableView();
            if (tv && !tv->copySelectionToClipboard()) {
                daWarning << tr("No cells selected to copy");  // cn:没有选中可复制的单元格
            }
        }
    } else if (isLastFocusedOnChartOptWidget()) {
        // 图表焦点：复制绘图到剪贴板
        mChartHandler->onActionCopyFigureToClipboardTriggered();
    }
}

/**
 * @brief 主页剪贴板-粘贴（按焦点路由）
 */
void DAAppController::onActionPasteTriggered()
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        if (QAction* act = getWorkFlowOperateWidget()->getInnerAction(DAPyWorkFlowOperateWidget::ActionPaste)) {
            act->trigger();
        }
    } else if (isLastFocusedOnDataOptWidget()) {
        if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
            if (dfopt->pasteFromClipboard() > 0) {
                setDirty();
            }
        }
    }
}

/**
 * @brief 主页剪贴板-删除（按焦点路由）
 */
void DAAppController::onActionDeleteTriggered()
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        if (QAction* act = getWorkFlowOperateWidget()->getInnerAction(DAPyWorkFlowOperateWidget::ActionDelete)) {
            act->trigger();
        }
    } else if (isLastFocusedOnDataOptWidget()) {
        if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
            // 删除 = 选中单元格设置为nan（可撤销，与右键"移除单元格"同语义）
            dfopt->removeSelectCell();
            setDirty();
        }
    }
}

/**
 * @brief 主页剪贴板-全选（按焦点路由）
 */
void DAAppController::onActionSelectAllTriggered()
{
    if (isLastFocusedOnWorkflowOptWidget()) {
        if (QAction* act = getWorkFlowOperateWidget()->getInnerAction(DAPyWorkFlowOperateWidget::ActionSelectAll)) {
            act->trigger();
        }
    } else if (isLastFocusedOnDataOptWidget()) {
        if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
            if (DADataTableView* tv = dfopt->getDataTableView()) {
                tv->selectAll();
            }
        }
    }
}

}  // end DA
