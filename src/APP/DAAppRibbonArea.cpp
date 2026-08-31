#include "DAAppRibbonArea.h"
#include "DAUiObjectNames.h"
#include "AppMainWindow.h"
#include <QActionGroup>
// SARibbon
#include "SARibbonMainWindow.h"
#include "SARibbonBar.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonContextCategory.h"
#include "SARibbonQuickAccessBar.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonMenu.h"
#include "SARibbonCtrlContainer.h"
#include "SARibbonApplicationButton.h"
#include "SARibbonLineWidgetContainer.h"
#include "SARibbonGallery.h"
#include "SARibbonColorToolButton.h"
// stl
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
#include "DAAppRibbonApplicationMenu.h"

// Py
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"
// Widget
#include "DAPyDTypeComboBox.h"
#include "DATableDisplayFormatComboBox.h"
#include "DADataOperateOfDataFrameWidget.h"
// Agent 提示词库（AI分析 标签页）
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include "Dialog/DAAgentManagerDialog.h"
#include "DAAgentPrompt.h"
#include "DAAgentPromptOps.h"

// api
#include "DAAppUI.h"
#include "DAAppCommand.h"
#include "DAAppCore.h"
#include "DAAppDockingArea.h"
#include "DAAppActions.h"
#include "DARecentFilesManager.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// command
// Widget
#include "DADataManageWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
// Dialog
// DAWidgets
#include "DAFontEditPannelWidget.h"
#include "DAShapeEditPannelWidget.h"
// Workflow
#include "DAPyWorkFlowOperateWidget.h"
#include "DAPyWorkFlowEditWidget.h"
#include "DAPyWorkFlowGraphicsView.h"
// project

// 快速链接信号槽
#define DAAPPRIBBONAREA_ACTION_BIND(actionname, functionname)                                                          \
    connect(actionname, &QAction::triggered, this, &DAAppRibbonArea::functionname)

#define DAAPPRIBBONAREA_COMMON_SETTING_CPP(MiddleName, ShapeEditPannelWidget, FontEditWidget)                          \
    QPen DAAppRibbonArea::get##MiddleName##Pen() const                                                                 \
    {                                                                                                                  \
        return ShapeEditPannelWidget->getBorderPen();                                                                  \
    }                                                                                                                  \
    QBrush DAAppRibbonArea::get##MiddleName##Brush() const                                                             \
    {                                                                                                                  \
        return ShapeEditPannelWidget->getBackgroundBrush();                                                            \
    }                                                                                                                  \
    QFont DAAppRibbonArea::get##MiddleName##Font() const                                                               \
    {                                                                                                                  \
        return FontEditWidget->getCurrentFont();                                                                       \
    }                                                                                                                  \
    QColor DAAppRibbonArea::get##MiddleName##FontColor() const                                                         \
    {                                                                                                                  \
        return FontEditWidget->getCurrentFontColor();                                                                  \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##Pen(const QPen& v)                                                          \
    {                                                                                                                  \
        QSignalBlocker b(ShapeEditPannelWidget);                                                                       \
        ShapeEditPannelWidget->setBorderPen(v);                                                                        \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##Brush(const QBrush& v)                                                      \
    {                                                                                                                  \
        QSignalBlocker b(ShapeEditPannelWidget);                                                                       \
        ShapeEditPannelWidget->setBackgroundBrush(v);                                                                  \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##Font(const QFont& v)                                                        \
    {                                                                                                                  \
        QSignalBlocker b(FontEditWidget);                                                                              \
        FontEditWidget->setCurrentFont(v);                                                                             \
    }                                                                                                                  \
    void DAAppRibbonArea::set##MiddleName##FontColor(const QColor& v)                                                  \
    {                                                                                                                  \
        QSignalBlocker b(FontEditWidget);                                                                              \
        FontEditWidget->setCurrentFontColor(v);                                                                        \
    }

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppRibbonArea
//===================================================

DAAppRibbonArea::DAAppRibbonArea(DAUIInterface* u) : DARibbonAreaInterface(u)
{
    DAAppUI* appui = qobject_cast< DAAppUI* >(u);
    mApp           = qobject_cast< AppMainWindow* >(appui->mainWindow());
    mActions       = qobject_cast< DAAppActions* >(u->getActionInterface());
    mAppCmd        = qobject_cast< DAAppCommand* >(u->getCommandInterface());
    // ribbon的构建在setDockingArea进行，为了保证ribbon在dock之后构建
}

DAAppRibbonArea::~DAAppRibbonArea()
{
}

/**
 * @brief 构建所有的action
 */
void DAAppRibbonArea::buildMenu()
{
    mMenuInsertRow = new SARibbonMenu(mApp);
    mMenuInsertRow->setObjectName(QString::fromUtf8(UiNames::Menu::InsertRow));
    mMenuInsertRow->addAction(mActions->actionInsertRowAbove);

    mMenuInsertColumn = new SARibbonMenu(mApp);
    mMenuInsertColumn->setObjectName(QString::fromUtf8(UiNames::Menu::InsertColumn));
    mMenuInsertColumn->addAction(mActions->actionInsertColumnLeft);
    //
    mExportWorkflowSceneToImageMenu = new SARibbonMenu(mApp);
    mExportWorkflowSceneToImageMenu->setObjectName(QString::fromUtf8(UiNames::Menu::ExportWorkflowSceneToImage));
    mExportWorkflowSceneToImageMenu->setIcon(QIcon(QStringLiteral(":/app/bright/Icon/exportToPic.svg")));
    mExportWorkflowSceneToImageMenu->setDefaultAction(mActions->actionExportWorkflowSceneToImage);
    mExportWorkflowSceneToImageMenu->addAction(mActions->actionExportWorkflowSceneToPNG);
    //
    mMenuChartPickSetting = new QMenu(mApp);
    mMenuChartPickSetting->setObjectName(QString::fromUtf8(UiNames::Menu::ChartPickSetting));
    mMenuChartPickSetting->addAction(mActions->actionChartPickerTextAtLeftTop);
    mMenuChartPickSetting->addAction(mActions->actionChartPickerTextAtLeftBottom);
    mMenuChartPickSetting->addAction(mActions->actionChartPickerTextAtRightTop);
    mMenuChartPickSetting->addAction(mActions->actionChartPickerTextAtRightBottom);
    mMenuChartPickSetting->addAction(mActions->actionChartPickerTextFollowMouse);
    mMenuChartPickSetting->addSeparator();
    mMenuChartPickSetting->addAction(mActions->actionChartYPickerShowXValueEnabled);
    mMenuChartPickSetting->addSeparator();
    mMenuChartPickSetting->addAction(mActions->actionChartDataPickerSetting);
    //
    mMenuChartLegendPosition = new QMenu(mApp);
    mMenuChartLegendPosition->setObjectName(QString::fromUtf8(UiNames::Menu::ChartLegendPosition));
    mMenuChartLegendPosition->addAction(mActions->actionChartLegendAtTop);
    mMenuChartLegendPosition->addAction(mActions->actionChartLegendAtBottom);
    mMenuChartLegendPosition->addAction(mActions->actionChartLegendAtLeft);
    mMenuChartLegendPosition->addAction(mActions->actionChartLegendAtRight);
}

/**
 * @brief 语言变更时触发，重置界面文本
 */
void DAAppRibbonArea::retranslateUi()
{
    resetText();
}

/**
 * @brief 重置所有ribbon界面的文字
 */
void DAAppRibbonArea::resetText()
{
    ribbonBar()->applicationButton()->setText(tr("File"));  // cn:文件

    mCategoryMain->setCategoryName(tr("Main"));              // cn:主页
    mPannelMainFileOpt->setPanelName(tr("File"));            // cn:文件
    mPannelMainClipboard->setPanelName(tr("Clipboard"));     // cn:剪贴板
    mPannelMainCreate->setPanelName(tr("Create"));           // cn:创建
    mPannelSetting->setPanelName(tr("Config"));              // cn:配置
    mCategoryData->setCategoryName(tr("Data"));              // cn:数据
    mPannelDataOperate->setPanelName(tr("Data Operation"));  // cn:数据操作
    mPannelDataExport->setPanelName(tr("Export"));           // cn:导出
    mMenuExportData->setTitle(tr("Export Format"));          // cn:导出格式

    mCategoryView->setCategoryName(tr("View"));                // cn:视图
    mPannelViewMainView->setPanelName(tr("Display"));          // cn:视图显示
    mPannelViewLayout->setPanelName(tr("Layout"));             // cn:布局
    mPannelViewAppearance->setPanelName(tr("Appearance"));     // cn:外观
    mComboxRibbonThemeContainer->setPrefix(tr("Theme"));       // cn:主题

    mContextDataFrame->setContextTitle(tr("DataFrame"));          // cn:DataFrame
    mCategoryDataframeOperate->setCategoryName(tr("Operate"));    // cn:操作
    mPannelDataframeOperateAxes->setPanelName(tr("Axes"));        // cn:坐标
    mPannelDataframeOperateColumn->setPanelName(tr("Column"));    // cn:列
    mPannelDataframeOperateDType->setPanelName(tr("Type"));       // cn:类型
    mPannelDataframeOperateFormat->setPanelName(tr("Format"));    // cn:格式
    mCategoryDataframeStyle->setCategoryName(tr("Table Style"));  // cn:表格样式
    mPannelDataframeStyleFill->setPanelName(tr("Fill"));          // cn:底色
    mPannelDataframeStyleFont->setPanelName(tr("Font"));          // cn:字体
    mPannelDataframeStyleClear->setPanelName(tr("Clear"));        // cn:清除
    mComboxColumnTypesContainer->setPrefix(tr("Type"));           // cn:类型
    mComboxDisplayFormatContainer->setPrefix(tr("Format"));       // cn:格式

    mContextWorkflow->setContextTitle(tr("Workflow"));  // cn:工作流
    // 表格
    mBtnTableFillColor->setText(tr("Fill Color"));  // cn:填充颜色

    mCategoryWorkflowGraphicsEdit->setCategoryName(tr("Workflow Edit"));  // cn:工作流编辑
    mPannelClipBoard->setPanelName(tr("Clipboard"));                      // cn:剪切板
    mPannelWorkflowItem->setPanelName(tr("Item"));                        // cn:图元
    mPannelWorkflowText->setPanelName(tr("Text"));                        // cn:文本
    mPannelWorkflowBackground->setPanelName(tr("Background"));            // cn:背景
    mPannelWorkflowView->setPanelName(tr("View"));                        // cn:视图

    mCategoryWorkflowRun->setCategoryName(tr("Workflow Run"));  // cn:工作流运行
    mPannelWorkflowRun->setPanelName(tr("Run"));                // cn:运行
                                                                //
    mCategoryFigure->setCategoryName(tr("Figure"));             // cn:绘图
    mPannelChartAdd->setPanelName(tr("Add Chart"));             // cn:添加绘图
    mPannelStatsPlot->setPanelName(tr("Stats Plot"));           // cn:统计绘图
    // 绘图上下文标签
    mContextChart->setContextTitle(tr("Chart"));                         // cn:绘图
    mCategoryChartStyle->setCategoryName(tr("Chart Style"));             // cn:绘图样式
    mPannelFigureSettingForContext->setPanelName(tr("Figure Setting"));  // cn:绘图窗口设置
    mPannelChartSetting->setPanelName(tr("Chart Setting"));              // cn:图表设置
    mPanelFigureTheme->setPanelName(tr("Figure Theme"));                 // cn:绘图主题
    mCategoryChartEdit->setCategoryName(tr("Chart Edit"));               // cn:图表编辑
    mPannelChartSelectTool->setPanelName(tr("Select Tool"));             // cn:选区工具
    mPannelChartAssistTool->setPanelName(tr("Chart Assist Tool"));       // cn:图表辅助工具
    //
    mMenuViewLineMarkers->setTitle(tr("View Marker"));    // cn:视图标记
    mMenuViewLineMarkers->setToolTip(tr("View Marker"));  // cn:视图标记
    //
    mExportWorkflowSceneToImageMenu->setTitle(tr("Export Image"));                               // cn:导出为图片
    mExportWorkflowSceneToImageMenu->setToolTip(tr("Export Workflow Graphics Scene To Image"));  // cn:把工作流的场景导出为图片
}

/**
 * @brief 构建ribbon
 */
void DAAppRibbonArea::buildRibbon()
{
    ribbonBar()->showMinimumModeButton();
    buildRibbonMainCategory();
    buildRibbonDataCategory();
    buildRibbonViewCategory();
    buildRibbonFigureCategory();
    buildRibbonAgentCategory();
    buildRibbonQuickAccessBar();
    // 上下文标签
    buildContextCategoryDataFrame();
    buildContextCategoryWorkflow();
    buildContextCategoryChartEdit();
    //
    buildApplicationMenu();
    //
    buildRightButtonBar();
}

/**
 * @brief 构建主页标签
 * 主页的category objname = da-ribbon-category-main
 *
 * 结构：| 文件 | 剪贴板 | 创建 | 配置 |，运行/停止仅保留在工作流上下文 Run 页（设计 D2）
 */
void DAAppRibbonArea::buildRibbonMainCategory()
{
    mCategoryMain = new SARibbonCategory(app());
    mCategoryMain->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainCategory));

    //--------文件--------------------------------------------------
    mPannelMainFileOpt = new SARibbonPanel(mCategoryMain);
    mPannelMainFileOpt->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainCommonPanel));
    mPannelMainFileOpt->addLargeAction(mActions->actionOpen);
    mPannelMainFileOpt->addSmallAction(mActions->actionSave);
    mPannelMainFileOpt->addSmallAction(mActions->actionSaveAs);
    mCategoryMain->addPanel(mPannelMainFileOpt);

    //--------剪贴板--------------------------------------------------
    // Undo/Redo 为 QUndoGroup 生成的 action（buildRedoUndo 中创建），
    // Cut/Copy/Paste/Delete/SelectAll 由 DAAppController 按焦点路由（工作流/表格/图表）
    mPannelMainClipboard = new SARibbonPanel(mCategoryMain);
    mPannelMainClipboard->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainClipboardPanel));
    mPannelMainClipboard->addMediumAction(mActions->actionUndo);
    mPannelMainClipboard->addMediumAction(mActions->actionRedo);
    mPannelMainClipboard->addSeparator();
    mPannelMainClipboard->addSmallAction(mActions->actionCut);
    mPannelMainClipboard->addSmallAction(mActions->actionCopy);
    mPannelMainClipboard->addSmallAction(mActions->actionPaste);
    mPannelMainClipboard->addSmallAction(mActions->actionDelete);
    mPannelMainClipboard->addSmallAction(mActions->actionSelectAll);
    mCategoryMain->addPanel(mPannelMainClipboard);

    //--------创建--------------------------------------------------
    // 原 5 面板中的三个「新建」面板合并为一个创建面板
    mPannelMainCreate = new SARibbonPanel(mCategoryMain);
    mPannelMainCreate->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainCreatePanel));
    mPannelMainCreate->addLargeAction(mActions->actionAddData);
    mPannelMainCreate->addLargeAction(mActions->actionAddFigure);
    mPannelMainCreate->addLargeAction(mActions->actionWorkflowNew);

    //--------配置--------------------------------------------------
    mPannelSetting = new SARibbonPanel(mCategoryMain);
    mPannelSetting->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainSettingPanel));
    mPannelSetting->addLargeAction(mActions->actionSetting);
    mPannelSetting->addLargeAction(mActions->actionPluginManager);
    mPannelSetting->addLargeAction(mActions->actionAbout);
    mCategoryMain->addPanel(mPannelSetting);
    //----------------------------------------------------------

    ribbonBar()->addCategoryPage(mCategoryMain);  // 主页
}

/**
 * @brief 构建数据标签
 * objectname=da-ribbon-category-data
 *
 * 结构：| 数据操作 | 导出 |
 */
void DAAppRibbonArea::buildRibbonDataCategory()
{
    mCategoryData = new SARibbonCategory(app());
    mCategoryData->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataCategory));

    //--------DataOperate--------------------------------------------------

    mPannelDataOperate = new SARibbonPanel(mCategoryData);
    mPannelDataOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataOperatePanel));
    mPannelDataOperate->addLargeAction(mActions->actionAddData);
    mPannelDataOperate->addLargeAction(mActions->actionRemoveData);
    mPannelDataOperate->addLargeAction(mActions->actionRenameData);
    mCategoryData->addPanel(mPannelDataOperate);

    //--------Export--------------------------------------------------
    mMenuExportData = new SARibbonMenu(mApp);
    mMenuExportData->setObjectName(QString::fromUtf8(UiNames::Menu::ExportData));
    mMenuExportData->setIcon(QIcon(QStringLiteral(":/app/bright/Icon/exportToPng.svg")));
    mMenuExportData->addAction(mActions->actionExportDataCsv);
    mMenuExportData->addAction(mActions->actionExportDataExcel);
    mMenuExportData->addAction(mActions->actionExportDataPickle);
    mMenuExportData->addAction(mActions->actionExportDataParquet);
    mActions->actionExportData->setMenu(mMenuExportData);

    mPannelDataExport = new SARibbonPanel(mCategoryData);
    mPannelDataExport->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataExportPanel));
    mPannelDataExport->addLargeAction(mActions->actionExportData, QToolButton::MenuButtonPopup);
    mCategoryData->addPanel(mPannelDataExport);

    //----------------------------------------------------------

    ribbonBar()->addCategoryPage(mCategoryData);
}

/**
 * @brief 构建视图标签
 * objectname=da-ribbon-category-view
 *
 * 结构：| 显示 | 布局 | 外观 |
 */
void DAAppRibbonArea::buildRibbonViewCategory()
{
    mCategoryView = new SARibbonCategory(app());
    mCategoryView->setObjectName(QString::fromUtf8(UiNames::Ribbon::ViewCategory));

    //--------显示--------------------------------------------------
    mPannelViewMainView = new SARibbonPanel(mCategoryView);
    mPannelViewMainView->setObjectName(QString::fromUtf8(UiNames::Ribbon::ViewMainPanel));
    mPannelViewMainView->addLargeAction(mActions->actionShowWorkFlowArea);
    mPannelViewMainView->addMediumAction(mActions->actionShowWorkFlowManagerArea);
    mPannelViewMainView->addLargeAction(mActions->actionShowChartArea);
    mPannelViewMainView->addMediumAction(mActions->actionShowChartManagerArea);
    mPannelViewMainView->addLargeAction(mActions->actionShowDataArea);
    mPannelViewMainView->addMediumAction(mActions->actionShowDataManagerArea);
    mPannelViewMainView->addSeparator();
    mPannelViewMainView->addSmallAction(mActions->actionShowMessageLogView);
    mPannelViewMainView->addSmallAction(mActions->actionShowSettingWidget);
    mPannelViewMainView->addSeparator();
    mPannelViewMainView->addMediumAction(mActions->actionShowLeftSideBar);
    mPannelViewMainView->addMediumAction(mActions->actionShowRightSideBar);
    mPannelViewMainView->addSeparator();
    mPannelViewMainView->addLargeAction(mActions->actionShowAgentArea);
    mCategoryView->addPanel(mPannelViewMainView);

    //--------布局--------------------------------------------------
    // 布局管理（打开对话框：保存/应用/删除布局方案）+ 恢复默认布局（需确认），
    // 方案列表与具体操作都在布局管理对话框中，由 DAAppLayoutManager 提供内容
    mPannelViewLayout = new SARibbonPanel(mCategoryView);
    mPannelViewLayout->setObjectName(QString::fromUtf8(UiNames::Ribbon::ViewLayoutPanel));
    mPannelViewLayout->addLargeAction(mActions->actionManageLayouts);
    mPannelViewLayout->addLargeAction(mActions->actionResetDefaultLayout);
    mCategoryView->addPanel(mPannelViewLayout);

    //--------外观--------------------------------------------------
    // Ribbon 主题下拉（完整 10 款枚举，与设置页一致），切换即生效并持久化
    mPannelViewAppearance = new SARibbonPanel(mCategoryView);
    mPannelViewAppearance->setObjectName(QString::fromUtf8(UiNames::Ribbon::ViewAppearancePanel));
    mComboxRibbonThemeContainer = new SARibbonLineWidgetContainer(mPannelViewAppearance);
    mComboxRibbonTheme          = new QComboBox(mComboxRibbonThemeContainer);
    mComboxRibbonThemeContainer->setWidget(mComboxRibbonTheme);
    mPannelViewAppearance->addWidget(mComboxRibbonThemeContainer, SARibbonPanelItem::Medium);
    mCategoryView->addPanel(mPannelViewAppearance);

    //----------------------------------------------------------

    ribbonBar()->addCategoryPage(mCategoryView);  // 视图
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
    mContextDataFrame = ribbonBar()->addContextCategory(tr("DataFrame"));  // cn:DataFrame
    mContextDataFrame->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameContextCategory));
    mCategoryDataframeOperate = mContextDataFrame->addCategoryPage(tr("Operate"));  // cn:操作
    mCategoryDataframeOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateCategory));
    // Axes pannel
    mPannelDataframeOperateAxes = mCategoryDataframeOperate->addPanel(tr("Axes"));  // cn:坐标
    mPannelDataframeOperateAxes->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateAxesPanel));
    mActions->actionInsertRow->setMenu(mMenuInsertRow);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionInsertRow, QToolButton::MenuButtonPopup);
    mActions->actionInsertColumnRight->setMenu(mMenuInsertColumn);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionInsertColumnRight, QToolButton::MenuButtonPopup);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionRemoveCell);
    mPannelDataframeOperateAxes->addMediumAction(mActions->actionRemoveRow);
    mPannelDataframeOperateAxes->addMediumAction(mActions->actionRemoveColumn);
    mPannelDataframeOperateAxes->addSeparator();
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionRenameColumns);
    mPannelDataframeOperateAxes->addLargeAction(mActions->actionChangeToIndex);
    // Column pannel - 列级五件套（原表头右键菜单上移，action 与右键共用）
    mPannelDataframeOperateColumn = mCategoryDataframeOperate->addPanel(tr("Column"));  // cn:列
    mPannelDataframeOperateColumn->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateColumnPanel));
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionRenameColumn);
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionCopyColumnName);
    mPannelDataframeOperateColumn->addSeparator();
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionGotoMax);
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionGotoMin);
    mPannelDataframeOperateColumn->addSeparator();
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionShowColumnDescribe);
    // Type pannel
    mPannelDataframeOperateDType = mCategoryDataframeOperate->addPanel(tr("Type"));  // cn:类型
    mPannelDataframeOperateDType->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateDTypePanel));
    mComboxColumnTypesContainer = new SARibbonLineWidgetContainer(mPannelDataframeOperateDType);
    mComboxColumnTypes          = new DAPyDTypeComboBox(mComboxColumnTypesContainer);
    mComboxColumnTypes->setMinimumWidth(Qt5Qt6Compat_fontMetrics_width(mApp->fontMetrics(), "timedelta64(scoll)"));  // 设置最小宽度
    mComboxColumnTypesContainer->setPrefix(tr("Type"));  // cn:类型
    mComboxColumnTypesContainer->setWidget(mComboxColumnTypes);
    mPannelDataframeOperateDType->addWidget(mComboxColumnTypesContainer, SARibbonPanelItem::Medium);
    mCastActionsButtonGroup = new SARibbonButtonGroupWidget();
    mCastActionsButtonGroup->addAction(mActions->actionCastToNum);
    mCastActionsButtonGroup->addAction(mActions->actionCastToString);
    mCastActionsButtonGroup->addSeparator();
    mCastActionsButtonGroup->addAction(mActions->actionCastToDatetime);
    mPannelDataframeOperateDType->addWidget(mCastActionsButtonGroup, SARibbonPanelItem::Medium);

    // Format pannel - 显示格式
    mPannelDataframeOperateFormat = mCategoryDataframeOperate->addPanel(tr("Format"));  // cn:格式
    mPannelDataframeOperateFormat->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateFormatPanel));
    mComboxDisplayFormatContainer = new SARibbonLineWidgetContainer(mPannelDataframeOperateFormat);
    mComboxDisplayFormat          = new DATableDisplayFormatComboBox(mComboxDisplayFormatContainer);
    mComboxDisplayFormatContainer->setPrefix(tr("Format"));  // cn:格式
    mComboxDisplayFormatContainer->setWidget(mComboxDisplayFormat);
    mPannelDataframeOperateFormat->addWidget(mComboxDisplayFormatContainer, SARibbonPanelItem::Medium);
    mPannelDataframeOperateFormat->addMediumAction(mActions->actionTableFormatCells);

    // ===== 表格样式 category =====
    mCategoryDataframeStyle = mContextDataFrame->addCategoryPage(tr("Table Style"));  // cn:表格样式
    mCategoryDataframeStyle->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleCategory));

    // Fill panel - 底色
    mPannelDataframeStyleFill = mCategoryDataframeStyle->addPanel(tr("Fill"));  // cn:底色
    mPannelDataframeStyleFill->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFillPanel));
    mBtnTableFillColor = new SARibbonColorToolButton(mPannelDataframeStyleFill);
    mBtnTableFillColor->setColorStyle(SARibbonColorToolButton::ColorFillToIcon);
    mBtnTableFillColor->setupStandardColorMenu();
    mPannelDataframeStyleFill->addWidget(mBtnTableFillColor, SARibbonPanelItem::Large);

    // Font panel - 字体
    mPannelDataframeStyleFont = mCategoryDataframeStyle->addPanel(tr("Font"));  // cn:字体
    mPannelDataframeStyleFont->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFontPanel));
    mWidgetTableFont = new DAFontEditPannelWidget(mPannelDataframeStyleFont);
    mPannelDataframeStyleFont->addWidget(mWidgetTableFont, SARibbonPanelItem::Large);

    // Clear panel - 清除
    mPannelDataframeStyleClear = mCategoryDataframeStyle->addPanel(tr("Clear"));  // cn:清除
    mPannelDataframeStyleClear->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleClearPanel));
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleSelected);
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleAll);
}

/**
 * @brief 构建绘图标签
 *
 * 精简后结构：| 添加绘图 | 统计绘图 |（原「绘图设置」面板与图表上下文 Chart Edit 页重复，已移除）
 */
void DAAppRibbonArea::buildRibbonFigureCategory()
{
    mCategoryFigure = ribbonBar()->addCategoryPage(tr("Figure"));  // cn:绘图
    mCategoryFigure->setObjectName(QString::fromUtf8(UiNames::Ribbon::FigureCategory));

    mPannelChartAdd = new SARibbonPanel(mCategoryFigure);
    mPannelChartAdd->setObjectName(QString::fromUtf8(UiNames::Ribbon::FigureChartAddPanel));
    mPannelChartAdd->addLargeAction(mActions->actionChartAddCurve);
    mPannelChartAdd->addLargeAction(mActions->actionChartAddScatter2D);
    mPannelChartAdd->addLargeAction(mActions->actionChartAddErrorBar);
    mPannelChartAdd->addLargeAction(mActions->actionChartAddBoxPlot);
    mPannelChartAdd->addLargeAction(mActions->actionChartAddBar);
    mPannelChartAdd->addMediumAction(mActions->actionChartAddMultiBar);
    mPannelChartAdd->addMediumAction(mActions->actionChartAddHistogramBar);
    mPannelChartAdd->addLargeAction(mActions->actionChartAddContourMap);
    mPannelChartAdd->addLargeAction(mActions->actionChartAddCloudMap);
    mPannelChartAdd->addLargeAction(mActions->actionChartAddVectorfield);
    mPannelChartAdd->addLargeAction(mActions->actionChartAdd3DSurface);
    mPannelChartAdd->addLargeAction(mActions->actionChartAdd3DBar);
    mPannelChartAdd->addLargeAction(mActions->actionChartAdd3DLine);

    mCategoryFigure->addPanel(mPannelChartAdd);

    // 统计绘图面板
    mPannelStatsPlot = new SARibbonPanel(mCategoryFigure);
    mPannelStatsPlot->setObjectName(QString::fromUtf8(UiNames::Ribbon::FigureStatsPlotPanel));
    mPannelStatsPlot->setPanelName(tr("Stats Plot"));  // cn:统计绘图
    mPannelStatsPlot->addLargeAction(mActions->actionStatsHistplot);
    mPannelStatsPlot->addLargeAction(mActions->actionStatsKdeplot1d);
    mPannelStatsPlot->addLargeAction(mActions->actionStatsKdeplot2d);
    mPannelStatsPlot->addMediumAction(mActions->actionStatsBoxplot);
    mPannelStatsPlot->addMediumAction(mActions->actionStatsHeatmap);
    mPannelStatsPlot->addMediumAction(mActions->actionStatsScatterplot);
    mPannelStatsPlot->addMediumAction(mActions->actionStatsBarplot);
    mPannelStatsPlot->addMediumAction(mActions->actionStatsRegplot);
    mPannelStatsPlot->addMediumAction(mActions->actionStatsECDFplot);
    mCategoryFigure->addPanel(mPannelStatsPlot);
}

/**
 * @brief 构建Workflow的上下文标签
 * @note 注意buildContextCategoryWorkflowEdit和buildContextCategoryWorkflowRun必须在此函数之后调用
 */
void DAAppRibbonArea::buildContextCategoryWorkflow()
{
    mContextWorkflow = ribbonBar()->addContextCategory(tr("Workflow"));  // cn:工作流
    mContextWorkflow->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowContextCategory));
    buildContextCategoryWorkflowView_();
    buildContextCategoryWorkflowEdit_();
    buildContextCategoryWorkflowRun_();
    ribbonBar()->showContextCategory(mContextWorkflow);
}

/**
 * @brief 构建Workflow-编辑的上下文标签
 */
void DAAppRibbonArea::buildContextCategoryWorkflowEdit_()
{
    DAPyWorkFlowOperateWidget* wfo = mDockArea->getWorkFlowOperateWidget();
    mCategoryWorkflowGraphicsEdit  = mContextWorkflow->addCategoryPage(tr("Workflow Edit"));  // cn:工作流编辑
    mCategoryWorkflowGraphicsEdit->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowEditCategory));
    // 条目pannel

    // 剪切板
    mPannelClipBoard = mCategoryWorkflowGraphicsEdit->addPanel(tr("Clipboard"));  // cn:剪切板
    mPannelClipBoard->addLargeAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionPaste));
    mPannelClipBoard->addSmallAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCut));
    mPannelClipBoard->addSmallAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCopy));
    //  Item
    mPannelWorkflowItem = mCategoryWorkflowGraphicsEdit->addPanel(tr("Item"));  // cn:图元
    mPannelWorkflowItem->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowItemPanel));
    mPannelWorkflowItem->addLargeAction(mActions->actionWorkflowLinkEnable);  // 连线
    mWorkflowShapeEditPannelWidget = new DAShapeEditPannelWidget(mPannelWorkflowItem);
    mPannelWorkflowItem->addWidget(mWorkflowShapeEditPannelWidget, SARibbonPanelItem::Large);
    mPannelWorkflowItem->addSeparator();
    mWorkflowFontEditPannel = new DAFontEditPannelWidget(mPannelWorkflowItem);
    mPannelWorkflowItem->addWidget(mWorkflowFontEditPannel, SARibbonPanelItem::Large);
    // Text
    mPannelWorkflowText = mCategoryWorkflowGraphicsEdit->addPanel(tr("Text"));  // cn:文本
    mPannelWorkflowText->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowTextPanel));
    mPannelWorkflowText->addLargeAction(mActions->actionWorkflowStartDrawRect);
    mPannelWorkflowText->addLargeAction(mActions->actionWorkflowStartDrawText);
    // Background
    mPannelWorkflowBackground = mCategoryWorkflowGraphicsEdit->addPanel(tr("Background"));  // cn:背景
    mPannelWorkflowBackground->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowBackgroundPanel));
    mPannelWorkflowBackground->addLargeAction(mActions->actionWorkflowAddBackgroundPixmap);
    mPannelWorkflowBackground->addMediumAction(mActions->actionWorkflowLockBackgroundPixmap);
    mPannelWorkflowBackground->addMediumAction(mActions->actionWorkflowEnableItemMoveWithBackground);

    // group
    mPannelWorkflowGroup = mCategoryWorkflowGraphicsEdit->addPanel(tr("Group"));  // cn:分组
    mPannelWorkflowGroup->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowGroupPanel));
    mPannelWorkflowGroup->addMediumAction(mActions->actionItemGrouping);
    mPannelWorkflowGroup->addMediumAction(mActions->actionItemUngroup);
    mPannelWorkflowGroup->addLargeAction(mActions->actionWorkflowEnableItemLinkageMove);
    //
    // connect
    connect(mWorkflowShapeEditPannelWidget,
            &DAShapeEditPannelWidget::borderPenChanged,
            this,
            &DAAppRibbonArea::selectedWorkflowItemPen);
    connect(mWorkflowShapeEditPannelWidget,
            &DAShapeEditPannelWidget::backgroundBrushChanged,
            this,
            &DAAppRibbonArea::selectedWorkflowItemBrush);
    connect(mWorkflowFontEditPannel, &DAFontEditPannelWidget::currentFontChanged, this, &DAAppRibbonArea::selectedWorkflowItemFont);
    connect(mWorkflowFontEditPannel,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAAppRibbonArea::selectedWorkflowItemFontColor);
}

/**
 * @brief 构建Workflow-视图的上下文标签
 */
void DAAppRibbonArea::buildContextCategoryWorkflowView_()
{
    DAPyWorkFlowOperateWidget* wfo = mDockArea->getWorkFlowOperateWidget();
    mCategoryWorkflowGraphicsView  = mContextWorkflow->addCategoryPage(tr("Workflow View"));  // cn:工作流视图
    mCategoryWorkflowGraphicsView->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowViewCategory));
    // View
    mPannelWorkflowView = mCategoryWorkflowGraphicsView->addPanel(tr("View"));  // cn:视图
    mPannelWorkflowView->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowViewPanel));
    mPannelWorkflowView->addLargeAction(mActions->actionWorkflowViewReadOnly);
    mPannelWorkflowView->addLargeAction(mActions->actionWorkflowShowGrid);
    mPannelWorkflowView->addSeparator();
    mPannelWorkflowView->addLargeAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionZoomFit));
    mPannelWorkflowView->addMediumAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionZoomIn));
    mPannelWorkflowView->addMediumAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionZoomOut));

    mMenuViewLineMarkers = new SARibbonMenu(mApp);
    mMenuViewLineMarkers->setObjectName(QString::fromUtf8(UiNames::Menu::ViewLineMarkers));
    mMenuViewLineMarkers->setIcon(QIcon(":/app/bright/Icon/view-marker.svg"));
    mMenuViewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCrossLineMarker));
    mMenuViewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionHLineMarker));
    mMenuViewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionVLineMarker));
    mMenuViewLineMarkers->addAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker));

    if (QActionGroup* ag = wfo->getLineMarkerActionGroup()) {
        connect(ag, &QActionGroup::triggered, this, [ this, wfo ](QAction* act) {
            QAction* actMarker = mActions->actionWorkflowViewMarker;
            QAction* noneAct   = wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker);
            if (act != noneAct && act->isChecked()) {
                actMarker->setIcon(act->icon());
                actMarker->setChecked(true);
            } else {
                actMarker->setIcon(QIcon(":/app/bright/Icon/view-marker.svg"));
                actMarker->setChecked(false);
            }
        });
    }
    mActions->actionWorkflowViewMarker->setMenu(mMenuViewLineMarkers);
    mPannelWorkflowView->addLargeAction(mActions->actionWorkflowViewMarker, QToolButton::MenuButtonPopup);

    mPannelWorkflowExport = mCategoryWorkflowGraphicsView->addPanel(tr("Export"));  // cn:导出
    mPannelWorkflowExport->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowExportPanel));
    mPannelWorkflowExport->addMenu(mExportWorkflowSceneToImageMenu, SARibbonPanelItem::Large, QToolButton::MenuButtonPopup);
}

/**
 * @brief 构建workflow-运行的上下文标签
 */
void DAAppRibbonArea::buildContextCategoryWorkflowRun_()
{
    mCategoryWorkflowRun = mContextWorkflow->addCategoryPage(tr("Workflow Run"));  // cn:工作流运行
    mCategoryWorkflowRun->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowRunCategory));
    // Run
    mPannelWorkflowRun = mCategoryWorkflowRun->addPanel(tr("Run"));  // cn:运行
    mPannelWorkflowRun->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowRunPanel));
    mPannelWorkflowRun->addLargeAction(mActions->actionWorkflowRun);
    mPannelWorkflowRun->addLargeAction(mActions->actionWorkflowTerminate);
}

/**
 * @brief 构建chart上下文
 *
 * 两页结构：Chart Style（样式）/ Chart Edit（编辑）；
 * 图例位置（上/下/左/右）作为 actionChartEnableLegend 的下拉菜单
 */
void DAAppRibbonArea::buildContextCategoryChartEdit()
{
    mContextChart = ribbonBar()->addContextCategory(tr("Chart Operate"));  // cn:绘图操作
    mContextChart->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartContextCategory));
    mCategoryChartStyle = mContextChart->addCategoryPage(tr("Chart Style"));  // cn:绘图样式
    mCategoryChartStyle->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartStyleCategory));
    // chart edit
    mPannelChartSetting = new SARibbonPanel(mCategoryChartStyle);
    mPannelChartSetting->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditChartSettingPanel));
    mPannelChartSetting->addLargeAction(mActions->actionFigureSettingApplyAllChart);
    mPannelChartSetting->addSeparator();
    // grid
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableGrid);
    mChartGridDirActionsButtonGroup = new SARibbonButtonGroupWidget(mPannelChartSetting);
    mChartGridDirActionsButtonGroup->addAction(mActions->actionChartEnableGridX);
    mChartGridDirActionsButtonGroup->addAction(mActions->actionChartEnableGridY);
    mPannelChartSetting->addSmallWidget(mChartGridDirActionsButtonGroup);
    mChartGridMinActionsButtonGroup = new SARibbonButtonGroupWidget(mPannelChartSetting);
    mChartGridMinActionsButtonGroup->addAction(mActions->actionChartEnableGridXMin);
    mChartGridMinActionsButtonGroup->addAction(mActions->actionChartEnableGridYMin);
    mPannelChartSetting->addSmallWidget(mChartGridMinActionsButtonGroup);
    // pan
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePan);
    // 缩放
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableZoom);
    mPannelChartSetting->addMediumAction(mActions->actionChartZoomIn);
    mPannelChartSetting->addMediumAction(mActions->actionChartZoomOut);
    mPannelChartSetting->addLargeAction(mActions->actionChartZoomAll);
    // picker
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerCross);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerXY);
    mActions->actionChartEnablePickerY->setMenu(mMenuChartPickSetting);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerY, QToolButton::MenuButtonPopup);
    mPannelChartSetting->addLargeAction(mActions->actionChartLinkAllPickerEnabled);
    // legend（下拉菜单为图例位置：上/下/左/右互斥）
    mActions->actionChartEnableLegend->setMenu(mMenuChartLegendPosition);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableLegend, QToolButton::MenuButtonPopup);

    mCategoryChartStyle->addPanel(mPannelChartSetting);

    mPanelFigureTheme   = new SARibbonPanel(mCategoryChartStyle);
    mFigureThemeGallery = mPanelFigureTheme->addGallery(true);
    mFigureThemeGallery->setMinimumWidth(200);
    SARibbonGalleryGroup* group1 =
        mFigureThemeGallery->addCategoryActions(tr("Theme"), mActions->actionListOfColorTheme);  // cn:主题
    group1->setGalleryGroupStyle(SARibbonGalleryGroup::IconWithText);
    group1->setGridMinimumWidth(80);
    group1->setIconSize(QSize(120, 50));
    group1->setGridSize(QSize(120, 60));
    mPanelFigureTheme->addAction(mActions->actionCopyFigureInClipboard);
    // 手动设置才能刷新当前界面的配置
    mFigureThemeGallery->setCurrentViewGroup(group1);
    mCategoryChartStyle->addPanel(mPanelFigureTheme);

    //================================
    // 绘图编辑
    //================================
    mCategoryChartEdit = mContextChart->addCategoryPage(tr("Chart Edit"));  // cn:图表编辑
    mCategoryChartEdit->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditCategory));
    // 绘图窗口设置（从 Chart Style 挪来）
    mPannelFigureSettingForContext = new SARibbonPanel(mCategoryChartEdit);
    mPannelFigureSettingForContext->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditFigureSettingPanel));
    mPannelFigureSettingForContext->addLargeAction(mActions->actionAddFigure);
    mPannelFigureSettingForContext->addLargeAction(mActions->actionChartEditorResizeSubChart);
    mPannelFigureSettingForContext->addLargeAction(mActions->actionFigureNewXYAxis);  // 新建坐标系
    mCategoryChartEdit->addPanel(mPannelFigureSettingForContext);
    // 选区工具
    mPannelChartSelectTool = mCategoryChartEdit->addPanel(tr("Select Tool"));  // cn:选区工具
    mPannelChartSelectTool->addLargeAction(mActions->actionChartEditorRectSelector);
    mPannelChartSelectTool->addMediumAction(mActions->actionChartEditorEllipseSelector);
    mPannelChartSelectTool->addMediumAction(mActions->actionChartEditorPolygonSelector);
    // 辅助工具
    mPannelChartAssistTool = mCategoryChartEdit->addPanel(tr("Assist Tools"));  // cn:辅助工具
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorAddCrossMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddHLineMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddVLineMarker);
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorAddArrowMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddTextMarker);
    mPannelChartAssistTool->addSeparator();
    // 数据探针（原死 action，从未上过 Ribbon）
    mPannelChartAssistTool->addMediumAction(mActions->actionAddVerticalPlotProbeMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionAddHorizontalPlotProbeMarker);
}

/**
 * @brief 构建ApplicationMenu
 */
void DAAppRibbonArea::buildApplicationMenu()
{
    mApplicationMenu = new DAAppRibbonApplicationMenu(app());
    mApplicationMenu->addAction(mActions->actionOpen);
    mApplicationMenu->addAction(mActions->actionOpenMarkdown);
    mApplicationMenu->addAction(mActions->actionSave);
    mApplicationMenu->addAction(mActions->actionSaveAs);
    mActions->recentFilesManager->attachToMenu(mApplicationMenu, tr("Recent Files"));  // cn:最近打开的文件
    SARibbonApplicationButton* appBtn = qobject_cast< SARibbonApplicationButton* >(ribbonBar()->applicationButton());
    if (nullptr == appBtn) {
        return;
    }
    mApplicationMenu->update();
    appBtn->setMenu(mApplicationMenu);
}

/**
 * @brief 构建右上角按钮区
 *
 * 原 4 款主题菜单已移除（主题切换迁移到视图页-外观面板，10 款完整枚举+持久化）
 */
void DAAppRibbonArea::buildRightButtonBar()
{
    ribbonBar()->activeRightButtonGroup();
}

/**
 * @brief 设置dock区，构建ribbon及菜单
 */
void DAAppRibbonArea::setDockingArea(DAAppDockingArea* dock)
{
    mDockArea = dock;
    buildMenu();
    // Undo/Redo 由 QUndoGroup 生成，必须先于 buildRibbon 创建（主页剪贴板面板会直接引用）
    buildRedoUndo();
    buildRibbon();
    resetText();
}

DAAPPRIBBONAREA_COMMON_SETTING_CPP(WorkFlowEdit, mWorkflowShapeEditPannelWidget, mWorkflowFontEditPannel)

AppMainWindow* DAAppRibbonArea::app() const
{
    return (mApp);
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
    return (mCategoryMain);
}

/**
 * @brief 通过DACommandInterface构建redo/undo的action
 * @param cmd
 */
void DAAppRibbonArea::buildRedoUndo()
{
    QUndoGroup& undoGroup = mAppCmd->undoGroup();
    // 设置redo,undo的action

    mActions->actionRedo = undoGroup.createRedoAction(this);
    mActions->actionRedo->setObjectName(QString::fromUtf8(UiNames::Action::Redo));
    mActions->actionRedo->setIcon(QIcon(":/app/bright/Icon/redo.svg"));
    mActions->actionRedo->setShortcut(QKeySequence::Redo);
    mActions->actionUndo = undoGroup.createUndoAction(this);
    mActions->actionUndo->setObjectName(QString::fromUtf8(UiNames::Action::Undo));
    mActions->actionUndo->setIcon(QIcon(":/app/bright/Icon/undo.svg"));
    mActions->actionUndo->setShortcut(QKeySequence::Undo);
    SARibbonQuickAccessBar* bar = ribbonBar()->quickAccessBar();
    if (!bar) {
        return;
    }
    bar->addAction(mActions->actionUndo);
    bar->addAction(mActions->actionRedo);
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
    mActions->actionChartLinkAllPickerEnabled->setChecked(fig->isDataPickerGroupEnabled());
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
    updateChartGridAboutRibbon(chart);
    updateChartZoomPanAboutRibbon(chart);
    updateChartPickerAboutRibbon(chart);
    updateChartLegendAboutRibbon(chart);
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
    mActions->actionChartEnableGrid->setChecked(chart->isGridEnabled());
    mActions->actionChartEnableGridX->setChecked(chart->isGridXEnabled());
    mActions->actionChartEnableGridY->setChecked(chart->isGridYEnabled());
    mActions->actionChartEnableGridXMin->setChecked(chart->isGridXMinEnabled());
    mActions->actionChartEnableGridYMin->setChecked(chart->isGridYMinEnabled());
    bool c = mActions->actionChartEnableGrid->isChecked();
    mActions->actionChartEnableGridX->setEnabled(c);
    mActions->actionChartEnableGridY->setEnabled(c);
    mActions->actionChartEnableGridXMin->setEnabled(c);
    mActions->actionChartEnableGridYMin->setEnabled(c);
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
    mActions->actionChartEnableZoom->setChecked(chart->isZoomEnabled());
    mActions->actionChartEnablePan->setChecked(chart->isPanEnabled());
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
    mActions->actionChartEnablePickerCross->setChecked(chart->isCrosshairEnabled());
    mActions->actionChartEnablePickerY->setChecked(chart->isYValuePickingEnabled());
    mActions->actionChartEnablePickerXY->setChecked(chart->isXYValuePickingEnabled());
}

/**
 * @brief 更新绘图的legend状态（含图例位置菜单勾选同步）
 * @param chart
 */
void DAAppRibbonArea::updateChartLegendAboutRibbon(DAChartWidget* chart)
{
    if (nullptr == chart) {
        return;
    }
    mActions->actionChartEnableLegend->setChecked(chart->isLegendEnabled());
    // 图例位置菜单勾选同步
    Qt::Alignment align = chart->getLegendPosition();
    QAction* checked    = nullptr;
    if (align & Qt::AlignTop) {
        checked = mActions->actionChartLegendAtTop;
    } else if (align & Qt::AlignBottom) {
        checked = mActions->actionChartLegendAtBottom;
    } else if (align & Qt::AlignLeft) {
        checked = mActions->actionChartLegendAtLeft;
    } else if (align & Qt::AlignRight) {
        checked = mActions->actionChartLegendAtRight;
    }
    for (QAction* a : mActions->actionGroupChartLegendPosition->actions()) {
        a->setChecked(a == checked);
    }
}

/**
 * @brief DAAppRibbonArea::updateWorkflowViewAboutRibbon
 * @param wf
 */
void DAAppRibbonArea::updateWorkflowAboutRibbon(DAPyWorkFlowOperateWidget* wfo)
{
    DAPyWorkFlowEditWidget* wf     = wfo->getCurrentWorkFlowWidget();
    DAPyWorkFlowGraphicsView* view = wf->getWorkFlowGraphicsView();
    // 要判断DAPyWorkFlowGraphicsView的MarkerStyle
    if (view) {
        QAction* actMarker = mActions->actionWorkflowViewMarker;
        QAction* act       = wfo->getLineMarkerActionGroup()->checkedAction();
        if (!act || act == wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker)) {
            actMarker->setIcon(QIcon(":/app/bright/Icon/view-marker.svg"));
            actMarker->setChecked(false);
        } else {
            actMarker->setIcon(act->icon());
            actMarker->setChecked(true);
        }
    }
}

/**
 * @brief 显示上下文(会把其他上下文隐藏)
 * @param type
 */
void DAAppRibbonArea::showContextCategory(DAAppRibbonArea::ContextCategoryType type)
{
    SARibbonBar* ribbon = ribbonBar();
    switch (type) {
    case ContextCategoryData: {
        ribbon->showContextCategory(mContextDataFrame);
        ribbon->hideContextCategory(mContextWorkflow);
        ribbon->hideContextCategory(mContextChart);
    } break;
    case ContextCategoryWorkflow: {
        ribbon->showContextCategory(mContextWorkflow);
        ribbon->hideContextCategory(mContextDataFrame);
        ribbon->hideContextCategory(mContextChart);
    } break;
    case ContextCategoryChart: {
        ribbon->hideContextCategory(mContextDataFrame);
        ribbon->hideContextCategory(mContextWorkflow);
        ribbon->showContextCategory(mContextChart);
    } break;
    case AllContextCategory: {
        ribbon->showContextCategory(mContextDataFrame);
        ribbon->showContextCategory(mContextWorkflow);
        ribbon->showContextCategory(mContextChart);
    } break;
    default:
        break;
    }
}

/**
 * @brief 隐藏上下文
 * @param type
 */
void DAAppRibbonArea::hideContextCategory(DAAppRibbonArea::ContextCategoryType type)
{
    SARibbonBar* ribbon = ribbonBar();
    switch (type) {
    case ContextCategoryData: {
        ribbon->hideContextCategory(mContextDataFrame);
    } break;
    case ContextCategoryWorkflow: {
        ribbon->hideContextCategory(mContextWorkflow);
    } break;
    case ContextCategoryChart: {
        ribbon->hideContextCategory(mContextChart);
    } break;
    case AllContextCategory: {
        ribbon->hideContextCategory(mContextDataFrame);
        ribbon->hideContextCategory(mContextWorkflow);
        ribbon->hideContextCategory(mContextChart);
    } break;
    default:
        break;
    }
}
/**
 * @brief 设置DataFrame的类型，[Context Category - dataframe] [Type] -> Type
 * @param d
 */
void DAAppRibbonArea::setDataframeOperateCurrentDType(const DAPyDType& d)
{
    // 先阻塞
    QSignalBlocker blocker(mComboxColumnTypes);
    Q_UNUSED(blocker);
    mComboxColumnTypes->setCurrentDType(d);
}

/**
 * @brief 构建 AI分析 标签页
 *
 * 布局：| agent管理(large) | agent gallery(最大宽度700px) | 执行agent(large) |
 * gallery 每个 action 对应一个 agent 提示词，选中后由"执行agent"触发 runAgent。
 */
void DAAppRibbonArea::buildRibbonAgentCategory()
{
    mCategoryAgent = ribbonBar()->addCategoryPage(tr("AI Agent"));  // cn:AI智能体
    mCategoryAgent->setObjectName(QString::fromUtf8(UiNames::Ribbon::AgentCategory));
    mPanelAgent = mCategoryAgent->addPanel(tr("AI Agent"));  // cn:AI智能体

    // agent 管理（large button）
    mActionAgentManage = new QAction(QIcon(":/da/icon/agent-manage.svg"), tr("Agent Manager"), this);  // cn:agent管理
    mActionAgentManage->setToolTip(tr("Manage agents: add, edit, delete prompts"));  // cn:管理 agent：新增、修改、删除提示词
    mPanelAgent->addLargeAction(mActionAgentManage);

    // agent gallery（最大宽度 700px）
    mAgentGallery = mPanelAgent->addGallery(true);
    mAgentGallery->setMaximumWidth(700);

    // 执行 agent（large button）
    mActionRunAgent = new QAction(QIcon(":/da/icon/run-agent.svg"), tr("Run Agent"), this);  // cn:执行agent
    mActionRunAgent->setToolTip(
        tr("Run AI analysis with the selected agent prompt"));  // cn:使用当前选中的 agent 提示词执行 AI 分析
    mPanelAgent->addLargeAction(mActionRunAgent);

    populateAgentGallery();

    connect(mActionAgentManage, &QAction::triggered, this, &DAAppRibbonArea::onActionAgentManage);
    connect(mActionRunAgent, &QAction::triggered, this, &DAAppRibbonArea::onActionRunAgent);
}

/**
 * @brief 填充 agent gallery，从 DAAgentInterface::agentPromptOps 取提示词列表
 */
void DAAppRibbonArea::populateAgentGallery()
{
    if (!mAgentGallery) {
        return;
    }
    // 清空旧 action
    for (QAction* act : std::as_const(mAgentActions)) {
        act->deleteLater();
    }
    mAgentActions.clear();

    static QIcon sAgentIcon(":/da/icon/agent.svg");
    DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface();
    QList< DA::DAAgentPrompt > prompts;
    if (agent) {
        DA::DAAgentPromptOps* ops = agent->agentPromptOps();
        if (ops) {
            prompts = ops->agentPrompts();
        }
    }
    for (const DA::DAAgentPrompt& a : std::as_const(prompts)) {
        QAction* act = new QAction(sAgentIcon, a.title, this);
        // tooltip 显示完整提示词内容（富文本换行）
        QString tooltip =
            QString("<html><body><div style=\"white-space:pre-wrap; max-width:480px;\">%1</div></body></html>")
                .arg(a.content.toHtmlEscaped());
        act->setToolTip(tooltip);
        act->setData(a.title);
        mAgentActions.append(act);
    }
    if (!mAgentGalleryGroup) {
        // 首次构建：添加分组 + 样式 + 连接信号
        mAgentGalleryGroup = mAgentGallery->addCategoryActions(tr("Agent"), mAgentActions);  // cn:Agent
        mAgentGalleryGroup->setGalleryGroupStyle(SARibbonGalleryGroup::IconWithWordWrapText);
        mAgentGalleryGroup->setGridMinimumWidth(80);
        connect(mAgentGalleryGroup, &SARibbonGalleryGroup::triggered, this, &DAAppRibbonArea::onAgentGalleryTriggered);
    } else {
        // 已有分组：清空 model 重新填充
        SARibbonGalleryGroupModel* model = mAgentGalleryGroup->groupModel();
        model->clear();
        mAgentGalleryGroup->addActionItemList(mAgentActions);
    }
    // 默认选中第一个 agent
    if (!mAgentActions.isEmpty()) {
        onAgentGalleryTriggered(mAgentActions.first());
    } else {
        mSelectedAgentTitle.clear();
    }
}

/**
 * @brief agent 管理按钮：打开管理对话框，关闭后刷新 gallery
 */
void DAAppRibbonArea::onActionAgentManage()
{
    DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface();
    if (!agent) {
        return;
    }
    DA::DAAgentPromptOps* ops = agent->agentPromptOps();
    if (!ops) {
        return;
    }
    DA::DAAgentManagerDialog dlg(ops, agent, app());
    QString prevTitle = mSelectedAgentTitle;
    dlg.exec();
    // 管理结束后刷新 gallery（agentPrompts 直接读磁盘最新状态）
    populateAgentGallery();
    if (!prevTitle.isEmpty()) {
        mSelectedAgentTitle = prevTitle;
    }
}

/**
 * @brief gallery 选中项变化：记录当前选中的 agent 标题
 */
void DAAppRibbonArea::onAgentGalleryTriggered(QAction* act)
{
    if (!act) {
        return;
    }
    mSelectedAgentTitle = act->data().toString();
}

/**
 * @brief 执行 agent 按钮：调用 DAAgentInterface::runAgent 触发 AI 分析
 */
void DAAppRibbonArea::onActionRunAgent()
{
    if (mSelectedAgentTitle.isEmpty()) {
        QMessageBox::warning(app(),
                             tr("Tip"),                                           // cn:提示
                             tr("Please select an agent in the gallery first"));  // cn:请先在 gallery 中选择一个 agent
        return;
    }
    DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface();
    if (!agent) {
        QMessageBox::warning(app(),
                             tr("Tip"),                         // cn:提示
                             tr("Agent module is not ready"));  // cn:Agent 模块未就绪
        return;
    }
    mActions->actionShowAgentArea->trigger();  // 确保 Agent dock 可见（ActionModeShow，不会 toggle 隐藏）
    agent->runAgent(mSelectedAgentTitle);
}
