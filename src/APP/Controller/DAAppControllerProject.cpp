#include "DAAppController.h"
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
            tr("Whether to overwrite the file: %1").arg(fi.absoluteFilePath()),  // cn:是否覆盖文件:%1
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
            QMessageBox::StandardButton::No);
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

bool DAAppController::openCheck()
{
    DAAppProject* project        = DA_APP_CORE.getAppProject();
    const bool hasProjectContent = !project->isEmpty();
    if (project->isDirty()) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            app(),
            tr("Question"),  // cn:提示
            tr("The current project has unsaved changes. Do you want to save before opening another project?"),  // cn:当前工程有未保存的更改，是否在打开其他工程之前保存？
            QMessageBox::StandardButton::Save | QMessageBox::StandardButton::Discard
                | QMessageBox::StandardButton::Cancel,
            QMessageBox::StandardButton::Save);
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
            tr("Another project already exists. Do you want to replace it?"),  // cn:已存在其他工程，是否要替换？
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
            QMessageBox::StandardButton::No);
        return btn == QMessageBox::Yes;
    }

    return true;
}

/**
 * @brief 新建工程
 *
 * 当前工程有未保存修改时先提示保存/放弃/取消，确认后清空为未命名的空工程
 * @return true 表示已创建
 */
bool DAAppController::newProject()
{
    DAAppProject* project = DA_APP_CORE.getAppProject();
    if (project->isDirty()) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            app(),
            tr("Question"),  // cn:提示
            tr("The current project has unsaved changes. Do you want to save before creating a new project?"),  // cn:当前工程有未保存的更改，是否在新建工程之前保存？
            QMessageBox::StandardButton::Save | QMessageBox::StandardButton::Discard
                | QMessageBox::StandardButton::Cancel,
            QMessageBox::StandardButton::Save);
        switch (btn) {
        case QMessageBox::StandardButton::Save:
            if (!save()) {
                return false;  // 保存失败（含用户取消保存对话框）时中止新建
            }
            break;
        case QMessageBox::StandardButton::Discard:
            break;  // 放弃未保存的修改，直接新建
        default:
            return false;  // 用户取消
        }
    }
    project->clear();
    daInfo << tr("New project created");  // cn:已新建工程
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
        // 权限层：保存/另存为后工程路径可能变化 → 重算 ${workspace} 并下发
        agentMod->setScriptWorkspaceDir(project->getScriptWorkspaceDir());
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
        // 权限层：加载后重算 ${workspace} 并下发（与 DAAppProject 加载回调同源，幂等）
        agentMod->setScriptWorkspaceDir(project->getScriptWorkspaceDir());
        agentMod->restoreLastActiveSession();       // 填充下拉 + 全新对话
    }
    daInfo << tr("Project loaded successfully, path: %1").arg(path);  // cn:工程加载成功，路径为:%1
}

}  // end DA
