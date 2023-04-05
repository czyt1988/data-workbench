#include "DAAppRibbonArea.h"
#include "AppMainWindow.h"
#include "ui_AppMainWindow.h"
// SARibbon
#include "SARibbonMainWindow.h"
#include "SARibbonBar.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonCategory.h"
#include "SARibbonPannel.h"
#include "SARibbonContextCategory.h"
#include "SARibbonQuickAccessBar.h"
#include "SARibbonLineWidgetContainer.h"
#include "SARibbonComboBox.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonMenu.h"
#include "SARibbonCtrlContainer.h"
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
// ui
#include "DAWaitCursorScoped.h"
// Py
#include "DAPyScripts.h"
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"
// api
#include "DAAppCommand.h"
#include "DAAppCore.h"
#include "DAAppDataManager.h"
#include "DADataManagerInterface.h"
#include "DAAppDockingAreaInterface.h"
#include "DAAppDockingArea.h"
#include "DAAppActions.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// command
#include "DACommandsDataManager.h"
// Widget
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

#ifndef DAAPPRIBBONAREA_WINDOW_NAME
#define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppRibbonArea", "DA", nullptr)  //
#endif

//快速链接信号槽
#define DAAPPRIBBONAREA_ACTION_BIND(actionname, functionname)                                                          \
    connect(actionname, &QAction::triggered, this, &DAAppRibbonArea::functionname)

//未实现的功能标记
#define DAAPPRIBBONAREA_PASS()                                                                                         \
    QMessageBox::                                                                                                      \
            warning(app(),                                                                                             \
                    QCoreApplication::translate("DAAppRibbonArea", "warning", nullptr),                                \
                    QCoreApplication::translate("DAAppRibbonArea",                                                     \
                                                "The current function is not implemented, only the UI is reserved, "   \
                                                "please pay attention: https://gitee.com/czyt1988/data-work-flow",     \
                                                nullptr))

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppRibbonArea
//===================================================

DAAppRibbonArea::DAAppRibbonArea(DAAppUIInterface* u) : DAAppRibbonAreaInterface(u), m_dockArea(nullptr)
{
    m_app     = qobject_cast< AppMainWindow* >(u->mainWindow());
    m_datas   = qobject_cast< DAAppDataManager* >(u->core()->getDataManagerInterface());
    m_actions = qobject_cast< DAAppActions* >(u->getActionInterface());
    m_appCmd  = qobject_cast< DAAppCommand* >(u->getCommandInterface());
    //设置当前的焦点窗口
    m_lastFocusedOpertateWidget = LastFocusedNoneOptWidget;
    buildMenu();
    buildRibbon();
    buildRedoUndo();
    initConnection();
    initScripts();
    resetText();
}

DAAppRibbonArea::~DAAppRibbonArea()
{
}

/**
 * @brief 构建所有的action
 */
void DAAppRibbonArea::buildMenu()
{
    m_menuInsertRow = new SARibbonMenu(m_app);
    m_menuInsertRow->setObjectName("menuInsertRow");
    m_menuInsertRow->addAction(m_actions->actionInsertRowAbove);
    m_menuInsertColumn = new SARibbonMenu(m_app);
    m_menuInsertColumn->setObjectName("menuInsertColumn");
    m_menuInsertColumn->addAction(m_actions->actionInsertColumnLeft);
    m_menuChartLegendProperty                 = new SARibbonMenu(m_app);
    m_actionOfMenuChartLegendAlignmentSection = m_menuChartLegendProperty->addSection(tr("Location"));
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInTopLeft);
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInTop);
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInTopRight);
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInRight);
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInBottomRight);
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInBottom);
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInBottomLeft);
    m_menuChartLegendProperty->addAction(m_actions->actionChartLegendAlignmentInLeft);
    //
    m_menuChartLegendProperty->addSection(tr("Property"));
    //构建最大列数的窗口
    auto addControlWidgetInMenu = [](QMenu* m, QWidget* w, const QString& title) {
        SARibbonCtrlContainer* container = new SARibbonCtrlContainer(w);
        container->setEnableShowTitle(true);
        w->setWindowTitle(title);
        QWidgetAction* act = new QWidgetAction(m);
        act->setDefaultWidget(container);
        m->addAction(act);
    };
    m_spinboxChartLegendMaxColumns = new QSpinBox(m_menuChartLegendProperty);
    addControlWidgetInMenu(m_menuChartLegendProperty, m_spinboxChartLegendMaxColumns, tr("Max Columns"));
}

void DAAppRibbonArea::retranslateUi()
{
    resetText();
}

void DAAppRibbonArea::resetText()
{
    ribbonBar()->applicationButton()->setText(tr("File"));  //文件

    m_categoryMain->setCategoryName(tr("Main"));               //主页
    m_pannelMainFileOpt->setPannelName(tr("File Operation"));  //文件操作
    m_pannelSetting->setPannelName(tr("Config"));              //配置
    m_pannelMainWorkflowOpt->setPannelName(tr("Workflow"));    //工作流
    m_pannelMainDataOpt->setPannelName(tr("Data Operation"));  //数据操作
    m_categoryData->setCategoryName(tr("Data"));               //数据
    m_pannelDataOperate->setPannelName(tr("Data Operation"));  //数据操作

    m_categoryView->setCategoryName(tr("View"));         //视图
    m_pannelViewMainView->setPannelName(tr("Display"));  //视图显示

    m_contextDataFrame->setContextTitle(tr("DataFrame"));               ///< DataFrame
    m_categoryDataframeOperate->setCategoryName(tr("Operate"));         ///< DataFrame -> Operate
    m_pannelDataframeOperateAxes->setPannelName(tr("Axes"));            ///< DataFrame -> Operate -> Axes
    m_pannelDataframeOperateDType->setPannelName(tr("Type"));           ///< DataFrame -> Type
    m_comboxColumnTypesContainer->setPrefix(tr("Type"));                ///< DataFrame -> Type -> Type
    m_pannelDataframeOperateStatistic->setPannelName(tr("Statistic"));  ///< DataFrame -> Statistic

    //编辑标签
    m_categoryEdit->setCategoryName(tr("Edit"));  //编辑

    m_contextWorkflow->setContextTitle(tr("Workflow"));  //工作流

    m_categoryWorkflowGraphicsEdit->setCategoryName(tr("Workflow Edit"));  //工作流编辑
    m_pannelWorkflowItem->setPannelName(tr("Item"));                       //图元
    m_pannelWorkflowText->setPannelName(tr("Text"));                       //文本
    m_pannelWorkflowBackground->setPannelName(tr("Background"));           //背景
    m_pannelWorkflowView->setPannelName(tr("View"));                       //视图
    m_pannelWorkflowRun->setPannelName(tr("Run"));                         //运行
    //绘图标签
    m_contextChart->setContextTitle(tr("Chart"));                        // cn:绘图
    m_categoryChartEdit->setCategoryName(tr("Chart Edit"));              // cn:绘图编辑
    m_actionOfMenuChartLegendAlignmentSection->setText(tr("Location"));  // cn:方位
    m_menuChartLegendProperty->setTitle(tr("Legend"));
    m_spinboxChartLegendMaxColumns->setWindowTitle(tr("Max Columns"));  // cn:最大列数
    //
    m_app->setWindowTitle(tr("Data Work Flow"));
}

/**
 * @brief 基本绑定
 * @note 在setDockAreaInterface函数中还有很多绑定操作
 */
void DAAppRibbonArea::initConnection()
{
    // Main Category
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionOpen, onActionOpenTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionSave, onActionSaveTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionSaveAs, onActionSaveAsTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionAppendProject, onActionAppendProjectTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionSetting, onActionSettingTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionPluginManager, onActionPluginManagerTriggered);
    // Data Category
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionAddData, onActionAddDataTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionRemoveData, onActionRemoveDataTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionAddDataFolder, onActionAddDataFolderTriggered);
    // Chart Category
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionAddFigure, onActionAddFigureTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionFigureResizeChart, onActionFigureResizeChartTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionFigureNewXYAxis, onActionFigureNewXYAxisTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnableGrid, onActionChartEnableGridTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnableGridX, onActionChartEnableGridXTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnableGridY, onActionChartEnableGridYTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnableGridXMin, onActionChartEnableGridXMinEnableTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnableGridYMin, onActionChartEnableGridYMinTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnableZoom, onActionChartEnableZoomTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartZoomIn, onActionChartZoomInTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartZoomOut, onActionChartZoomOutTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartZoomAll, onActionChartZoomAllTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnablePan, onActionChartEnablePanTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnablePickerCross, onActionChartEnablePickerCrossTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnablePickerY, onActionChartEnablePickerYTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnablePickerXY, onActionChartEnablePickerXYTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChartEnableLegend, onActionChartEnableLegendTriggered);
    connect(m_actions->actionGroupChartLegendAlignment, &QActionGroup::triggered, this, &DAAppRibbonArea::onActionGroupChartLegendAlignmentTriggered);
    // 数据操作的上下文标签 Data Operate Context Category
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionInsertRow, onActionInsertRowTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionInsertRowAbove, onActionInsertRowAboveTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionInsertColumnRight, onActionInsertColumnRightTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionInsertColumnLeft, onActionInsertColumnLeftTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionRemoveRow, onActionRemoveRowTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionRemoveColumn, onActionRemoveColumnTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionRemoveCell, onActionRemoveCellTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionRenameColumns, onActionRenameColumnsTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionCreateDataDescribe, onActionCreateDataDescribeTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionCastToNum, onActionCastToNumTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionCastToString, onActionCastToStringTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionCastToDatetime, onActionCastToDatetimeTriggered);
    //不知为何使用函数指针无法关联信号和槽
    // connect(m_comboxColumnTypes, &DAPyDTypeComboBox::currentDTypeChanged, this,&DAAppRibbonArea::onComboxColumnTypesCurrentDTypeChanged);
    // QObject::connect: signal not found in DAPyDTypeComboBox
    connect(m_comboxColumnTypes, SIGNAL(currentDTypeChanged(DAPyDType)), this, SLOT(onComboxColumnTypesCurrentDTypeChanged(DAPyDType)));
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionChangeToIndex, onActionChangeToIndexTriggered);
    // View Category
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionShowWorkFlowArea, onActionShowWorkFlowAreaTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionShowChartArea, onActionShowChartAreaTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionShowDataArea, onActionShowDataAreaTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionShowMessageLogView, onActionShowMessageLogViewTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionShowSettingWidget, onActionSettingWidgetTriggered);

    // workflow edit 工作流编辑
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionWorkflowNew, onActionNewWorkflowTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionWorkflowRun, onActionRunCurrentWorkflowTriggered);
    // workflow edit 工作流编辑/data edit 绘图编辑
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionWorkflowStartDrawRect, onActionStartDrawRectTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionWorkflowStartDrawText, onActionStartDrawTextTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionWorkflowAddBackgroundPixmap, onActionAddBackgroundPixmapTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionWorkflowLockBackgroundPixmap, onActionLockBackgroundPixmapTriggered);
    DAAPPRIBBONAREA_ACTION_BIND(m_actions->actionWorkflowEnableItemMoveWithBackground, onActionEnableItemMoveWithBackgroundTriggered);

    //===================================================
    // setDockAreaInterface 有其他的绑定
    //===================================================
    //! 注意！！
    //! 在setDockAreaInterface函数中还有很多绑定操作
    //
    DAProject* p = DA_APP_CORE.getProject();
    if (p) {
        connect(p, &DAProject::projectSaved, this, &DAAppRibbonArea::onProjectSaved);
        connect(p, &DAProject::projectLoaded, this, &DAAppRibbonArea::onProjectLoaded);
    }

    //===================================================
    // name
    //===================================================

    connect(m_workflowFontEditPannel, &DA::DAFontEditPannelWidget::currentFontChanged, this, &DAAppRibbonArea::onCurrentWorkflowFontChanged);
    connect(m_workflowFontEditPannel, &DA::DAFontEditPannelWidget::currentFontColorChanged, this, &DAAppRibbonArea::onCurrentWorkflowFontColorChanged);
    connect(m_workflowShapeEditPannelWidget, &DAShapeEditPannelWidget::backgroundBrushChanged, this, &DAAppRibbonArea::onCurrentWorkflowShapeBackgroundBrushChanged);
    connect(m_workflowShapeEditPannelWidget, &DAShapeEditPannelWidget::borderPenChanged, this, &DAAppRibbonArea::onCurrentWorkflowShapeBorderPenChanged);
}

/**
 * @brief 由于无法判断DAAppRibbonArea和DAAppDockingArea的创建顺序，因此DAAppDockingArea的指针手动设置进去
 * @param d
 */
void DAAppRibbonArea::setDockAreaInterface(DAAppDockingArea* d)
{
    m_dockArea = d;
    connect(d->dockManager(), &ads::CDockManager::focusedDockWidgetChanged, this, &DAAppRibbonArea::onFocusedDockWidgetChanged);
    // DADataManageWidget 数据操作
    DADataManageWidget* dmw = getDataManageWidget();
    connect(dmw, &DADataManageWidget::dataViewModeChanged, this, &DAAppRibbonArea::onDataManageWidgetDataViewModeChanged);
    // DADataOperateWidget
    DADataOperateWidget* dow = m_dockArea->getDataOperateWidget();
    connect(dow, &DADataOperateWidget::pageAdded, this, &DAAppRibbonArea::onDataOperatePageAdded);
    // DAChartOperateWidget
    DAChartOperateWidget* cow = m_dockArea->getChartOperateWidget();
    connect(cow, &DAChartOperateWidget::figureCreated, this, &DAAppRibbonArea::onFigureCreated);
    connect(cow, &DAChartOperateWidget::currentFigureChanged, this, &DAAppRibbonArea::onCurrentFigureChanged);
    connect(cow, &DAChartOperateWidget::chartAdded, this, &DAAppRibbonArea::onChartAdded);
    connect(cow, &DAChartOperateWidget::currentChartChanged, this, &DAAppRibbonArea::onCurrentChartChanged);
    //鼠标动作完成的触发
    connect(m_dockArea->getWorkFlowOperateWidget(),
            &DAWorkFlowOperateWidget::mouseActionFinished,
            this,
            &DAAppRibbonArea::onWorkFlowGraphicsSceneMouseActionFinished);
    //
    DAWorkFlowOperateWidget* workflowOpt = m_dockArea->getWorkFlowOperateWidget();
    connect(workflowOpt, &DAWorkFlowOperateWidget::selectionItemChanged, this, &DAAppRibbonArea::onSelectionItemChanged);
    connect(m_actions->actionWorkflowShowGrid, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowShowGrid);
    connect(m_actions->actionWorkflowNew, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::appendWorkflowWithDialog);
    connect(m_actions->actionWorkflowWholeView, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowWholeView);
    connect(m_actions->actionWorkflowZoomIn, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomIn);
    connect(m_actions->actionWorkflowZoomOut, &QAction::triggered, workflowOpt, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomOut);
}

/**
 * @brief 构建ribbon
 */
void DAAppRibbonArea::buildRibbon()
{
    buildRibbonMainCategory();
    buildRibbonDataCategory();
    buildRibbonViewCategory();
    buildRibbonEditCategory();
    buildRibbonQuickAccessBar();
    //上下文标签
    buildContextCategoryDataFrame();
    buildContextCategoryWorkflowEdit();
    buildContextCategoryChart();
}

/**
 * @brief 构建主页标签
 * 主页的category objname = da-ribbon-category-main
 */
void DAAppRibbonArea::buildRibbonMainCategory()
{
    m_categoryMain = new SARibbonCategory(app());
    m_categoryMain->setObjectName(QStringLiteral("da-ribbon-category-main"));

    //--------Common--------------------------------------------------

    m_pannelMainFileOpt = new SARibbonPannel(m_categoryMain);
    m_pannelMainFileOpt->setObjectName(QStringLiteral("da-ribbon-pannel-main.common"));
    m_pannelMainFileOpt->addLargeAction(m_actions->actionOpen);
    m_pannelMainFileOpt->addSmallAction(m_actions->actionSave);
    m_pannelMainFileOpt->addSmallAction(m_actions->actionSaveAs);
    m_pannelMainFileOpt->addSeparator();
    m_pannelMainFileOpt->addSmallAction(m_actions->actionAppendProject);
    m_categoryMain->addPannel(m_pannelMainFileOpt);

    //--------Data Opt--------------------------------------------------
    //这里演示通过addPannel的重载函数来创建pannel
    m_pannelMainDataOpt = m_categoryMain->addPannel("Data Opt");
    ;
    m_pannelMainDataOpt->setObjectName(QStringLiteral("da-pannel-main.data-opt"));
    m_pannelMainDataOpt->addLargeAction(m_actions->actionAddData);

    // Chart Opt
    m_pannelMainChartOpt = new SARibbonPannel(m_categoryMain);
    m_pannelMainChartOpt->setObjectName(QStringLiteral("da-pannel-main.chart-opt"));
    m_pannelMainChartOpt->addLargeAction(m_actions->actionAddFigure);
    m_categoryMain->addPannel(m_pannelMainChartOpt);
    //--------Workflow Opt----------------------------------------------
    m_pannelMainWorkflowOpt = m_categoryMain->addPannel(tr("Workflow"));
    m_pannelMainWorkflowOpt->setObjectName(QStringLiteral("da-pannel-main.workflow"));
    m_pannelMainWorkflowOpt->addLargeAction(m_actions->actionWorkflowAddBackgroundPixmap);
    m_pannelMainWorkflowOpt->addMediumAction(m_actions->actionWorkflowStartDrawRect);
    m_pannelMainWorkflowOpt->addMediumAction(m_actions->actionWorkflowStartDrawText);
    m_pannelMainWorkflowOpt->addLargeAction(m_actions->actionWorkflowRun);

    //--------Setting--------------------------------------------------

    m_pannelSetting = new SARibbonPannel(m_categoryMain);
    m_pannelSetting->setObjectName(QStringLiteral("da-pannel-main.setting"));
    m_pannelSetting->addLargeAction(m_actions->actionSetting);
    m_pannelSetting->addLargeAction(m_actions->actionPluginManager);
    m_categoryMain->addPannel(m_pannelSetting);
    //----------------------------------------------------------

    ribbonBar()->addCategoryPage(m_categoryMain);  //主页
}

/**
 * @brief 构建数据标签
 * objectname=da-ribbon-category-data
 */
void DAAppRibbonArea::buildRibbonDataCategory()
{
    m_categoryData = new SARibbonCategory(app());
    m_categoryData->setObjectName(QStringLiteral("da-ribbon-category-data"));

    //--------DataOperate--------------------------------------------------

    m_pannelDataOperate = new SARibbonPannel(m_categoryData);
    m_pannelDataOperate->setObjectName(QStringLiteral("da-pannel-data.data-opt"));
    m_pannelDataOperate->addLargeAction(m_actions->actionAddData);
    m_pannelDataOperate->addLargeAction(m_actions->actionRemoveData);
    m_categoryData->addPannel(m_pannelDataOperate);

    //--------FolderOperate--------------------------------------------------

    m_pannelDataFolderOperate = new SARibbonPannel(m_categoryData);
    m_pannelDataFolderOperate->setObjectName(QStringLiteral("da-pannel-data.folder-opt"));
    m_pannelDataFolderOperate->addLargeAction(m_actions->actionAddDataFolder);
    m_actions->actionAddDataFolder->setDisabled(true);  //默认不可点
    m_categoryData->addPannel(m_pannelDataFolderOperate);

    //----------------------------------------------------------

    ribbonBar()->addCategoryPage(m_categoryData);
}

/**
 * @brief 构建视图标签
 * objectname=da-ribbon-category-view
 */
void DAAppRibbonArea::buildRibbonViewCategory()
{
    m_categoryView = new SARibbonCategory(app());
    m_categoryView->setObjectName(QStringLiteral("da-ribbon-category-view"));

    //--------MainView--------------------------------------------------

    m_pannelViewMainView = new SARibbonPannel(m_categoryView);
    m_pannelViewMainView->setObjectName(QStringLiteral("da-pannel-view.main"));
    m_pannelViewMainView->addLargeAction(m_actions->actionShowWorkFlowArea);
    m_pannelViewMainView->addLargeAction(m_actions->actionShowChartArea);
    m_pannelViewMainView->addLargeAction(m_actions->actionShowDataArea);
    m_pannelViewMainView->addSeparator();
    m_pannelViewMainView->addSmallAction(m_actions->actionShowMessageLogView);
    m_pannelViewMainView->addSmallAction(m_actions->actionShowSettingWidget);
    m_categoryView->addPannel(m_pannelViewMainView);

    //----------------------------------------------------------

    ribbonBar()->addCategoryPage(m_categoryView);  //视图
}

/**
 * @brief 构建ribbon的QuickAccessBar
 */
void DAAppRibbonArea::buildRibbonQuickAccessBar()
{
}

/**
 * @brief 构建DataFrame上下文标签
 * objectname=da-ribbon-contextcategory-dataframe
 */
void DAAppRibbonArea::buildContextCategoryDataFrame()
{
    m_contextDataFrame = ribbonBar()->addContextCategory(tr("DataFrame"));
    m_contextDataFrame->setObjectName(QStringLiteral("da-ribbon-contextcategory-dataframe"));
    m_categoryDataframeOperate = m_contextDataFrame->addCategoryPage(tr("Operate"));
    // Axes pannel
    m_pannelDataframeOperateAxes = m_categoryDataframeOperate->addPannel(tr("Axes"));
    m_pannelDataframeOperateAxes->addLargeActionMenu(m_actions->actionInsertRow, m_menuInsertRow);
    m_pannelDataframeOperateAxes->addLargeActionMenu(m_actions->actionInsertColumnRight, m_menuInsertColumn);
    m_pannelDataframeOperateAxes->addLargeAction(m_actions->actionRemoveCell);
    m_pannelDataframeOperateAxes->addMediumAction(m_actions->actionRemoveRow);
    m_pannelDataframeOperateAxes->addMediumAction(m_actions->actionRemoveColumn);
    m_pannelDataframeOperateAxes->addSeparator();
    m_pannelDataframeOperateAxes->addLargeAction(m_actions->actionRenameColumns);
    m_pannelDataframeOperateAxes->addLargeAction(m_actions->actionChangeToIndex);
    // Type pannel
    m_pannelDataframeOperateDType = m_categoryDataframeOperate->addPannel(tr("Type"));
    m_comboxColumnTypesContainer  = new SARibbonLineWidgetContainer(m_pannelDataframeOperateDType);
    m_comboxColumnTypes           = new DAPyDTypeComboBox(m_comboxColumnTypesContainer);
    m_comboxColumnTypes->setMinimumWidth(m_app->fontMetrics().width("timedelta64(scoll)"));  //设置最小宽度
    m_comboxColumnTypesContainer->setPrefix(tr("Type"));
    m_comboxColumnTypesContainer->setWidget(m_comboxColumnTypes);
    m_pannelDataframeOperateDType->addWidget(m_comboxColumnTypesContainer, SARibbonPannelItem::Medium);
    m_castActionsButtonGroup = new SARibbonButtonGroupWidget();
    m_castActionsButtonGroup->addAction(m_actions->actionCastToNum);
    m_castActionsButtonGroup->addAction(m_actions->actionCastToString);
    m_castActionsButtonGroup->addSeparator();
    m_castActionsButtonGroup->addAction(m_actions->actionCastToDatetime);
    m_pannelDataframeOperateDType->addWidget(m_castActionsButtonGroup, SARibbonPannelItem::Medium);
    // Statistic Pannel
    m_pannelDataframeOperateStatistic = m_categoryDataframeOperate->addPannel(tr("Statistic"));
    m_pannelDataframeOperateStatistic->addLargeAction(m_actions->actionCreateDataDescribe);
}

/**
 * @brief 构建Edit标签
 * da-ribbon-category-edit
 * pannel:da-ribbon-pannel-edit-workflow
 */
void DAAppRibbonArea::buildRibbonEditCategory()
{
    m_categoryEdit = new SARibbonCategory(app());
    m_categoryEdit->setObjectName(QStringLiteral("da-ribbon-category-edit"));

    //--------MainView--------------------------------------------------

    m_pannelEditWorkflow = new SARibbonPannel(m_categoryEdit);
    m_pannelEditWorkflow->setObjectName(QStringLiteral("da-pannel-edit.workflow"));
    m_pannelEditWorkflow->addLargeAction(m_actions->actionWorkflowNew);
    m_pannelEditWorkflow->addSeparator();
    m_pannelEditWorkflow->addLargeAction(m_actions->actionWorkflowStartDrawRect);
    m_pannelEditWorkflow->addLargeAction(m_actions->actionWorkflowStartDrawText);
    m_categoryEdit->addPannel(m_pannelEditWorkflow);
    //----------------------------------------------------------

    ribbonBar()->addCategoryPage(m_categoryEdit);  //编辑
}

void DAAppRibbonArea::buildContextCategoryWorkflowEdit()
{
    m_contextWorkflow = ribbonBar()->addContextCategory(tr("Workflow"));
    m_contextWorkflow->setObjectName(QStringLiteral("da-ribbon-contextcategory-workflow"));
    m_categoryWorkflowGraphicsEdit = m_contextWorkflow->addCategoryPage(tr("Workflow Edit"));
    m_categoryWorkflowGraphicsEdit->setObjectName(QStringLiteral("da-ribbon-category-workflow.edit"));
    //条目pannel
    // Item
    m_pannelWorkflowItem = m_categoryWorkflowGraphicsEdit->addPannel(tr("Item"));
    m_pannelWorkflowItem->setObjectName(QStringLiteral("da-pannel-context.workflow.item"));
    m_pannelWorkflowItem->addLargeAction(m_actions->actionWorkflowNew);
    m_pannelWorkflowItem->addSeparator();
    m_pannelWorkflowItem->addLargeAction(m_actions->actionWorkflowStartDrawRect);
    m_workflowShapeEditPannelWidget = new DAShapeEditPannelWidget(m_pannelWorkflowItem);
    m_pannelWorkflowItem->addWidget(m_workflowShapeEditPannelWidget, SARibbonPannelItem::Large);
    // Text
    m_pannelWorkflowText = m_categoryWorkflowGraphicsEdit->addPannel(tr("Text"));
    m_pannelWorkflowText->setObjectName(QStringLiteral("da-pannel-context.workflow.text"));
    m_pannelWorkflowText->addLargeAction(m_actions->actionWorkflowStartDrawText);
    m_workflowFontEditPannel = new DAFontEditPannelWidget(m_pannelWorkflowText);
    m_pannelWorkflowText->addWidget(m_workflowFontEditPannel, SARibbonPannelItem::Large);
    // Background
    m_pannelWorkflowBackground = m_categoryWorkflowGraphicsEdit->addPannel(tr("Background"));
    m_pannelWorkflowBackground->setObjectName(QStringLiteral("da-pannel-context.workflow.background"));
    m_pannelWorkflowBackground->addLargeAction(m_actions->actionWorkflowAddBackgroundPixmap);
    m_pannelWorkflowBackground->addMediumAction(m_actions->actionWorkflowLockBackgroundPixmap);
    m_pannelWorkflowBackground->addMediumAction(m_actions->actionWorkflowEnableItemMoveWithBackground);

    // View
    m_pannelWorkflowView = m_categoryWorkflowGraphicsEdit->addPannel(tr("View"));
    m_pannelWorkflowView->setObjectName(QStringLiteral("da-pannel-context.workflow.view"));
    m_pannelWorkflowView->addLargeAction(m_actions->actionWorkflowShowGrid);
    m_pannelWorkflowView->addSeparator();
    m_pannelWorkflowView->addLargeAction(m_actions->actionWorkflowWholeView);
    m_pannelWorkflowView->addMediumAction(m_actions->actionWorkflowZoomIn);
    m_pannelWorkflowView->addMediumAction(m_actions->actionWorkflowZoomOut);
    // Run
    m_pannelWorkflowRun = m_categoryWorkflowGraphicsEdit->addPannel(tr("Run"));
    m_pannelWorkflowRun->setObjectName(QStringLiteral("da-pannel-context.workflow.run"));
    m_pannelWorkflowRun->addLargeAction(m_actions->actionWorkflowRun);
    ribbonBar()->showContextCategory(m_contextWorkflow);
}

/**
 * @brief 构建chart上下文
 */
void DAAppRibbonArea::buildContextCategoryChart()
{
    m_contextChart = ribbonBar()->addContextCategory(tr("Chart Edit"));  // cn:绘图编辑
    m_contextChart->setObjectName(QStringLiteral("da-ribbon-contextcategory-chart"));
    m_categoryChartEdit = m_contextChart->addCategoryPage(tr("Chart Edit"));
    m_categoryChartEdit->setObjectName(QStringLiteral("da-ribbon-category-chart.edit"));
    // fig edit
    m_pannelFigureSetting = new SARibbonPannel(m_categoryChartEdit);
    m_pannelFigureSetting->setObjectName(QStringLiteral("da-pannel-context-chartedit.fig_setting"));
    m_pannelFigureSetting->addLargeAction(m_actions->actionAddFigure);
    m_pannelFigureSetting->addLargeAction(m_actions->actionFigureResizeChart);
    m_pannelFigureSetting->addLargeAction(m_actions->actionFigureNewXYAxis);  //新建坐标系
    m_categoryChartEdit->addPannel(m_pannelFigureSetting);
    // chart edit
    m_pannelChartSetting = new SARibbonPannel(m_categoryChartEdit);
    m_pannelChartSetting->setObjectName(QStringLiteral("da-pannel-context-chartedit.chart_setting"));
    // grid
    m_pannelChartSetting->addLargeAction(m_actions->actionChartEnableGrid);
    m_chartGridDirActionsButtonGroup = new SARibbonButtonGroupWidget(m_pannelChartSetting);
    m_chartGridDirActionsButtonGroup->addAction(m_actions->actionChartEnableGridX);
    m_chartGridDirActionsButtonGroup->addAction(m_actions->actionChartEnableGridY);
    m_pannelChartSetting->addSmallWidget(m_chartGridDirActionsButtonGroup);
    m_chartGridMinActionsButtonGroup = new SARibbonButtonGroupWidget(m_pannelChartSetting);
    m_chartGridMinActionsButtonGroup->addAction(m_actions->actionChartEnableGridXMin);
    m_chartGridMinActionsButtonGroup->addAction(m_actions->actionChartEnableGridYMin);
    m_pannelChartSetting->addSmallWidget(m_chartGridMinActionsButtonGroup);
    // pan
    m_pannelChartSetting->addLargeAction(m_actions->actionChartEnablePan);
    //缩放
    m_pannelChartSetting->addLargeAction(m_actions->actionChartEnableZoom);
    m_pannelChartSetting->addMediumAction(m_actions->actionChartZoomIn);
    m_pannelChartSetting->addMediumAction(m_actions->actionChartZoomOut);
    m_pannelChartSetting->addLargeAction(m_actions->actionChartZoomAll);
    // picker
    m_pannelChartSetting->addLargeAction(m_actions->actionChartEnablePickerCross);
    m_pannelChartSetting->addLargeAction(m_actions->actionChartEnablePickerXY);
    m_pannelChartSetting->addLargeAction(m_actions->actionChartEnablePickerY);
    // legend
    m_pannelChartSetting->addLargeActionMenu(m_actions->actionChartEnableLegend, m_menuChartLegendProperty);

    m_categoryChartEdit->addPannel(m_pannelChartSetting);
}

/**
 * @brief 脚本定义的内容初始化
 */
void DAAppRibbonArea::initScripts()
{
    DAAppCore* c = qobject_cast< DAAppCore* >(core());
    if (!c->isPythonInterpreterInitialized()) {
        return;
    }
    m_fileReadFilters = QStringList(DAPyScripts::getInstance().getIO().getFileReadFilters());
    qDebug() << m_fileReadFilters;
}

/**
 * @brief 判断当前是否是在绘图操作模式，就算绘图操作不在焦点，但绘图操作在前端，此函数也返回true
 * @return
 */
bool DAAppRibbonArea::isLastFocusedOnChartOptWidget() const
{
    return m_lastFocusedOpertateWidget.testFlag(LastFocusedOnChartOpt);
}

/**
 * @brief 判断当前是否是在工作流操作模式，就算工作流操作不在焦点，但工作流操作在前端，此函数也返回true
 * @return
 */
bool DAAppRibbonArea::isLastFocusedOnWorkflowOptWidget() const
{
    return m_lastFocusedOpertateWidget.testFlag(LastFocusedOnWorkflowOpt);
}

/**
 * @brief 判断当前是否是在数据操作模式，就算数据操作不在焦点，但工作流操作在前端，此函数也返回true
 * @return
 */
bool DAAppRibbonArea::isLastFocusedOnDataOptWidget() const
{
    return m_lastFocusedOpertateWidget.testFlag(LastFocusedOnDataOpt);
}

/**
 * @brief DADataManageWidget查看数据的模式改变
 * @param v
 */
void DAAppRibbonArea::onDataManageWidgetDataViewModeChanged(DADataManageWidget::DataViewMode v)
{
    m_actions->actionAddDataFolder->setEnabled(v == DADataManageWidget::ViewDataInTree);
}

/**
 * @brief GraphicsScene的鼠标动作执行完成，把action的选中标记清除
 * @param mf
 */
void DAAppRibbonArea::onWorkFlowGraphicsSceneMouseActionFinished(DAWorkFlowGraphicsScene::MouseActionFlag mf)
{
    switch (mf) {
    case DAWorkFlowGraphicsScene::StartAddRect:
        m_actions->actionWorkflowStartDrawRect->setChecked(false);
        break;
    case DAWorkFlowGraphicsScene::StartAddText:
        m_actions->actionWorkflowStartDrawText->setChecked(false);
        break;
    default:
        break;
    }
}

AppMainWindow* DAAppRibbonArea::app() const
{
    return (m_app);
}

SARibbonBar* DAAppRibbonArea::ribbonBar() const
{
    return (app()->ribbonBar());
}

/**
 * @brief mian标签
 * @return
 */
SARibbonCategory* DAAppRibbonArea::getRibbonCategoryMain() const
{
    return (m_categoryMain);
}

QStringList DAAppRibbonArea::getFileReadFilters() const
{
    return m_fileReadFilters;
}

/**
 * @brief 通过DACommandInterface构建redo/undo的action
 * @param cmd
 */
void DAAppRibbonArea::buildRedoUndo()
{
    QUndoGroup& undoGroup = m_appCmd->undoGroup();
    //设置redo,undo的action
    m_actions->actionRedo = undoGroup.createRedoAction(this);
    m_actions->actionRedo->setObjectName("actionRedo");
    m_actions->actionRedo->setIcon(QIcon(":/Icon/Icon/redo.svg"));
    m_actions->actionUndo = undoGroup.createUndoAction(this);
    m_actions->actionUndo->setObjectName("actionUndo");
    m_actions->actionUndo->setIcon(QIcon(":/Icon/Icon/undo.svg"));
    SARibbonQuickAccessBar* bar = ribbonBar()->quickAccessBar();
    if (!bar) {
        return;
    }
    bar->addAction(m_actions->actionUndo);
    bar->addAction(m_actions->actionRedo);
}

void DAAppRibbonArea::updateActionLockBackgroundPixmapCheckStatue(bool c)
{
    //    QSignalBlocker l(m_actionLockBackgroundPixmap);
    //    m_actionLockBackgroundPixmap->setChecked(c);
}

/**
 * @brief 插件管理 [config category] - [Plugin Manager]
 * @param on
 */
void DAAppRibbonArea::onActionPluginManagerTriggered(bool on)
{
    Q_UNUSED(on);
    DAPluginManagerDialog dlg(ui()->mainWindow());

    dlg.exec();
}

/**
 * @brief 设定界面
 */
void DAAppRibbonArea::onActionSettingTriggered()
{
    DAAPPRIBBONAREA_PASS();
}

/**
 * @brief 获取当前dataframeOperateWidget,如果没有返回nullptr
 *
 * 此函数不返回nullptr的前提是:DataOperateWidget处于焦点，且是DataFrameOperateWidget
 * @param checkDataOperateAreaFocused 是否检测DataOperateWidget是否处于焦点，默认为true
 * @return
 */
DADataOperateOfDataFrameWidget* DAAppRibbonArea::getCurrentDataFrameOperateWidget(bool checkDataOperateAreaFocused)
{
    if (nullptr == m_dockArea) {
        return nullptr;
    }
    if (checkDataOperateAreaFocused) {
        if (!(m_dockArea->isDockingAreaFocused(DAAppDockingArea::DockingAreaDataOperate))) {
            //窗口未选中就退出
            return nullptr;
        }
    }
    return m_dockArea->getDataOperateWidget()->getCurrentDataFrameWidget();
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAWorkFlowOperateWidget* DAAppRibbonArea::getWorkFlowOperateWidget() const
{
    return m_dockArea->getWorkFlowOperateWidget();
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataOperateWidget* DAAppRibbonArea::getDataOperateWidget() const
{
    return m_dockArea->getDataOperateWidget();
}

/**
 * @brief 获取绘图操作窗口
 * @return
 */
DAChartOperateWidget* DAAppRibbonArea::getChartOperateWidget() const
{
    return m_dockArea->getChartOperateWidget();
}

/**
 * @brief 获取数据管理窗口
 * @return
 */
DADataManageWidget* DAAppRibbonArea::getDataManageWidget() const
{
    return m_dockArea->getDataManageWidget();
}
/**
 * @brief 获取当前的绘图
 * @return 如果没有回返回nullptr
 */
DAFigureWidget* DAAppRibbonArea::getCurrentFigure()
{
    return getChartOperateWidget()->getCurrentFigure();
}

/**
 * @brief 获取当前的图表
 * @return
 */
DAChartWidget* DAAppRibbonArea::getCurrentChart() const
{
    return getChartOperateWidget()->getCurrentChart();
}

/**
 * @brief 更新绘图相关的ribbon
 * @param fig
 */
void DAAppRibbonArea::updateFigureAboutRibbon(DAFigureWidget* fig)
{
    if (nullptr == fig) {
        qDebug() << "updateFigureAboutRibbon(fig:nullptr)";
        return;
    }
    DAChartWidget* chart = fig->getCurrentChart();
    updateChartAboutRibbon(chart);
}

/**
 * @brief 更新图表相关的ribbon
 * @param chart
 */
void DAAppRibbonArea::updateChartAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        qDebug() << "updateChartAboutRibbon(chart:nullptr)";
        return;
    }
    qDebug() << "updateChartAboutRibbon";
    updateChartGridAboutRibbon(chart);
    updateChartZoomPanAboutRibbon(chart);
    updateChartPickerAboutRibbon(chart);
}

/**
 * @brief 更新Ribbon图表网格相关的界面
 * @param chart
 */
void DAAppRibbonArea::updateChartGridAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    m_actions->actionChartEnableGrid->setChecked(chart->isEnableGrid());
    m_actions->actionChartEnableGridX->setChecked(chart->isEnableGridX());
    m_actions->actionChartEnableGridY->setChecked(chart->isEnableGridY());
    m_actions->actionChartEnableGridXMin->setChecked(chart->isEnableGridXMin());
    m_actions->actionChartEnableGridYMin->setChecked(chart->isEnableGridYMin());
    bool c = m_actions->actionChartEnableGrid->isChecked();
    m_actions->actionChartEnableGridX->setEnabled(c);
    m_actions->actionChartEnableGridY->setEnabled(c);
    m_actions->actionChartEnableGridXMin->setEnabled(c);
    m_actions->actionChartEnableGridYMin->setEnabled(c);
}

/**
 * @brief 更新Ribbon图表缩放相关的界面
 * @param chart
 */
void DAAppRibbonArea::updateChartZoomPanAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    m_actions->actionChartEnableZoom->setChecked(chart->isEnableZoomer());
    m_actions->actionChartEnablePan->setChecked(chart->isEnablePanner());
}

/**
 * @brief 更新绘图的picker状态
 * @param chart
 */
void DAAppRibbonArea::updateChartPickerAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    m_actions->actionChartEnablePickerCross->setChecked(chart->isEnableCrossPicker());
    m_actions->actionChartEnablePickerY->setChecked(chart->isEnableYDataPicker());
    m_actions->actionChartEnablePickerXY->setChecked(chart->isEnableXYDataPicker());
}

/**
 * @brief 设置DataFrame的类型，[Context Category - dataframe] [Type] -> Type
 * @param d
 */
void DAAppRibbonArea::setDataframeOperateCurrentDType(const DAPyDType& d)
{
    //先阻塞
    QSignalBlocker blocker(m_comboxColumnTypes);
    Q_UNUSED(blocker);
    m_comboxColumnTypes->setCurrentDType(d);
}

/**
 * @brief DockWidget的焦点变化
 * @param old
 * @param now
 */
void DAAppRibbonArea::onFocusedDockWidgetChanged(ads::CDockWidget* old, ads::CDockWidget* now)
{
    Q_UNUSED(old);
    SARibbonBar* ribbon = ribbonBar();
    if (nullptr == now) {
        ribbon->hideContextCategory(m_contextDataFrame);
        ribbon->hideContextCategory(m_contextWorkflow);
        ribbon->hideContextCategory(m_contextChart);
        return;
    }
    //数据操作窗口激活时，检查是否需要显示m_contextDataFrame
    if (now->widget() == getDataOperateWidget()) {
        //数据窗口激活
        ribbon->showContextCategory(m_contextDataFrame);
        ribbon->hideContextCategory(m_contextWorkflow);
        ribbon->hideContextCategory(m_contextChart);
        m_lastFocusedOpertateWidget = LastFocusedOnDataOpt;
    } else if (now->widget() == getWorkFlowOperateWidget()) {
        //工作流窗口激活
        getWorkFlowOperateWidget()->setUndoStackActive();
        ribbon->showContextCategory(m_contextWorkflow);
        ribbon->hideContextCategory(m_contextDataFrame);
        ribbon->hideContextCategory(m_contextChart);
        m_lastFocusedOpertateWidget = LastFocusedOnWorkflowOpt;
    } else if (now->widget() == getChartOperateWidget()) {
        //绘图窗口激活
        ribbon->hideContextCategory(m_contextDataFrame);
        ribbon->hideContextCategory(m_contextWorkflow);
        ribbon->showContextCategory(m_contextChart);
        m_lastFocusedOpertateWidget = LastFocusedOnChartOpt;
    } else if (now->widget() == getDataManageWidget()) {
        if (m_appCmd) {
            QUndoStack* stack = m_appCmd->getDataManagerStack();
            if (stack && !(stack->isActive())) {  // Data 相关的窗口 undostack激活
                stack->setActive();
            }
        }
    }
}
/**
 * @brief 打开文件
 */
void DAAppRibbonArea::onActionOpenTriggered()
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
    DAProject* project = DA_APP_CORE.getProject();
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
void DAAppRibbonArea::onActionSaveAsTriggered()
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
    DAProject* project = DA_APP_CORE.getProject();
    if (!project->save(projectPath)) {
        qCritical() << tr("Project saved failed!,path is %1").arg(projectPath);  //工程保存失败！路径位于:%1
        return;
    }
    app()->setWindowTitle(QString("%1").arg(project->getProjectBaseName()));
    qInfo() << tr("Project saved successfully,path is %1").arg(projectPath);  //工程保存成功，路径位于:%1
}

void DAAppRibbonArea::onActionAppendProjectTriggered()
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
    DAProject* project = DA_APP_CORE.getProject();
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
void DAAppRibbonArea::onActionSaveTriggered()
{
    DAProject* project      = DA_APP_CORE.getProject();
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
void DAAppRibbonArea::onProjectSaved(const QString& path)
{
    DAProject* project = DA_APP_CORE.getProject();
    m_app->setWindowTitle(QString("%1-%2").arg(DAAPPRIBBONAREA_WINDOW_NAME, project->getProjectBaseName()));
    if (m_dockArea) {
        DAWorkFlowOperateWidget* wf = m_dockArea->getWorkFlowOperateWidget();
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
void DAAppRibbonArea::onProjectLoaded(const QString& path)
{
    DAProject* project = DA_APP_CORE.getProject();
    m_app->setWindowTitle(QString("%1-%2").arg(DAAPPRIBBONAREA_WINDOW_NAME, project->getProjectBaseName()));
    if (m_dockArea) {
        DAWorkFlowOperateWidget* wf = m_dockArea->getWorkFlowOperateWidget();
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
void DAAppRibbonArea::onDataOperatePageAdded(DADataOperatePageWidget* page)
{
    switch (page->getDataOperatePageType()) {
    case DADataOperatePageWidget::DataOperateOfDataFrame: {
        DADataOperateOfDataFrameWidget* w = static_cast< DADataOperateOfDataFrameWidget* >(page);
        connect(w, &DADataOperateOfDataFrameWidget::selectTypeChanged, this, &DAAppRibbonArea::onDataOperateDataFrameWidgetSelectTypeChanged);
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
void DAAppRibbonArea::onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DAPyDType dt)
{
    Q_UNUSED(column);
    setDataframeOperateCurrentDType(dt);
}

/**
 * @brief 添加背景图
 */
void DAAppRibbonArea::onActionAddBackgroundPixmapTriggered()
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
        DAWorkFlowOperateWidget* ow = m_dockArea->getWorkFlowOperateWidget();
        ow->addBackgroundPixmap(f.first());
        m_dockArea->raiseDockingArea(DAAppDockingArea::DockingAreaWorkFlowOperate);
    }
}

void DAAppRibbonArea::onActionLockBackgroundPixmapTriggered(bool on)
{
    m_dockArea->getWorkFlowOperateWidget()->setBackgroundPixmapLock(on);
}

void DAAppRibbonArea::onActionEnableItemMoveWithBackgroundTriggered(bool on)
{
    m_dockArea->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()->enableItemMoveWithBackground(on);
}

void DAAppRibbonArea::onActionRunCurrentWorkflowTriggered()
{
    qDebug() << "onActionRunTriggered";
    //先检查是否有工程
    DAProject* p = DA_APP_CORE.getProject();
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
    m_dockArea->getWorkFlowOperateWidget()->runCurrentWorkFlow();
}

void DAAppRibbonArea::onCurrentWorkflowFontChanged(const QFont& f)
{
    DAWorkFlowOperateWidget* wf = m_dockArea->getWorkFlowOperateWidget();
    wf->setDefaultTextFont(f);
    wf->setSelectTextFont(f);
}

void DAAppRibbonArea::onCurrentWorkflowFontColorChanged(const QColor& c)
{
    DAWorkFlowOperateWidget* wf = m_dockArea->getWorkFlowOperateWidget();
    wf->setDefaultTextColor(c);
    wf->setSelectTextColor(c);
}

void DAAppRibbonArea::onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b)
{
    DAWorkFlowOperateWidget* wf = m_dockArea->getWorkFlowOperateWidget();
    wf->setSelectShapeBackgroundBrush(b);
}

void DAAppRibbonArea::onCurrentWorkflowShapeBorderPenChanged(const QPen& p)
{
    DAWorkFlowOperateWidget* wf = m_dockArea->getWorkFlowOperateWidget();
    wf->setSelectShapeBorderPen(p);
}

void DAAppRibbonArea::onSelectionItemChanged(QGraphicsItem* lastSelectItem)
{
    if (lastSelectItem == nullptr) {
        return;
    }

    if (DAGraphicsItem* daitem = dynamic_cast< DAGraphicsItem* >(lastSelectItem)) {
        //属于DAGraphicsItem系列
        QSignalBlocker b(m_workflowShapeEditPannelWidget);
        m_workflowShapeEditPannelWidget->setBackgroundBrush(daitem->getBackgroundBrush());
        m_workflowShapeEditPannelWidget->setBorderPen(daitem->getBorderPen());
    } else if (DAStandardGraphicsTextItem* titem = dynamic_cast< DAStandardGraphicsTextItem* >(lastSelectItem)) {
        m_workflowFontEditPannel->setCurrentFontColor(titem->defaultTextColor());
        m_workflowFontEditPannel->setCurrentFont(titem->font());
    }
}

/**
 * @brief 新fig创建
 * @param f
 */
void DAAppRibbonArea::onFigureCreated(DAFigureWidget* f)
{
    if (nullptr == f) {
        return;
    }
    qDebug() << "DAAppRibbonArea::onFigureCreate";
    f->getUndoStack()->setActive();
    // updateFigureAboutRibbon(f);//在onActionAddFigureTriggered中调用了
}

/**
 * @brief 当前的fig变化
 * @param f
 * @param index
 */
void DAAppRibbonArea::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
    Q_UNUSED(index);
    if (nullptr == f) {
        return;
    }
    qDebug() << "DAAppRibbonArea::onCurrentFigureChanged";
    f->getUndoStack()->setActive();
    updateFigureAboutRibbon(f);
}

/**
 * @brief 新的chart创建
 * @param c
 */
void DAAppRibbonArea::onChartAdded(DAChartWidget* c)
{
    if (nullptr == c) {
        return;
    }
    // qDebug() << "DAAppRibbonArea::onChartAdded";
    // updateChartAboutRibbon(c);//在onActionFigureNewXYAxisTriggered中调用了
}

/**
 * @brief 当前的chart改变
 * @param c
 */
void DAAppRibbonArea::onCurrentChartChanged(DAChartWidget* c)
{
    if (nullptr == c) {
        return;
    }
    qDebug() << "DAAppRibbonArea::onCurrentChartChanged";
    updateChartAboutRibbon(c);
}

/**
 * @brief 添加数据
 */
void DAAppRibbonArea::onActionAddDataTriggered()
{
    QFileDialog dialog(app());
    dialog.setNameFilters(getFileReadFilters());
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    DA_WAIT_CURSOR_SCOPED();
    int importdataCount = m_datas->importFromFiles(fileNames);
    if (importdataCount > 0) {
        m_dockArea->raiseDockByWidget((QWidget*)(m_dockArea->getDataManageWidget()));
    }
}

/**
 * @brief 移除数据
 */
void DAAppRibbonArea::onActionRemoveDataTriggered()
{
    DADataManageWidget* dmw = m_dockArea->getDataManageWidget();
    dmw->removeSelectData();
}

/**
 * @brief 添加数据文件夹
 */
void DAAppRibbonArea::onActionAddDataFolderTriggered()
{
    DADataManageWidget* dmw = m_dockArea->getDataManageWidget();
    dmw->addDataFolder();
}

/**
 * @brief 添加一个figure
 */
void DAAppRibbonArea::onActionAddFigureTriggered()
{
    DAChartOperateWidget* chartopt = getChartOperateWidget();
    DAFigureWidget* fig            = chartopt->createFigure();
    //把fig的undostack添加
    m_appCmd->addStack(fig->getUndoStack());
    //这里不需要回退
    DAChartWidget* chart = fig->createChart();
    QVector< double > x, y;
    for (int i = 0; i < 10000; ++i) {
        x.append(i);
        y.append(std::sin(double(i / 1000.0)));
    }
    chart->setXLabel("x");
    chart->setYLabel("y");
    chart->addCurve(x, y);
    updateFigureAboutRibbon(fig);
    m_dockArea->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 改变绘图的大小
 * @param on
 */
void DAAppRibbonArea::onActionFigureResizeChartTriggered(bool on)
{
    DAFigureWidget* fig = getCurrentFigure();
    if (nullptr == fig) {
        return;
    }
    m_dockArea->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    fig->enableChartEditor(on);
}

/**
 * @brief 新建一个坐标系
 */
void DAAppRibbonArea::onActionFigureNewXYAxisTriggered()
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
    w->addCurve({ 1e-4, 2e-4, 3e-4, 4e-4, 5e-4 }, { 3e-4, 5e-4, 8e-4, 0, -3e-4 });
    w->addCurve({ 1e-4, 2e-4, 3e-4, 4e-4, 5e-4 }, { 5e-4, 7e-4, 0e-4, -1e-3, 1e-4 });
    updateChartAboutRibbon(w);
    m_dockArea->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 允许网格
 * @param on
 */
void DAAppRibbonArea::onActionChartEnableGridTriggered(bool on)
{
    qDebug() << "onActionChartGridEnableTriggered";
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableGrid(on);
        updateChartGridAboutRibbon(w);
    }
}

/**
 * @brief 横向网格
 * @param on
 */
void DAAppRibbonArea::onActionChartEnableGridXTriggered(bool on)
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
void DAAppRibbonArea::onActionChartEnableGridYTriggered(bool on)
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
void DAAppRibbonArea::onActionChartEnableGridXMinEnableTriggered(bool on)
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
void DAAppRibbonArea::onActionChartEnableGridYMinTriggered(bool on)
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
void DAAppRibbonArea::onActionChartEnableZoomTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enableZoomer(on);
        updateChartZoomPanAboutRibbon(w);
    }
}

/**
 * @brief 当前图表放大
 */
void DAAppRibbonArea::onActionChartZoomInTriggered()
{
    DAChartWidget* w = getCurrentChart();
    if (w && w->isEnableZoomer()) {
        w->zoomIn();
    }
}

/**
 * @brief 当前图表缩小
 */
void DAAppRibbonArea::onActionChartZoomOutTriggered()
{
    DAChartWidget* w = getCurrentChart();
    if (w && w->isEnableZoomer()) {
        w->zoomOut();
    }
}

/**
 * @brief 当前图表全部显示
 */
void DAAppRibbonArea::onActionChartZoomAllTriggered()
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
void DAAppRibbonArea::onActionChartEnablePanTriggered(bool on)
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        w->enablePan(on);
        updateChartZoomPanAboutRibbon(w);
    }
}

/**
 * @brief 允许绘图拾取
 * @param on
 */
void DAAppRibbonArea::onActionChartEnablePickerCrossTriggered(bool on)
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
void DAAppRibbonArea::onActionChartEnablePickerYTriggered(bool on)
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
void DAAppRibbonArea::onActionChartEnablePickerXYTriggered(bool on)
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
void DAAppRibbonArea::onActionChartEnableLegendTriggered(bool on)
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
void DAAppRibbonArea::onActionGroupChartLegendAlignmentTriggered(QAction* a)
{
    DAChartWidget* w = getCurrentChart();
    if (!w) {
        return;
    }
    QwtPlotLegendItem* legend = w->getLegend();
    if (!legend) {
        w->enableLegend(true);
        legend = w->getLegend();
        m_actions->actionChartEnableLegend->setChecked(true);
    }
    if (!a->isChecked()) {
        //无法支持uncheck
        a->setChecked(true);
    }
    Qt::Alignment al = static_cast< Qt::Alignment >(a->data().toInt());
    legend->setAlignmentInCanvas(al);
}

/**
 * @brief dataframe删除行
 */
void DAAppRibbonArea::onActionRemoveRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectRow();
    }
}

/**
 * @brief dataframe删除列
 */
void DAAppRibbonArea::onActionRemoveColumnTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectColumn();
    }
}

/**
 * @brief 移除单元格
 */
void DAAppRibbonArea::onActionRemoveCellTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectCell();
    }
}

/**
 * @brief 插入行
 */
void DAAppRibbonArea::onActionInsertRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowBelowBySelect();
    }
}

/**
 * @brief 在选中位置上面插入一行
 */
void DAAppRibbonArea::onActionInsertRowAboveTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowAboveBySelect();
    }
}
/**
 * @brief 在选中位置右边插入一列
 */
void DAAppRibbonArea::onActionInsertColumnRightTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnRightBySelect();
    }
}
/**
 * @brief 在选中位置左边插入一列
 */
void DAAppRibbonArea::onActionInsertColumnLeftTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnLeftBySelect();
    }
}

/**
 * @brief dataframe列重命名
 */
void DAAppRibbonArea::onActionRenameColumnsTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->renameColumns();
    }
}

/**
 * @brief 创建数据描述
 */
void DAAppRibbonArea::onActionCreateDataDescribeTriggered()
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
        m_datas->addData(data);
        // showDataOperate要在m_dataManagerStack.push之后，因为m_dataManagerStack.push可能会导致data的名字改变
        m_dockArea->showDataOperateWidget(data);
    }
}

/**
 * @brief dataframe的列数据类型改变
 * @param index
 */
void DAAppRibbonArea::onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt)
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->changeSelectColumnType(dt);
    }
}

/**
 * @brief 选中列转换为数值
 */
void DAAppRibbonArea::onActionCastToNumTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->castSelectToNum();
    }
}

/**
 * @brief 选中列转换为文字
 */
void DAAppRibbonArea::onActionCastToStringTriggered()
{
    DAAPPRIBBONAREA_PASS();
}

/**
 * @brief 选中列转换为日期
 */
void DAAppRibbonArea::onActionCastToDatetimeTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->castSelectToDatetime();
    }
}

/**
 * @brief 选中列转换为索引
 */
void DAAppRibbonArea::onActionChangeToIndexTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->changeSelectColumnToIndex();
    }
}

/**
 * @brief 显示工作流区域
 */
void DAAppRibbonArea::onActionShowWorkFlowAreaTriggered()
{
    m_dockArea->raiseDockByWidget((QWidget*)(m_dockArea->getWorkFlowOperateWidget()));
}

/**
 * @brief 显示绘图区域
 */
void DAAppRibbonArea::onActionShowChartAreaTriggered()
{
    m_dockArea->raiseDockByWidget((QWidget*)(m_dockArea->getChartOperateWidget()));
}

/**
 * @brief 显示数据区域
 */
void DAAppRibbonArea::onActionShowDataAreaTriggered()
{
    m_dockArea->raiseDockByWidget((QWidget*)(m_dockArea->getDataOperateWidget()));
}

/**
 * @brief 显示信息区域
 */
void DAAppRibbonArea::onActionShowMessageLogViewTriggered()
{
    m_dockArea->raiseDockByWidget((QWidget*)(m_dockArea->getMessageLogViewWidget()));
}

/**
 * @brief 显示设置区域
 */
void DAAppRibbonArea::onActionSettingWidgetTriggered()
{
    m_dockArea->raiseDockByWidget((QWidget*)(m_dockArea->getSettingContainerWidget()));
}

/**
 * @brief 新建工作流
 */
void DAAppRibbonArea::onActionNewWorkflowTriggered()
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
    DAWorkFlowOperateWidget* wf = m_dockArea->getWorkFlowOperateWidget();
    wf->appendWorkflow(text);
}

/**
 * @brief 绘制矩形
 *
 * * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppRibbonArea::onActionStartDrawRectTriggered(bool on)
{
    if (on) {
        m_dockArea->getWorkFlowOperateWidget()->setMouseActionFlag(DAWorkFlowGraphicsScene::StartAddRect, false);
    }
}

/**
 * @brief 绘制文本
 *
 * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppRibbonArea::onActionStartDrawTextTriggered(bool on)
{
    if (on) {
        m_dockArea->getWorkFlowOperateWidget()->setMouseActionFlag(DAWorkFlowGraphicsScene::StartAddText, false);
    }
}
