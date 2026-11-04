/**
 * @file DAAppRibbonArea.cpp
 * @brief DAAppRibbonArea 的协调者实现
 *
 * 本文件只负责「装配与调度」：
 * - 下拉菜单的创建（buildMenu）
 * - 固定标签页：主页、数据、视图、绘图（内容较短，直接写在本文件）
 * - 整体装配顺序（buildRibbon）与上下文标签页的互斥显隐
 * - 撤销/重做 action 的生成、快速访问栏
 * - Backstage 应用面板与右上角按钮区
 * - 全量界面文本重置（resetText）
 *
 * 按标签页拆出的实现在同目录的其他 .cpp 中，文件划分见 DAAppRibbonArea.h 的类注释。
 *
 * 依赖注入约定：所有 `build*` 函数都不读 `mContextXxx` 之类的隐式状态，
 * 父容器（SARibbonBar / SARibbonContextCategory）由调用方通过参数传入。
 */
#include "DAAppRibbonArea.h"
#include "DAUiObjectNames.h"
#include "AppMainWindow.h"
// Qt
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QToolButton>
// SARibbon
#include "SARibbonMainWindow.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonLineWidgetContainer.h"
#include "SARibbonApplicationButton.h"
#include "SARibbonQuickAccessBar.h"
#include "SARibbonMenu.h"
#include "SARibbonColorToolButton.h"
// ui
#include "DAAppApplicationWidget.h"
// api
#include "DAAppUI.h"
#include "DAAppCommand.h"
#include "DAAppDockingArea.h"
#include "DAAppActions.h"

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
 * @brief 构建所有的下拉菜单
 *
 * @note 必须先于 buildRibbon() 调用：表格/绘图页的构建会把这些菜单通过
 *       action->setMenu() 挂到按钮上。
 * @note 菜单的 objectName 同样属于插件契约（见 DAUiObjectNames.h），不要随意修改。
 */
void DAAppRibbonArea::buildMenu()
{
    mMenus.insertRow = new SARibbonMenu(mApp);
    mMenus.insertRow->setObjectName(QString::fromUtf8(UiNames::Menu::InsertRow));
    mMenus.insertRow->addAction(mActions->actionInsertRowAbove);

    mMenus.insertColumn = new SARibbonMenu(mApp);
    mMenus.insertColumn->setObjectName(QString::fromUtf8(UiNames::Menu::InsertColumn));
    mMenus.insertColumn->addAction(mActions->actionInsertColumnLeft);
    // 填充下拉菜单：向下填充、向上填充、向下填充空值、向上填充空值、序列填充，挂在 actionFill 下
    mMenus.fill = new SARibbonMenu(mApp);
    mMenus.fill->setObjectName(QString::fromUtf8(UiNames::Menu::Fill));
    mMenus.fill->addAction(mActions->actionFillDown);
    mMenus.fill->addAction(mActions->actionFillUp);
    mMenus.fill->addAction(mActions->actionFillForward);
    mMenus.fill->addAction(mActions->actionFillBackward);
    mMenus.fill->addSeparator();
    mMenus.fill->addAction(mActions->actionFillSeries);
    // 批注下拉菜单：插入批注、编辑批注、删除批注、清除本表批注，挂在 actionNote 下
    mMenus.note = new SARibbonMenu(mApp);
    mMenus.note->setObjectName(QString::fromUtf8(UiNames::Menu::Note));
    mMenus.note->addAction(mActions->actionInsertNote);
    mMenus.note->addAction(mActions->actionEditNote);
    mMenus.note->addAction(mActions->actionDeleteNote);
    mMenus.note->addSeparator();
    mMenus.note->addAction(mActions->actionClearNotes);
    // 冻结列下拉菜单：冻结首列、冻结前 N 列（追加）、取消全部冻结，挂在 actionFreezeColumns 下
    mMenus.freezeColumns = new SARibbonMenu(mApp);
    mMenus.freezeColumns->setObjectName(QString::fromUtf8(UiNames::Menu::FreezeColumns));
    mMenus.freezeColumns->addAction(mActions->actionFreezeFirstColumn);
    mMenus.freezeColumns->addAction(mActions->actionFreezeColumnsPrompt);
    mMenus.freezeColumns->addSeparator();
    mMenus.freezeColumns->addAction(mActions->actionUnfreezeColumns);
    // 排序下拉菜单：升序、降序、自定义排序…、清除排序，挂在 actionSort 下
    // （面板大按钮主体直接打开自定义排序对话框，与 Excel 的「排序」按钮一致）
    mMenus.sort = new SARibbonMenu(mApp);
    mMenus.sort->setObjectName(QString::fromUtf8(UiNames::Menu::Sort));
    mMenus.sort->addAction(mActions->actionSortAscending);
    mMenus.sort->addAction(mActions->actionSortDescending);
    mMenus.sort->addSeparator();
    mMenus.sort->addAction(mActions->actionSortCustom);
    mMenus.sort->addSeparator();
    mMenus.sort->addAction(mActions->actionSortClear);
    // 数据导出格式菜单（插件加载后还会向其追加导出动作）
    mMenus.exportData = new SARibbonMenu(mApp);
    mMenus.exportData->setObjectName(QString::fromUtf8(UiNames::Menu::ExportData));
    mMenus.exportData->setIcon(QIcon(QStringLiteral(":/app/bright/Icon/exportToPng.svg")));
    mMenus.exportData->addAction(mActions->actionExportDataCsv);
    mMenus.exportData->addAction(mActions->actionExportDataExcel);
    mMenus.exportData->addAction(mActions->actionExportDataPickle);
    mMenus.exportData->addAction(mActions->actionExportDataParquet);
    // 工作流场景导出为图片
    mMenus.exportWorkflowSceneToImage = new SARibbonMenu(mApp);
    mMenus.exportWorkflowSceneToImage->setObjectName(QString::fromUtf8(UiNames::Menu::ExportWorkflowSceneToImage));
    mMenus.exportWorkflowSceneToImage->setIcon(QIcon(QStringLiteral(":/app/bright/Icon/exportToPic.svg")));
    mMenus.exportWorkflowSceneToImage->setDefaultAction(mActions->actionExportWorkflowSceneToImage);
    mMenus.exportWorkflowSceneToImage->addAction(mActions->actionExportWorkflowSceneToPNG);
    // 图表的拾取设置
    mMenus.chartPickSetting = new QMenu(mApp);
    mMenus.chartPickSetting->setObjectName(QString::fromUtf8(UiNames::Menu::ChartPickSetting));
    mMenus.chartPickSetting->addAction(mActions->actionChartPickerTextAtLeftTop);
    mMenus.chartPickSetting->addAction(mActions->actionChartPickerTextAtLeftBottom);
    mMenus.chartPickSetting->addAction(mActions->actionChartPickerTextAtRightTop);
    mMenus.chartPickSetting->addAction(mActions->actionChartPickerTextAtRightBottom);
    mMenus.chartPickSetting->addAction(mActions->actionChartPickerTextFollowMouse);
    mMenus.chartPickSetting->addSeparator();
    mMenus.chartPickSetting->addAction(mActions->actionChartYPickerShowXValueEnabled);
    mMenus.chartPickSetting->addSeparator();
    mMenus.chartPickSetting->addAction(mActions->actionChartDataPickerSetting);
    // 图例位置（上/下/左/右互斥，勾选状态由 updateChartLegendAboutRibbon 同步）
    mMenus.chartLegendPosition = new QMenu(mApp);
    mMenus.chartLegendPosition->setObjectName(QString::fromUtf8(UiNames::Menu::ChartLegendPosition));
    mMenus.chartLegendPosition->addAction(mActions->actionChartLegendAtTop);
    mMenus.chartLegendPosition->addAction(mActions->actionChartLegendAtBottom);
    mMenus.chartLegendPosition->addAction(mActions->actionChartLegendAtLeft);
    mMenus.chartLegendPosition->addAction(mActions->actionChartLegendAtRight);
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
 *
 * @note 本类的界面文本集中在这里设置。新增面板或改面板用途时请同步本函数，
 *       否则切换语言后该面板标题不会更新。
 */
void DAAppRibbonArea::resetText()
{
    ribbonBar()->applicationButton()->setText(tr("File"));  // cn:文件

    mCategoryMain->setCategoryName(tr("Main"));           // cn:主页
    mPannelMainFileOpt->setPanelName(tr("File"));         // cn:文件
    mPannelMainClipboard->setPanelName(tr("Clipboard"));  // cn:剪贴板
    mPannelMainCreate->setPanelName(tr("Create"));        // cn:创建
    mPannelSetting->setPanelName(tr("Config"));           // cn:配置
    mCategoryData->setCategoryName(tr("Data"));           // cn:数据
    mPannelDataOperate->setPanelName(tr("Dataset"));      // cn:数据集
    mPannelDataExport->setPanelName(tr("Export"));        // cn:导出
    mMenus.exportData->setTitle(tr("Export Format"));     // cn:导出格式

    mCategoryView->setCategoryName(tr("View"));        // cn:视图
    mPannelViewMainView->setPanelName(tr("Display"));  // cn:视图显示
    mPannelViewLayout->setPanelName(tr("Layout"));     // cn:布局

    mContextDataFrame->setContextTitle(tr("DataFrame"));                      // cn:DataFrame
    mCategoryDataframeTableOperate->setCategoryName(tr("Table Operations"));  // cn:表格操作
    mPannelDataframeOperateAxes->setPanelName(tr("Rows & Columns"));           // cn:行与列
    mPannelDataframeOperateColumn->setPanelName(tr("Column"));                // cn:列
    mPannelDataframeOperateDType->setPanelName(tr("Type & Format"));          // cn:类型与格式
    mPannelDataframeOperateEdit->setPanelName(tr("Edit"));                    // cn:编辑
    mPannelDataframeOperateView->setPanelName(tr("View"));                    // cn:视图
    mCategoryDataframeDataOperate->setCategoryName(tr("Data Operations"));    // cn:数据操作
    mPannelDataframeDataOperateSortFilter->setPanelName(tr("Sort & Filter"));   // cn:排序与筛选
    mPannelDataframeDataOperateComputed->setPanelName(tr("Computed Column"));  // cn:计算列
    mPannelDataframeDataOperateTransform->setPanelName(tr("Data Transform"));  // cn:数据变换
    mPannelDataframeDataOperateValidation->setPanelName(tr("Data Validation"));  // cn:数据校验
    mCategoryDataframeStyle->setCategoryName(tr("Table Style"));               // cn:表格样式
    mPannelDataframeStyleFill->setPanelName(tr("Fill"));                       // cn:底色
    mPannelDataframeStyleFont->setPanelName(tr("Font"));                       // cn:字体
    mPannelDataframeStyleClear->setPanelName(tr("Clear"));                     // cn:清除
    mPannelDataframeStyleConditional->setPanelName(tr("Conditional"));         // cn:条件格式
    mComboxColumnTypesContainer->setPrefix(tr("Type"));                        // cn:类型
    mComboxDisplayFormatContainer->setPrefix(tr("Format"));                    // cn:格式

    mContextWorkflow->setContextTitle(tr("Workflow"));  // cn:工作流
    // 表格
    mBtnTableFillColor->setText(tr("Fill Color"));  // cn:填充颜色

    mCategoryWorkflowGraphicsEdit->setCategoryName(tr("Workflow Edit"));  // cn:工作流编辑
    mCategoryWorkflowGraphicsView->setCategoryName(tr("Workflow View"));  // cn:工作流视图
    mPannelClipBoard->setPanelName(tr("Clipboard"));                      // cn:剪贴板
    mPannelWorkflowItem->setPanelName(tr("Item"));                        // cn:图元
    mPannelWorkflowText->setPanelName(tr("Text"));                        // cn:文本
    mPannelWorkflowBackground->setPanelName(tr("Background"));            // cn:背景
    mPannelWorkflowGroup->setPanelName(tr("Group"));                      // cn:分组
    mPannelWorkflowView->setPanelName(tr("View"));                        // cn:视图
    mPannelWorkflowExport->setPanelName(tr("Export"));                    // cn:导出

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
    // 统计上下文标签（统计窗口 dock 显示时出现）
    if (mContextStats) {
        mContextStats->setContextTitle(tr("Statistics"));               // cn:统计
        mCategoryStats->setCategoryName(tr("Statistics Operations"));   // cn:统计操作
        mPannelStatsParams->setPanelName(tr("Statistics Parameters"));  // cn:统计参数
        mPannelStatsTests->setPanelName(tr("Tests"));                   // cn:检验
        mPannelContextStatsPlot->setPanelName(tr("Plots"));             // cn:绘图
    }
    //
    mMenus.viewLineMarkers->setTitle(tr("View Marker"));    // cn:视图标记
    mMenus.viewLineMarkers->setToolTip(tr("View Marker"));  // cn:视图标记
    //
    mMenus.exportWorkflowSceneToImage->setTitle(tr("Export Image"));  // cn:导出为图片
    mMenus.exportWorkflowSceneToImage->setToolTip(
        tr("Export Workflow Graphics Scene To Image"));  // cn:把工作流的场景导出为图片
}

/**
 * @brief 构建 ribbon 的总入口
 *
 * 装配顺序有语义，调整前先看每一步的约束：
 * 1. 固定标签页：动作全部来自 DAAppActions，彼此独立
 * 2. 上下文标签页：表格、工作流、绘图各自建一个 SARibbonContextCategory；
 *    统计页单独由 dock 可见性驱动，不参与互斥显隐
 * 3. 最后挂应用面板（Backstage）与右上角按钮区，它们要求 ribbonBar 已就绪
 */
void DAAppRibbonArea::buildRibbon()
{
    // 顶层父容器只取一次，之后全部通过参数注入给各页面构建函数
    SARibbonBar* ribbon = ribbonBar();

    ribbon->showMinimumModeButton();
    // 固定标签页
    buildRibbonMainCategory(ribbon);
    buildRibbonDataCategory(ribbon);
    buildRibbonViewCategory(ribbon);
    buildRibbonFigureCategory(ribbon);
    buildRibbonAgentCategory(ribbon);
    buildRibbonQuickAccessBar();
    // 上下文标签页
    buildContextCategoryTable(ribbon);
    buildContextCategoryWorkflow(ribbon);
    buildContextCategoryChart(ribbon);
    // 统计页的动作由统计窗口持有，这里只注入窗口指针；窗口未就绪时该函数自行跳过
    buildContextCategoryStats(ribbon, mDockArea ? mDockArea->getDataStatsWidget() : nullptr);
    // 应用面板（Backstage）与右上角按钮区
    buildApplicationWidget();
    buildRightButtonBar(ribbon);
}

/**
 * @brief 构建主页标签
 * objectname=da-ribbon-category-main
 *
 * 结构：文件、剪贴板、创建、配置；运行/停止只在工作流上下文的运行页提供。
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildRibbonMainCategory(SARibbonBar* ribbon)
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

    ribbon->addCategoryPage(mCategoryMain);  // 主页
}

/**
 * @brief 构建数据标签
 * objectname=da-ribbon-category-data
 *
 * 结构：数据集、导出。
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildRibbonDataCategory(SARibbonBar* ribbon)
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
    mActions->actionExportData->setMenu(mMenus.exportData);

    mPannelDataExport = new SARibbonPanel(mCategoryData);
    mPannelDataExport->setObjectName(QString::fromUtf8(UiNames::Ribbon::DataExportPanel));
    mPannelDataExport->addLargeAction(mActions->actionExportData, QToolButton::MenuButtonPopup);
    mCategoryData->addPanel(mPannelDataExport);

    ribbon->addCategoryPage(mCategoryData);
}

/**
 * @brief 构建视图标签
 * objectname=da-ribbon-category-view
 *
 * 结构：显示、布局。（原外观面板已删除，Ribbon 主题切换由 设置→常规 页提供）
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildRibbonViewCategory(SARibbonBar* ribbon)
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

    ribbon->addCategoryPage(mCategoryView);  // 视图
}

/**
 * @brief 构建 ribbon 的快速访问栏
 *
 * @note 当前没有额外按钮：撤销/重做由 buildRedoUndo() 直接挂到快速访问栏上
 *       （见 setDockingArea 的调用顺序）。保留此函数作为后续增加固定快捷按钮的入口。
 */
void DAAppRibbonArea::buildRibbonQuickAccessBar()
{
}

/**
 * @brief 构建绘图标签
 * objectname=da-ribbon-category-figure
 *
 * 结构：添加绘图、统计绘图。
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildRibbonFigureCategory(SARibbonBar* ribbon)
{
    mCategoryFigure = ribbon->addCategoryPage(tr("Figure"));  // cn:绘图
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
 * @brief 设置dock区，构建ribbon及菜单
 *
 * 装配顺序：菜单 -> 撤销/重做动作 -> ribbon -> 文本。
 * 撤销/重做必须先于 ribbon 构建：主页剪贴板面板会直接引用这两个 action。
 * @param dock dock 区（由 DAAppUI 注入）
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
 *
 * 生成的 action 会挂到快速访问栏，并被主页剪贴板面板引用，
 * 因此本函数必须早于 buildRibbon() 调用。
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
 * @brief 显示上下文(会把其他上下文隐藏)
 * @param type 要显示的上下文类型
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
 * @param type 要隐藏的上下文类型
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
 * 原 4 款主题菜单已移除（主题切换迁移到 视图→外观面板，10 款完整枚举+持久化）
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildRightButtonBar(SARibbonBar* ribbon)
{
    ribbon->activeRightButtonGroup();
}
