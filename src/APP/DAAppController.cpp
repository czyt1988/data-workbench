﻿#include "DAAppController.h"
// stl
#include <memory>
// Qt
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QFontComboBox>
#include <QComboBox>
#include <QInputDialog>
#include <QMenu>
// API
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "DAAppRibbonArea.h"
#include "DAAppDockingArea.h"
#include "DAAppCommand.h"
#include "DAAppActions.h"
#include "DAAppDataManager.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// command
#include "DACommandsDataManager.h"
// Widget
#include "DAWaitCursorScoped.h"
#include "DADataOperateWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include "DAPyDTypeComboBox.h"
#include "DADataManageWidget.h"
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
// Dialog
#include "DAPluginManagerDialog.h"
#include "Dialog/DARenameColumnsNameDialog.h"
// DACommonWidgets
#include "DAFontEditPannelWidget.h"
#include "DAShapeEditPannelWidget.h"
// Workflow
#include "DAWorkFlowOperateWidget.h"
#include "DAWorkFlowGraphicsView.h"
#include "DADataWorkFlow.h"
// project
#include "DAProject.h"
// Py
#include "DAPyScripts.h"
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"

#ifndef DAAPPRIBBONAREA_WINDOW_NAME
#define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppController", "DA", nullptr)  //
#endif

//未实现的功能标记
#define DAAPPCONTROLLER_PASS()                                                                                         \
    QMessageBox::                                                                                                      \
            warning(app(),                                                                                             \
                    QCoreApplication::translate("DAAppRibbonArea", "warning", nullptr),                                \
                    QCoreApplication::translate("DAAppRibbonArea",                                                     \
                                                "The current function is not implemented, only the UI is reserved, "   \
                                                "please pay attention: https://gitee.com/czyt1988/data-work-flow",     \
                                                nullptr))

//快速链接信号槽
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
    _mainWindow = mainWindow;
    return (*this);
}

/**
 * @brief 设置core
 * @param core
 * @return
 */
DAAppController& DAAppController::setAppCore(DAAppCore* core)
{
    _core = core;
    return (*this);
}
/**
 * @brief 设置ribbon
 * @param ribbon
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppRibbonArea(DAAppRibbonArea* ribbon)
{
    _ribbon = ribbon;
    return (*this);
}

/**
 * @brief 设置dock
 * @param dock
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppDockingArea(DAAppDockingArea* dock)
{
    _dock = dock;
    return (*this);
}

/**
 * @brief 设置AppCommand
 * @param cmd
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppCommand(DAAppCommand* cmd)
{
    _command = cmd;
    return (*this);
}

/**
 * @brief 设置AppActions
 * @param act
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppActions(DAAppActions* act)
{
    _actions = act;
    return (*this);
}

/**
 * @brief 设置AppDataManager
 * @param d
 * @return 返回自身引用,方便链式调用
 */
DAAppController& DAAppController::setAppDataManager(DAAppDataManager* d)
{
    _datas = d;
    return (*this);
}

/**
 * @brief 获取app
 * @return
 */
AppMainWindow* DAAppController::app() const
{
    return _mainWindow;
}

/**
 * @brief 控制层初始化
 */
void DAAppController::initialize()
{
    initConnection();
    initScripts();
}

/**
 * @brief 获取当前dataframeOperateWidget,如果没有返回nullptr
 *
 * 此函数不返回nullptr的前提是:DataOperateWidget处于焦点，且是DataFrameOperateWidget
 * @param checkDataOperateAreaFocused 是否检测DataOperateWidget是否处于焦点，默认为true
 * @return
 */
DADataOperateOfDataFrameWidget* DAAppController::getCurrentDataFrameOperateWidget(bool checkDataOperateAreaFocused)
{
    if (nullptr == _dock) {
        return nullptr;
    }
    if (checkDataOperateAreaFocused) {
        if (!(_dock->isDockingAreaFocused(DAAppDockingArea::DockingAreaDataOperate))) {
            //窗口未选中就退出
            return nullptr;
        }
    }
    return _dock->getDataOperateWidget()->getCurrentDataFrameWidget();
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAWorkFlowOperateWidget* DAAppController::getWorkFlowOperateWidget() const
{
    return _dock->getWorkFlowOperateWidget();
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataOperateWidget* DAAppController::getDataOperateWidget() const
{
    return _dock->getDataOperateWidget();
}

/**
 * @brief 获取绘图操作窗口
 * @return
 */
DAChartOperateWidget* DAAppController::getChartOperateWidget() const
{
    return _dock->getChartOperateWidget();
}

/**
 * @brief 获取数据管理窗口
 * @return
 */
DADataManageWidget* DAAppController::getDataManageWidget() const
{
    return _dock->getDataManageWidget();
}
/**
 * @brief 获取当前的绘图
 * @return 如果没有回返回nullptr
 */
DAFigureWidget* DAAppController::getCurrentFigure()
{
    return getChartOperateWidget()->getCurrentFigure();
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
 * @brief 判断当前是否是在绘图操作模式，就算绘图操作不在焦点，但绘图操作在前端，此函数也返回true
 * @return
 */
bool DAAppController::isLastFocusedOnChartOptWidget() const
{
    return m_lastFocusedOpertateWidget.testFlag(LastFocusedOnChartOpt);
}

/**
 * @brief 判断当前是否是在工作流操作模式，就算工作流操作不在焦点，但工作流操作在前端，此函数也返回true
 * @return
 */
bool DAAppController::isLastFocusedOnWorkflowOptWidget() const
{
    return m_lastFocusedOpertateWidget.testFlag(LastFocusedOnWorkflowOpt);
}

/**
 * @brief 判断当前是否是在数据操作模式，就算数据操作不在焦点，但工作流操作在前端，此函数也返回true
 * @return
 */
bool DAAppController::isLastFocusedOnDataOptWidget() const
{
    return m_lastFocusedOpertateWidget.testFlag(LastFocusedOnDataOpt);
}

/**
 * @brief 基本绑定
 * @note 在setDockAreaInterface函数中还有很多绑定操作
 */
void DAAppController::initConnection()
{
    // Main Category
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionOpen, onActionOpenTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionSave, onActionSaveTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionSaveAs, onActionSaveAsTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionAppendProject, onActionAppendProjectTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionSetting, onActionSettingTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionPluginManager, onActionPluginManagerTriggered);
    // Data Category
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionAddData, onActionAddDataTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionRemoveData, onActionRemoveDataTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionAddDataFolder, onActionAddDataFolderTriggered);
    // Chart Category
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionAddFigure, onActionAddFigureTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionFigureResizeChart, onActionFigureResizeChartTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionFigureNewXYAxis, onActionFigureNewXYAxisTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnableGrid, onActionChartEnableGridTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnableGridX, onActionChartEnableGridXTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnableGridY, onActionChartEnableGridYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnableGridXMin, onActionChartEnableGridXMinEnableTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnableGridYMin, onActionChartEnableGridYMinTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnableZoom, onActionChartEnableZoomTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartZoomIn, onActionChartZoomInTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartZoomOut, onActionChartZoomOutTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartZoomAll, onActionChartZoomAllTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnablePan, onActionChartEnablePanTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnablePickerCross, onActionChartEnablePickerCrossTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnablePickerY, onActionChartEnablePickerYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnablePickerXY, onActionChartEnablePickerXYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChartEnableLegend, onActionChartEnableLegendTriggered);
    connect(_actions->actionGroupChartLegendAlignment, &QActionGroup::triggered, this, &DAAppController::onActionGroupChartLegendAlignmentTriggered);
    connect(_ribbon->m_spinboxChartLegendMaxColumns, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAAppController::onChartLegendMaxColumnsValueChanged);
    connect(_ribbon->m_spinboxChartLegendMargin, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAAppController::onChartLegendMarginValueChanged);
    connect(_ribbon->m_spinboxChartLegendSpacing, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAAppController::onChartLegendSpacingValueChanged);
    connect(_ribbon->m_spinboxChartLegendItemMargin, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAAppController::onChartLegendItemMarginValueChanged);
    connect(_ribbon->m_spinboxChartLegendItemSpacing, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAAppController::onChartLegendItemSpacingValueChanged);
    connect(_ribbon->m_spinboxChartLegendBorderRadius,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DAAppController::onChartLegendBorderRadiusValueChanged);
    // 数据操作的上下文标签 Data Operate Context Category
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionInsertRow, onActionInsertRowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionInsertRowAbove, onActionInsertRowAboveTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionInsertColumnRight, onActionInsertColumnRightTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionInsertColumnLeft, onActionInsertColumnLeftTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionRemoveRow, onActionRemoveRowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionRemoveColumn, onActionRemoveColumnTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionRemoveCell, onActionRemoveCellTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionRenameColumns, onActionRenameColumnsTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionCreateDataDescribe, onActionCreateDataDescribeTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionCastToNum, onActionCastToNumTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionCastToString, onActionCastToStringTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionCastToDatetime, onActionCastToDatetimeTriggered);
    //不知为何使用函数指针无法关联信号和槽
    // connect(m_comboxColumnTypes, &DAPyDTypeComboBox::currentDTypeChanged, this,&DAAppRibbonArea::onComboxColumnTypesCurrentDTypeChanged);
    // QObject::connect: signal not found in DAPyDTypeComboBox
    connect(_ribbon->m_comboxColumnTypes, SIGNAL(currentDTypeChanged(DAPyDType)), this, SLOT(onComboxColumnTypesCurrentDTypeChanged(DAPyDType)));
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionChangeToIndex, onActionChangeToIndexTriggered);
    // View Category
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionShowWorkFlowArea, onActionShowWorkFlowAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionShowChartArea, onActionShowChartAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionShowDataArea, onActionShowDataAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionShowMessageLogView, onActionShowMessageLogViewTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionShowSettingWidget, onActionSettingWidgetTriggered);

    // workflow edit 工作流编辑
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionWorkflowNew, onActionNewWorkflowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionWorkflowRun, onActionRunCurrentWorkflowTriggered);
    // workflow edit 工作流编辑/data edit 绘图编辑
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionWorkflowStartDrawRect, onActionStartDrawRectTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionWorkflowStartDrawText, onActionStartDrawTextTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionWorkflowAddBackgroundPixmap, onActionAddBackgroundPixmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionWorkflowLockBackgroundPixmap, onActionLockBackgroundPixmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(_actions->actionWorkflowEnableItemMoveWithBackground, onActionEnableItemMoveWithBackgroundTriggered);

    //===================================================
    // setDockAreaInterface 有其他的绑定
    //===================================================
    //! 注意！！
    //! 在setDockAreaInterface函数中还有很多绑定操作
    //
    DAProject* p = _core->getAppProject();
    if (p) {
        connect(p, &DAProject::projectSaved, this, &DAAppController::onProjectSaved);
        connect(p, &DAProject::projectLoaded, this, &DAAppController::onProjectLoaded);
    }

    //===================================================
    // workflow窗口字体相关信号槽
    //===================================================

    connect(_ribbon->m_workflowFontEditPannel, &DA::DAFontEditPannelWidget::currentFontChanged, this, &DAAppController::onCurrentWorkflowFontChanged);
    connect(_ribbon->m_workflowFontEditPannel, &DA::DAFontEditPannelWidget::currentFontColorChanged, this, &DAAppController::onCurrentWorkflowFontColorChanged);
    connect(_ribbon->m_workflowShapeEditPannelWidget,
            &DAShapeEditPannelWidget::backgroundBrushChanged,
            this,
            &DAAppController::onCurrentWorkflowShapeBackgroundBrushChanged);
    connect(_ribbon->m_workflowShapeEditPannelWidget, &DAShapeEditPannelWidget::borderPenChanged, this, &DAAppController::onCurrentWorkflowShapeBorderPenChanged);

    //===================================================
    // name
    //===================================================
    connect(_dock->dockManager(), &ads::CDockManager::focusedDockWidgetChanged, this, &DAAppController::onFocusedDockWidgetChanged);
    // DADataManageWidget 数据操作
    DADataManageWidget* dmw = getDataManageWidget();
    connect(dmw, &DADataManageWidget::dataViewModeChanged, this, &DAAppController::onDataManageWidgetDataViewModeChanged);
    // DADataOperateWidget
    DADataOperateWidget* dow = _dock->getDataOperateWidget();
    connect(dow, &DADataOperateWidget::pageAdded, this, &DAAppController::onDataOperatePageAdded);
    // DAChartOperateWidget
    DAChartOperateWidget* cow = _dock->getChartOperateWidget();
    connect(cow, &DAChartOperateWidget::figureCreated, this, &DAAppController::onFigureCreated);
    connect(cow, &DAChartOperateWidget::currentFigureChanged, this, &DAAppController::onCurrentFigureChanged);
    connect(cow, &DAChartOperateWidget::chartAdded, this, &DAAppController::onChartAdded);
    connect(cow, &DAChartOperateWidget::currentChartChanged, this, &DAAppController::onCurrentChartChanged);
    //鼠标动作完成的触发
    connect(_dock->getWorkFlowOperateWidget(), &DAWorkFlowOperateWidget::mouseActionFinished, this, &DAAppController::onWorkFlowGraphicsSceneMouseActionFinished);
    //
    DAWorkFlowOperateWidget* workflowOpt = _dock->getWorkFlowOperateWidget();
    connect(workflowOpt, &DAWorkFlowOperateWidget::selectionItemChanged, this, &DAAppController::onSelectionItemChanged);
    connect(_actions->actionWorkflowShowGrid, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowShowGrid);
    connect(_actions->actionWorkflowNew, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::appendWorkflowWithDialog);
    connect(_actions->actionWorkflowWholeView, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowWholeView);
    connect(_actions->actionWorkflowZoomIn, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomIn);
    connect(_actions->actionWorkflowZoomOut, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomOut);
}

/**
 * @brief 脚本定义的内容初始化
 */
void DAAppController::initScripts()
{
    if (!_core->isPythonInterpreterInitialized()) {
        return;
    }
    _fileReadFilters = QStringList(DAPyScripts::getInstance().getIO().getFileReadFilters());
    qDebug() << _fileReadFilters;
}

/**
 * @brief DADataManageWidget查看数据的模式改变
 * @param v
 */
void DAAppController::onDataManageWidgetDataViewModeChanged(DADataManageWidget::DataViewMode v)
{
    _actions->actionAddDataFolder->setEnabled(v == DADataManageWidget::ViewDataInTree);
}

/**
 * @brief GraphicsScene的鼠标动作执行完成，把action的选中标记清除
 * @param mf
 */
void DAAppController::onWorkFlowGraphicsSceneMouseActionFinished(DAWorkFlowGraphicsScene::MouseActionFlag mf)
{
    switch (mf) {
    case DAWorkFlowGraphicsScene::StartAddRect:
        _actions->actionWorkflowStartDrawRect->setChecked(false);
        break;
    case DAWorkFlowGraphicsScene::StartAddText:
        _actions->actionWorkflowStartDrawText->setChecked(false);
        break;
    default:
        break;
    }
}

/**
 * @brief 插件管理 [config category] - [Plugin Manager]
 * @param on
 */
void DAAppController::onActionPluginManagerTriggered(bool on)
{
    Q_UNUSED(on);
    DAPluginManagerDialog dlg(app());

    dlg.exec();
}

/**
 * @brief 设定界面
 */
void DAAppController::onActionSettingTriggered()
{
    DAAPPCONTROLLER_PASS();
}

/**
 * @brief DockWidget的焦点变化
 * @param old
 * @param now
 */
void DAAppController::onFocusedDockWidgetChanged(ads::CDockWidget* old, ads::CDockWidget* now)
{
    Q_UNUSED(old);

    if (nullptr == now) {
        _ribbon->hideContextCategory(DAAppRibbonArea::AllContextCategory);
        return;
    }
    //数据操作窗口激活时，检查是否需要显示m_contextDataFrame
    if (now->widget() == getDataOperateWidget()) {
        //数据窗口激活
        m_lastFocusedOpertateWidget = LastFocusedOnDataOpt;
        _ribbon->showContextCategory(DAAppRibbonArea::ContextCategoryData);
    } else if (now->widget() == getWorkFlowOperateWidget()) {
        //工作流窗口激活
        m_lastFocusedOpertateWidget = LastFocusedOnWorkflowOpt;
        getWorkFlowOperateWidget()->setUndoStackActive();
        _ribbon->showContextCategory(DAAppRibbonArea::ContextCategoryWorkflow);
    } else if (now->widget() == getChartOperateWidget()) {
        //绘图窗口激活
        m_lastFocusedOpertateWidget = LastFocusedOnChartOpt;
        _ribbon->showContextCategory(DAAppRibbonArea::ContextCategoryChart);
    } else if (now->widget() == getDataManageWidget()) {
        if (_command) {
            QUndoStack* stack = _command->getDataManagerStack();
            if (stack && !(stack->isActive())) {  // Data 相关的窗口 undostack激活
                stack->setActive();
            }
        }
    }
}
/**
 * @brief 打开文件
 */
void DAAppController::onActionOpenTriggered()
{
    // TODO : 这里要加上工程文件的打开支持
    QFileDialog dialog(app());
    QStringList filters;
    filters << tr("project file(*.%1)").arg(DAProject::getProjectFileSuffix());
    dialog.setNameFilters(filters);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    DAProject* project = DA_APP_CORE.getAppProject();
    if (!project->getProjectDir().isEmpty()) {
        if (project->isDirty()) {
            // TODO 没有保存。先询问是否保存
            QMessageBox::StandardButton
                    btn = QMessageBox::question(nullptr,
                                                tr("Question"),  //提示
                                                tr("Another project already exists. Do you want to replace it?")  //已存在其他工程，是否要替换？
                    );
            if (btn == QMessageBox::Yes) {
                project->clear();
            } else {
                return;
            }
        } else {
            project->clear();
        }
    }
    DA_WAIT_CURSOR_SCOPED();

    if (!project->load(fileNames.first())) {
        qCritical() << tr("failed to load project file:%1").arg(fileNames.first());
        return;
    }
    //设置工程名称给标题
    app()->setWindowTitle(QString("%1").arg(project->getProjectBaseName()));
}

/**
 * @brief 另存为
 */
void DAAppController::onActionSaveAsTriggered()
{
    QString projectPath = QFileDialog::getSaveFileName(app(),
                                                       tr("Save Project"),  //保存工程
                                                       QString(),
                                                       tr("project file (*.%1)").arg(DAProject::getProjectFileSuffix())  // 工程文件
    );
    if (projectPath.isEmpty()) {
        //取消退出
        return;
    }
    QFileInfo fi(projectPath);
    if (fi.exists()) {
        //说明是目录
        QMessageBox::StandardButton btn = QMessageBox::question(nullptr,
                                                                tr("Warning"),
                                                                tr("Whether to overwrite the file:%1").arg(fi.absoluteFilePath()));
        if (btn != QMessageBox::Yes) {
            return;
        }
    }
    //另存为
    DA_WAIT_CURSOR_SCOPED();
    DAProject* project = DA_APP_CORE.getAppProject();
    if (!project->save(projectPath)) {
        qCritical() << tr("Project saved failed!,path is %1").arg(projectPath);  //工程保存失败！路径位于:%1
        return;
    }
    app()->setWindowTitle(QString("%1").arg(project->getProjectBaseName()));
    qInfo() << tr("Project saved successfully,path is %1").arg(projectPath);  //工程保存成功，路径位于:%1
}

void DAAppController::onActionAppendProjectTriggered()
{
    QFileDialog dialog(app());
    QStringList filters;
    filters << tr("project file(*.%1)").arg(DAProject::getProjectFileSuffix());
    dialog.setNameFilters(filters);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    DAProject* project = DA_APP_CORE.getAppProject();
    DA_WAIT_CURSOR_SCOPED();

    if (!project->appendWorkflowInProject(fileNames.first(), true)) {
        qCritical() << tr("failed to load project file:%1").arg(fileNames.first());
        return;
    }
    //设置工程名称给标题
    if (project->getProjectBaseName().isEmpty()) {
        app()->setWindowTitle(QString("untitle"));
    } else {
        app()->setWindowTitle(QString("%1").arg(project->getProjectBaseName()));
    }
}

/**
 * @brief 保存工程
 */
void DAAppController::onActionSaveTriggered()
{
    DAProject* project      = DA_APP_CORE.getAppProject();
    QString projectFilePath = project->getProjectFilePath();
    qDebug() << "Save Project,Path=" << projectFilePath;
    if (projectFilePath.isEmpty()) {
        QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        projectFilePath = QFileDialog::getSaveFileName(nullptr,
                                                       tr("Save Project"),  //保存工程
                                                       desktop,
                                                       tr("Project Files (*.%1)").arg(DAProject::getProjectFileSuffix())  // 工程文件 (*.%1)
        );
        if (projectFilePath.isEmpty()) {
            //取消退出
            return;
        }
    }
    bool saveRet = project->save(projectFilePath);
    if (!saveRet) {
        qCritical() << tr("Project saved failed!,path is %1").arg(projectFilePath);  //工程保存失败！路径位于:%1
    }
}

/**
 * @brief 工程成功保存触发的信号
 * @param path
 */
void DAAppController::onProjectSaved(const QString& path)
{
    DAProject* project = DA_APP_CORE.getAppProject();
    app()->setWindowTitle(QString("%1-%2").arg(DAAPPRIBBONAREA_WINDOW_NAME, project->getProjectBaseName()));
    if (_dock) {
        DAWorkFlowOperateWidget* wf = _dock->getWorkFlowOperateWidget();
        if (wf) {
            wf->setCurrentWorkflowName(project->getProjectBaseName());
        }
    }
    qInfo() << tr("Project saved successfully,path is %1").arg(path);  //工程保存成功，路径位于:%1
}

/**
 * @brief 工程成功加载触发的信号
 * @param path
 */
void DAAppController::onProjectLoaded(const QString& path)
{
    DAProject* project = DA_APP_CORE.getAppProject();
    app()->setWindowTitle(QString("%1-%2").arg(DAAPPRIBBONAREA_WINDOW_NAME, project->getProjectBaseName()));
    if (_dock) {
        DAWorkFlowOperateWidget* wf = _dock->getWorkFlowOperateWidget();
        if (wf) {
            wf->setCurrentWorkflowName(project->getProjectBaseName());
        }
    }
    qInfo() << tr("Project load successfully,path is %1").arg(path);  //工程保存成功，路径位于:%1
}

/**
 * @brief 数据操作窗口添加，需要绑定相关信号槽到ribbon的页面
 * @param page
 */
void DAAppController::onDataOperatePageAdded(DADataOperatePageWidget* page)
{
    switch (page->getDataOperatePageType()) {
    case DADataOperatePageWidget::DataOperateOfDataFrame: {
        DADataOperateOfDataFrameWidget* w = static_cast< DADataOperateOfDataFrameWidget* >(page);
        connect(w, &DADataOperateOfDataFrameWidget::selectTypeChanged, this, &DAAppController::onDataOperateDataFrameWidgetSelectTypeChanged);
    } break;
    default:
        break;
    }
}

/**
 * @brief 选择的样式改变信号
 * @param column
 * @param dt
 */
void DAAppController::onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DAPyDType dt)
{
    Q_UNUSED(column);
    _ribbon->setDataframeOperateCurrentDType(dt);
}

/**
 * @brief 添加背景图
 */
void DAAppController::onActionAddBackgroundPixmapTriggered()
{
    QStringList filters;
    filters << tr("Image files (*.png *.jpg)")  //图片文件 (*.png *.jpg)
            << tr("Any files (*)")              //任意文件 (*)
            ;

    QFileDialog dialog(app());
    dialog.setNameFilters(filters);

    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList f = dialog.selectedFiles();
    if (!f.isEmpty()) {
        DAWorkFlowOperateWidget* ow = _dock->getWorkFlowOperateWidget();
        ow->addBackgroundPixmap(f.first());
        _dock->raiseDockingArea(DAAppDockingArea::DockingAreaWorkFlowOperate);
    }
}

void DAAppController::onActionLockBackgroundPixmapTriggered(bool on)
{
    _dock->getWorkFlowOperateWidget()->setBackgroundPixmapLock(on);
}

void DAAppController::onActionEnableItemMoveWithBackgroundTriggered(bool on)
{
    _dock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()->enableItemMoveWithBackground(on);
}

void DAAppController::onActionRunCurrentWorkflowTriggered()
{
    qDebug() << "onActionRunTriggered";
    //先检查是否有工程
    DAProject* p = DA_APP_CORE.getAppProject();
    if (nullptr == p) {
        qCritical() << tr("get null project");  // cn:空工程，接口异常
        return;
    }
    QString bn = p->getProjectBaseName();
    if (bn.isEmpty()) {
        QMessageBox::warning(app(),
                             tr("warning"),                                                   // cn:警告
                             tr("Before running the workflow, you need to save the project")  // cn：在运行工作流之前，需要先保存工程
        );
        return;
    }
    _dock->getWorkFlowOperateWidget()->runCurrentWorkFlow();
}

void DAAppController::onCurrentWorkflowFontChanged(const QFont& f)
{
    DAWorkFlowOperateWidget* wf = _dock->getWorkFlowOperateWidget();
    wf->setDefaultTextFont(f);
    wf->setSelectTextFont(f);
}

void DAAppController::onCurrentWorkflowFontColorChanged(const QColor& c)
{
    DAWorkFlowOperateWidget* wf = _dock->getWorkFlowOperateWidget();
    wf->setDefaultTextColor(c);
    wf->setSelectTextColor(c);
}

void DAAppController::onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b)
{
    DAWorkFlowOperateWidget* wf = _dock->getWorkFlowOperateWidget();
    wf->setSelectShapeBackgroundBrush(b);
}

void DAAppController::onCurrentWorkflowShapeBorderPenChanged(const QPen& p)
{
    DAWorkFlowOperateWidget* wf = _dock->getWorkFlowOperateWidget();
    wf->setSelectShapeBorderPen(p);
}

void DAAppController::onSelectionItemChanged(QGraphicsItem* lastSelectItem)
{
    if (lastSelectItem == nullptr) {
        return;
    }
    _ribbon->updateWorkflowItemAboutRibbon(lastSelectItem);
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
    qDebug() << "DAAppController::onFigureCreate";
    f->getUndoStack()->setActive();
    // updateFigureAboutRibbon(f);//在onActionAddFigureTriggered中调用了
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
    f->getUndoStack()->setActive();
    _ribbon->updateFigureAboutRibbon(f);
}

/**
 * @brief 新的chart创建
 * @param c
 */
void DAAppController::onChartAdded(DAChartWidget* c)
{
    if (nullptr == c) {
        return;
    }
    // qDebug() << "DAAppController::onChartAdded";
    // updateChartAboutRibbon(c);//在onActionFigureNewXYAxisTriggered中调用了
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
    qDebug() << "DAAppController::onCurrentChartChanged";
    _ribbon->updateChartAboutRibbon(c);
}

/**
 * @brief 添加数据
 */
void DAAppController::onActionAddDataTriggered()
{
    QFileDialog dialog(app());
    dialog.setNameFilters(_fileReadFilters);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    DA_WAIT_CURSOR_SCOPED();
    int importdataCount = _datas->importFromFiles(fileNames);
    if (importdataCount > 0) {
        _dock->raiseDockByWidget((QWidget*)(_dock->getDataManageWidget()));
    }
}

/**
 * @brief 移除数据
 */
void DAAppController::onActionRemoveDataTriggered()
{
    DADataManageWidget* dmw = _dock->getDataManageWidget();
    dmw->removeSelectData();
}

/**
 * @brief 添加数据文件夹
 */
void DAAppController::onActionAddDataFolderTriggered()
{
    DADataManageWidget* dmw = _dock->getDataManageWidget();
    dmw->addDataFolder();
}

/**
 * @brief 添加一个figure
 */
void DAAppController::onActionAddFigureTriggered()
{
    DAChartOperateWidget* chartopt = getChartOperateWidget();
    DAFigureWidget* fig            = chartopt->createFigure();
    //把fig的undostack添加
    _command->addStack(fig->getUndoStack());
    //这里不需要回退
    DAChartWidget* chart = fig->createChart();
    QVector< double > x, y;
    for (int i = 0; i < 10000; ++i) {
        x.append(i);
        y.append(std::sin(double(i / 1000.0)));
    }
    chart->setXLabel("x");
    chart->setYLabel("y");
    chart->addCurve(x, y)->setTitle("curve");
    chart->enableLegendPanel();
    _ribbon->updateFigureAboutRibbon(fig);
    _dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 改变绘图的大小
 * @param on
 */
void DAAppController::onActionFigureResizeChartTriggered(bool on)
{
    DAFigureWidget* fig = getCurrentFigure();
    if (nullptr == fig) {
        return;
    }
    _dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    fig->enableSubChartEditor(on);
}

/**
 * @brief 新建一个坐标系
 */
void DAAppController::onActionFigureNewXYAxisTriggered()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        qWarning() << tr("Before creating a new coordinate,you need to create a figure");  // cn:在创建一个坐标系之前，需要先创建一个绘图窗口
        return;
    }
    DAChartWidget* w = fig->createChart_(0.1, 0.1, 0.4, 0.4);
    w->enableGrid();
    w->enablePan();
    w->enableXYDataPicker();
    w->addCurve({ 1e-4, 2e-4, 3e-4, 4e-4, 5e-4 }, { 3e-4, 5e-4, 8e-4, 0, -3e-4 })->setTitle("curve1");
    w->addCurve({ 1e-4, 2e-4, 3e-4, 4e-4, 5e-4 }, { 5e-4, 7e-4, 0e-4, -1e-3, 1e-4 })->setTitle("curve2");
    _ribbon->updateChartAboutRibbon(w);
    _dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 允许网格
 * @param on
 */
void DAAppController::onActionChartEnableGridTriggered(bool on)
{
    qDebug() << "onActionChartGridEnableTriggered";
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableGrid(on);
        _ribbon->updateChartGridAboutRibbon(w);
    }
}

/**
 * @brief 横向网格
 * @param on
 */
void DAAppController::onActionChartEnableGridXTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableGridX(on);
    }
}
/**
 * @brief 纵向网格
 * @param on
 */
void DAAppController::onActionChartEnableGridYTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableGridY(on);
    }
}
/**
 * @brief 横向密集网格
 * @param on
 */
void DAAppController::onActionChartEnableGridXMinEnableTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableGridXMin(on);
    }
}
/**
 * @brief 纵向密集网格
 * @param on
 */
void DAAppController::onActionChartEnableGridYMinTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableGridYMin(on);
    }
}

/**
 * @brief 当前图表允许缩放
 * @param on
 */
void DAAppController::onActionChartEnableZoomTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableZoomer(on);
        _ribbon->updateChartZoomPanAboutRibbon(w);
    }
}

/**
 * @brief 当前图表放大
 */
void DAAppController::onActionChartZoomInTriggered()
{
    DAChartWidget* w = getCurrentChart();
    if (w && w->isEnableZoomer()) {
        w->zoomIn();
    }
}

/**
 * @brief 当前图表缩小
 */
void DAAppController::onActionChartZoomOutTriggered()
{
    DAChartWidget* w = getCurrentChart();
    if (w && w->isEnableZoomer()) {
        w->zoomOut();
    }
}

/**
 * @brief 当前图表全部显示
 */
void DAAppController::onActionChartZoomAllTriggered()
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->zoomInCompatible();
    }
}

/**
 * @brief 允许绘图拖动
 * @param on
 */
void DAAppController::onActionChartEnablePanTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enablePan(on);
        _ribbon->updateChartZoomPanAboutRibbon(w);
    }
}

/**
 * @brief 允许绘图拾取
 * @param on
 */
void DAAppController::onActionChartEnablePickerCrossTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableCrossPicker(on);
        if (on) {
            w->enableYDataPicker(false);
            w->enableXYDataPicker(false);
        }
    }
}

/**
 * @brief 允许绘图拾取Y
 * @param on
 */
void DAAppController::onActionChartEnablePickerYTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableYDataPicker(on);
        if (on) {
            w->enableCrossPicker(false);
            w->enableXYDataPicker(false);
        }
    }
}

/**
 * @brief 允许绘图拾取XY
 * @param on
 */
void DAAppController::onActionChartEnablePickerXYTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableXYDataPicker(on);
        if (on) {
            w->enableCrossPicker(false);
            w->enableYDataPicker(false);
        }
    }
}

/**
 * @brief 允许图例
 * @param on
 */
void DAAppController::onActionChartEnableLegendTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    w->enableLegend(on);
}

/**
 * @brief 绘图图例对齐的actiongroup
 * @param a
 */
void DAAppController::onActionGroupChartLegendAlignmentTriggered(QAction* a)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        w->enableLegend(true);
        legend = w->getLegend();
        _actions->actionChartEnableLegend->setChecked(true);
    }
    if (!a->isChecked()) {
        //无法支持uncheck
        a->setChecked(true);
    }
    Qt::Alignment al = static_cast< Qt::Alignment >(a->data().toInt());
    legend->setAlignmentInCanvas(al);
}

/**
 * @brief 绘图图例的最大列数发生改变
 * @param v
 */
void DAAppController::onChartLegendMaxColumnsValueChanged(int v)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        return;
    }
    legend->setMaxColumns(v);
}

/**
 * @brief legend的margin发生变化
 * @param v
 */
void DAAppController::onChartLegendMarginValueChanged(int v)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        return;
    }
    legend->setMargin(v);
}

/**
 * @brief legend的Spacing发生变化
 * @param v
 */
void DAAppController::onChartLegendSpacingValueChanged(int v)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        return;
    }
    legend->setSpacing(v);
}

/**
 * @brief legend的item margin发生变化
 * @param v
 */
void DAAppController::onChartLegendItemMarginValueChanged(int v)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        return;
    }
    legend->setItemMargin(v);
}

/**
 * @brief legend的item spacing发生变化
 * @param v
 */
void DAAppController::onChartLegendItemSpacingValueChanged(int v)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        return;
    }
    legend->setItemSpacing(v);
}

/**
 * @brief legend的圆角发生变化
 * @param v
 */
void DAAppController::onChartLegendBorderRadiusValueChanged(double v)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        return;
    }
    legend->setBorderRadius(v);
}

/**
 * @brief dataframe删除行
 */
void DAAppController::onActionRemoveRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectRow();
    }
}

/**
 * @brief dataframe删除列
 */
void DAAppController::onActionRemoveColumnTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectColumn();
    }
}

/**
 * @brief 移除单元格
 */
void DAAppController::onActionRemoveCellTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectCell();
    }
}

/**
 * @brief 插入行
 */
void DAAppController::onActionInsertRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowBelowBySelect();
    }
}

/**
 * @brief 在选中位置上面插入一行
 */
void DAAppController::onActionInsertRowAboveTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowAboveBySelect();
    }
}
/**
 * @brief 在选中位置右边插入一列
 */
void DAAppController::onActionInsertColumnRightTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnRightBySelect();
    }
}
/**
 * @brief 在选中位置左边插入一列
 */
void DAAppController::onActionInsertColumnLeftTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnLeftBySelect();
    }
}

/**
 * @brief dataframe列重命名
 */
void DAAppController::onActionRenameColumnsTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->renameColumns();
    }
}

/**
 * @brief 创建数据描述
 */
void DAAppController::onActionCreateDataDescribeTriggered()
{
    // TODO 此函数应该移动到dataOperateWidget中
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        DAPyDataFrame df = dfopt->createDataDescribe();
        if (df.isNone()) {
            return;
        }
        DAData data = df;
        data.setName(tr("%1_Describe").arg(dfopt->data().getName()));
        data.setDescribe(tr("Generate descriptive statistics that summarize the central tendency, dispersion and "
                            "shape of a [%1]’s distribution, excluding NaN values")
                                 .arg(dfopt->data().getName()));
        _datas->addData(data);
        // showDataOperate要在m_dataManagerStack.push之后，因为m_dataManagerStack.push可能会导致data的名字改变
        _dock->showDataOperateWidget(data);
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
 * @brief 选中列转换为数值
 */
void DAAppController::onActionCastToNumTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->castSelectToNum();
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
    }
}

/**
 * @brief 选中列转换为索引
 */
void DAAppController::onActionChangeToIndexTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->changeSelectColumnToIndex();
    }
}

/**
 * @brief 显示工作流区域
 */
void DAAppController::onActionShowWorkFlowAreaTriggered()
{
    _dock->raiseDockByWidget((QWidget*)(_dock->getWorkFlowOperateWidget()));
}

/**
 * @brief 显示绘图区域
 */
void DAAppController::onActionShowChartAreaTriggered()
{
    _dock->raiseDockByWidget((QWidget*)(_dock->getChartOperateWidget()));
}

/**
 * @brief 显示数据区域
 */
void DAAppController::onActionShowDataAreaTriggered()
{
    _dock->raiseDockByWidget((QWidget*)(_dock->getDataOperateWidget()));
}

/**
 * @brief 显示信息区域
 */
void DAAppController::onActionShowMessageLogViewTriggered()
{
    _dock->raiseDockByWidget((QWidget*)(_dock->getMessageLogViewWidget()));
}

/**
 * @brief 显示设置区域
 */
void DAAppController::onActionSettingWidgetTriggered()
{
    _dock->raiseDockByWidget((QWidget*)(_dock->getSettingContainerWidget()));
}

/**
 * @brief 新建工作流
 */
void DAAppController::onActionNewWorkflowTriggered()
{
    bool ok      = false;
    QString text = QInputDialog::getText(app(),
                                         tr("new workflow name"),   // cn:新工作流名称
                                         tr("new workflow name:"),  // cn:新工作流名称
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (!ok || text.isEmpty()) {
        return;
    }
    DAWorkFlowOperateWidget* wf = _dock->getWorkFlowOperateWidget();
    wf->appendWorkflow(text);
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
        _dock->getWorkFlowOperateWidget()->setMouseActionFlag(DAWorkFlowGraphicsScene::StartAddRect, false);
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
        _dock->getWorkFlowOperateWidget()->setMouseActionFlag(DAWorkFlowGraphicsScene::StartAddText, false);
    }
}
}  // end DA
