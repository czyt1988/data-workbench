#include "DAAppActions.h"
#include "DAUiObjectNames.h"
#include <QActionGroup>
#include <QKeySequence>
#include <QPainter>
#include <QVector>
#include <cmath>
#include "DAColorTheme.h"
#include "qwt_plot_series_data_picker.h"
#include "DAFigureWidget.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================
#include "DARecentFilesManager.h"
using namespace DA;

//===================================================
// DAAppActions
//===================================================
DAAppActions::DAAppActions(DAUIInterface* u) : DAActionsInterface(u)
{
    buildActions();
    recentFilesManager = new DARecentFilesManager(this, 10, QStringLiteral("DA"), QStringLiteral("DAWorkBench"));
}

DAAppActions::~DAAppActions()
{
}

void DAAppActions::buildActions()
{
    // 构建主页action
    buildMainAction();
    // 构建数据相关的action
    buildDataAction();
    // 构建绘图相关的action
    buildChartAction();
    // 构建view相关的action
    buildViewAction();
    // 构建workflow相关的action
    buildWorkflowAction();
    //
    buildOtherActions();
    //
    buildColorThemeActions();
}

void DAAppActions::buildMainAction()
{
    // Main Category
    actionNewProject   = createAction(UiNames::Action::NewProject, ":/app/bright/Icon/file-new.svg");
    actionOpen          = createAction(UiNames::Action::Open, ":/app/bright/Icon/file.svg");
    actionSave          = createAction(UiNames::Action::Save, ":/app/bright/Icon/save.svg");
    actionSaveAs        = createAction(UiNames::Action::SaveAs, ":/app/bright/Icon/save-as.svg");
    actionAppendProject = createAction(UiNames::Action::AppendProject, ":/app/bright/Icon/appendProject.svg");
    actionOpenMarkdown  = createAction(UiNames::Action::OpenMarkdown, ":/app/bright/Icon/markdown.svg");
    // 注意Redo/undo action在ribbonArea中生成
    actionRedo    = nullptr;
    actionUndo    = nullptr;
    actionSetting = createAction(UiNames::Action::Setting, ":/app/bright/Icon/setting.svg");
    // Config Category
    actionPluginManager = createAction(UiNames::Action::PluginManager, ":/app/bright/Icon/plugin.svg");
    //
    actionAbout = createAction(UiNames::Action::About, ":/app/bright/Icon/about.svg");
    // 剪贴板（按焦点路由，图标取自 DAGui 图标库）
    actionCut       = createAction(UiNames::Action::Cut, ":/DAGui/icon/cut.svg");
    actionCopy      = createAction(UiNames::Action::Copy, ":/DAGui/icon/copy.svg");
    actionPaste     = createAction(UiNames::Action::Paste, ":/DAGui/icon/paste.svg");
    actionDelete    = createAction(UiNames::Action::Delete, ":/DAGui/icon/delete.svg");
    actionSelectAll = createAction(UiNames::Action::SelectAll, ":/DAGui/icon/select-all.svg");
    // 默认快捷键就地定义（DAShortcutManager 注册时收集为默认值，用户覆盖见 shortcuts.json）；
    // 剪贴板组默认不设键：工作流编辑器内部同功能 action 已占用 Ctrl+C/X/V 等标准键，
    // 两者同属主窗口会真冲突，该组命令在「设置-快捷键」页可见并由用户自行分配
    actionNewProject->setShortcut(QKeySequence(QStringLiteral("Ctrl+N")));
    actionOpen->setShortcut(QKeySequence::Open);
    actionSave->setShortcut(QKeySequence::Save);
    actionSaveAs->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    actionSetting->setShortcut(QKeySequence(QStringLiteral("Ctrl+,")));
}

void DAAppActions::buildDataAction()
{
    // Data Category
    actionAddData    = createAction(UiNames::Action::AddData, ":/app/bright/Icon/addData.svg");
    actionRemoveData = createAction(UiNames::Action::RemoveData, ":/app/bright/Icon/removeData.svg");
    // 重命名数据集（树视图定位选中项并触发编辑）
    actionRenameData = createAction(UiNames::Action::RenameData, ":/app/bright/Icon/renameColumn.svg");
    // 数据导出（大按钮 + 三格式子项，挂在导出菜单中）
    actionExportData        = createAction(UiNames::Action::ExportData, ":/app/bright/Icon/exportToPic.svg");
    actionExportDataCsv     = createAction(UiNames::Action::ExportDataCsv, ":/app/bright/Icon/exportToPng.svg");
    actionExportDataExcel   = createAction(UiNames::Action::ExportDataExcel, ":/app/bright/Icon/exportToPng.svg");
    actionExportDataPickle  = createAction(UiNames::Action::ExportDataPickle, ":/app/bright/Icon/exportToPng.svg");
    actionExportDataParquet = createAction(UiNames::Action::ExportDataParquet, ":/app/bright/Icon/exportToPng.svg");
    // 数据操作的上下文标签 Data Operate Context Category
    actionRemoveRow         = createAction(UiNames::Action::RemoveRow, ":/app/bright/Icon/removeRow.svg");
    actionRemoveColumn      = createAction(UiNames::Action::RemoveColumn, ":/app/bright/Icon/removeColumn.svg");
    actionInsertRow         = createAction(UiNames::Action::InsertRow, ":/app/bright/Icon/insertRow.svg");
    actionInsertRowAbove    = createAction(UiNames::Action::InsertRowAbove, ":/app/bright/Icon/insertRowAbove.svg");
    actionInsertColumnRight = createAction(UiNames::Action::InsertColumnRight, ":/app/bright/Icon/insertColumnRight.svg");
    actionInsertColumnLeft  = createAction(UiNames::Action::InsertColumnLeft, ":/app/bright/Icon/insertColumnLeft.svg");
    actionRenameColumns     = createAction(UiNames::Action::RenameColumns, ":/app/bright/Icon/renameColumns.svg");
    actionRenameColumn      = createAction(UiNames::Action::RenameColumn, ":/app/bright/Icon/renameColumn.svg");
    actionCopyColumnName    = createAction(UiNames::Action::CopyColumnName, ":/app/bright/Icon/copyColumnName.svg");
    actionGotoMax           = createAction(UiNames::Action::GotoMax, ":/app/bright/Icon/gotoMax.svg");
    actionGotoMin           = createAction(UiNames::Action::GotoMin, ":/app/bright/Icon/gotoMin.svg");
    actionShowColumnDescribe = createAction(UiNames::Action::ShowColumnDescribe, ":/app/bright/Icon/showInfomation.svg");
    actionRemoveCell        = createAction(UiNames::Action::RemoveCell, ":/app/bright/Icon/removeCell.svg");
    actionCastToNum         = createAction(UiNames::Action::CastToNum, ":/app/bright/Icon/castToNum.svg");
    actionCastToString      = createAction(UiNames::Action::CastToString, ":/app/bright/Icon/castToString.svg");
    actionCastToDatetime    = createAction(UiNames::Action::CastToDatetime, ":/app/bright/Icon/castToDatetime.svg");

    actionChangeToIndex = createAction(UiNames::Action::ChangeToIndex, ":/app/bright/Icon/changeToIndex.svg");

    // 表格样式清除 action
    actionClearStyleSelected = createAction(UiNames::Action::ClearStyleSelected, ":/app/bright/Icon/removeCell.svg");
    actionClearStyleAll      = createAction(UiNames::Action::ClearStyleAll, ":/app/bright/Icon/removeData.svg");
    // 表格列显示格式 action
    actionTableFormatCells   = createAction(UiNames::Action::TableFormatCells, ":/app/bright/Icon/tableFormat.svg");
    // 表内查找/替换 action（002 T1.4；图标取 DAGui 图标库，快捷键在 DADataTableView，此处不设 shortcut 红线5）
    actionFind               = createAction(UiNames::Action::Find, ":/DAGui/icon/find.svg");
    actionReplace            = createAction(UiNames::Action::Replace, ":/DAGui/icon/replace.svg");
    // 条件格式 action（003 T1.5；图标取 DAGui 图标库，不引用 APP 资源）
    actionConditionalFormat  = createAction(UiNames::Action::ConditionalFormat, ":/DAGui/icon/conditional-format.svg");
    // 数据工具 action（004；图标取 DAGui 图标库，不引用 APP 资源）
    actionComputedColumn              = createAction(UiNames::Action::ComputedColumn, ":/DAGui/icon/computed-column.svg");
    actionRecomputeAllComputedColumns = createAction(UiNames::Action::RecomputeAllComputedColumns, ":/DAGui/icon/refresh-all.svg");
    actionSplitColumn                 = createAction(UiNames::Action::SplitColumn, ":/DAGui/icon/split-column.svg");
    actionMergeColumns                = createAction(UiNames::Action::MergeColumns, ":/DAGui/icon/merge-columns.svg");
    actionGroupBy                     = createAction(UiNames::Action::GroupBy, ":/DAGui/icon/group-by.svg");
    actionTranspose                   = createAction(UiNames::Action::Transpose, ":/DAGui/icon/transpose.svg");
    actionMelt                        = createAction(UiNames::Action::Melt, ":/DAGui/icon/melt.svg");
    // 填充 action（005 T1.24；图标取 DAGui 图标库，快捷键 Ctrl+D 在 DADataTableView，此处不设 shortcut 红线5）
    actionFill                        = createAction(UiNames::Action::Fill, ":/DAGui/icon/fill.svg");
    actionFillDown                    = createAction(UiNames::Action::FillDown, ":/DAGui/icon/fill-down.svg");
    actionFillUp                      = createAction(UiNames::Action::FillUp, ":/DAGui/icon/fill-up.svg");
    actionFillForward                 = createAction(UiNames::Action::FillForward, ":/DAGui/icon/fill-forward.svg");
    actionFillBackward                = createAction(UiNames::Action::FillBackward, ":/DAGui/icon/fill-backward.svg");
    actionFillSeries                  = createAction(UiNames::Action::FillSeries, ":/DAGui/icon/fill-series.svg");
    // 数据校验 action（005 T1.25；图标取 DAGui 图标库，不引用 APP 资源）
    actionValidationRules             = createAction(UiNames::Action::ValidationRules, ":/DAGui/icon/validation.svg");
    actionRunValidation               = createAction(UiNames::Action::RunValidation, ":/DAGui/icon/validation-run.svg");
    // 批注 action（005 T1.26；图标取 DAGui 图标库）
    actionNote                        = createAction(UiNames::Action::Note, ":/DAGui/icon/cell-note.svg");
    actionInsertNote                  = createAction(UiNames::Action::InsertNote, ":/DAGui/icon/cell-note.svg");
    actionEditNote                    = createAction(UiNames::Action::EditNote, ":/DAGui/icon/cell-note.svg");
    actionDeleteNote                  = createAction(UiNames::Action::DeleteNote, ":/DAGui/icon/delete.svg");
    actionClearNotes                  = createAction(UiNames::Action::ClearNotes, ":/DAGui/icon/note-clear.svg");
    // 列视图 action（006 T1.8；图标取 DAGui 图标库，不设快捷键——表格快捷键唯一挂点在 DADataTableView 红线5）
    actionFreezeColumns               = createAction(UiNames::Action::FreezeColumns, ":/DAGui/icon/freeze-column.svg");
    actionHideColumn                  = createAction(UiNames::Action::HideColumn, ":/DAGui/icon/hide-column.svg");
    actionShowAllColumns              = createAction(UiNames::Action::ShowAllColumns, ":/DAGui/icon/show-all-columns.svg");
    actionColumnViewDialog            = createAction(UiNames::Action::ColumnViewDialog, ":/DAGui/icon/column-view.svg");
    // 冻结菜单三项（无图标，照 actionInsertRowAbove 先例单参 createAction）
    actionFreezeFirstColumn           = createAction(UiNames::Action::FreezeFirstColumn);
    actionFreezeColumnsPrompt         = createAction(UiNames::Action::FreezeColumnsPrompt);
    actionUnfreezeColumns             = createAction(UiNames::Action::UnfreezeColumns);
    // 定位 action（007 T1.7；快捷键 Ctrl+G 在 DADataTableView，action 不设 shortcut 红线5）
    actionGotoCell                    = createAction(UiNames::Action::GotoCell, ":/DAGui/icon/goto-cell.svg");
}

void DAAppActions::buildChartAction()
{
    // 绘图标签 Chart Category
    actionAddFigure       = createAction(UiNames::Action::AddFigure, ":/app/bright/Icon/addFigure.svg");
    actionFigureNewXYAxis = createAction(UiNames::Action::FigureNewXYAxis, ":/app/bright/Icon/newAxis.svg");
    actionChartAddCurve   = createAction(UiNames::Action::ChartAddCurve, ":/DAGui/ChartType/icon/chart-type/chart-curve.svg");
    actionChartAddScatter2D =
        createAction(UiNames::Action::ChartAddScatter2D, ":/DAGui/ChartType/icon/chart-type/chart-scatter-2d.svg");
    actionChartAddErrorBar =
        createAction(UiNames::Action::ChartAddErrorBar, ":/DAGui/ChartType/icon/chart-type/chart-intervalcurve.svg");
    actionChartAddBoxPlot = createAction(UiNames::Action::ChartAddBoxPlot, ":/DAGui/ChartType/icon/chart-type/chart-OHLC.svg");
    actionChartAddBar     = createAction(UiNames::Action::ChartAddBar, ":/DAGui/ChartType/icon/chart-type/chart-bar.svg");
    actionChartAddMultiBar = createAction(UiNames::Action::ChartAddMultiBar, ":/DAGui/ChartType/icon/chart-type/chart-multibar.svg");
    actionChartAddHistogramBar =
        createAction(UiNames::Action::ChartAddHistogramBar, ":/DAGui/ChartType/icon/chart-type/chart-histogram.svg");
    actionChartAddContourMap =
        createAction(UiNames::Action::ChartAddContourMap, ":/DAGui/ChartType/icon/chart-type/chart-spectrocurve.svg");
    actionChartAddCloudMap =
        createAction(UiNames::Action::ChartAddCloudMap, ":/DAGui/ChartType/icon/chart-type/chart-spectrogram.svg");
    actionChartAddVectorfield =
        createAction(UiNames::Action::ChartAddVectorfield, ":/DAGui/ChartType/icon/chart-type/chart-vectorfield.svg");
    actionChartAdd3DSurface = createAction(UiNames::Action::ChartAdd3DSurface, ":/DAGui/ChartType/icon/chart-type/chart-surface3d.svg");
    actionChartAdd3DBar     = createAction(UiNames::Action::ChartAdd3DBar, ":/DAGui/ChartType/icon/chart-type/chart-bar3d.svg");
    actionChartAdd3DLine    = createAction(UiNames::Action::ChartAdd3DLine, ":/DAGui/ChartType/icon/chart-type/chart-line3d.svg");

    // 统计绘图 Stats Plot
    actionStatsHistplot     = createAction(UiNames::Action::StatsHistplot, ":/DAGui/ChartType/icon/chart-type/stats-histplot.svg");
    actionStatsKdeplot1d    = createAction(UiNames::Action::StatsKdeplot1d, ":/DAGui/ChartType/icon/chart-type/stats-kdeplot.svg");
    actionStatsKdeplot2d    = createAction(UiNames::Action::StatsKdeplot2d, ":/DAGui/ChartType/icon/chart-type/stats-kdeplot-2d.svg");
    actionStatsBoxplot      = createAction(UiNames::Action::StatsBoxplot, ":/DAGui/ChartType/icon/chart-type/stats-boxplot.svg");
    actionStatsHeatmap      = createAction(UiNames::Action::StatsHeatmap, ":/DAGui/ChartType/icon/chart-type/stats-heatmap.svg");
    actionStatsScatterplot  = createAction(UiNames::Action::StatsScatterplot, ":/DAGui/ChartType/icon/chart-type/stats-scatterplot.svg");
    actionStatsBarplot      = createAction(UiNames::Action::StatsBarplot, ":/DAGui/ChartType/icon/chart-type/stats-barplot.svg");
    actionStatsRegplot      = createAction(UiNames::Action::StatsRegplot, ":/DAGui/ChartType/icon/chart-type/stats-regplot.svg");
    actionStatsECDFplot     = createAction(UiNames::Action::StatsECDFplot, ":/DAGui/ChartType/icon/chart-type/stats-ecdfplot.svg");

    actionFigureSettingApplyAllChart =
        createAction(UiNames::Action::FigureSettingApplyAllChart, ":/app/bright/Icon/apply-to-all-figure.svg", true, true);
    actionChartEnableGrid  = createAction(UiNames::Action::ChartEnableGrid, ":/app/bright/Icon/chart-grid.svg", true, false);
    actionChartEnableGridX = createAction(UiNames::Action::ChartEnableGridX, ":/app/bright/Icon/chart-grid-x.svg", true, false);
    actionChartEnableGridY = createAction(UiNames::Action::ChartEnableGridY, ":/app/bright/Icon/chart-grid-y.svg", true, false);
    actionChartEnableGridXMin =
        createAction(UiNames::Action::ChartEnableGridXMin, ":/app/bright/Icon/chart-grid-xmin.svg", true, false);
    actionChartEnableGridYMin =
        createAction(UiNames::Action::ChartEnableGridYMin, ":/app/bright/Icon/chart-grid-ymin.svg", true, false);
    actionChartEnableZoom = createAction(UiNames::Action::ChartEnableZoom, ":/app/bright/Icon/chart-zoomer.svg", true, false);
    actionChartZoomIn     = createAction(UiNames::Action::ChartZoomIn, ":/app/bright/Icon/zoomIn.svg");
    actionChartZoomOut    = createAction(UiNames::Action::ChartZoomOut, ":/app/bright/Icon/zoomOut.svg");
    actionChartZoomAll    = createAction(UiNames::Action::ChartZoomAll, ":/app/bright/Icon/viewAll.svg");
    actionChartEnablePan  = createAction(UiNames::Action::ChartEnablePan, ":/app/bright/Icon/chart-pan.svg", true, false);
    actionChartDisableZoomX =
        createAction(UiNames::Action::ChartDisableZoomX, ":/app/bright/Icon/chart-zoom-lock-x.svg", true, false);
    actionChartDisableZoomY =
        createAction(UiNames::Action::ChartDisableZoomY, ":/app/bright/Icon/chart-zoom-lock-y.svg", true, false);

    actionGroupChartPickers = new QActionGroup(this);
    actionGroupChartPickers->setObjectName(QString::fromUtf8(UiNames::Action::GroupChartPickers));
    actionGroupChartPickers->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  // 允许都不选中
    actionChartEnablePickerCross = createAction(
        UiNames::Action::ChartEnablePickerCross, ":/app/bright/Icon/chart-picker.svg", true, false, actionGroupChartPickers);
    actionChartEnablePickerXY = createAction(
        UiNames::Action::ChartEnablePickerXY, ":/app/bright/Icon/chart-picker-xy.svg", true, false, actionGroupChartPickers);
    actionChartEnablePickerY = createAction(
        UiNames::Action::ChartEnablePickerY, ":/app/bright/Icon/chart-picker-y.svg", true, false, actionGroupChartPickers);
    actionChartLinkAllPickerEnabled = createAction(UiNames::Action::LinkAllPicker, ":/app/bright/Icon/link-pick.svg", true, false);

    actionGroupChartPickerTextRegion = new QActionGroup(this);
    actionGroupChartPickerTextRegion->setObjectName(QString::fromUtf8(UiNames::Action::GroupChartPickerTextRegion));
    actionGroupChartPickerTextRegion->setExclusive(true);
    actionChartPickerTextAtLeftTop = createAction(
        UiNames::Action::ChartPickerTextAtLeftTop, ":/app/bright/Icon/left-top.svg", true, false, actionGroupChartPickerTextRegion);
    actionChartPickerTextAtLeftTop->setData(QwtPlotSeriesDataPicker::TextOnCanvasTopLeft);
    actionChartPickerTextAtLeftBottom = createAction(
        UiNames::Action::ChartPickerTextAtLeftBottom, ":/app/bright/Icon/left-bottom.svg", true, false, actionGroupChartPickerTextRegion);
    actionChartPickerTextAtLeftBottom->setData(QwtPlotSeriesDataPicker::TextOnCanvasBottomLeft);
    actionChartPickerTextAtRightTop = createAction(
        UiNames::Action::ChartPickerTextAtRightTop, ":/app/bright/Icon/right-top.svg", true, false, actionGroupChartPickerTextRegion);
    actionChartPickerTextAtRightTop->setData(QwtPlotSeriesDataPicker::TextOnCanvasTopRight);
    actionChartPickerTextAtRightBottom = createAction(UiNames::Action::ChartPickerTextAtRightBottom,
                                                      ":/app/bright/Icon/right-bottom.svg",
                                                      true,
                                                      false,
                                                      actionGroupChartPickerTextRegion);
    actionChartPickerTextAtRightBottom->setData(QwtPlotSeriesDataPicker::TextOnCanvasBottomRight);
    actionChartPickerTextFollowMouse = createAction(
        UiNames::Action::ChartPickerTextFollowMouse, ":/app/bright/Icon/follow-mouse.svg", true, true, actionGroupChartPickerTextRegion);
    actionChartPickerTextFollowMouse->setData(QwtPlotSeriesDataPicker::TextFollowMouse);
    actionChartYPickerShowXValueEnabled = createAction(UiNames::Action::ChartYPickerShowXValueEnabled, true, true);

    actionChartEnableLegend = createAction(UiNames::Action::ChartEnableLegend, ":/app/bright/Icon/chart-legend.svg", true, false);
    actionCopyFigureInClipboard = createAction(UiNames::Action::CopyFigureInClipboard, ":/app/bright/Icon/copy-figure.svg");

    // 图例位置（checkable 互斥组）
    actionGroupChartLegendPosition = new QActionGroup(this);
    actionGroupChartLegendPosition->setObjectName(QString::fromUtf8(UiNames::Action::GroupChartLegendPosition));
    actionGroupChartLegendPosition->setExclusive(true);
    actionChartLegendAtTop    = createAction(
        UiNames::Action::ChartLegendAtTop, ":/app/bright/Icon/left-top.svg", true, false, actionGroupChartLegendPosition);
    actionChartLegendAtBottom = createAction(
        UiNames::Action::ChartLegendAtBottom, ":/app/bright/Icon/left-bottom.svg", true, false, actionGroupChartLegendPosition);
    actionChartLegendAtLeft   = createAction(
        UiNames::Action::ChartLegendAtLeft, ":/app/bright/Icon/left-top.svg", true, false, actionGroupChartLegendPosition);
    actionChartLegendAtRight  = createAction(
        UiNames::Action::ChartLegendAtRight, ":/app/bright/Icon/right-top.svg", true, false, actionGroupChartLegendPosition);
    actionChartLegendAtTop->setData(static_cast< int >(Qt::AlignTop));
    actionChartLegendAtBottom->setData(static_cast< int >(Qt::AlignBottom));
    actionChartLegendAtLeft->setData(static_cast< int >(Qt::AlignLeft));
    actionChartLegendAtRight->setData(static_cast< int >(Qt::AlignRight));
    actionChartLegendAtRight->setChecked(true);  ///< 默认图例在右侧

    actionGroupChartEditor = new QActionGroup(this);
    actionGroupChartEditor->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  // 允许所有都不选择
    actionChartEditorResizeSubChart = createAction(
        UiNames::Action::ChartEditorResizeSubChart, ":/app/bright/Icon/figureResizeChart.svg", true, false, actionGroupChartEditor);
    actionChartEditorPointerSelector = createAction(
        UiNames::Action::ChartEditorPointerSelector, ":/app/bright/Icon/chart-pointer-selector.svg", true, false, actionGroupChartEditor);
    actionChartEditorRectSelector = createAction(
        UiNames::Action::ChartEditorRectSelector, ":/app/bright/Icon/chart-selector-rect.svg", true, false, actionGroupChartEditor);
    actionChartEditorEllipseSelector = createAction(
        UiNames::Action::ChartEditorEllipseSelector, ":/app/bright/Icon/chart-selector-ellipse.svg", true, false, actionGroupChartEditor);
    actionChartEditorPolygonSelector = createAction(
        UiNames::Action::ChartEditorPolygonSelector, ":/app/bright/Icon/chart-selector-polygon.svg", true, false, actionGroupChartEditor);
    actionChartEditorAddCrossMarker = createAction(
        UiNames::Action::ChartEditorAddCrossMarker, ":/app/bright/Icon/chart-corss-marker.svg", true, false, actionGroupChartEditor);
    actionChartEditorAddHLineMarker = createAction(
        UiNames::Action::ChartEditorAddHLineMarker, ":/app/bright/Icon/chart-hline-marker.svg", true, false, actionGroupChartEditor);
    actionChartEditorAddVLineMarker = createAction(
        UiNames::Action::ChartEditorAddVLineMarker, ":/app/bright/Icon/chart-vline-marker.svg", true, false, actionGroupChartEditor);
    actionChartEditorAddArrowMarker = createAction(
        UiNames::Action::ChartEditorAddArrowMarker, ":/app/bright/Icon/arrow.svg", true, false, actionGroupChartEditor);
    actionChartEditorAddTextMarker = createAction(
        UiNames::Action::ChartEditorAddTextMarker, ":/app/bright/Icon/chart-text-marker.svg", true, false, actionGroupChartEditor);
    actionChartEditorDataProbe = createAction(
        UiNames::Action::ChartEditorDataProbe, ":/app/bright/Icon/plot-probe.svg", true, false, actionGroupChartEditor);
    actionChartDataPickerSetting   = createAction(UiNames::Action::ChartDataPickerSetting, ":/DAGui/icon/data-picker-setting.svg");
    actionChartEditorResizeSubChart->setData(static_cast< int >(DAFigureWidget::SubChartEditor));
    actionChartEditorPointerSelector->setData(static_cast< int >(DAFigureWidget::PointerSelector));
    actionChartEditorRectSelector->setData(static_cast< int >(DAFigureWidget::RectSelectEditor));
    actionChartEditorEllipseSelector->setData(static_cast< int >(DAFigureWidget::EllipseSelectEditor));
    actionChartEditorPolygonSelector->setData(static_cast< int >(DAFigureWidget::PolygonSelectEditor));
    actionChartEditorAddHLineMarker->setData(static_cast< int >(DAFigureWidget::HLineMarker));
    actionChartEditorAddVLineMarker->setData(static_cast< int >(DAFigureWidget::VLineMarker));
    actionChartEditorAddCrossMarker->setData(static_cast< int >(DAFigureWidget::CrossMarker));
    actionChartEditorAddArrowMarker->setData(static_cast< int >(DAFigureWidget::ArrowMarker));
    actionChartEditorAddTextMarker->setData(static_cast< int >(DAFigureWidget::TextMarker));
    actionChartEditorDataProbe->setData(static_cast< int >(DAFigureWidget::DataProbeEditor));
}

void DAAppActions::buildViewAction()
{
    // View Category
    actionShowWorkFlowArea = createAction(UiNames::Action::ShowWorkFlowArea, ":/app/bright/Icon/showWorkFlow.svg");
    actionShowWorkFlowManagerArea =
        createAction(UiNames::Action::ShowWorkFlowManagerArea, ":/app/bright/Icon/workflow-manager-view.svg");
    actionShowChartArea        = createAction(UiNames::Action::ShowChartArea, ":/app/bright/Icon/showChart.svg");
    actionShowChartManagerArea = createAction(UiNames::Action::ShowChartManagerArea, ":/app/bright/Icon/chart-manager-view.svg");
    actionShowDataArea         = createAction(UiNames::Action::ShowDataArea, ":/app/bright/Icon/showTable.svg");
    actionShowDataManagerArea  = createAction(UiNames::Action::ShowDataManagerArea, ":/app/bright/Icon/data-manager-view.svg");
    actionShowMessageLogView   = createAction(UiNames::Action::ShowMessageLogView, ":/app/bright/Icon/showInfomation.svg");
    actionShowUndoHistory      = createAction(UiNames::Action::ShowUndoHistory, ":/DAGui/icon/undo-history.svg");  // 005 T1.21（图标取 DAGui 图标库）
    actionShowSettingWidget    = createAction(UiNames::Action::ShowSettingWidget, ":/app/bright/Icon/showSettingWidget.svg");
    actionShowLeftSideBar = createAction(UiNames::Action::ShowLeftSideBar, ":/app/bright/Icon/left-sider-bar.svg", true, true);
    actionShowRightSideBar = createAction(UiNames::Action::ShowRightSideBar, ":/app/bright/Icon/right-sider-bar.svg", true, true);
    actionShowAgentArea = createAction(UiNames::Action::ShowAgentArea, ":/app/bright/Icon/showAgent.svg");
    actionResetDefaultLayout = createAction(UiNames::Action::ResetDefaultLayout, ":/app/bright/Icon/viewAll.svg");
    actionManageLayouts      = createAction(UiNames::Action::ManageLayouts, ":/app/bright/Icon/layoutManage.svg");
}

void DAAppActions::buildWorkflowAction()
{
    // workflow 编辑
    actionWorkflowNew = createAction(UiNames::Action::WorkflowNew, ":/app/bright/Icon/newWorkflow.svg");
    actionWorkflowEnableItemLinkageMove =
        createAction(UiNames::Action::WorkflowEnableItemLinkageMove, ":/app/bright/Icon/itemLinkageMove.svg", true, false);
    actionItemGrouping       = createAction(UiNames::Action::ItemSetGroup, ":/app/bright/Icon/item-set-group.svg");
    actionItemUngroup        = createAction(UiNames::Action::ItemCancelGroup, ":/app/bright/Icon/item-cancel-group.svg");
    actionWorkflowLinkEnable = createAction(UiNames::Action::WorkflowLinkEnable, ":/app/bright/Icon/link.svg", true, true);
    // workflow下面的状态action都是checkable状态的
    actionGroupWorkflowStartEdit = new QActionGroup(this);
    actionGroupWorkflowStartEdit->setObjectName(QString::fromUtf8(UiNames::Action::GroupWorkflowStartEdit));
    actionGroupWorkflowStartEdit->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  // 允许都不选中
    actionWorkflowStartDrawRect =
        createAction(UiNames::Action::StartDrawRect, ":/app/bright/Icon/drawRect.svg", true, false, actionGroupWorkflowStartEdit);
    actionWorkflowStartDrawText =
        createAction(UiNames::Action::StartDrawText, ":/app/bright/Icon/drawText.svg", true, false, actionGroupWorkflowStartEdit);
    // workflow-背景图相关
    actionWorkflowAddBackgroundPixmap = createAction(UiNames::Action::AddBackgroundPixmap, ":/app/bright/Icon/backgroundPixmap.svg");
    actionWorkflowLockBackgroundPixmap =
        createAction(UiNames::Action::LockBackgroundPixmap, ":/app/bright/Icon/lock-bk.svg", true, false);
    actionWorkflowEnableItemMoveWithBackground =
        createAction(UiNames::Action::EnableItemMoveWithBackground, ":/app/bright/Icon/itemMoveWithBackground.svg", true, false);
    // workflow-视图操作
    actionWorkflowShowGrid     = createAction(UiNames::Action::WorkflowShowGrid, ":/app/bright/Icon/showGrid.svg", true, true);
    actionWorkflowViewReadOnly = createAction(UiNames::Action::WorkflowViewLock, ":/app/bright/Icon/lock-view.svg", true, false);
    actionWorkflowViewMarker = createAction(UiNames::Action::WorkflowViewMarker, ":/app/bright/Icon/view-marker.svg", true, false);
    // 运行
    actionWorkflowRun       = createAction(UiNames::Action::WorkflowRun, ":/app/bright/Icon/run.svg");
    actionWorkflowTerminate = createAction(UiNames::Action::WorkflowTerminate, ":/app/bright/Icon/stop.svg");
    actionWorkflowTerminate->setEnabled(false);
    // 导出
    actionExportWorkflowSceneToImage = createAction(UiNames::Action::ExportWorkflowSceneToImage, ":/app/bright/Icon/exportToPic.svg");
    actionExportWorkflowSceneToPNG = createAction(UiNames::Action::ExportWorkflowSceneToPNG, ":/app/bright/Icon/exportToPng.svg");
    // 默认快捷键（IDE 习惯：F5 运行 / Shift+F5 停止）
    actionWorkflowNew->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+N")));
    actionWorkflowRun->setShortcut(QKeySequence(Qt::Key_F5));
    actionWorkflowTerminate->setShortcut(QKeySequence(QStringLiteral("Shift+F5")));
}

void DAAppActions::buildOtherActions()
{
}

void DAAppActions::buildColorThemeActions()
{
    actionListOfColorTheme.clear();
    for (int i = DAColorTheme::BuiltInStyle_Begin; i < DAColorTheme::BuiltInStyle_End; ++i) {
        DAColorTheme::ColorThemeStyle style = static_cast< DAColorTheme::ColorThemeStyle >(i);
        DAColorTheme theme                  = DAColorTheme(style);
        // 创建actions
        QString themeName = DAColorTheme::colorThemeStyleName(style);
        QAction* act      = new QAction(themeName);
        act->setObjectName(QString("actionColorTheme%1").arg(themeName));
        QPixmap icon = createColorThemePixmap(theme.toColorList(), QSize(100, 50));
        act->setIcon(QIcon(icon));
        recordAction(act);
        // 记录枚举值
        act->setData(i);
        actionListOfColorTheme.append(act);
    }
}

QPixmap DAAppActions::createColorThemePixmap(const QList< QColor >& clrs, const QSize& size) const
{
    // 创建指定大小的pixmap
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    if (clrs.isEmpty()) {
        // 如果没有颜色，返回透明pixmap
        return pixmap;
    }

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 设置边距，防止边框被裁剪
    const int margin          = 2;
    const int availableWidth  = size.width() - 2 * margin;
    const int availableHeight = size.height() - 2 * margin;

    // 计算每个柱子的宽度
    const int barCount = clrs.size();
    const int barWidth = availableWidth / barCount;

    // 创建从低到高再到低的高度模式
    // 使用正弦波或抛物线形状
    QVector< int > barHeights(barCount);

    if (barCount == 1) {
        // 只有一个颜色时，柱子高度为可用高度的80%
        barHeights[ 0 ] = availableHeight * 0.8;
    } else {
        // 多个颜色时，创建波浪形高度
        for (int i = 0; i < barCount; i++) {
            // 使用正弦函数创建从低到高再到低的效果
            // 将i映射到[0, π]区间
            const double PI = 3.14159265359;
            double x        = static_cast< double >(i) / (barCount - 1) * PI;
            double sinValue = sin(x);

            // 将sin值从[-1,1]映射到[0.3, 1.0]区间
            double heightRatio = 0.35 + 0.65 * (sinValue + 1) / 2;
            barHeights[ i ]    = static_cast< int >(availableHeight * heightRatio);
        }
    }

    // 绘制柱子
    for (int i = 0; i < barCount; i++) {
        int x      = margin + i * barWidth;
        int height = barHeights[ i ];

        // 计算y坐标（柱子底部对齐）
        int y = margin + availableHeight - height;

        // 绘制柱子填充
        painter.setPen(Qt::NoPen);
        painter.setBrush(clrs[ i ]);
        painter.drawRect(x, y, barWidth - 1, height);

        // 绘制1像素边框
        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(x, y, barWidth - 1, height);

        // 如果需要，可以添加柱子上方的边框线
        painter.setPen(QPen(QColor(255, 255, 255, 50), 1));
        painter.drawLine(x + 1, y + 1, x + barWidth - 2, y + 1);
        painter.drawLine(x + 1, y + 1, x + 1, y + 3);
    }

    // 绘制外边框
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, size.width() - 1, size.height() - 1);

    return pixmap;
}

void DAAppActions::retranslateUi()
{
    // Main Category
    actionNewProject->setText(tr("New Project"));                  // cn:新建工程
    actionNewProject->setToolTip(tr("Create a new empty project"));  // cn:创建新的空工程
    actionOpen->setText(tr("Open"));                     // cn:打开
    actionOpen->setToolTip(tr("Open file or project"));  // cn:打开文件或项目
    actionSave->setText(tr("Save"));                     // cn:保存
    actionSave->setToolTip(tr("Save file or project"));                                // cn:保存文件或项目
    actionSaveAs->setText(tr("Save As"));                                              // cn:另存为
    actionSaveAs->setToolTip(tr("Save file or project as"));                           // cn:保存文件或项目为
    actionAppendProject->setText(tr("Append To Project"));                             // cn:追加到工程
    actionAppendProject->setToolTip(tr("Append file or project to current project"));  // cn:附加文件或项目到当前项目
    actionOpenMarkdown->setText(tr("Open Markdown"));  // cn:打开 Markdown
    actionOpenMarkdown->setToolTip(tr("Open a Markdown file and display it in the central area"));  // cn:打开 Markdown 文件并在中央区显示

    actionRenameColumns->setText(tr("Rename Columns"));                           // cn:重命名列
    actionRenameColumns->setToolTip(tr("Rename columns in the selected table"));  // cn:重命名选中表格的列
    actionSetting->setText(tr("Setting"));                                        // cn:设置
    actionSetting->setToolTip(tr("Setting for the application"));                 // cn:应用程序设置
    actionAbout->setText(tr("About"));                                            // cn:关于
    actionAbout->setToolTip(tr("About the application"));                         // cn:关于应用程序
                                                                                  // Data Category
    actionAddData->setText(tr("Add Data"));                                     // cn:添加数据
    actionAddData->setToolTip(tr("Add data to the table"));                       // cn:添加数据到表格
    actionRemoveData->setText(tr("Remove Data"));                               // cn:移除数据
    actionRemoveData->setToolTip(tr("Remove data from the table"));               // cn:从表格中移除数据
    actionRenameData->setText(tr("Rename Data"));                               // cn:重命名\n数据
    actionRenameData->setToolTip(tr("Rename the selected dataset"));              // cn:重命名选中的数据集
    actionExportData->setText(tr("Export Data"));                               // cn:导出数据
    actionExportData->setToolTip(tr("Export the selected data to a file"));       // cn:导出选中的数据到文件
    actionExportDataCsv->setText(tr("Export CSV"));                               // cn:导出 CSV
    actionExportDataCsv->setToolTip(tr("Export the selected data to a CSV file"));  // cn:导出选中的数据为 CSV 文件
    actionExportDataExcel->setText(tr("Export Excel"));                           // cn:导出 Excel
    actionExportDataExcel->setToolTip(tr("Export the selected data to an Excel file"));  // cn:导出选中的数据为 Excel 文件
    actionExportDataPickle->setText(tr("Export Pickle"));                         // cn:导出 Pickle
    actionExportDataPickle->setToolTip(tr("Export the selected data to a pickle file"));  // cn:导出选中的数据为 Pickle 文件
    actionExportDataParquet->setText(tr("Export Parquet"));                       // cn:导出 Parquet
    actionExportDataParquet->setToolTip(tr("Export the selected data to a parquet file"));  // cn:导出选中的数据为 Parquet 文件
    //-----------------------------------------------------
    // Chart Category
    //-----------------------------------------------------
    actionAddFigure->setText(tr("Add Figure"));                                  // cn:添加绘图
    actionAddFigure->setToolTip(tr("Add a figure to the workspace"));              // cn:添加绘图到工作区
    actionChartEditorResizeSubChart->setText(tr("Resize Chart"));                // cn:绘图尺寸
    actionChartEditorResizeSubChart->setToolTip(tr("Resize the sub-chart"));       // cn:调整子图尺寸
    actionFigureNewXYAxis->setText(tr("New XY Axis"));                           // cn:新建\n坐标系
    actionFigureNewXYAxis->setToolTip(tr("Add a new XY axis to the figure"));      // cn:新建坐标系
    actionChartAddCurve->setText(tr("Add Curve"));                               // cn:折线图
    actionChartAddCurve->setToolTip(tr("Add a curve to the chart"));               // cn:添加折线图
    actionChartAddScatter2D->setText(tr("Add Scatter"));                         // cn:散点图
    actionChartAddScatter2D->setToolTip(tr("Add a scatter plot to the chart"));    // cn:添加散点图
    actionChartAddErrorBar->setText(tr("Add Error Bar"));                        // cn:误差棒图
    actionChartAddErrorBar->setToolTip(tr("Add an error bar to the chart"));       // cn:添加误差棒图
    actionChartAddBoxPlot->setText(tr("Add Box Plot"));                          // cn:箱线图
    actionChartAddBoxPlot->setToolTip(tr("Add a box plot to the chart"));          // cn:添加箱线图
    actionChartAddBar->setText(tr("Add Bar"));                                   // cn:柱状图
    actionChartAddBar->setToolTip(tr("Add a bar chart to the chart"));             // cn:添加柱状图
    actionChartAddMultiBar->setText(tr("Add MultiBar"));                         // cn:多重柱状图
    actionChartAddMultiBar->setToolTip(tr("Add a multi-bar chart to the chart"));  // cn:添加多重柱状图
    actionChartAddHistogramBar->setText(tr("Add Histogram"));                    // cn:分布图
    actionChartAddHistogramBar->setToolTip(tr("Add a histogram to the chart"));    // cn:添加分布图
    actionChartAddContourMap->setText(tr("Add Contour Map"));                    // cn:等高线图
    actionChartAddContourMap->setToolTip(tr("Add a contour map to the chart"));    // cn:添加等高线图
    actionChartAddCloudMap->setText(tr("Add Cloud Map"));                        // cn:云图
    actionChartAddCloudMap->setToolTip(tr("Add a cloud map to the chart"));        // cn:添加云图
    actionChartAddVectorfield->setText(tr("Add Vector Field"));                  // cn:向量场图
    actionChartAddVectorfield->setToolTip(tr("Add a vector field to the chart"));  // cn:添加向量场图
    actionChartAdd3DSurface->setText(tr("Add Surface 3D"));                        // cn:3D曲面图
    actionChartAdd3DSurface->setToolTip(tr("Add a 3D surface plot to the chart"));   // cn:添加3D曲面图
    actionChartAdd3DBar->setText(tr("Add Bar 3D"));                                 // cn:3D柱状图
    actionChartAdd3DBar->setToolTip(tr("Add a 3D bar chart to the chart"));           // cn:添加3D柱状图
    actionChartAdd3DLine->setText(tr("Add Line 3D"));                              // cn:3D线图
    actionChartAdd3DLine->setToolTip(tr("Add a 3D line plot to the chart"));          // cn:添加3D线图

    // Stats Plot
    actionStatsHistplot->setText(tr("Histplot"));                                        // cn:直方图
    actionStatsHistplot->setToolTip(tr("Histogram with optional KDE overlay"));         // cn:直方图（可叠加核密度曲线）
    actionStatsKdeplot1d->setText(tr("KDE 1D"));                                        // cn:一维核密度
    actionStatsKdeplot1d->setToolTip(tr("1D Kernel Density Estimation plot"));          // cn:一维核密度图
    actionStatsKdeplot2d->setText(tr("KDE 2D"));                                        // cn:二维核密度
    actionStatsKdeplot2d->setToolTip(tr("2D Kernel Density Estimation plot"));          // cn:二维核密度图
    actionStatsBoxplot->setText(tr("Boxplot"));                                         // cn:箱线图
    actionStatsBoxplot->setToolTip(tr("Box plot with statistics"));                     // cn:统计箱线图
    actionStatsHeatmap->setText(tr("Heatmap"));                                         // cn:热力图
    actionStatsHeatmap->setToolTip(tr("Heatmap matrix plot"));                          // cn:热力图
    actionStatsScatterplot->setText(tr("Scatter"));                                    // cn:散点图
    actionStatsScatterplot->setToolTip(tr("Scatter plot with grouping"));               // cn:统计散点图
    actionStatsBarplot->setText(tr("Barplot"));                                         // cn:柱状图
    actionStatsBarplot->setToolTip(tr("Bar plot with aggregation"));                    // cn:统计柱状图
    actionStatsRegplot->setText(tr("Regplot"));                                         // cn:回归图
    actionStatsRegplot->setToolTip(tr("Regression plot with CI"));                     // cn:回归图（含置信区间）
    actionStatsECDFplot->setText(tr("ECDF"));                                           // cn:经验累积分布
    actionStatsECDFplot->setToolTip(tr("Empirical Cumulative Distribution Function"));  // cn:经验累积分布图

    actionFigureSettingApplyAllChart->setText(tr("Apply All Charts"));  // cn:应用到\n所有绘图
    actionFigureSettingApplyAllChart->setToolTip(tr(
        "When this feature is selected, operations on the figure will "
        "apply to all plots, not just the currently selected one"));  // cn:此功能选中后，figure上的操作将应用到所有绘图，而不仅仅是当前选中的绘图
    actionChartEnableGrid->setText(tr("Enable Grid"));                                // cn:网格
    actionChartEnableGrid->setToolTip(tr("Enable or disable grid in the chart"));     // cn:启用或禁用图表中的网格
    actionChartEnableGridX->setText(tr("X Grid"));                                    // cn:横向网格
    actionChartEnableGridX->setToolTip(tr("Enable or disable X grid in the chart"));  // cn:启用或禁用图表中的横向网格
    actionChartEnableGridY->setText(tr("Y Grid"));                                    // cn:纵向网格
    actionChartEnableGridY->setToolTip(tr("Enable or disable Y grid in the chart"));  // cn:启用或禁用图表中的纵向网格
    actionChartEnableGridXMin->setText(tr("Xmin Grid"));                              // cn:横向密集网格
    actionChartEnableGridXMin->setToolTip(tr("Enable or disable Xmin grid in the chart"));  // cn:启用或禁用图表中的横向密集网格
    actionChartEnableGridYMin->setText(tr("Ymin Grid"));                                    // cn:纵向密集网格
    actionChartEnableGridYMin->setToolTip(tr("Enable or disable Ymin grid in the chart"));  // cn:启用或禁用图表中的纵向密集网格
    actionChartEnableZoom->setText(tr("Zoom"));                                             // cn:缩放
    actionChartEnableZoom->setToolTip(tr("Enable or disable zoom in the chart"));           // cn:启用或禁用图表中的缩放
    actionChartZoomIn->setText(tr("Zoom In"));                                              // cn:放大
    actionChartZoomIn->setToolTip(tr("Zoom in on the chart"));                              // cn:放大
    actionChartZoomOut->setText(tr("Zoom Out"));                                            // cn:缩小
    actionChartZoomOut->setToolTip(tr("Zoom out of the chart"));                            // cn:缩小
    actionChartZoomAll->setText(tr("Show All"));                                          // cn:显示全部
    actionChartZoomAll->setToolTip(tr("Zoom to show all data in the chart"));               // cn:缩放以显示所有数据
    actionChartEnablePan->setText(tr("Pan"));                                               // cn:拖动
    actionChartEnablePan->setToolTip(tr("Enable or disable pan in the chart"));             // cn:启用或禁用图表中的拖动
    actionChartDisableZoomX->setText(tr("Disable X Zoom"));                                 // cn:禁止水平缩放
    actionChartDisableZoomX->setToolTip(
        tr("Disable zoom on the X axis"));  // cn:禁止水平缩放，滚轮和放大/缩小按钮将仅作用于垂直方向
    actionChartDisableZoomY->setText(tr("Disable Y Zoom"));                                 // cn:禁止垂直缩放
    actionChartDisableZoomY->setToolTip(
        tr("Disable zoom on the Y axis"));  // cn:禁止垂直缩放，滚轮和放大/缩小按钮将仅作用于水平方向
    actionChartEnablePickerCross->setText(tr("Cross"));                                     // cn:十字标记
    actionChartEnablePickerCross->setToolTip(tr("Enable or disable cross picker in the chart"));  // cn:启用或禁用图表中的十字标记
    actionChartEnablePickerY->setText(tr("Y Picker"));                                            // cn:y值拾取
    actionChartPickerTextAtLeftTop->setText(tr("At Canvas Left Top"));                            // cn:在画布左上
    actionChartPickerTextAtLeftTop->setToolTip(tr("Set picker text at canvas left top corner"));  // cn:设置拾取文本在画布左上角
    actionChartPickerTextAtLeftBottom->setText(tr("At Canvas Left Bottom"));                      // cn:在画布左下
    actionChartPickerTextAtLeftBottom->setToolTip(tr("Set picker text at canvas left bottom corner"));  // cn:设置拾取文本在画布左下角
    actionChartPickerTextAtRightTop->setText(tr("At Canvas Right Top"));                                // cn:在画布右上
    actionChartPickerTextAtRightTop->setToolTip(tr("Set picker text at canvas right top corner"));  // cn:设置拾取文本在画布右上角
    actionChartYPickerShowXValueEnabled->setText(tr("Y Picker Show X Value"));                      // cn:y拾取显示x值
    actionChartYPickerShowXValueEnabled->setToolTip(tr("Enable or disable X value display for Y picker"));  // cn:启用或禁用y拾取显示x值
    actionChartPickerTextAtRightBottom->setText(tr("At Canvas Right Bottom"));  // cn:在画布右下
    actionChartPickerTextAtRightBottom->setToolTip(tr("Set picker text at canvas right bottom corner"));  // cn:设置拾取文本在画布右下角
    actionChartPickerTextFollowMouse->setText(tr("Follow Mouse"));                                        // cn:跟随鼠标
    actionChartPickerTextFollowMouse->setToolTip(tr("Set picker text to follow mouse cursor"));  // cn:设置拾取文本跟随鼠标
    actionChartEnablePickerXY->setText(tr("XY Picker"));                                         // cn:点拾取
    actionChartEnablePickerXY->setToolTip(tr("Enable or disable XY picker in the chart"));  // cn:启用或禁用图表中的点拾取
    actionChartLinkAllPickerEnabled->setText(tr("Link All Picker"));                        // cn:联动拾取
    actionChartLinkAllPickerEnabled->setToolTip(tr("Enable or disable all picker linked"));  // cn:启用或禁用所有拾取联动
    actionChartEnableLegend->setText(tr("Legend"));                                          // cn:图例
    actionChartEnableLegend->setToolTip(tr("Enable or disable legend in the chart"));  // cn:启用或禁用图表中的图例
    actionCopyFigureInClipboard->setText(tr("Copy To Clipboard"));                     // cn:复制到\n剪贴板
    actionCopyFigureInClipboard->setToolTip(tr("Copy the figure to the clipboard"));   // cn:将绘图复制到剪贴板
    actionChartLegendAtTop->setText(tr("Legend Top"));                                  // cn:图例在上
    actionChartLegendAtTop->setToolTip(tr("Place the legend at the top of the chart"));  // cn:图例置于图表上方
    actionChartLegendAtBottom->setText(tr("Legend Bottom"));                            // cn:图例在下
    actionChartLegendAtBottom->setToolTip(tr("Place the legend at the bottom of the chart"));  // cn:图例置于图表下方
    actionChartLegendAtLeft->setText(tr("Legend Left"));                                // cn:图例在左
    actionChartLegendAtLeft->setToolTip(tr("Place the legend at the left of the chart"));  // cn:图例置于图表左侧
    actionChartLegendAtRight->setText(tr("Legend Right"));                              // cn:图例在右
    actionChartLegendAtRight->setToolTip(tr("Place the legend at the right of the chart"));  // cn:图例置于图表右侧

    actionChartEditorPointerSelector->setText(tr("Pointer"));  // cn:指针
    actionChartEditorPointerSelector->setToolTip(
        tr("Select chart elements by clicking, drag to move, Delete to remove"));  // cn:点击选择绘图元素（曲线/坐标轴/标题等），拖动移动，Delete删除
    actionChartEditorRectSelector->setText(tr("Add Rect"));                                // cn:添加矩形
    actionChartEditorRectSelector->setToolTip(tr("Add a rectangle to the chart"));         // cn:添加矩形
    actionChartEditorEllipseSelector->setText(tr("Add Ellipse"));                          // cn:添加椭圆
    actionChartEditorEllipseSelector->setToolTip(tr("Add an ellipse to the chart"));       // cn:添加椭圆
    actionChartEditorPolygonSelector->setText(tr("Add Polygon"));                          // cn:添加多边形
    actionChartEditorPolygonSelector->setToolTip(tr("Add a polygon to the chart"));        // cn:添加多边形
    actionChartEditorAddCrossMarker->setText(tr("Add Cross Marker"));                      // cn:添加十字
    actionChartEditorAddCrossMarker->setToolTip(tr("Add a cross marker to the chart"));    // cn:添加十字标记
    actionChartEditorAddHLineMarker->setText(tr("Add H Line Marker"));                     // cn:添加水平线
    actionChartEditorAddHLineMarker->setToolTip(tr("Add an H line marker to the chart"));  // cn:添加水平线
    actionChartEditorAddVLineMarker->setText(tr("Add V Line Marker"));                     // cn:添加垂直线
    actionChartEditorAddVLineMarker->setToolTip(tr("Add a V line marker to the chart"));   // cn:添加垂直标记
    actionChartEditorAddArrowMarker->setText(tr("Add Arrow Marker"));                      // cn:添加箭头
    actionChartEditorAddArrowMarker->setToolTip(tr("Add an arrow marker to the chart"));   // cn:添加箭头标记
    actionChartEditorAddTextMarker->setText(tr("Add Text"));                               // cn:添加文本
    actionChartEditorAddTextMarker->setToolTip(tr("Add a text marker to the chart"));      // cn:添加文本标注
    actionChartEditorDataProbe->setText(tr("Data Probe"));                                 // cn:数据探针
    actionChartEditorDataProbe->setToolTip(
        tr("Data probe: click to create probes on all charts, values are listed in the data link table"));  // cn:数据探针：点击在各子图上创建探针，数值汇总到数据联动表
    actionChartDataPickerSetting->setText(tr("Data Picker Setting"));                     // cn:数据拾取设置
    actionChartDataPickerSetting->setToolTip(tr("Configure data picker properties"));     // cn:配置数据拾取器属性
    //-----------------------------------------------------
    // 数据操作的上下文标签 Data Operate Context Category
    //-----------------------------------------------------
    actionRemoveRow->setText(tr("Remove Row"));                                  // cn:删除行
    actionRemoveRow->setToolTip(tr("Remove a row from the table"));              // cn:从表中删除行
    actionRemoveColumn->setText(tr("Remove Column"));                            // cn:删除列
    actionRemoveColumn->setToolTip(tr("Remove a column from the table"));        // cn:从表中删除列
    actionRemoveCell->setText(tr("Remove Cell"));                                // cn:删除单元格
    actionRemoveCell->setToolTip(tr("Remove a cell from the table"));            // cn:从表中移除单元格
    actionInsertRow->setText(tr("Insert Row"));                                // cn:插入行
    actionInsertRow->setToolTip(tr("Insert a row into the table"));              // cn:插入行
    actionInsertRowAbove->setText(tr("Insert Row(Above)"));                      // cn:插入行(上)
    actionInsertRowAbove->setToolTip(tr("Insert a row above the current row"));  // cn:插入行(上)
    actionInsertColumnRight->setText(tr("Insert Column"));                     // cn:插入\n列(右)
    actionInsertColumnRight->setToolTip(tr("Insert a column to the right of the current column"));  // cn:插入列(右)
    actionInsertColumnLeft->setText(tr("Insert Column(Left)"));                                     // cn:插入列(左)
    actionInsertColumnLeft->setToolTip(tr("Insert a column to the left of the current column"));    // cn:插入列(左)
    actionCastToNum->setText(tr("To Numeric"));                                                 // cn:转换为数值类型
    actionCastToNum->setToolTip(tr("Cast to numeric type"));                                    // cn:转换为数值类型
    actionCastToString->setText(tr("To String"));                      // cn:转换为字符串类型
    actionCastToString->setToolTip(tr("Cast to string type"));      // cn:转换为字符串类型
    actionCastToDatetime->setText(tr("To Datetime"));               // cn:转换为日期类型
    actionCastToDatetime->setToolTip(tr("Cast to datetime type"));  // cn:转换为日期类型

    actionChangeToIndex->setText(tr("To Index"));               // cn:转换为\n索引
    actionChangeToIndex->setToolTip(tr("Cast to index type"));  // cn:转换为索引类型
    actionRenameColumn->setText(tr("Rename Column"));                          // cn:重命名此列
    actionRenameColumn->setToolTip(tr("Rename the column under the cursor"));  // cn:重命名光标所在列
    actionCopyColumnName->setText(tr("Copy Column Name"));                     // cn:复制列名
    actionCopyColumnName->setToolTip(tr("Copy the column name to clipboard")); // cn:复制列名到剪贴板
    actionGotoMax->setText(tr("Goto Max"));                                              // cn:跳转到最大值
    actionGotoMax->setToolTip(tr("Scroll to the maximum value of this column"));         // cn:滚动到此列最大值处
    actionGotoMin->setText(tr("Goto Min"));                                              // cn:跳转到最小值
    actionGotoMin->setToolTip(tr("Scroll to the minimum value of this column"));         // cn:滚动到此列最小值处
    actionShowColumnDescribe->setText(tr("Show Statistics"));                           // cn:显示统计信息
    actionShowColumnDescribe->setToolTip(tr("Show statistics of this column via pandas describe")); // cn:通过pandas describe显示此列的统计信息
    actionClearStyleSelected->setText(tr("Clear Selected Style"));      // cn:清除选中样式
    actionClearStyleSelected->setToolTip(tr("Clear styles in selected cells"));  // cn:清除选中单元格的样式
    actionClearStyleAll->setText(tr("Clear All Style"));               // cn:清除所有样式
    actionClearStyleAll->setToolTip(tr("Clear all styles in current table"));    // cn:清除当前表格所有样式
    actionTableFormatCells->setText(tr("Format Cells..."));           // cn:设置单元格格式...
    actionTableFormatCells->setToolTip(tr("Set number/date display format of the selected column"));  // cn:设置选中列的数值/日期显示格式
    actionFind->setText(tr("Find"));                                  // cn:查找
    actionFind->setToolTip(tr("Find cells matching the keyword (Ctrl+F)"));  // cn:查找匹配关键字的单元格（Ctrl+F）
    actionReplace->setText(tr("Replace"));                            // cn:替换
    actionReplace->setToolTip(tr("Find and replace cells (Ctrl+H)"));  // cn:查找并替换单元格（Ctrl+H）
    actionConditionalFormat->setText(tr("Conditional Formatting"));   // cn:条件格式
    actionConditionalFormat->setToolTip(tr("Manage conditional formatting rules (cell rules / color scale / data bar) of the selected columns"));  // cn:管理选中列的条件格式规则（单元格规则/色阶/数据条）
    actionComputedColumn->setText(tr("Computed Column"));             // cn:计算列
    actionComputedColumn->setToolTip(tr("Create a column computed from a formula of other columns"));  // cn:用表达式基于其他列创建计算列
    actionRecomputeAllComputedColumns->setText(tr("Recompute All"));  // cn:全部重算
    actionRecomputeAllComputedColumns->setToolTip(tr("Recompute all stale/invalid computed columns of the current table"));  // cn:重算当前表全部待重算/失效的计算列
    actionSplitColumn->setText(tr("Split Column"));                 // cn:分列
    actionSplitColumn->setToolTip(tr("Split the selected column into multiple columns by delimiter / regex / fixed widths"));  // cn:把选中列按分隔符/正则/固定宽度拆分为多列
    actionMergeColumns->setText(tr("Merge Columns"));               // cn:合并列
    actionMergeColumns->setToolTip(tr("Merge the selected columns into one column in the given order"));  // cn:把选中的多列按给定顺序合并为一列
    actionGroupBy->setText(tr("Group By"));                         // cn:分组聚合
    actionGroupBy->setToolTip(tr("Aggregate rows by group keys into a new long-format result dataset"));  // cn:按分组键聚合为新数据集（长格式结果表）
    actionTranspose->setText(tr("Transpose"));                      // cn:转置
    actionTranspose->setToolTip(tr("Swap rows and columns into a new dataset (in-place optional)"));  // cn:行列互换产出新数据集（可选就地替换）
    actionMelt->setText(tr("Unpivot (Melt)"));                      // cn:宽转长（melt）
    actionMelt->setToolTip(tr("Unpivot wide columns into a long-format new dataset (in-place optional)"));  // cn:把宽表展开为长格式新数据集（可选就地替换）
    actionFill->setText(tr("Fill"));                                // cn:填充
    actionFill->setToolTip(tr("Fill the selection (down / up / ffill / bfill / series)"));  // cn:填充选区（向下/向上/ffill/bfill/序列）
    actionFillDown->setText(tr("Fill Down"));                       // cn:向下填充
    actionFillDown->setToolTip(tr("Fill the selection down with the value of the first row (Ctrl+D)"));  // cn:用选区首行值向下填充（Ctrl+D）
    actionFillUp->setText(tr("Fill Up"));                           // cn:向上填充
    actionFillUp->setToolTip(tr("Fill the selection up with the value of the last row"));  // cn:用选区末行值向上填充
    actionFillForward->setText(tr("Fill Forward (ffill)"));         // cn:向下填充空值(ffill)
    actionFillForward->setToolTip(tr("Propagate the last valid value down within the selection"));  // cn:选区内向下传递最后一个非空值
    actionFillBackward->setText(tr("Fill Backward (bfill)"));       // cn:向上填充空值(bfill)
    actionFillBackward->setToolTip(tr("Propagate the next valid value up within the selection"));  // cn:选区内向上传递下一个非空值
    actionFillSeries->setText(tr("Fill Series..."));                // cn:序列填充...
    actionFillSeries->setToolTip(tr("Generate a linear or date series into the selected columns"));  // cn:对选中列生成数值等差或日期步进序列
    actionValidationRules->setText(tr("Validation Rules"));         // cn:校验规则
    actionValidationRules->setToolTip(tr("Manage data validation rules (range / enum / not-empty / unique / regex) of the current table"));  // cn:管理当前表的数据校验规则（区间/枚举/非空/唯一/正则）
    actionRunValidation->setText(tr("Run Validation"));             // cn:执行校验
    actionRunValidation->setToolTip(tr("Validate the table against enabled rules, highlight violations and show the report (read-only)"));  // cn:按启用规则校验全表，高亮违规并出报告（只读）
    actionNote->setText(tr("Note"));                                // cn:批注
    actionNote->setToolTip(tr("Insert or edit a note on the current cell"));  // cn:在当前单元格插入或编辑批注
    actionInsertNote->setText(tr("Insert Note"));                   // cn:插入批注
    actionInsertNote->setToolTip(tr("Insert a note on the current cell"));  // cn:在当前单元格插入批注
    actionEditNote->setText(tr("Edit Note"));                       // cn:编辑批注
    actionEditNote->setToolTip(tr("Edit the note of the current cell"));  // cn:编辑当前单元格的批注
    actionDeleteNote->setText(tr("Delete Note"));                   // cn:删除批注
    actionDeleteNote->setToolTip(tr("Delete the note of the current cell"));  // cn:删除当前单元格的批注
    actionClearNotes->setText(tr("Clear All Notes"));               // cn:清除本表批注
    actionClearNotes->setToolTip(tr("Remove all notes of the current table (undoable)"));  // cn:清除本表全部批注（可撤销）
    // 列视图（006 T1.8）
    actionFreezeColumns->setText(tr("Freeze Columns"));                               // cn:冻结列
    actionFreezeColumns->setToolTip(tr("Freeze the leftmost columns of the table"));  // cn:冻结表格左侧的列
    actionHideColumn->setText(tr("Hide Column"));                                     // cn:隐藏列
    actionHideColumn->setToolTip(tr("Hide the selected columns"));                    // cn:隐藏选中的列
    actionShowAllColumns->setText(tr("Show All Columns"));                            // cn:显示所有列
    actionShowAllColumns->setToolTip(tr("Show all hidden columns"));                  // cn:显示所有被隐藏的列
    actionColumnViewDialog->setText(tr("Column View..."));                            // cn:列视图...
    actionColumnViewDialog->setToolTip(tr("Open the column view settings dialog"));   // cn:打开列视图设置对话框
    actionFreezeFirstColumn->setText(tr("Freeze First Column"));                      // cn:冻结首列
    actionFreezeColumnsPrompt->setText(tr("Freeze Columns..."));                      // cn:冻结前 N 列…
    actionUnfreezeColumns->setText(tr("Unfreeze Columns"));                           // cn:取消冻结
    actionGotoCell->setText(tr("Go To..."));                                          // cn:定位...
    actionGotoCell->setToolTip(tr("Jump to a row / index label / column"));           // cn:跳转到指定行、索引标签或列
    //-----------------------------------------------------
    // workflow 编辑
    //-----------------------------------------------------
    actionWorkflowNew->setText(tr("New Workflow"));                    // cn:新建\n工作流
    actionWorkflowNew->setToolTip(tr("Create a new workflow"));          // cn:创建新工作流
    actionWorkflowEnableItemLinkageMove->setText(tr("Linkage Move"));  // cn:联动
    actionWorkflowEnableItemLinkageMove->setToolTip(
        tr("When moving elements, other elements linked to this element follow the movement"));  // cn:允许移动图元时，其它和此图元链接起来的图元跟随移动
    actionItemGrouping->setText(tr("Grouping"));                                                 // cn:分组
    actionItemGrouping->setToolTip(tr("Group selected elements"));   // cn:对选中的元素进行分组
    actionItemUngroup->setText(tr("Ungroup"));                       // cn:取消分组
    actionItemUngroup->setToolTip(tr("Ungroup selected elements"));  // cn:对选中的元素进行取消分组
    actionWorkflowStartDrawRect->setText(tr("Draw Rect"));         // cn:绘制矩形
    actionWorkflowStartDrawRect->setToolTip(tr("Draw a rectangle on the workflow scene"));  // cn:在工作流场景中绘制矩形
    actionWorkflowStartDrawText->setText(tr("Draw Text"));                                // cn:绘制文本
    actionWorkflowStartDrawText->setToolTip(tr("Draw text on the workflow scene"));         // cn:在工作流场景中绘制文本
    actionWorkflowShowGrid->setText(tr("Show Grid"));                                     // cn:显示网格
    actionWorkflowShowGrid->setToolTip(tr("Show grid on the workflow scene"));              // cn:在工作流场景中显示网格
    actionWorkflowViewReadOnly->setText(tr("Lock View"));                                 // cn:锁定视图
    actionWorkflowViewReadOnly->setToolTip(tr("Lock the workflow view"));                   // cn:锁定工作流视图
    actionWorkflowRun->setText(tr("Run Workflow"));                                       // cn:运行\n工作流
    actionWorkflowRun->setToolTip(tr("Run the workflow"));                                  // cn:运行工作流
    actionWorkflowTerminate->setText(tr("Terminate Workflow"));                           // cn:停止\n工作流
    actionWorkflowTerminate->setToolTip(tr("Terminate the workflow"));                      // cn:停止工作流
    actionWorkflowLinkEnable->setText(tr("Link"));                                          // cn:连线
    actionWorkflowLinkEnable->setToolTip(tr("Enable or disable link between elements"));    // cn:启用或禁用元素之间连线
    actionWorkflowAddBackgroundPixmap->setText(tr("Add Background"));                     // cn:添加背景
    actionWorkflowAddBackgroundPixmap->setToolTip(tr("Add a background pixmap to the workflow scene"));  // cn:在工作流场景中添加背景图片
    actionWorkflowLockBackgroundPixmap->setText(tr("Lock Background"));                                  // cn:锁定背景
    actionWorkflowLockBackgroundPixmap->setToolTip(
        tr("Lock the background pixmap in the workflow scene"));                      // cn:锁定工作流场景中的背景图片
    actionWorkflowEnableItemMoveWithBackground->setText(tr("Move With Background"));  // cn:元件随背景移动
    actionWorkflowEnableItemMoveWithBackground->setToolTip(
        tr("Enable or disable item move with background pixmap"));  // cn:启用或禁用元件随背景移动
    //-----------------------------------------------------
    // workflow 视图
    //-----------------------------------------------------
    actionExportWorkflowSceneToImage->setText(tr("Export To Image"));                           // cn:导出为图片
    actionExportWorkflowSceneToImage->setToolTip(tr("Export the workflow scene to an image"));  // cn:导出工作流场景为图片
    actionExportWorkflowSceneToPNG->setText(tr("Export To PNG"));                               // cn:导出PNG
    actionExportWorkflowSceneToPNG->setToolTip(tr("Export the workflow scene to a PNG image"));  // cn:导出工作流场景为PNG图片
    actionWorkflowViewMarker->setText(tr("Show Marker"));                                        // cn:显示标记
    actionWorkflowViewMarker->setToolTip(tr("Show marker on the workflow scene"));  // cn:在工作流场景中显示标记
    //-----------------------------------------------------
    // View Category
    //-----------------------------------------------------
    actionShowWorkFlowArea->setText(tr("Show Workflow Area"));                      // cn:工作流\n操作视图
    actionShowWorkFlowArea->setToolTip(tr("Show the workflow area"));                 // cn:显示工作流区域
    actionShowWorkFlowManagerArea->setText(tr("Show Workflow Manager"));            // cn:工作流\n管理视图
    actionShowWorkFlowManagerArea->setToolTip(tr("Show the workflow manager area"));  // cn:显示工作流管理区域
    actionShowChartArea->setText(tr("Show Chart Area"));                            // cn:绘图\n操作视图
    actionShowChartArea->setToolTip(tr("Show the chart area"));                       // cn:显示绘图区域
    actionShowChartManagerArea->setText(tr("Show Chart Manager"));                  // cn:绘图\n管理视图
    actionShowChartManagerArea->setToolTip(tr("Show the chart manager area"));        // cn:显示绘图管理区域
    actionShowDataArea->setText(tr("Show Table Area"));                             // cn:数据\n操作区域
    actionShowDataArea->setToolTip(tr("Show the data area"));                         // cn:显示数据区域
    actionShowDataManagerArea->setText(tr("Show Data Manager"));                    // cn:数据\n管理视图
    actionShowDataManagerArea->setToolTip(tr("Show the data manager area"));          // cn:显示数据管理区域
    actionShowMessageLogView->setText(tr("Show Information Window"));                  // cn:信息窗口
    actionShowMessageLogView->setToolTip(tr("Show the message log window"));          // cn:显示信息窗口
    actionShowUndoHistory->setText(tr("Undo History"));                                // cn:撤销历史
    actionShowUndoHistory->setToolTip(tr("Show the undo history panel (follows the active page, click an entry to undo/redo to it)"));  // cn:显示撤销历史面板（跟随当前激活页面，点击条目可回退/前进）
    actionShowSettingWidget->setText(tr("Show Setting Window"));                      // cn:设置窗口
    actionShowSettingWidget->setToolTip(tr("Show the setting window"));               // cn:显示设置窗口
    actionShowLeftSideBar->setText(tr("Show Left Side Bar"));                       // cn:左侧边栏
    actionShowLeftSideBar->setToolTip(tr("Show the left side bar"));  // cn:显示左侧边栏
    actionShowRightSideBar->setText(tr("Show Right Side Bar"));       // cn:右侧边栏
    actionShowRightSideBar->setToolTip(tr("Show the right side bar"));  // cn:显示右侧边栏
    actionShowAgentArea->setText(tr("Show Agent Area"));  // cn:Agent 助手
    actionShowAgentArea->setToolTip(tr("Show the agent assistant area"));  // cn:显示 Agent 助手区域
    // Config Category
    actionPluginManager->setText(tr("Plugin Config"));             // cn:插件设置
    actionPluginManager->setToolTip(tr("Show the plugin manager"));  // cn:显示插件管理器
    // Other
    actionCut->setText(tr("Cut"));                                   // cn:剪切
    actionCut->setToolTip(tr("Cut the selection to the clipboard"));  // cn:剪切选中内容到剪贴板
    actionCopy->setText(tr("Copy"));                                  // cn:复制
    actionCopy->setToolTip(tr("Copy the selection to the clipboard"));  // cn:复制选中内容到剪贴板
    actionPaste->setText(tr("Paste"));                                // cn:粘贴
    actionPaste->setToolTip(tr("Paste from the clipboard"));          // cn:从剪贴板粘贴
    actionDelete->setText(tr("Delete"));                              // cn:删除
    actionDelete->setToolTip(tr("Delete the selection"));             // cn:删除选中内容
    actionSelectAll->setText(tr("Select All"));                       // cn:全选
    actionSelectAll->setToolTip(tr("Select all content"));            // cn:全选内容
    actionResetDefaultLayout->setText(tr("Reset Layout"));          // cn:恢复\n默认布局
    actionResetDefaultLayout->setToolTip(tr("Restore the default window layout (confirmation required)"));  // cn:恢复默认窗口布局（需确认）
    actionManageLayouts->setText(tr("Manage Layouts"));             // cn:布局管理
    actionManageLayouts->setToolTip(tr("Open the layout manager to save, apply or remove layout schemes"));  // cn:打开布局管理对话框，保存、应用或删除布局方案

    //
    if (actionRedo) {
        actionRedo->setText(tr("Redo"));                     // cn:重做
        actionRedo->setToolTip(tr("Redo the last action"));  // cn:重做
    }
    if (actionUndo) {
        actionUndo->setText(tr("Undo"));                     // cn:撤销
        actionUndo->setToolTip(tr("Undo the last action"));  // cn:撤销
    }
}
