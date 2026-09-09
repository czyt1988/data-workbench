#ifndef DAUOBJECTNAMES_H
#define DAUOBJECTNAMES_H
//===================================================
// DAWorkbench UI objectName 契约常量（单一事实来源）
//
// 本文件集中定义主程序 UI 元素（ribbon category/panel、action、dock）的 objectName。
// 这些名称是主程序与插件之间的公开契约：
// - 主程序侧赋值（setObjectName / createAction）必须引用此处的常量，禁止手写字面量；
// - 插件侧查询（getCategoryByObjectName / getPanelByObjectName / findAction）必须引用此处的常量；
// - 修改任何常量的值都属于兼容性破坏：dock 名称参与 UI 状态持久化（saveUIState/restoreUIState），
//   全部名称参与插件的字符串查找，改名必须同步所有插件并重编译。
//
// 注意：历史名称中存在既成事实的拼写不一致（如 pannel 拼写、da-ribbon-pannel-main.common
// 与其他 da-pannel-* 前缀不统一），为保持兼容性一律原样保留，不做修正。
//===================================================
#include <QString>

namespace DA
{

namespace UiNames
{
/**
 * @brief Ribbon 的 category 与 panel 的 objectName 契约
 */
namespace Ribbon
{
// ---- 固定 category ----
constexpr char MainCategory[]   = "da-ribbon-category-main";    ///< 主页标签
constexpr char DataCategory[]   = "da-ribbon-category-data";    ///< 数据标签
constexpr char ViewCategory[]   = "da-ribbon-category-view";    ///< 视图标签
constexpr char FigureCategory[] = "da-ribbon-category-figure";  ///< 绘图标签
constexpr char AgentCategory[]  = "da-ribbon-category-agent";   ///< AI分析标签
// ---- 上下文标签(context category) ----
constexpr char DataFrameContextCategory[] = "da-ribbon-contextcategory-dataframe";  ///< DataFrame上下文标签
constexpr char WorkflowContextCategory[]  = "da-ribbon-contextcategory-workflow";   ///< 工作流上下文标签
constexpr char ChartContextCategory[]     = "da-ribbon-contextcategory-chart";      ///< 图表上下文标签
// ---- 上下文标签内部的 category ----
constexpr char DataFrameOperateCategory[] = "da-ribbon-category-dataframe.operate";  ///< DataFrame操作category
constexpr char DataFrameStyleCategory[]   = "da-ribbon-category-dataframe.style";    ///< DataFrame样式category
constexpr char WorkflowEditCategory[]     = "da-ribbon-category-workflow.edit";      ///< 工作流编辑category
constexpr char WorkflowViewCategory[]     = "da-ribbon-category-workflow.view";      ///< 工作流视图category
constexpr char WorkflowRunCategory[]      = "da-ribbon-category-workflow.run";       ///< 工作流运行category
constexpr char ChartStyleCategory[]       = "da-ribbon-category-chart.style";        ///< 图表样式category
constexpr char ChartEditCategory[]        = "da-ribbon-category-chart.edit";         ///< 图表编辑category
// ---- 主页标签下的 panel ----
constexpr char MainCommonPanel[] =
    "da-ribbon-pannel-main.common";  ///< 主页文件操作pannel（历史命名，前缀与其他panel不一致，保持兼容）
constexpr char MainClipboardPanel[] = "da-pannel-main.clipboard";  ///< 主页剪贴板pannel
constexpr char MainCreatePanel[]    = "da-pannel-main.create";     ///< 主页创建pannel
constexpr char MainSettingPanel[]   = "da-pannel-main.setting";    ///< 主页设置pannel
// ---- 数据标签下的 panel ----
constexpr char DataOperatePanel[] = "da-pannel-data.data-opt";  ///< 数据操作pannel
constexpr char DataExportPanel[]  = "da-pannel-data.export";    ///< 数据导出pannel
// ---- 视图标签下的 panel ----
constexpr char ViewMainPanel[]       = "da-pannel-view.main";        ///< 视图显示pannel
constexpr char ViewLayoutPanel[]     = "da-pannel-view.layout";      ///< 视图布局pannel
constexpr char ViewAppearancePanel[] = "da-pannel-view.appearance";  ///< 视图外观pannel
// ---- 绘图标签下的 panel ----
constexpr char FigureChartAddPanel[]  = "da-pannel-figure.chart-add";   ///< 添加绘图pannel
constexpr char FigureStatsPlotPanel[] = "da-pannel-figure.stats-plot";  ///< 统计绘图pannel
// ---- DataFrame上下文标签下的 panel ----
constexpr char DataFrameOperateAxesPanel[]   = "da-pannel-dataframe.operate.axes";    ///< 坐标设置pannel
constexpr char DataFrameOperateColumnPanel[] = "da-pannel-dataframe.operate.column";  ///< 列操作pannel
constexpr char DataFrameOperateDTypePanel[]  = "da-pannel-dataframe.operate.type";    ///< 类型设置pannel
constexpr char DataFrameOperateFormatPanel[] = "da-pannel-dataframe.operate.format";  ///< 显示格式pannel
constexpr char DataFrameStyleFillPanel[]     = "da-pannel-dataframe.style.fill";      ///< 填充样式pannel
constexpr char DataFrameStyleFontPanel[]     = "da-pannel-dataframe.style.font";      ///< 字体样式pannel
constexpr char DataFrameStyleClearPanel[]    = "da-pannel-dataframe.style.clear";     ///< 样式清除pannel
// ---- 工作流上下文标签下的 panel ----
constexpr char WorkflowItemPanel[]       = "da-pannel-context.workflow.item";        ///< 条目pannel
constexpr char WorkflowTextPanel[]       = "da-pannel-context.workflow.text";        ///< 文本pannel
constexpr char WorkflowBackgroundPanel[] = "da-pannel-context.workflow.background";  ///< 背景pannel
constexpr char WorkflowGroupPanel[]      = "da-pannel-context.workflow.group";       ///< 分组pannel
constexpr char WorkflowViewPanel[]       = "da-pannel-context.workflow.view";        ///< 视图pannel
constexpr char WorkflowExportPanel[]     = "da-pannel-context.workflow.export";      ///< 导出pannel
constexpr char WorkflowRunPanel[]        = "da-pannel-context.workflow.run";         ///< 运行pannel
// ---- 图表上下文标签下的 panel ----
constexpr char ChartEditFigureSettingPanel[] = "da-pannel-context-chartedit.fig_setting";    ///< 绘图窗口设置pannel
constexpr char ChartEditChartSettingPanel[]  = "da-pannel-context-chartedit.chart_setting";  ///< 图表设置pannel
}  // namespace Ribbon

/**
 * @brief 主程序注册到 DAActionsInterface 的 action objectName 契约
 *
 * 常量名 = action objectName 去掉 "action" 前缀
 */
namespace Action
{
// ---- 主页 ----
constexpr char Open[]          = "actionOpen";
constexpr char Save[]          = "actionSave";
constexpr char SaveAs[]        = "actionSaveAs";
constexpr char AppendProject[] = "actionAppendProject";
constexpr char OpenMarkdown[]  = "actionOpenMarkdown";
constexpr char Setting[]       = "actionSetting";
constexpr char PluginManager[] = "actionPluginManager";
constexpr char About[]         = "actionAbout";
constexpr char Redo[]          = "actionRedo";  ///< 由QUndoGroup创建，在DAAppRibbonArea中补充objectName
constexpr char Undo[]          = "actionUndo";  ///< 由QUndoGroup创建，在DAAppRibbonArea中补充objectName
// ---- 主页剪贴板（按焦点路由到工作流/表格/图表） ----
constexpr char Cut[]       = "actionCut";
constexpr char Copy[]      = "actionCopy";
constexpr char Paste[]     = "actionPaste";
constexpr char Delete[]    = "actionDelete";
constexpr char SelectAll[] = "actionSelectAll";
// ---- 数据 ----
constexpr char AddData[]            = "actionAddData";
constexpr char RemoveData[]         = "actionRemoveData";
constexpr char RenameData[]         = "actionRenameData";
constexpr char ExportData[]         = "actionExportData";
constexpr char ExportDataCsv[]      = "actionExportDataCsv";
constexpr char ExportDataExcel[]    = "actionExportDataExcel";
constexpr char ExportDataPickle[]   = "actionExportDataPickle";
constexpr char ExportDataParquet[]  = "actionExportDataParquet";
constexpr char RemoveRow[]          = "actionRemoveRow";
constexpr char RemoveColumn[]       = "actionRemoveColumn";
constexpr char InsertRow[]          = "actionInsertRow";
constexpr char InsertRowAbove[]     = "actionInsertRowAbove";
constexpr char InsertColumnRight[]  = "actionInsertColumnRight";
constexpr char InsertColumnLeft[]   = "actionInsertColumnLeft";
constexpr char RenameColumns[]      = "actionRenameColumns";
constexpr char RenameColumn[]       = "actionRenameColumn";
constexpr char CopyColumnName[]     = "actionCopyColumnName";
constexpr char GotoMax[]            = "actionGotoMax";
constexpr char GotoMin[]            = "actionGotoMin";
constexpr char ShowColumnDescribe[] = "actionShowColumnDescribe";
constexpr char RemoveCell[]         = "actionRemoveCell";
constexpr char CastToNum[]          = "actionCastToNum";
constexpr char CastToString[]       = "actionCastToString";
constexpr char CastToDatetime[]     = "actionCastToDatetime";
constexpr char ChangeToIndex[]      = "actionChangeToIndex";
constexpr char ClearStyleSelected[] = "actionClearStyleSelected";
constexpr char ClearStyleAll[]      = "actionClearStyleAll";
constexpr char TableFormatCells[]   = "actionTableFormatCells";
// ---- 绘图 ----
constexpr char AddFigure[]                  = "actionAddFigure";
constexpr char FigureNewXYAxis[]            = "actionFigureNewXYAxis";
constexpr char FigureSettingApplyAllChart[] = "actionFigureSettingApplyAllChart";
constexpr char ChartAddCurve[]              = "actionChartAddCurve";
constexpr char ChartAddScatter2D[]          = "actionChartAddScatter2D";
constexpr char ChartAddErrorBar[]           = "actionChartAddErrorBar";
constexpr char ChartAddBoxPlot[]            = "actionChartAddBoxPlot";
constexpr char ChartAddBar[]                = "actionChartAddBar";
constexpr char ChartAddMultiBar[]           = "actionChartAddMultiBar";
constexpr char ChartAddHistogramBar[]       = "actionChartAddHistogramBar";
constexpr char ChartAddContourMap[]         = "actionChartAddContourMap";
constexpr char ChartAddCloudMap[]           = "actionChartAddCloudMap";
constexpr char ChartAddVectorfield[]        = "actionChartAddVectorfield";
constexpr char ChartAdd3DSurface[]          = "actionChartAdd3DSurface";
constexpr char ChartAdd3DBar[]              = "actionChartAdd3DBar";
constexpr char ChartAdd3DLine[]             = "actionChartAdd3DLine";
// ---- 统计绘图 ----
constexpr char StatsHistplot[]    = "actionStatsHistplot";
constexpr char StatsKdeplot1d[]   = "actionStatsKdeplot1d";
constexpr char StatsKdeplot2d[]   = "actionStatsKdeplot2d";
constexpr char StatsBoxplot[]     = "actionStatsBoxplot";
constexpr char StatsHeatmap[]     = "actionStatsHeatmap";
constexpr char StatsScatterplot[] = "actionStatsScatterplot";
constexpr char StatsBarplot[]     = "actionStatsBarplot";
constexpr char StatsRegplot[]     = "actionStatsRegplot";
constexpr char StatsECDFplot[]    = "actionStatsECDFplot";
// ---- 图表操作 ----
constexpr char ChartEnableGrid[]               = "actionChartEnableGrid";
constexpr char ChartEnableGridX[]              = "actionChartEnableGridX";
constexpr char ChartEnableGridY[]              = "actionChartEnableGridY";
constexpr char ChartEnableGridXMin[]           = "actionChartEnableGridXMin";
constexpr char ChartEnableGridYMin[]           = "actionChartEnableGridYMin";
constexpr char ChartEnableZoom[]               = "actionChartEnableZoom";
constexpr char ChartZoomIn[]                   = "actionChartZoomIn";
constexpr char ChartZoomOut[]                  = "actionChartZoomOut";
constexpr char ChartZoomAll[]                  = "actionChartZoomAll";
constexpr char ChartEnablePan[]                = "actionChartEnablePan";
constexpr char ChartDisableZoomX[]             = "actionChartDisableZoomX";
constexpr char ChartDisableZoomY[]             = "actionChartDisableZoomY";
constexpr char ChartEnablePickerCross[]        = "actionChartEnablePickerCross";
constexpr char ChartEnablePickerXY[]           = "actionChartEnablePickerXY";
constexpr char ChartEnablePickerY[]            = "actionChartEnablePickerY";
constexpr char LinkAllPicker[]                 = "actionLinkAllPicker";
constexpr char ChartPickerTextAtLeftTop[]      = "actionChartPickerTextAtLeftTop";
constexpr char ChartPickerTextAtLeftBottom[]   = "actionChartPickerTextAtLeftBottom";
constexpr char ChartPickerTextAtRightTop[]     = "actionChartPickerTextAtRightTop";
constexpr char ChartPickerTextAtRightBottom[]  = "actionChartPickerTextAtRightBottom";
constexpr char ChartPickerTextFollowMouse[]    = "actionChartPickerTextFollowMouse";
constexpr char ChartYPickerShowXValueEnabled[] = "actionChartYPickerShowXValueEnabled";
constexpr char ChartEnableLegend[]             = "actionChartEnableLegend";
constexpr char CopyFigureInClipboard[]         = "actionCopyFigureInClipboard";
constexpr char ChartLegendAtTop[]              = "actionChartLegendAtTop";
constexpr char ChartLegendAtBottom[]           = "actionChartLegendAtBottom";
constexpr char ChartLegendAtLeft[]             = "actionChartLegendAtLeft";
constexpr char ChartLegendAtRight[]            = "actionChartLegendAtRight";
constexpr char GroupChartLegendPosition[]      = "actionGroupChartLegendPosition";
constexpr char ChartEditorResizeSubChart[]     = "actionChartEditorResizeSubChart";
constexpr char ChartEditorPointerSelector[]    = "actionChartEditorPointerSelector";
constexpr char ChartEditorRectSelector[]       = "actionChartEditorRectSelector";
constexpr char ChartEditorEllipseSelector[]    = "actionChartEditorEllipseSelector";
constexpr char ChartEditorPolygonSelector[]    = "actionChartEditorPolygonSelector";
constexpr char ChartEditorAddCrossMarker[]     = "actionChartEditorAddCrossMarker";
constexpr char ChartEditorAddHLineMarker[]     = "actionChartEditorAddHLineMarker";
constexpr char ChartEditorAddVLineMarker[]     = "actionChartEditorAddVLineMarker";
constexpr char ChartEditorAddArrowMarker[]     = "actionChartEditorAddArrowMarker";
constexpr char ChartEditorAddTextMarker[]      = "actionChartEditorAddTextMarker";
constexpr char AddHorizontalPlotProbeMarker[]  = "actionAddHorizontalPlotProbeMarker";
constexpr char AddVerticalPlotProbeMarker[]    = "actionAddVerticalPlotProbeMarker";
constexpr char ChartDataPickerSetting[]        = "actionChartDataPickerSetting";
// ---- 视图 ----
constexpr char ShowWorkFlowArea[]        = "actionShowWorkFlowArea";
constexpr char ShowWorkFlowManagerArea[] = "actionShowWorkFlowManagerArea";
constexpr char ShowChartArea[]           = "actionShowChartArea";
constexpr char ShowChartManagerArea[]    = "actionShowChartManagerArea";
constexpr char ShowDataArea[]            = "actionShowDataArea";
constexpr char ShowDataManagerArea[]     = "actionShowDataManagerArea";
constexpr char ShowMessageLogView[]      = "actionShowMessageLogView";
constexpr char ShowSettingWidget[]       = "actionShowSettingWidget";
constexpr char ShowLeftSideBar[]         = "actionShowLeftSideBar";
constexpr char ShowRightSideBar[]        = "actionShowRightSideBar";
constexpr char ShowAgentArea[]           = "actionShowAgentArea";
constexpr char ResetDefaultLayout[]      = "actionResetDefaultLayout";
constexpr char ManageLayouts[]           = "actionManageLayouts";
// ---- 工作流 ----
constexpr char WorkflowNew[]                   = "actionWorkflowNew";
constexpr char WorkflowEnableItemLinkageMove[] = "actionWorkflowEnableItemLinkageMove";
constexpr char ItemSetGroup[]                  = "actionItemSetGroup";
constexpr char ItemCancelGroup[]               = "actionItemCancelGroup";
constexpr char WorkflowLinkEnable[]            = "actionWorkflowLinkEnable";
constexpr char StartDrawRect[]                 = "actionStartDrawRect";
constexpr char StartDrawText[]                 = "actionStartDrawText";
constexpr char AddBackgroundPixmap[]           = "actionAddBackgroundPixmap";
constexpr char LockBackgroundPixmap[]          = "actionLockBackgroundPixmap";
constexpr char EnableItemMoveWithBackground[]  = "actionEnableItemMoveWithBackground";
constexpr char WorkflowShowGrid[]              = "actionWorkflowShowGrid";
constexpr char WorkflowViewLock[]              = "actionWorkflowViewLock";
constexpr char WorkflowViewMarker[]            = "actionWorkflowViewMarker";
constexpr char WorkflowRun[]                   = "actionWorkflowRun";
constexpr char WorkflowTerminate[]             = "actionWorkflowTerminate";
constexpr char ExportWorkflowSceneToImage[]    = "actionExportWorkflowSceneToImage";
constexpr char ExportWorkflowSceneToPNG[]      = "actionExportWorkflowSceneToPNG";
// ---- QActionGroup（objectName，非action，仅作统一登记） ----
constexpr char GroupChartPickers[]          = "actionGroupChartPickers";
constexpr char GroupChartPickerTextRegion[] = "actionGroupChartPickerTextRegion";
constexpr char GroupWorkflowStartEdit[]     = "actionGroupWorkflowStartEdit";
}  // namespace Action

/**
 * @brief 主程序创建的固定 dock 窗口及其内容 widget 的 objectName 契约
 *
 * dock 名称参与 UI 状态持久化（ADS saveState/restoreState 按名称序列化），禁止改值
 */
namespace Dock
{
// ---- dock 内容 widget ----
constexpr char WorkFlowOperateWidget[]  = "da_workFlowOperateWidget";
constexpr char WorkflowNodeListWidget[] = "da_workflowNodeListWidget";
constexpr char ChartOperateWidget[]     = "da_chartOperateWidget";
constexpr char ChartManageWidget[]      = "da_chartManageWidget";
constexpr char DataOperateWidget[]      = "da_dataOperateWidget";
constexpr char DataManageWidget[]       = "da_dataManageWidget";
constexpr char SettingContainerWidget[] = "da_settingContainerWidget";
constexpr char MessageLogViewWidget[]   = "da_messageLogViewWidget";
constexpr char AgentDockWidget[]        = "da_agentDockWidget";
constexpr char MarkdownView[]           = "da_markdownView";
// ---- dock 窗口 ----
constexpr char WorkFlowOperateWidgetDock[]  = "da_workFlowOperateWidgetDock";
constexpr char ChartOperateWidgetDock[]     = "da_chartOperateWidgetDock";
constexpr char DataOperateWidgetDock[]      = "da_dataOperateWidgetDock";
constexpr char WorkflowNodeListWidgetDock[] = "da_workflowNodeListWidgetDock";
constexpr char ChartManageWidgetDock[]      = "da_chartManageWidgetDock";
constexpr char DataManageWidgetDock[]       = "da_dataManageWidgetDock";
constexpr char SettingDock[]                = "da_settingDock";
constexpr char AgentDockWidgetDock[]        = "da_agentDockWidgetDock";
constexpr char MessageLogViewWidgetDock[]   = "da_messageLogViewWidgetDock";
constexpr char MarkdownViewDock[]           = "da_markdownViewDock";
}  // namespace Dock

/**
 * @brief 主程序创建的菜单 objectName 契约
 */
namespace Menu
{
constexpr char InsertRow[]                  = "menuInsertRow";
constexpr char InsertColumn[]               = "menuInsertColumn";
constexpr char ExportData[]                 = "menuExportData";
constexpr char ExportWorkflowSceneToImage[] = "exportWorkflowSceneToImageMenu";
constexpr char ChartPickSetting[]           = "mMenuChartPickSetting";  ///< 历史命名，保留
constexpr char ChartLegendPosition[]        = "mMenuChartLegendPosition";  ///< 图例位置菜单（挂在actionChartEnableLegend下）
constexpr char ViewLineMarkers[]            = "menuViewLineMarkers";
}  // namespace Menu

/**
 * @brief 把契约常量转为 QString 的便捷函数
 */
inline QString uiName(const char* name)
{
    return QString::fromUtf8(name);
}

}  // namespace UiNames
}  // namespace DA
#endif  // DAUOBJECTNAMES_H
