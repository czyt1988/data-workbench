#include "DAAppTableActionHandler.h"
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
#include <QIcon>
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
#include "DATextAlignmentEditWidget.h"
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
        mDeps.controller->app(),                                                                                                         \
        QCoreApplication::translate("DAAppRibbonArea", "Warning", nullptr),                                             \
        QCoreApplication::translate("DAAppRibbonArea",                                                                 \
                                    "The current function is not implemented, only the UI is reserved, "               \
                                    "please pay attention: https://gitee.com/czyt1988/data-work-flow",                 \
                                    nullptr))

// 快速链接信号槽
#define DAAPPCONTROLLER_ACTION_BIND(actionname, functionname)                                                          \
    connect(actionname, &QAction::triggered, this, &DAAppController::functionname)

// 表格域动作绑定
#define DAAPPTABLE_ACTION_BIND(actionname, functionname) \
    connect(actionname, &QAction::triggered, this, &DAAppTableActionHandler::functionname)

namespace DA
{

DAAppTableActionHandler::DAAppTableActionHandler(const DAAppActionHandlerDeps& deps, QObject* par)
    : QObject(par), mDeps(deps)
{
}

void DAAppTableActionHandler::initConnections()
{
    // 由控制器在 DAAppController::initConnection 中调用；绑定见下方各域分组
    // 数据操作的上下文标签 Data Operate Context Category
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionInsertRow, onActionInsertRowTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionInsertRowAbove, onActionInsertRowAboveTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionInsertColumnRight, onActionInsertColumnRightTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionInsertColumnLeft, onActionInsertColumnLeftTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionRemoveRow, onActionRemoveRowTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionRemoveColumn, onActionRemoveColumnTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionRemoveCell, onActionRemoveCellTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionManageColumns, onActionManageColumnsTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionRenameColumn, onActionRenameColumnTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionCopyColumnName, onActionCopyColumnNameTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionGotoMax, onActionGotoMaxTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionGotoMin, onActionGotoMinTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionShowColumnDescribe, onActionShowColumnDescribeTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionQuickPlot, onActionQuickPlotTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionCastToNum, onActionCastToNumTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionCastToString, onActionCastToStringTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionCastToDatetime, onActionCastToDatetimeTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionChangeToIndex, onActionChangeToIndexTriggered);
    // 表格样式
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionClearStyleSelected, onActionClearStyleSelectedTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionClearStyleAll, onActionClearStyleAllTriggered);
    // 表格列显示格式
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionTableFormatCells, onActionTableFormatCellsTriggered);
    // 单元格大小（A3）：主体 = 列宽自适应（下拉项挂在 Ribbon 侧预建的菜单上）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionCellSize, onActionCellSizeTriggered);
    // 行高（Table Style 视图面板）：主体 = 弹设置对话框（下拉紧凑/普通在下方构建）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionTableRowHeight, onActionRowHeightTriggered);
    // 表内查找/替换（002 T1.4，Ribbon「编辑」面板；快捷键在 DADataTableView::keyPressEvent，action 不设 shortcut 红线5）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFind, onActionFindTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionReplace, onActionReplaceTriggered);
    // 条件格式（003 T1.5，Ribbon「Table Style」面板；与表体右键菜单共用页面入口，R16）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionConditionalFormat, onActionConditionalFormatTriggered);
    // 数据工具（004，Ribbon「Data Tools」面板；计算列/分列/合并列与表体右键菜单共用页面入口）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionComputedColumn, onActionComputedColumnTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionRecomputeAllComputedColumns, onActionRecomputeAllComputedColumnsTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionSplitColumn, onActionSplitColumnTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionMergeColumns, onActionMergeColumnsTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionGroupBy, onActionGroupByTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionTranspose, onActionTransposeTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionMelt, onActionMeltTriggered);
    // 填充（005 T1.24，Ribbon「Edit」面板下拉；快捷键 Ctrl+D 在 DADataTableView，action 不设 shortcut 红线5）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFill, onActionFillDownTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFillDown, onActionFillDownTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFillUp, onActionFillUpTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFillForward, onActionFillForwardTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFillBackward, onActionFillBackwardTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFillSeries, onActionFillSeriesTriggered);
    // 数据校验（005 T1.25，Ribbon「Data Tools」面板；与表体右键菜单共用页面入口）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionValidationRules, onActionValidationRulesTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionRunValidation, onActionRunValidationTriggered);
    // 批注（005 T1.26，Ribbon「Edit」面板下拉；插入/编辑共用弹层入口）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionNote, onActionNoteTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionInsertNote, onActionNoteTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionEditNote, onActionNoteTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionDeleteNote, onActionDeleteNoteTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionClearNotes, onActionClearNotesTriggered);
    // 列视图（006 T1.8，Ribbon「View」面板；与列头右键「列」子菜单共用页面入口）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFreezeColumns, onActionFreezeColumnsTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionHideColumn, onActionHideColumnTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionShowAllColumns, onActionShowAllColumnsTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFreezeFirstColumn, onActionFreezeFirstColumnTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionFreezeColumnsPrompt, onActionFreezeColumnsPromptTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionUnfreezeColumns, onActionUnfreezeColumnsTriggered);
    // 定位（007 T1.7，Ribbon「View」面板；快捷键 Ctrl+G 在 DADataTableView 红线5）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionGotoCell, onActionGotoCellTriggered);
    // 排序（Ribbon「Sort」面板；升/降作用于当前列，大按钮主体与「自定义排序…」同一入口）
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionSort, onActionSortTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionSortAscending, onActionSortAscendingTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionSortDescending, onActionSortDescendingTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionSortCustom, onActionSortTriggered);
    DAAPPTABLE_ACTION_BIND(mDeps.actions->actionSortClear, onActionSortClearTriggered);
    // 列数据类型下拉：当前数据页的选中类型变化
    // 不知为何使用函数指针无法关联信号和槽
    //  connect(mComboxColumnTypes, &DAPyDTypeComboBox::currentDTypeChanged, this,&DAAppRibbonArea::onComboxColumnTypesCurrentDTypeChanged);
    //  QObject::connect: signal not found in DAPyDTypeComboBox
    connect(mDeps.ribbon->mComboxColumnTypes,
            &DAPyDTypeComboBox::currentDTypeChanged,
            this,
            &DAAppTableActionHandler::onComboxColumnTypesCurrentDTypeChanged);
    // 表格样式
    connect(mDeps.ribbon->mBtnTableFillColor,
            &SARibbonColorToolButton::colorChanged,
            this,
            &DAAppTableActionHandler::onTableStyleFillColorChanged);
    connect(mDeps.ribbon->mWidgetTableFont,
            &DAFontEditPannelWidget::currentFontChanged,
            this,
            &DAAppTableActionHandler::onTableStyleFontChanged);
    connect(mDeps.ribbon->mWidgetTableFont,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAAppTableActionHandler::onTableStyleFontColorChanged);
    // 对齐（A1）：水平/垂直按钮组点击即应用
    connect(mDeps.ribbon->mWidgetTableAlign,
            &DATextAlignmentEditWidget::alignmentChanged,
            this,
            &DAAppTableActionHandler::onTableStyleAlignmentChanged);
    // 自动换行（A4 轻量）：勾选开关切换即应用（反向同步时用 QSignalBlocker 防循环）
    connect(mDeps.actions->actionWrapText,
            &QAction::toggled,
            this,
            &DAAppTableActionHandler::onTableStyleWrapTextToggled);
    // 边框（A2）：预设菜单（aboutToShow 重填，随语言刷新；与条件格式菜单同款模式）
    {
        QMenu* borderMenu = new QMenu(mDeps.mainWindow);
        mDeps.actions->actionTableBorder->setMenu(borderMenu);
        connect(borderMenu, &QMenu::aboutToShow, this, [ this, borderMenu ]() {
            borderMenu->clear();
            // 每项配一个「单元格网格 + 高亮边」示意图标（样式对齐 Excel 的边框菜单）：
            // 浅灰 #B2B2B2 网格表示未设置的线，深蓝灰 #5D6D7E 表示本次设置的线。
            auto addPreset = [ this, borderMenu ](const QString& text, const QString& iconPath,
                                                 quint8 sides, bool outline) {
                QAction* act = borderMenu->addAction(QIcon(iconPath), text);
                connect(act, &QAction::triggered, this, [ this, sides, outline ]() {
                    applyBorderPreset(sides, outline);
                });
            };
            addPreset(QCoreApplication::translate("DA::DAAppController", "All Borders"),  // cn:全框线
                      QStringLiteral(":/DAGui/icon/cellBorders.svg"),
                      DA::DATableCellStyle::BorderAll, false);
            addPreset(QCoreApplication::translate("DA::DAAppController", "Outside Borders"),  // cn:外框线
                      QStringLiteral(":/DAGui/icon/cellBordersOutside.svg"),
                      0, true);
            borderMenu->addSeparator();
            addPreset(QCoreApplication::translate("DA::DAAppController", "No Border"),  // cn:无框线
                      QStringLiteral(":/DAGui/icon/cellBordersNone.svg"),
                      0, false);
            addPreset(QCoreApplication::translate("DA::DAAppController", "Top Border"),  // cn:上框线
                      QStringLiteral(":/DAGui/icon/cellBordersTop.svg"),
                      DA::DATableCellStyle::BorderTop, false);
            addPreset(QCoreApplication::translate("DA::DAAppController", "Bottom Border"),  // cn:下框线
                      QStringLiteral(":/DAGui/icon/cellBordersBottom.svg"),
                      DA::DATableCellStyle::BorderBottom, false);
            addPreset(QCoreApplication::translate("DA::DAAppController", "Left Border"),  // cn:左框线
                      QStringLiteral(":/DAGui/icon/cellBordersLeft.svg"),
                      DA::DATableCellStyle::BorderLeft, false);
            addPreset(QCoreApplication::translate("DA::DAAppController", "Right Border"),  // cn:右框线
                      QStringLiteral(":/DAGui/icon/cellBordersRight.svg"),
                      DA::DATableCellStyle::BorderRight, false);
        });
    }
    // 单元格大小（A3）：菜单由 Ribbon 侧先建好（保证下拉箭头），这里 aboutToShow 重填，
    // 与边框/条件格式菜单同款模式（随语言刷新）
    if (QMenu* cellSizeMenu = mDeps.actions->actionCellSize->menu()) {
        connect(cellSizeMenu, &QMenu::aboutToShow, this, [ this, cellSizeMenu ]() {
            cellSizeMenu->clear();
            QAction* actAutoFit =
                cellSizeMenu->addAction(QCoreApplication::translate("DA::DAAppController", "AutoFit Column Width"));  // cn:列宽自适应
            connect(actAutoFit, &QAction::triggered, this, &DAAppTableActionHandler::onActionCellSizeTriggered);
            QAction* actSetWidth =
                cellSizeMenu->addAction(QCoreApplication::translate("DA::DAAppController", "Set Column Width..."));  // cn:设置列宽...
            connect(actSetWidth, &QAction::triggered, this, [ this ]() {
                if (DADataOperateOfDataFrameWidget* w =
                        mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
                    w->showSetColumnWidthDialog();
                }
            });
            QAction* actSetHeight =
                cellSizeMenu->addAction(QCoreApplication::translate("DA::DAAppController", "Set Row Height..."));  // cn:设置行高...
            connect(actSetHeight, &QAction::triggered, this, [ this ]() {
                if (DADataOperateOfDataFrameWidget* w =
                        mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
                    w->showSetRowHeightDialog();
                }
            });
        });
    }
    // 行高（Table Style 视图面板）：菜单由 Ribbon 侧先建好（保证下拉箭头），这里 aboutToShow
    // 重填「紧凑/普通」，与边框/单元格大小菜单同款模式（随语言刷新）
    if (QMenu* rowHeightMenu = mDeps.actions->actionTableRowHeight->menu()) {
        connect(rowHeightMenu, &QMenu::aboutToShow, this, [ this, rowHeightMenu ]() {
            rowHeightMenu->clear();
            QAction* actCompact =
                rowHeightMenu->addAction(QCoreApplication::translate("DA::DAAppController", "Compact"));  // cn:紧凑
            connect(actCompact, &QAction::triggered, this, [ this ]() {
                if (DADataOperateOfDataFrameWidget* w =
                        mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
                    w->setCompactRowHeight();
                }
            });
            QAction* actNormal =
                rowHeightMenu->addAction(QCoreApplication::translate("DA::DAAppController", "Normal"));  // cn:普通
            connect(actNormal, &QAction::triggered, this, [ this ]() {
                if (DADataOperateOfDataFrameWidget* w =
                        mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
                    w->setNormalRowHeight();
                }
            });
        });
    }
    // 表格列显示格式
    connect(mDeps.ribbon->mComboxDisplayFormat,
            &DATableDisplayFormatComboBox::currentFormatCategoryChanged,
            this,
            &DAAppTableActionHandler::onTableDisplayFormatCategoryChanged);
    // WPS 式预设菜单：action 挂菜单后 Ribbon 按钮（InstantPopup）点击即弹；
    // 内容每次 aboutToShow 由当前数据页重填（作用列/惰性源可用性实时）
    {
        // QMenu 是 QWidget，父对象必须也是 QWidget：经 QObject::setParent 挂到非 widget 的对象上
        // 会触发 Qt Debug 断言 "!d->isWidget" 并直接中止进程（Release 版的父子语义同样不安全）
        QMenu* cfMenu = new QMenu(mDeps.mainWindow);
        mDeps.actions->actionConditionalFormat->setMenu(cfMenu);
        connect(cfMenu, &QMenu::aboutToShow, this, [ this, cfMenu ]() {
            cfMenu->clear();
            DADataOperateOfDataFrameWidget* dfopt =
                mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
            if (!dfopt) {
                QAction* na =
                    cfMenu->addAction(QCoreApplication::translate("DA::DAAppController", "No active table"));  // cn:无活动表格
                na->setEnabled(false);
                return;
            }
            dfopt->populateConditionalFormatMenu(cfMenu);
        });
    }
}
/**
 * @brief 选择的样式改变信号
 * @param column
 * @param dt
 */
void DAAppTableActionHandler::onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DAPyDType dt)
{
    Q_UNUSED(column);
    mDeps.ribbon->setDataframeOperateCurrentDType(dt);
    // 按 dtype 重建显示格式下拉框的可用类别
    if (mDeps.ribbon && mDeps.ribbon->mComboxDisplayFormat) {
        mDeps.ribbon->mComboxDisplayFormat->updateDType(dt);
    }
}

/**
 * @brief dataframe的列数据类型改变
 * @param index
 */
void DAAppTableActionHandler::onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt)
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->changeSelectColumnType(dt);
    }
}

/**
 * @brief dataframe删除行
 */
void DAAppTableActionHandler::onActionRemoveRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->removeSelectRow();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief dataframe删除列
 */
void DAAppTableActionHandler::onActionRemoveColumnTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->removeSelectColumn();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 移除单元格
 */
void DAAppTableActionHandler::onActionRemoveCellTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->removeSelectCell();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 插入行
 */
void DAAppTableActionHandler::onActionInsertRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->insertRowBelowBySelect();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 在选中位置上面插入一行
 */
void DAAppTableActionHandler::onActionInsertRowAboveTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->insertRowAboveBySelect();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 在选中位置右边插入一列
 */
void DAAppTableActionHandler::onActionInsertColumnRightTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->insertColumnRightBySelect();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 在选中位置左边插入一列
 */
void DAAppTableActionHandler::onActionInsertColumnLeftTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->insertColumnLeftBySelect();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 列管理（Ribbon「View」面板；列名/顺序/隐藏/冻结合一）
 */
void DAAppTableActionHandler::onActionManageColumnsTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->showManageColumnsDialog();
    }
}

/**
 * @brief dataframe单列重命名（表头右键）
 *
 * 弹出 QInputDialog 让用户输入新列名，调用 widget->renameColumn。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppTableActionHandler::onActionRenameColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    renameColumnOnPage(dfopt, col);
}

/**
 * @brief 在指定页面重命名列（表头右键直入口）
 *
 * 不经焦点检查、不依赖选区——右键菜单是独立窗口会抢走停靠区焦点，信号里
 * 携带的 (w, col) 即操作目标。弹输入框（预填当前列名）→ w->renameColumn。
 * @param w 目标数据页（信号接线的 lambda 捕获）
 * @param col 目标列（逻辑索引）
 */
void DAAppTableActionHandler::renameColumnOnPage(DADataOperateOfDataFrameWidget* w, int col)
{
    if (!w || col < 0) {
        return;
    }
    DADataTableView* tv = w->getDataTableView();
    if (!tv) {
        return;
    }
    QString oldName = tv->actualColumnName(col);
    bool ok         = false;
    QString newName = QInputDialog::getText(w,
                                            QCoreApplication::translate("DA::DAAppController", "Rename Column"),     // cn:重命名列
                                            QCoreApplication::translate("DA::DAAppController", "New column name:"),  // cn:新列名：
                                            QLineEdit::Normal,
                                            oldName,
                                            &ok);
    if (!ok) {
        return;
    }
    if (w->renameColumn(col, newName.trimmed())) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 复制列名到剪贴板（表头右键）
 *
 * 依赖表头右键菜单已先选中目标列（B-2-a），列名来源走 view 的 actualColumnName。
 */
void DAAppTableActionHandler::onActionCopyColumnNameTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
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
 * 取当前选中列的最大值位置索引，经定位单入口滚动并选中。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppTableActionHandler::onActionGotoMaxTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
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
        daWarning << QCoreApplication::translate("DA::DAAppController", "Cannot find the maximum value in this column (empty, all-NaN, or incomparable types)");  // cn:此列无法找到最大值（空列、全为NaN或类型不可比较）
        return;
    }
    // 选中并滚动到目标单元格（007：走定位单入口，顺带获得隐藏列/冻结区处置）
    dfopt->locateActualCell(static_cast< int >(pos), col);
    // 打印结果到信息栏
    {
        pybind11::gil_scoped_acquire gil;
        try {
            pybind11::object val = s.iat(pos);
            QString valStr       = pybind11::str(val).cast< QString >();
            daInfo << QCoreApplication::translate("DA::DAAppController", "Column [%1] maximum value: %2, row: %3").arg(colNm, valStr).arg(pos);  // cn:列[%1] 最大值: %2, 第 %3 行
        } catch (const std::exception& e) {
            qCritical() << e.what();
            daInfo << QCoreApplication::translate("DA::DAAppController", "Column [%1] maximum value at row %2").arg(colNm).arg(pos);  // cn:列[%1] 最大值位于第 %2 行
        }
    }
}

/**
 * @brief 跳转到最小值（表头右键）
 *
 * 取当前选中列的最小值位置索引，经定位单入口滚动并选中。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppTableActionHandler::onActionGotoMinTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
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
        daWarning << QCoreApplication::translate("DA::DAAppController", "Cannot find the minimum value in this column (empty, all-NaN, or incomparable types)");  // cn:此列无法找到最小值（空列、全为NaN或类型不可比较）
        return;
    }
    // 选中并滚动到目标单元格（007：走定位单入口，顺带获得隐藏列/冻结区处置）
    dfopt->locateActualCell(static_cast< int >(pos), col);
    // 打印结果到信息栏
    {
        pybind11::gil_scoped_acquire gil;
        try {
            pybind11::object val = s.iat(pos);
            QString valStr       = pybind11::str(val).cast< QString >();
            daInfo << QCoreApplication::translate("DA::DAAppController", "Column [%1] minimum value: %2, row: %3").arg(colNm, valStr).arg(pos);  // cn:列[%1] 最小值: %2, 第 %3 行
        } catch (...) {
            daInfo << QCoreApplication::translate("DA::DAAppController", "Column [%1] minimum value at row %2").arg(colNm).arg(pos);  // cn:列[%1] 最小值位于第 %2 行
        }
    }
}

/**
 * @brief 显示统计信息（Ribbon「列」面板 + 列头右键）
 *
 * 用当前页选区构造快速绘图请求（数据集句柄 + 有序选中列 + 角色），交给
 * DAAppDockingArea::showDataStatsWidget 在浮动 dock 里做深度统计分析
 * （多列统计表格 + 箱线图，支持正态性检验/置信区间/离群点等扩展分析）；
 * 支持多选列：选中几列就统计并对比几列。
 */
void DAAppTableActionHandler::onActionShowColumnDescribeTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const DA::DATableChartRequest request = dfopt->buildQuickPlotRequest();
    if (!request.isValid() || request.columns.isEmpty()) {
        daWarning << QCoreApplication::translate("DA::DAAppController", "Please select a valid column");  // cn:请选择正确的列
        return;
    }
    mDeps.dock->showDataStatsWidget(request);
    // 绑定当前页的数据变化通知（刷新提示条不自动重算；重复触发防重复连接）
    if (DADataStatsWidget* statsWidget = mDeps.dock->getDataStatsWidget()) {
        connect(dfopt, &DADataOperateOfDataFrameWidget::dataModified,
                statsWidget, &DADataStatsWidget::notifyDataChanged, Qt::UniqueConnection);
    }
}

/**
 * @brief 快速绘图（Ribbon「列」面板 + 列头右键）
 *
 * 只负责取当前页并转给页面：选区 → 请求 → 浮窗的规则都在页面与 DAGui 的
 * 快速绘图模块里，APP 不重复实现建图逻辑。
 */
void DAAppTableActionHandler::onActionQuickPlotTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    if (!dfopt->isDataframeTableHaveSelection()) {
        daWarning << QCoreApplication::translate("DA::DAAppController", "Select the columns to plot first");  // cn:请先选择要绘图的列
        return;
    }
    dfopt->showQuickPlot();
}

/**
 * @brief 刷新「快速绘图」动作的可用状态
 *
 * 当前没有数据表页、或表格里没有选区时置灰（实施方案 §3.1）。
 */
void DAAppTableActionHandler::updateQuickPlotActionState()
{
    if (!mDeps.actions || !mDeps.actions->actionQuickPlot || !mDeps.dock) {
        return;
    }
    // 早于数据操作区创建时也要安全（getCurrentDataFrameOperateWidget 内部未判空）
    DADataOperateWidget* dataWidget = mDeps.dock->getDataOperateWidget();
    DADataOperateOfDataFrameWidget* dfopt = dataWidget ? dataWidget->getCurrentDataFrameWidget() : nullptr;
    mDeps.actions->actionQuickPlot->setEnabled(dfopt && dfopt->isDataframeTableHaveSelection());
}

/**
 * @brief 表内查找（002 T1.4，Ribbon「编辑」面板）
 *
 * 唤出当前数据页的非模态浮动查找条（仅查找行）。快捷键 Ctrl+F 在 DADataTableView
 * 处理（红线5：action 不设 shortcut，避免双触发），此处仅 Ribbon 入口。
 */
void DAAppTableActionHandler::onActionFindTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showFindReplace(false);
}

/**
 * @brief 表内替换（002 T1.4，Ribbon「编辑」面板）
 *
 * 唤出当前数据页浮动条并展开替换行。快捷键 Ctrl+H 在 DADataTableView 处理。
 */
void DAAppTableActionHandler::onActionReplaceTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showFindReplace(true);
}

/**
 * @brief 条件格式（003 T1.5，Ribbon「Table Style」面板）
 *
 * 唤出当前数据页的条件格式规则编辑对话框（作用于选区所在列）。
 * 与表体右键菜单「条件格式...」共用页面 showConditionalFormatDialog 入口（R16）。
 */
void DAAppTableActionHandler::onActionConditionalFormatTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showConditionalFormatDialog();
}

/**
 * @brief 计算列（004 T1.23，Ribbon「Data Tools」面板）
 *
 * 唤出当前数据页的计算列对话框（新建）。与表体右键菜单「计算列...」共用页面
 * showComputedColumnDialog 入口；脏标记经页面 dataModified 信号统一接线。
 */
void DAAppTableActionHandler::onActionComputedColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showComputedColumnDialog(-1);
}

/**
 * @brief 全部重算计算列（004 T1.23，Ribbon「Data Tools」面板）
 *
 * 重算当前数据页全部待重算/失效的计算列（R8 手动重算；一个宏命令 = 一个撤销单元）。
 */
void DAAppTableActionHandler::onActionRecomputeAllComputedColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->recomputeAllStaleComputedColumns();
}

/**
 * @brief 分列（004 T1.9，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「分列...」共用页面 showSplitColumnDialog 入口：
 * 作用于当前选区的唯一数据列（页面内校验并提示）。
 */
void DAAppTableActionHandler::onActionSplitColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showSplitColumnDialog(-1);
}

/**
 * @brief 合并列（004 T1.9，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「合并列...」共用页面 showMergeColumnsDialog 入口：
 * 作用于当前选区的多列（页面内校验并提示）。
 */
void DAAppTableActionHandler::onActionMergeColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showMergeColumnsDialog();
}

/**
 * @brief 分组聚合（004 T1.22，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「分组聚合...」共用页面 showGroupByDialog 入口：产出新数据集
 * （addData_ 可撤销注册），新页面激活经页面 newDataCreated 信号统一接线。
 */
void DAAppTableActionHandler::onActionGroupByTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showGroupByDialog();
}

/**
 * @brief 转置（004 T1.27，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「转置...」共用页面 showReshapeDialog(false) 入口；
 * 默认产出新数据集，就地替换为对话框勾选项（R14）。
 */
void DAAppTableActionHandler::onActionTransposeTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showReshapeDialog(false);
}

/**
 * @brief 宽转长 melt（004 T1.27，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「宽转长（melt）...」共用页面 showReshapeDialog(true) 入口；
 * melt 规模守卫在对话框 OK 确认点（行数×展开列数 > 5000 万默认拒绝，可强制继续）。
 */
void DAAppTableActionHandler::onActionMeltTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showReshapeDialog(true);
}

/**
 * @brief 向下填充（005 T1.24，Ribbon「Edit」面板下拉 + Ctrl+D + 表体右键）
 *
 * 与表体右键「填充 → 向下填充」、DADataTableView 的 Ctrl+D 共用页面 fillSelection 入口；
 * 求值走 Python 算子（空值判定），一次填充一条 paste 命令入栈（D4）。
 */
void DAAppTableActionHandler::onActionFillDownTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Down);
}

/**
 * @brief 向上填充（005 T1.24）
 */
void DAAppTableActionHandler::onActionFillUpTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Up);
}

/**
 * @brief 向下填充空值 ffill（005 T1.24）
 */
void DAAppTableActionHandler::onActionFillForwardTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Forward);
}

/**
 * @brief 向上填充空值 bfill（005 T1.24）
 */
void DAAppTableActionHandler::onActionFillBackwardTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Backward);
}

/**
 * @brief 序列填充（005 T1.24，Ribbon「Edit」面板下拉 + 表体右键）
 *
 * 唤出序列填充对话框（数值等差/日期步进，Python 求值+预览）。
 */
void DAAppTableActionHandler::onActionFillSeriesTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showFillSeriesDialog();
}

/**
 * @brief 校验规则管理（005 T1.25，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「校验规则...」共用页面 showValidationRulesDialog 入口；
 * 规则增删改经 setRules 命令入页撤销栈（D4）。
 */
void DAAppTableActionHandler::onActionValidationRulesTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showValidationRulesDialog();
}

/**
 * @brief 执行校验（005 T1.25，Ribbon「Data Tools」面板）
 *
 * 只读操作（红线2：不改数据、不入撤销栈）；违规装载 Source::Validation 高亮并弹报告。
 */
void DAAppTableActionHandler::onActionRunValidationTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->runValidationAndShowReport();
}

/**
 * @brief 插入/编辑批注（005 T1.26，Ribbon「Edit」面板下拉）
 *
 * 与表体右键菜单共用页面弹层入口（无批注=插入、有批注=编辑且弹层内可删除）。
 */
void DAAppTableActionHandler::onActionNoteTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showNoteEditorForCurrentCell();
}

/**
 * @brief 冻结列（006 T1.8，Ribbon「View」面板；主体 = 弹「冻结前 N 列」输入框）
 */
void DAAppTableActionHandler::onActionFreezeColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->promptFreezeColumns();
}

/**
 * @brief 隐藏列（006 T1.8）：隐藏选中的列（入撤销栈）
 */
void DAAppTableActionHandler::onActionHideColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const QList< int > cols = dfopt->getSelectedDataframeCoumns();
    if (cols.isEmpty()) {
        daWarning << QCoreApplication::translate("DA::DAAppController", "Please select a column first");  // cn:请先选择列（复用页面既有源串，本 context 另建条目）
        return;
    }
    for (int c : cols) {
        dfopt->hideColumnAt(c);
    }
}

/**
 * @brief 显示所有列（006 T1.8）
 */
void DAAppTableActionHandler::onActionShowAllColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showAllColumns();
}

/**
 * @brief 冻结首列（006 T1.8，冻结菜单项）
 */
void DAAppTableActionHandler::onActionFreezeFirstColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->freezeFirstColumn();
}

/**
 * @brief 冻结前 N 列…（006 T1.8，冻结菜单项，与主体同一实现）
 */
void DAAppTableActionHandler::onActionFreezeColumnsPromptTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->promptFreezeColumns();
}

/**
 * @brief 取消全部冻结（006 T1.8，冻结菜单项）
 */
void DAAppTableActionHandler::onActionUnfreezeColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->unfreezeAllColumns();
}

/**
 * @brief 定位（007 T1.7）：打开定位对话框（Ctrl+G 与滚动条右键共用同一入口）
 */
void DAAppTableActionHandler::onActionGotoCellTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showGotoDialog();
}

/**
 * @brief 刷新「排序」动作的可用状态
 *
 * 没有数据表页、或当前表为只读源（惰性/数据库表）时整组置灰——面板大按钮禁用同时
 * 也让下拉菜单不可展开。菜单项与面板按钮共用同一批 QAction，状态自然同步。
 */
void DAAppTableActionHandler::updateSortActionState()
{
    if (!mDeps.actions || !mDeps.actions->actionSort || !mDeps.dock) {
        return;
    }
    DADataOperateWidget* dataWidget        = mDeps.dock->getDataOperateWidget();
    DADataOperateOfDataFrameWidget* dfopt  = dataWidget ? dataWidget->getCurrentDataFrameWidget() : nullptr;
    const bool enabled                     = dfopt && dfopt->haveData() && dfopt->isTableEditable();
    mDeps.actions->actionSort->setEnabled(enabled);
    mDeps.actions->actionSortAscending->setEnabled(enabled);
    mDeps.actions->actionSortDescending->setEnabled(enabled);
    mDeps.actions->actionSortCustom->setEnabled(enabled);
    mDeps.actions->actionSortClear->setEnabled(enabled);
}

/**
 * @brief 排序大按钮 / 自定义排序…：打开自定义排序对话框
 *
 * 预置规则由页面决定：已有排序规格时按规格预填，否则以当前列为唯一主键。
 */
void DAAppTableActionHandler::onActionSortTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showSortDialog();
}

/**
 * @brief 升序排序（按当前列；无当前列时提示先选列）
 */
void DAAppTableActionHandler::onActionSortAscendingTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const int col = dfopt->currentDataColumnForSort();
    if (col < 0) {
        daWarning << QCoreApplication::translate("DA::DAAppController", "Please select a column to sort first");  // cn:请先选择要排序的列
        return;
    }
    dfopt->sortColumn(col, Qt::AscendingOrder);
}

/**
 * @brief 降序排序（按当前列；无当前列时提示先选列）
 */
void DAAppTableActionHandler::onActionSortDescendingTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const int col = dfopt->currentDataColumnForSort();
    if (col < 0) {
        daWarning << QCoreApplication::translate("DA::DAAppController", "Please select a column to sort first");  // cn:请先选择要排序的列
        return;
    }
    dfopt->sortColumn(col, Qt::DescendingOrder);
}

/**
 * @brief 清除排序：按索引升序恢复原始行顺序（行头的 index 从乱序还原，可撤销）
 */
void DAAppTableActionHandler::onActionSortClearTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->restoreOriginalSortOrder();
}

/**
 * @brief 删除当前格批注（005 T1.26）
 */
void DAAppTableActionHandler::onActionDeleteNoteTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->deleteNoteAtCurrentCell();
}

/**
 * @brief 清除本表批注（005 T1.26，一次命令可撤销）
 */
void DAAppTableActionHandler::onActionClearNotesTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->clearAllNotes();
}

/**
 * @brief 填充共用的列操作右键菜单项
 *
 * 供数据管理树 series 节点右键复用（表头右键菜单已迁入 DADataHeaderView，R12）。
 * 6 项 4 组：重命名列 / 删除列 ‖ 复制列名 ‖ 定位最大 / 定位最小 ‖ 描述统计。
 * 走到这里 col 已由调用方选中，菜单项全部 enable。
 * @param menu 目标菜单
 */
void DAAppTableActionHandler::populateColumnContextMenu(QMenu& menu)
{
    menu.addAction(mDeps.actions->actionRenameColumn);
    menu.addAction(mDeps.actions->actionRemoveColumn);
    menu.addSeparator();
    menu.addAction(mDeps.actions->actionCopyColumnName);
    menu.addSeparator();
    menu.addAction(mDeps.actions->actionGotoMax);
    menu.addAction(mDeps.actions->actionGotoMin);
    menu.addSeparator();
    menu.addAction(mDeps.actions->actionShowColumnDescribe);
}

void DAAppTableActionHandler::selectColumnInDataFrameWidget(DADataOperateOfDataFrameWidget* w, int col)
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
 * @brief 为数据管理树注入右键菜单
 *
 * - series 节点：取 DAData + seriesName → 列名转列索引 → 找到（或打开）对应的
 *   DataFrame 操作窗口 → 选中该列 → 弹出与表头右键相同的菜单（共用列操作 action）。
 * - 数据集（dataframe）条目：右键先选中该条目（rename 槽基于树 currentIndex，
 *   remove/export 槽基于树选中集），再弹出「重命名 / 移除数据 / 导出」菜单，
 *   菜单项全部复用 ribbon 数据标签页已有 action（重命名=actionRenameData，
 *   移除=actionRemoveData，导出子菜单=actionExportData{Csv,Excel,Pickle,Parquet}）。
 * - 属性列（第二列）上的点击统一映射回名称列。
 * @param w 数据管理树窗口
 */
void DAAppTableActionHandler::setupDataManagerTreeContextMenu(DADataManagerTreeWidget* w)
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
        // 属性列（第二列）条目映射回名称列
        if (proxyIndex.column() != 0) {
            proxyIndex = proxyIndex.siblingAtColumn(0);
        }
        // 映射到 source model
        DADataManagerTreeFilterProxyModel* proxy = w->getProxyModel();
        QModelIndex srcIndex                     = proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;
        QStandardItem* item                      = w->getModel()->itemFromIndex(srcIndex);
        if (!item) {
            return;
        }
        if (DADataManagerTreeModel::isDataframeSeriesItem(item)) {
            // ---- series 节点：与表头右键一致的列操作菜单 ----
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
            DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getDataOperateWidget()->findDataFrameWidget(data);
            if (!dfopt) {
                mDeps.controller->getDataOperateWidget()->showData(data);
                dfopt = mDeps.controller->getDataOperateWidget()->findDataFrameWidget(data);
            }
            if (!dfopt) {
                return;
            }
            // 弹共用菜单，但重命名/删除列换成临时 action 绑直入口（不经焦点检查——
            // 树右键时焦点在数据管理器 dock，共用 action 绑定的槽会弹
            // 「请选中数据操作窗口」）；其余动作先选中该列，共用槽经
            // getSelectedOneDataframeColumn 取列
            QMenu menu(w);
            populateColumnContextMenu(menu);
            {
                QList< QAction* > acts = menu.actions();
                for (int i = 0; i < acts.size(); ++i) {
                    if (acts.at(i) == mDeps.actions->actionRenameColumn) {
                        QAction* local = new QAction(mDeps.actions->actionRenameColumn->icon(),
                                                     mDeps.actions->actionRenameColumn->text(), &menu);
                        connect(local, &QAction::triggered, this, [ this, dfopt, col ]() {
                            renameColumnOnPage(dfopt, col);
                        });
                        menu.insertAction(acts.at(i), local);
                        menu.removeAction(acts.at(i));
                    } else if (acts.at(i) == mDeps.actions->actionRemoveColumn) {
                        QAction* local = new QAction(mDeps.actions->actionRemoveColumn->icon(),
                                                     mDeps.actions->actionRemoveColumn->text(), &menu);
                        connect(local, &QAction::triggered, this, [ this, dfopt, col ]() {
                            if (dfopt->removeColumnAt(col) > 0) {
                                mDeps.controller->setDirty();
                            }
                        });
                        menu.insertAction(acts.at(i), local);
                        menu.removeAction(acts.at(i));
                    }
                }
            }
            // 选中该列（非重命名/删除的共用槽经 getSelectedOneDataframeColumn 取列）
            selectColumnInDataFrameWidget(dfopt, col);
            menu.exec(tv->viewport()->mapToGlobal(pos));
            return;
        }
        if (!DADataManagerTreeModel::isDataframeItem(item)) {
            return;
        }
        // ---- 数据集（dataframe）条目：重命名 / 移除数据 / 复制为 / 导出 ----
        // 右键先选中该条目，保证后续槽函数操作的就是此条目
        tv->setCurrentIndex(proxyIndex);
        QMenu menu(w);
        menu.addAction(mDeps.actions->actionRenameData);
        menu.addAction(mDeps.actions->actionRemoveData);
        // 复制为（整表复制到剪贴板；Tab/Markdown/HTML，与导出共用 DATableExporter）
        QMenu* copyMenu = menu.addMenu(mDeps.actions->actionCopy->icon(), QCoreApplication::translate("DA::DAAppController", "Copy As"));  // cn:复制为
        QAction* actCopyTab = copyMenu->addAction(QCoreApplication::translate("DA::DAAppController", "Tab-separated Text"));  // cn:Tab 分隔文本
        QAction* actCopyMd  = copyMenu->addAction(QCoreApplication::translate("DA::DAAppController", "Markdown Table"));      // cn:Markdown 表格
        QAction* actCopyHtm = copyMenu->addAction(QCoreApplication::translate("DA::DAAppController", "HTML Table (with styles)"));  // cn:HTML 表格（含样式）
        connect(actCopyTab, &QAction::triggered, this, [ this ]() {
            mDeps.controller->copySelectedDataAsTable(DATableExporter::Format::TabText);
        });
        connect(actCopyMd, &QAction::triggered, this, [ this ]() {
            mDeps.controller->copySelectedDataAsTable(DATableExporter::Format::Markdown);
        });
        connect(actCopyHtm, &QAction::triggered, this, [ this ]() {
            mDeps.controller->copySelectedDataAsTable(DATableExporter::Format::Html);
        });
        // 导出（csv/excel/pickle/parquet 复用 ribbon action；Markdown/HTML 走文档导出）
        QMenu* exportMenu = menu.addMenu(QCoreApplication::translate("DA::DAAppController", "Export"));  // cn:导出
        exportMenu->setIcon(mDeps.actions->actionExportData->icon());
        exportMenu->addAction(mDeps.actions->actionExportDataCsv);
        exportMenu->addAction(mDeps.actions->actionExportDataExcel);
        exportMenu->addAction(mDeps.actions->actionExportDataPickle);
        exportMenu->addAction(mDeps.actions->actionExportDataParquet);
        exportMenu->addSeparator();
        QAction* actExportMd  = exportMenu->addAction(QCoreApplication::translate("DA::DAAppController", "Markdown Document..."));  // cn:Markdown 文档...
        QAction* actExportHtm = exportMenu->addAction(QCoreApplication::translate("DA::DAAppController", "HTML Document (with styles)..."));  // cn:HTML 文档（含样式）...
        connect(actExportMd, &QAction::triggered, this, [ this ]() {
            mDeps.controller->exportSelectedDataAsDocument(DATableExporter::Format::Markdown);
        });
        connect(actExportHtm, &QAction::triggered, this, [ this ]() {
            mDeps.controller->exportSelectedDataAsDocument(DATableExporter::Format::Html);
        });
        menu.exec(tv->viewport()->mapToGlobal(pos));
    });
}

/**
 * @brief 选中列转换为数值
 */
void DAAppTableActionHandler::onActionCastToNumTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->castSelectToNum();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 选中列转换为文字
 */
void DAAppTableActionHandler::onActionCastToStringTriggered()
{
    DAAPPCONTROLLER_PASS();
}

/**
 * @brief 选中列转换为日期
 */
void DAAppTableActionHandler::onActionCastToDatetimeTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->castSelectToDatetime();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 选中列转换为索引
 */
void DAAppTableActionHandler::onActionChangeToIndexTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = mDeps.controller->getCurrentDataFrameOperateWidget(false, false)) {
        dfopt->changeSelectColumnToIndex();
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 表格底色变更
 * @param c 颜色
 */
void DAAppTableActionHandler::onTableStyleFillColorChanged(const QColor& c)
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setBackground(QBrush(c));
    w->mergeStyleToSelection(fragment);
    qDebug() << "DAAppTableActionHandler::onTableStyleFillColorChanged(" << c << ")";
}

/**
 * @brief 表格字体变更
 * @param f 字体
 */
void DAAppTableActionHandler::onTableStyleFontChanged(const QFont& f)
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
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
void DAAppTableActionHandler::onTableStyleFontColorChanged(const QColor& c)
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
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
void DAAppTableActionHandler::onActionClearStyleSelectedTriggered()
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->clearStyleSelection();
    }
}

/**
 * @brief 清除整表所有样式
 */
void DAAppTableActionHandler::onActionClearStyleAllTriggered()
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->clearStyleAll();
    }
}

/**
 * @brief 对齐按钮组点击：对选中区应用对齐（单条命令，一次撤销）
 * @param al 水平位 + 垂直位组合
 */
void DAAppTableActionHandler::onTableStyleAlignmentChanged(Qt::Alignment al)
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setAlignment(al);
    w->mergeStyleToSelection(fragment);
}

/**
 * @brief 自动换行开关切换：对选中区应用换行样式位（单条命令，一次撤销）
 *
 * 轻量方案（A4）：只改绘制（Qt::TextWordWrap），行高不随内容增长；
 * 需要更高行高时用「单元格大小 → 设置行高」手动调整。
 * @param on 是否换行
 */
void DAAppTableActionHandler::onTableStyleWrapTextToggled(bool on)
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setWrapWord(on);
    w->mergeStyleToSelection(fragment);
}

/**
 * @brief 边框预设：外框线走专用路径，其余按边位组合应用（0 = 无框线）
 * @param sides BorderSide 位组合（outline=true 时忽略）
 * @param outline true=外框线（对选区包围矩形）
 */
void DAAppTableActionHandler::applyBorderPreset(quint8 sides, bool outline)
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    if (outline) {
        w->applyBorderOutlineToSelection();
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setBorder(sides);
    w->mergeStyleToSelection(fragment);
}

/**
 * @brief 单元格大小大按钮主体：选中列宽自适应（A3，直接改视图状态、不进撤销栈）
 */
void DAAppTableActionHandler::onActionCellSizeTriggered()
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->autoFitSelectedColumnWidths();
    }
}

/**
 * @brief 行高按钮主体：弹「设置整表行高」输入框（下拉紧凑/普通走各自 lambda）
 */
void DAAppTableActionHandler::onActionRowHeightTriggered()
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->showSetRowHeightDialog();
    }
}

/**
 * @brief 选中区样式反向同步 ribbon 控件
 *
 * 根据选中区代表样式更新 ribbon 的底色按钮/字体面板/对齐按钮组。
 * 各属性独立判断：valid 的属性更新对应控件，invalid 的不调整。
 * 用 blockSignals 避免控件信号触发循环。
 * @param style 选中区代表样式
 */
void DAAppTableActionHandler::onTableStyleCurrentChanged(const DA::DATableCellStyle& style)
{
    // 底色按钮
    if (mDeps.ribbon && mDeps.ribbon->mBtnTableFillColor) {
        QSignalBlocker block(mDeps.ribbon->mBtnTableFillColor);
        if (style.backgroundValid()) {
            mDeps.ribbon->mBtnTableFillColor->setColor(style.background().color());
        }
    }
    // 字体面板
    if (mDeps.ribbon && mDeps.ribbon->mWidgetTableFont) {
        QSignalBlocker block(mDeps.ribbon->mWidgetTableFont);
        if (style.fontValid()) {
            mDeps.ribbon->mWidgetTableFont->setCurrentFont(style.font());
        }
        if (style.foregroundValid()) {
            mDeps.ribbon->mWidgetTableFont->setCurrentFontColor(style.foreground());
        }
    }
    // 对齐按钮组（A1）
    if (mDeps.ribbon && mDeps.ribbon->mWidgetTableAlign) {
        QSignalBlocker block(mDeps.ribbon->mWidgetTableAlign);
        if (style.alignmentValid()) {
            mDeps.ribbon->mWidgetTableAlign->setAlignment(style.alignment());
        }
    }
    // 自动换行开关（A4 轻量）：不一致时不调整（与其它属性同口径）
    if (mDeps.actions->actionWrapText) {
        QSignalBlocker block(mDeps.actions->actionWrapText);
        if (style.wrapWordValid()) {
            mDeps.actions->actionWrapText->setChecked(style.wrapWord());
        }
    }
}

/**
 * @brief 显示格式下拉框类别变化：对选中列应用该类别（默认参数）
 * @param c 选中的格式类别
 */
void DAAppTableActionHandler::onTableDisplayFormatCategoryChanged(DA::DATableDisplayFormat::Category c)
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    if (c == DA::DATableDisplayFormat::General) {
        w->clearDisplayFormatFromSelection();
    } else {
        DA::DATableDisplayFormat fmt(c);
        w->setDisplayFormatToSelection(fmt);
    }
    mDeps.controller->setDirty();
}

/**
 * @brief “设置单元格格式”按钮：打开对话框，OK 后应用结果
 *
 * 对话框堆分配、首次创建后复用，每次 setup(current/dtype/sample) 重置状态再 exec。
 */
void DAAppTableActionHandler::onActionTableFormatCellsTriggered()
{
    DADataOperateOfDataFrameWidget* w = mDeps.controller->getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    int col = w->getSelectedOneDataframeColumn(false);
    if (col < 0) {
        daWarning << QCoreApplication::translate("DA::DAAppController", "Please select a valid column");  // cn:请选择正确的列
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
    if (!mDialogTableDisplayFormat) {
        mDialogTableDisplayFormat = new DA::DADialogTableDisplayFormat(mDeps.controller->app());
    }
    mDialogTableDisplayFormat->setup(w->getCurrentColumnDisplayFormat(), dt, sample);
    if (mDialogTableDisplayFormat->exec() == QDialog::Accepted) {
        DA::DATableDisplayFormat fmt = mDialogTableDisplayFormat->getResult();
        if (fmt.isValid()) {
            w->setDisplayFormatToSelection(fmt);
        } else {
            w->clearDisplayFormatFromSelection();
        }
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 选中列显示格式反向同步 ribbon 格式下拉框
 * @param fmt 当前代表格式（invalid 表示无格式）
 */
void DAAppTableActionHandler::onTableDisplayFormatCurrentChanged(const DA::DATableDisplayFormat& fmt)
{
    if (mDeps.ribbon && mDeps.ribbon->mComboxDisplayFormat) {
        QSignalBlocker block(mDeps.ribbon->mComboxDisplayFormat);
        mDeps.ribbon->mComboxDisplayFormat->setCurrentFormat(fmt);
    }
}
}  // namespace DA
