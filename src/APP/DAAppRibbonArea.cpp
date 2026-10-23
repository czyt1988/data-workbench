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
#include "DAAppApplicationWidget.h"

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
// Agent 助手会话标签页宿主（session-tabs：新建会话/会话管理按钮入口）
#include "Agent/DAAgentDockWidget.h"

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
    // 填充下拉菜单：向下填充、向上填充、向下填充空值、向上填充空值、序列填充，挂在 actionFill 下
    mMenuFill = new SARibbonMenu(mApp);
    mMenuFill->setObjectName(QString::fromUtf8(UiNames::Menu::Fill));
    mMenuFill->addAction(mActions->actionFillDown);
    mMenuFill->addAction(mActions->actionFillUp);
    mMenuFill->addAction(mActions->actionFillForward);
    mMenuFill->addAction(mActions->actionFillBackward);
    mMenuFill->addSeparator();
    mMenuFill->addAction(mActions->actionFillSeries);
    // 批注下拉菜单：插入批注、编辑批注、删除批注、清除本表批注，挂在 actionNote 下
    mMenuNote = new SARibbonMenu(mApp);
    mMenuNote->setObjectName(QString::fromUtf8(UiNames::Menu::Note));
    mMenuNote->addAction(mActions->actionInsertNote);
    mMenuNote->addAction(mActions->actionEditNote);
    mMenuNote->addAction(mActions->actionDeleteNote);
    mMenuNote->addSeparator();
    mMenuNote->addAction(mActions->actionClearNotes);
    // 冻结列下拉菜单：冻结首列、冻结前 N 列、取消冻结，挂在 actionFreezeColumns 下
    mMenuFreezeColumns = new SARibbonMenu(mApp);
    mMenuFreezeColumns->setObjectName(QString::fromUtf8(UiNames::Menu::FreezeColumns));
    mMenuFreezeColumns->addAction(mActions->actionFreezeFirstColumn);
    mMenuFreezeColumns->addAction(mActions->actionFreezeColumnsPrompt);
    mMenuFreezeColumns->addSeparator();
    mMenuFreezeColumns->addAction(mActions->actionUnfreezeColumns);
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
    if (mApplicationWidget) {
        mApplicationWidget->retranslateUi();
    }
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
    mPannelDataOperate->setPanelName(tr("Dataset"));         // cn:数据集
    mPannelDataExport->setPanelName(tr("Export"));           // cn:导出
    mMenuExportData->setTitle(tr("Export Format"));          // cn:导出格式

    mCategoryView->setCategoryName(tr("View"));             // cn:视图
    mPannelViewMainView->setPanelName(tr("Display"));       // cn:视图显示
    mPannelViewLayout->setPanelName(tr("Layout"));          // cn:布局

    mContextDataFrame->setContextTitle(tr("DataFrame"));                // cn:DataFrame
    mCategoryDataframeTableOperate->setCategoryName(tr("Table Operations"));  // cn:表格操作
    mPannelDataframeOperateAxes->setPanelName(tr("Axes"));              // cn:坐标轴
    mPannelDataframeOperateColumn->setPanelName(tr("Column"));          // cn:列
    mPannelDataframeOperateDType->setPanelName(tr("Type"));             // cn:类型
    mPannelDataframeOperateFormat->setPanelName(tr("Format"));          // cn:格式
    mPannelDataframeOperateEdit->setPanelName(tr("Edit"));              // cn:编辑
    mPannelDataframeOperateView->setPanelName(tr("View"));              // cn:视图
    mCategoryDataframeDataOperate->setCategoryName(tr("Data Operations"));  // cn:数据操作
    mPannelDataframeDataOperateComputed->setPanelName(tr("Computed Column"));  // cn:计算列
    mPannelDataframeDataOperateTransform->setPanelName(tr("Data Transform"));  // cn:数据变换
    mCategoryDataframeStyle->setCategoryName(tr("Table Style"));        // cn:表格样式
    mPannelDataframeStyleFill->setPanelName(tr("Fill"));                // cn:底色
    mPannelDataframeStyleFont->setPanelName(tr("Font"));                // cn:字体
    mPannelDataframeStyleClear->setPanelName(tr("Clear"));              // cn:清除
    mPannelDataframeStyleConditional->setPanelName(tr("Conditional"));  // cn:条件格式
    mComboxColumnTypesContainer->setPrefix(tr("Type"));                 // cn:类型
    mComboxDisplayFormatContainer->setPrefix(tr("Format"));             // cn:格式

    mContextWorkflow->setContextTitle(tr("Workflow"));  // cn:工作流
    // 表格
    mBtnTableFillColor->setText(tr("Fill Color"));  // cn:填充颜色

    mCategoryWorkflowGraphicsEdit->setCategoryName(tr("Workflow Edit"));  // cn:工作流编辑
    mPannelClipBoard->setPanelName(tr("Clipboard"));                      // cn:剪贴板
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
    buildApplicationWidget();
    //
    buildRightButtonBar();
}

/**
 * @brief 构建主页标签
 * 主页的category objname = da-ribbon-category-main
 *
 * 结构：文件、剪贴板、创建、配置；运行/停止只在工作流上下文的运行页提供。
 */
void DAAppRibbonArea::buildRibbonMainCategory()
{
    mCategoryMain = new SARibbonCategory(app());
    mCategoryMain->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainCategory));

    // 文件面板：打开、保存、另存为
    mPannelMainFileOpt = new SARibbonPanel(mCategoryMain);
    mPannelMainFileOpt->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainCommonPanel));
    mPannelMainFileOpt->addLargeAction(mActions->actionOpen);
    mPannelMainFileOpt->addSmallAction(mActions->actionSave);
    mPannelMainFileOpt->addSmallAction(mActions->actionSaveAs);
    mCategoryMain->addPanel(mPannelMainFileOpt);

    // 剪贴板面板：撤销、重做、剪切、复制、粘贴、删除、全选
    // 撤销/重做由 QUndoGroup 生成（buildRedoUndo 中创建）；
    // 剪切、复制、粘贴、删除、全选由 DAAppController 按焦点路由到工作流、表格或图表
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

    // 创建面板：添加数据、添加绘图、新建工作流
    mPannelMainCreate = new SARibbonPanel(mCategoryMain);
    mPannelMainCreate->setObjectName(QString::fromUtf8(UiNames::Ribbon::MainCreatePanel));
    mPannelMainCreate->addLargeAction(mActions->actionAddData);
    mPannelMainCreate->addLargeAction(mActions->actionAddFigure);
    mPannelMainCreate->addLargeAction(mActions->actionWorkflowNew);
    mCategoryMain->addPanel(mPannelMainCreate);

    // 配置面板：设置、插件管理、关于
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
 * 结构：数据集、导出。
 */
void DAAppRibbonArea::buildRibbonDataCategory()
{
    mCategoryData = new SARibbonCategory(app());
    mCategoryData->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataCategory));

    // 数据集面板：添加数据、删除数据、重命名数据集

    mPannelDataOperate = new SARibbonPanel(mCategoryData);
    mPannelDataOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataOperatePanel));
    mPannelDataOperate->addLargeAction(mActions->actionAddData);
    mPannelDataOperate->addLargeAction(mActions->actionRemoveData);
    mPannelDataOperate->addLargeAction(mActions->actionRenameData);
    mCategoryData->addPanel(mPannelDataOperate);

    // 导出面板：导出数据（下拉选择 CSV、Excel、Pickle、Parquet）；
    // 插件加载后会追加导出单个数据、导出多个数据、导出到 Excel
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
 * 结构：显示、布局。（原外观面板已删除，Ribbon 主题切换由 设置→常规 页提供）
 */
void DAAppRibbonArea::buildRibbonViewCategory()
{
    mCategoryView = new SARibbonCategory(app());
    mCategoryView->setObjectName(QString::fromUtf8(UiNames::Ribbon::ViewCategory));

    // 显示面板：工作流区、工作流管理区、绘图区、绘图管理区、数据区、数据管理区、
    // 消息日志、撤销历史、设置、左右侧边栏、智能体区、数据联动表、Markdown 查看器
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
    mPannelViewMainView->addSmallAction(mActions->actionShowUndoHistory);  // 撤销历史：点击即打开并置顶
    mPannelViewMainView->addSmallAction(mActions->actionShowSettingWidget);
    mPannelViewMainView->addSmallAction(mActions->actionShowDataLinkTable);  // 数据联动表：默认隐藏，数据探针后自动显示
    mPannelViewMainView->addSmallAction(mActions->actionShowMarkdownView);  // Markdown 查看器：懒创建，关闭后可从此处找回
    mPannelViewMainView->addSeparator();
    mPannelViewMainView->addMediumAction(mActions->actionShowLeftSideBar);
    mPannelViewMainView->addMediumAction(mActions->actionShowRightSideBar);
    mPannelViewMainView->addSeparator();
    mPannelViewMainView->addLargeAction(mActions->actionShowAgentArea);
    mCategoryView->addPanel(mPannelViewMainView);

    // 布局面板：布局管理、恢复默认布局
    // 布局管理打开对话框，可保存、应用、删除布局方案；恢复默认布局前会弹确认框
    mPannelViewLayout = new SARibbonPanel(mCategoryView);
    mPannelViewLayout->setObjectName(QString::fromUtf8(UiNames::Ribbon::ViewLayoutPanel));
    mPannelViewLayout->addLargeAction(mActions->actionManageLayouts);
    mPannelViewLayout->addLargeAction(mActions->actionResetDefaultLayout);
    mCategoryView->addPanel(mPannelViewLayout);

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
 *
 * 结构：表格操作、数据操作、表格样式。
 */
void DAAppRibbonArea::buildContextCategoryDataFrame()
{
    mContextDataFrame = ribbonBar()->addContextCategory(tr("DataFrame"));  // cn:DataFrame
    mContextDataFrame->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameContextCategory));
    mCategoryDataframeTableOperate = mContextDataFrame->addCategoryPage(tr("Table Operations"));  // cn:表格操作
    mCategoryDataframeTableOperate->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateCategory));
    // 坐标轴面板：插入行（下拉含插入行到上方）、插入列（下拉含向左插入列）、删除单元格、删除行、删除列、更改列名、列转索引
    mPannelDataframeOperateAxes = mCategoryDataframeTableOperate->addPanel(tr("Axes"));  // cn:坐标轴
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
    // 列面板：重命名列、复制列名、定位最大、定位最小、描述统计
    mPannelDataframeOperateColumn = mCategoryDataframeTableOperate->addPanel(tr("Column"));  // cn:列
    mPannelDataframeOperateColumn->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateColumnPanel));
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionRenameColumn);
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionCopyColumnName);
    mPannelDataframeOperateColumn->addSeparator();
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionGotoMax);
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionGotoMin);
    mPannelDataframeOperateColumn->addSeparator();
    mPannelDataframeOperateColumn->addMediumAction(mActions->actionShowColumnDescribe);
    // 类型面板：列类型下拉框、转换为数值、转换为字符串、转换为日期
    mPannelDataframeOperateDType = mCategoryDataframeTableOperate->addPanel(tr("Type"));  // cn:类型
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

    // 格式面板：显示格式下拉框、设置单元格格式
    mPannelDataframeOperateFormat = mCategoryDataframeTableOperate->addPanel(tr("Format"));  // cn:格式
    mPannelDataframeOperateFormat->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateFormatPanel));
    mComboxDisplayFormatContainer = new SARibbonLineWidgetContainer(mPannelDataframeOperateFormat);
    mComboxDisplayFormat          = new DATableDisplayFormatComboBox(mComboxDisplayFormatContainer);
    mComboxDisplayFormatContainer->setPrefix(tr("Format"));  // cn:格式
    mComboxDisplayFormatContainer->setWidget(mComboxDisplayFormat);
    mPannelDataframeOperateFormat->addWidget(mComboxDisplayFormatContainer, SARibbonPanelItem::Medium);
    mPannelDataframeOperateFormat->addMediumAction(mActions->actionTableFormatCells);

    // 编辑面板：查找、替换、填充（下拉）、批注（下拉）
    mPannelDataframeOperateEdit = mCategoryDataframeTableOperate->addPanel(tr("Edit"));  // cn:编辑
    mPannelDataframeOperateEdit->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateEditPanel));
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionFind);
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionReplace);
    // 填充按钮点击即向下填充（快捷键 Ctrl+D）；下拉含向下、向上、向下填充空值、向上填充空值、序列填充
    mActions->actionFill->setMenu(mMenuFill);
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionFill, QToolButton::MenuButtonPopup);
    // 批注按钮点击即插入或编辑当前单元格批注；下拉含插入、编辑、删除、清除本表批注
    mActions->actionNote->setMenu(mMenuNote);
    mPannelDataframeOperateEdit->addLargeAction(mActions->actionNote, QToolButton::MenuButtonPopup);

    // 视图面板：冻结列（下拉）、隐藏列、显示所有列、列视图、定位
    // setMenu 必须在 addLargeAction 之前（mMenuInsertRow 同款顺序）
    mActions->actionFreezeColumns->setMenu(mMenuFreezeColumns);
    mPannelDataframeOperateView = mCategoryDataframeTableOperate->addPanel(tr("View"));  // cn:视图
    mPannelDataframeOperateView->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameOperateViewPanel));
    mPannelDataframeOperateView->addLargeAction(mActions->actionFreezeColumns, QToolButton::MenuButtonPopup);
    mPannelDataframeOperateView->addLargeAction(mActions->actionHideColumn);
    mPannelDataframeOperateView->addLargeAction(mActions->actionShowAllColumns);
    mPannelDataframeOperateView->addLargeAction(mActions->actionColumnViewDialog);
    // 定位：跳转到指定行、索引标签或列；快捷键在 DADataTableView 中处理，action 本身不设快捷键
    mPannelDataframeOperateView->addMediumAction(mActions->actionGotoCell);

    // ===== 数据操作页：计算列、数据变换；插件的数据清洗、数据过滤、数据统计在加载后追加 =====
    // 核心数据加工入口；插件的数据清洗/过滤/统计面板在加载时追加到本页
    mCategoryDataframeDataOperate = mContextDataFrame->addCategoryPage(tr("Data Operations"));  // cn:数据操作
    mCategoryDataframeDataOperate->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateCategory));

    // 计算列面板：计算列、全部重算
    mPannelDataframeDataOperateComputed =
        mCategoryDataframeDataOperate->addPanel(tr("Computed Column"));  // cn:计算列
    mPannelDataframeDataOperateComputed->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateComputedPanel));
    mPannelDataframeDataOperateComputed->addLargeAction(mActions->actionComputedColumn);
    mPannelDataframeDataOperateComputed->addMediumAction(mActions->actionRecomputeAllComputedColumns);

    // 数据变换面板：分列、合并列、分组聚合、转置、宽转长、校验规则、执行校验
    mPannelDataframeDataOperateTransform = mCategoryDataframeDataOperate->addPanel(tr("Data Transform"));  // cn:数据变换
    mPannelDataframeDataOperateTransform->setObjectName(
        QString::fromUtf8(UiNames::Ribbon::DataFrameDataOperateTransformPanel));
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionSplitColumn);
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionMergeColumns);
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionGroupBy);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionTranspose);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionMelt);
    // 校验规则管理，以及按启用规则校验全表（只读，不进撤销栈）
    mPannelDataframeDataOperateTransform->addSeparator();
    mPannelDataframeDataOperateTransform->addLargeAction(mActions->actionValidationRules);
    mPannelDataframeDataOperateTransform->addMediumAction(mActions->actionRunValidation);

    // ===== 表格样式页：底色、字体、清除、条件格式 =====
    mCategoryDataframeStyle = mContextDataFrame->addCategoryPage(tr("Table Style"));  // cn:表格样式
    mCategoryDataframeStyle->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleCategory));

    // 底色面板：设置选中单元格或区域的填充颜色
    mPannelDataframeStyleFill = mCategoryDataframeStyle->addPanel(tr("Fill"));  // cn:底色
    mPannelDataframeStyleFill->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFillPanel));
    mBtnTableFillColor = new SARibbonColorToolButton(mPannelDataframeStyleFill);
    mBtnTableFillColor->setColorStyle(SARibbonColorToolButton::ColorFillToIcon);
    mBtnTableFillColor->setupStandardColorMenu();
    mPannelDataframeStyleFill->addWidget(mBtnTableFillColor, SARibbonPanelItem::Large);

    // 字体面板：设置字体、字号、颜色等
    mPannelDataframeStyleFont = mCategoryDataframeStyle->addPanel(tr("Font"));  // cn:字体
    mPannelDataframeStyleFont->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleFontPanel));
    mWidgetTableFont = new DAFontEditPannelWidget(mPannelDataframeStyleFont);
    mPannelDataframeStyleFont->addWidget(mWidgetTableFont, SARibbonPanelItem::Large);

    // 清除面板：清除选中样式、清除所有样式
    mPannelDataframeStyleClear = mCategoryDataframeStyle->addPanel(tr("Clear"));  // cn:清除
    mPannelDataframeStyleClear->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleClearPanel));
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleSelected);
    mPannelDataframeStyleClear->addLargeAction(mActions->actionClearStyleAll);

    // 条件格式面板：打开条件格式设置，与表体右键菜单中的「条件格式」共用同一个页面
    mPannelDataframeStyleConditional = mCategoryDataframeStyle->addPanel(tr("Conditional"));  // cn:条件格式
    mPannelDataframeStyleConditional->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataFrameStyleConditionalPanel));
    // 点击按钮直接弹出条件格式预设菜单
    mPannelDataframeStyleConditional->addLargeAction(mActions->actionConditionalFormat, QToolButton::InstantPopup);
}

/**
 * @brief 构建绘图标签
 *
 * 结构：添加绘图、统计绘图。
 */
void DAAppRibbonArea::buildRibbonFigureCategory()
{
    mCategoryFigure = ribbonBar()->addCategoryPage(tr("Figure"));  // cn:绘图
    mCategoryFigure->setObjectName(QString::fromUtf8(UiNames::Ribbon::FigureCategory));

    // 添加绘图面板：折线图、散点图、误差棒图、箱线图、柱状图、多重柱状图、分布图、
    // 等高线图、云图、向量场图、三维曲面图、三维柱状图、三维线图
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

    // 统计绘图面板：直方图、一维核密度、二维核密度、箱线图、热力图、散点图、柱状图、回归图、累积分布
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

    // 剪贴板面板：粘贴、剪切、复制
    mPannelClipBoard = mCategoryWorkflowGraphicsEdit->addPanel(tr("Clipboard"));  // cn:剪贴板
    mPannelClipBoard->addLargeAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionPaste));
    mPannelClipBoard->addSmallAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCut));
    mPannelClipBoard->addSmallAction(wfo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCopy));
    // 图元面板：连线、轮廓与背景、字体
    mPannelWorkflowItem = mCategoryWorkflowGraphicsEdit->addPanel(tr("Item"));  // cn:图元
    mPannelWorkflowItem->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowItemPanel));
    mPannelWorkflowItem->addLargeAction(mActions->actionWorkflowLinkEnable);  // 连线
    mWorkflowShapeEditPannelWidget = new DAShapeEditPannelWidget(mPannelWorkflowItem);
    mPannelWorkflowItem->addWidget(mWorkflowShapeEditPannelWidget, SARibbonPanelItem::Large);
    mPannelWorkflowItem->addSeparator();
    mWorkflowFontEditPannel = new DAFontEditPannelWidget(mPannelWorkflowItem);
    mPannelWorkflowItem->addWidget(mWorkflowFontEditPannel, SARibbonPanelItem::Large);
    // 文本面板：绘制矩形、绘制文本
    mPannelWorkflowText = mCategoryWorkflowGraphicsEdit->addPanel(tr("Text"));  // cn:文本
    mPannelWorkflowText->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowTextPanel));
    mPannelWorkflowText->addLargeAction(mActions->actionWorkflowStartDrawRect);
    mPannelWorkflowText->addLargeAction(mActions->actionWorkflowStartDrawText);
    // 背景面板：添加背景图、锁定背景图、元件随背景移动
    mPannelWorkflowBackground = mCategoryWorkflowGraphicsEdit->addPanel(tr("Background"));  // cn:背景
    mPannelWorkflowBackground->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowBackgroundPanel));
    mPannelWorkflowBackground->addLargeAction(mActions->actionWorkflowAddBackgroundPixmap);
    mPannelWorkflowBackground->addMediumAction(mActions->actionWorkflowLockBackgroundPixmap);
    mPannelWorkflowBackground->addMediumAction(mActions->actionWorkflowEnableItemMoveWithBackground);

    // 分组面板：分组、取消分组、联动移动
    mPannelWorkflowGroup = mCategoryWorkflowGraphicsEdit->addPanel(tr("Group"));  // cn:分组
    mPannelWorkflowGroup->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowGroupPanel));
    mPannelWorkflowGroup->addMediumAction(mActions->actionItemGrouping);
    mPannelWorkflowGroup->addMediumAction(mActions->actionItemUngroup);
    mPannelWorkflowGroup->addLargeAction(mActions->actionWorkflowEnableItemLinkageMove);
    //
    // 信号连接
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
    // 视图面板：锁定视图、显示网格、缩放适应、放大、缩小、视图标记（下拉）
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

    // 导出面板：导出为图片（下拉选择 PNG 等格式）
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
    // 运行面板：运行工作流、停止工作流
    mPannelWorkflowRun = mCategoryWorkflowRun->addPanel(tr("Run"));  // cn:运行
    mPannelWorkflowRun->setObjectName(QString::fromUtf8(UiNames::Ribbon::WorkflowRunPanel));
    mPannelWorkflowRun->addLargeAction(mActions->actionWorkflowRun);
    mPannelWorkflowRun->addLargeAction(mActions->actionWorkflowTerminate);
}

/**
 * @brief 构建chart上下文
 *
 * 两页结构：绘图样式、图表编辑。
 * 图例位置（上、下、左、右）作为显示图例按钮的下拉菜单。
 */
void DAAppRibbonArea::buildContextCategoryChartEdit()
{
    mContextChart = ribbonBar()->addContextCategory(tr("Chart Operate"));  // cn:绘图操作
    mContextChart->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartContextCategory));
    mCategoryChartStyle = mContextChart->addCategoryPage(tr("Chart Style"));  // cn:绘图样式
    mCategoryChartStyle->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartStyleCategory));
    // 图表设置面板：应用到所有图表、网格显示（总开关、X/Y 方向、X/Y 最小刻度）、
    // 拖动、禁止水平缩放、禁止垂直缩放、缩放开关、放大、缩小、显示全部、
    // 十字拾取、XY 拾取、Y 拾取、联动拾取、显示图例（下拉选择位置）
    mPannelChartSetting = new SARibbonPanel(mCategoryChartStyle);
    mPannelChartSetting->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditChartSettingPanel));
    mPannelChartSetting->addLargeAction(mActions->actionFigureSettingApplyAllChart);
    mPannelChartSetting->addSeparator();
    // 网格
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableGrid);
    mChartGridDirActionsButtonGroup = new SARibbonButtonGroupWidget(mPannelChartSetting);
    mChartGridDirActionsButtonGroup->addAction(mActions->actionChartEnableGridX);
    mChartGridDirActionsButtonGroup->addAction(mActions->actionChartEnableGridY);
    mPannelChartSetting->addSmallWidget(mChartGridDirActionsButtonGroup);
    mChartGridMinActionsButtonGroup = new SARibbonButtonGroupWidget(mPannelChartSetting);
    mChartGridMinActionsButtonGroup->addAction(mActions->actionChartEnableGridXMin);
    mChartGridMinActionsButtonGroup->addAction(mActions->actionChartEnableGridYMin);
    mPannelChartSetting->addSmallWidget(mChartGridMinActionsButtonGroup);
    // 拖动
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePan);
    mPannelChartSetting->addMediumAction(mActions->actionChartDisableZoomX);
    mPannelChartSetting->addMediumAction(mActions->actionChartDisableZoomY);
    // 缩放
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableZoom);
    mPannelChartSetting->addMediumAction(mActions->actionChartZoomIn);
    mPannelChartSetting->addMediumAction(mActions->actionChartZoomOut);
    mPannelChartSetting->addLargeAction(mActions->actionChartZoomAll);
    // 数据拾取
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerCross);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerXY);
    mActions->actionChartEnablePickerY->setMenu(mMenuChartPickSetting);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnablePickerY, QToolButton::MenuButtonPopup);
    mPannelChartSetting->addLargeAction(mActions->actionChartLinkAllPickerEnabled);
    // legend（下拉菜单为图例位置：上/下/左/右互斥）
    mActions->actionChartEnableLegend->setMenu(mMenuChartLegendPosition);
    mPannelChartSetting->addLargeAction(mActions->actionChartEnableLegend, QToolButton::MenuButtonPopup);

    mCategoryChartStyle->addPanel(mPannelChartSetting);

    // 绘图主题面板：主题画廊、复制绘图到剪贴板
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
    // 绘图窗口设置面板：添加绘图、调整子图大小、新建坐标系
    mPannelFigureSettingForContext = new SARibbonPanel(mCategoryChartEdit);
    mPannelFigureSettingForContext->setObjectName(QString::fromUtf8(UiNames::Ribbon::ChartEditFigureSettingPanel));
    mPannelFigureSettingForContext->addLargeAction(mActions->actionAddFigure);
    mPannelFigureSettingForContext->addLargeAction(mActions->actionChartEditorResizeSubChart);
    mPannelFigureSettingForContext->addLargeAction(mActions->actionFigureNewXYAxis);  // 新建坐标系
    mCategoryChartEdit->addPanel(mPannelFigureSettingForContext);
    // 选区工具面板：指针选择、矩形选框、椭圆选框、多边形选框
    mPannelChartSelectTool = mCategoryChartEdit->addPanel(tr("Select Tool"));  // cn:选区工具
    mPannelChartSelectTool->addLargeAction(mActions->actionChartEditorPointerSelector);
    mPannelChartSelectTool->addLargeAction(mActions->actionChartEditorRectSelector);
    mPannelChartSelectTool->addMediumAction(mActions->actionChartEditorEllipseSelector);
    mPannelChartSelectTool->addMediumAction(mActions->actionChartEditorPolygonSelector);
    // 辅助工具面板：十字标记、水平标记、垂直标记、箭头标记、文本标注、数据探针
    mPannelChartAssistTool = mCategoryChartEdit->addPanel(tr("Assist Tools"));  // cn:辅助工具
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorAddCrossMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddHLineMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddVLineMarker);
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorAddArrowMarker);
    mPannelChartAssistTool->addMediumAction(mActions->actionChartEditorAddTextMarker);
    mPannelChartAssistTool->addSeparator();
    // 数据探针（多子图联动拾取+点击建探针+数据联动表）
    mPannelChartAssistTool->addLargeAction(mActions->actionChartEditorDataProbe);
}

/**
 * @brief 构建应用面板并挂载到应用按钮
 * @details 面板替代原来的应用按钮菜单：点击应用按钮切换面板显隐，
 *          原菜单的功能全部收编进面板的各页签与底部动作
 */
void DAAppRibbonArea::buildApplicationWidget()
{
    mApplicationWidget = new DAAppApplicationWidget(app(), mActions);
    // 挂载后点击应用按钮自动切换面板显隐（见 SARibbonMainWindow::setApplicationWidget）
    app()->setApplicationWidget(mApplicationWidget);
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
    mActions->recordAction(mActions->actionRedo);
    mActions->actionUndo = undoGroup.createUndoAction(this);
    mActions->actionUndo->setObjectName(QString::fromUtf8(UiNames::Action::Undo));
    mActions->actionUndo->setIcon(QIcon(":/app/bright/Icon/undo.svg"));
    mActions->actionUndo->setShortcut(QKeySequence::Undo);
    mActions->recordAction(mActions->actionUndo);
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
    mActions->actionChartDisableZoomX->setChecked(!chart->isXAxisZoomEnabled());
    mActions->actionChartDisableZoomY->setChecked(!chart->isYAxisZoomEnabled());
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
    mActionAgentManage = new QAction(QIcon(":/da/icon/agent-manage.svg"), tr("Agent Manager"), this);  // cn:Agent 管理
    mActionAgentManage->setToolTip(tr("Manage agents: add, edit, delete prompts"));  // cn:管理 agent：新增、修改、删除提示词
    mPanelAgent->addLargeAction(mActionAgentManage);

    // agent gallery（最大宽度 700px）
    mAgentGallery = mPanelAgent->addGallery(true);
    mAgentGallery->setMaximumWidth(700);

    // 执行 agent（large button）
    mActionRunAgent = new QAction(QIcon(":/da/icon/run-agent.svg"), tr("Run Agent"), this);  // cn:执行 Agent
    mActionRunAgent->setToolTip(
        tr("Run AI analysis with the selected agent prompt"));  // cn:使用当前选中的 agent 提示词执行 AI 分析
    mPanelAgent->addLargeAction(mActionRunAgent);

    // 会话操作（session-tabs，large button）：与 Agent 助手标题栏「+/会话管理」同一入口
    mActionAgentNewSession = new QAction(QIcon(":/DAGui/icon/session-new.svg"), tr("New Session"), this);  // cn:新建会话
    mActionAgentNewSession->setToolTip(tr("Open a new agent chat session"));  // cn:打开一个新的 Agent 会话标签页
    mPanelAgent->addLargeAction(mActionAgentNewSession);
    mActionAgentSessionManager = new QAction(QIcon(":/DAGui/icon/session-manager.svg"), tr("Session Manager"), this);  // cn:会话管理
    mActionAgentSessionManager->setToolTip(
        tr("Manage agent sessions: switch, rename, delete"));  // cn:管理 Agent 会话：切换、重命名、删除
    mPanelAgent->addLargeAction(mActionAgentSessionManager);

    populateAgentGallery();

    connect(mActionAgentManage, &QAction::triggered, this, &DAAppRibbonArea::onActionAgentManage);
    connect(mActionRunAgent, &QAction::triggered, this, &DAAppRibbonArea::onActionRunAgent);
    connect(mActionAgentNewSession, &QAction::triggered, this, &DAAppRibbonArea::onActionAgentNewSession);
    connect(mActionAgentSessionManager, &QAction::triggered, this, &DAAppRibbonArea::onActionAgentSessionManager);
    // gallery 首次构建早于插件加载（createUi 先于 initPlugins），插件注入的内置
    // agent 依赖此信号兜底刷新（保存/删除/插件 registerBuiltinAgent 均触发）；
    // 保留当前选中项，与管理对话框关闭后的刷新行为一致
    if (DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface()) {
        connect(agent, &DA::DAAgentInterface::agentListChanged, this, [ this ]() {
            QString prevTitle = mSelectedAgentTitle;
            populateAgentGallery();
            if (!prevTitle.isEmpty()) {
                mSelectedAgentTitle = prevTitle;
            }
        });
    }
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

/**
 * @brief 新建会话按钮：确保 Agent dock 可见后走宿主「+」统一入口
 *
 * 与 Agent 助手标题栏「+」按钮同一入口（requestNewSession：已有 unbound 视图
 * 则 raise，否则创建懒建会话草稿区——首条消息发出后才落盘建会话）。
 */
void DAAppRibbonArea::onActionAgentNewSession()
{
    if (!mDockArea) {
        return;
    }
    mActions->actionShowAgentArea->trigger();  // 确保 Agent dock 可见
    if (DA::DAAgentDockWidget* dock = mDockArea->getAgentDockWidget()) {
        dock->requestNewSession();
    }
}

/**
 * @brief 会话管理按钮：确保 Agent dock 可见后走宿主「会话管理」统一入口
 *
 * 与 Agent 助手标题栏「会话管理」按钮同一入口（requestShowSessionManager：
 * 弹出 DADialogAgentSessionManager，操作经 Dock 信号链转发到接口层）。
 */
void DAAppRibbonArea::onActionAgentSessionManager()
{
    if (!mDockArea) {
        return;
    }
    mActions->actionShowAgentArea->trigger();  // 确保 Agent dock 可见
    if (DA::DAAgentDockWidget* dock = mDockArea->getAgentDockWidget()) {
        dock->requestShowSessionManager();
    }
}
