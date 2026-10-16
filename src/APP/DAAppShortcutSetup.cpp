#include "DAAppShortcutSetup.h"
#include "DAShortcutManager.h"
#include "DAAppActions.h"
#include "DAUiObjectNames.h"
#include "DAPyWorkFlowOperateWidget.h"
#include <QAction>
#include <QHash>

namespace DA
{
namespace
{
// 分类键（稳定英文标识，显示文本由设置页翻译；新增分类需同步设置页的翻译与排序表）
constexpr char kCatMain[]           = "Main";
constexpr char kCatData[]           = "Data";
constexpr char kCatFigure[]         = "Figure";
constexpr char kCatChart[]          = "Chart";
constexpr char kCatView[]           = "View";
constexpr char kCatWorkflow[]       = "Workflow";
constexpr char kCatWorkflowEditor[] = "WorkflowEditor";
constexpr char kCatOther[]          = "Other";

/**
 * @brief 命令分类表：action objectName -> 分类键
 *
 * 与 DAUiObjectNames.h 中 UiNames::Action 的分组注释一一对应；
 * 未列入表中的已注册 action（颜色主题、插件动态注册等）归入 Other
 */
const QHash< QString, QString >& actionCategoryTable()
{
    static const QHash< QString, QString > table = []() {
        QHash< QString, QString > t;
        auto put = [&t](const char* objname, const char* cat) {
            t.insert(QString::fromUtf8(objname), QString::fromUtf8(cat));
        };
        // ---- 主页 ----
        put(UiNames::Action::Open, kCatMain);
        put(UiNames::Action::Save, kCatMain);
        put(UiNames::Action::SaveAs, kCatMain);
        put(UiNames::Action::AppendProject, kCatMain);
        put(UiNames::Action::OpenMarkdown, kCatMain);
        put(UiNames::Action::Setting, kCatMain);
        put(UiNames::Action::PluginManager, kCatMain);
        put(UiNames::Action::About, kCatMain);
        put(UiNames::Action::Redo, kCatMain);
        put(UiNames::Action::Undo, kCatMain);
        put(UiNames::Action::Cut, kCatMain);
        put(UiNames::Action::Copy, kCatMain);
        put(UiNames::Action::Paste, kCatMain);
        put(UiNames::Action::Delete, kCatMain);
        put(UiNames::Action::SelectAll, kCatMain);
        // ---- 数据 ----
        put(UiNames::Action::AddData, kCatData);
        put(UiNames::Action::RemoveData, kCatData);
        put(UiNames::Action::RenameData, kCatData);
        put(UiNames::Action::ExportData, kCatData);
        put(UiNames::Action::ExportDataCsv, kCatData);
        put(UiNames::Action::ExportDataExcel, kCatData);
        put(UiNames::Action::ExportDataPickle, kCatData);
        put(UiNames::Action::ExportDataParquet, kCatData);
        put(UiNames::Action::RemoveRow, kCatData);
        put(UiNames::Action::RemoveColumn, kCatData);
        put(UiNames::Action::InsertRow, kCatData);
        put(UiNames::Action::InsertRowAbove, kCatData);
        put(UiNames::Action::InsertColumnRight, kCatData);
        put(UiNames::Action::InsertColumnLeft, kCatData);
        put(UiNames::Action::RenameColumns, kCatData);
        put(UiNames::Action::RenameColumn, kCatData);
        put(UiNames::Action::CopyColumnName, kCatData);
        put(UiNames::Action::GotoMax, kCatData);
        put(UiNames::Action::GotoMin, kCatData);
        put(UiNames::Action::ShowColumnDescribe, kCatData);
        put(UiNames::Action::RemoveCell, kCatData);
        put(UiNames::Action::CastToNum, kCatData);
        put(UiNames::Action::CastToString, kCatData);
        put(UiNames::Action::CastToDatetime, kCatData);
        put(UiNames::Action::ChangeToIndex, kCatData);
        put(UiNames::Action::ClearStyleSelected, kCatData);
        put(UiNames::Action::ClearStyleAll, kCatData);
        put(UiNames::Action::TableFormatCells, kCatData);
        // ---- 绘图（含统计绘图） ----
        put(UiNames::Action::AddFigure, kCatFigure);
        put(UiNames::Action::FigureNewXYAxis, kCatFigure);
        put(UiNames::Action::FigureSettingApplyAllChart, kCatFigure);
        put(UiNames::Action::ChartAddCurve, kCatFigure);
        put(UiNames::Action::ChartAddScatter2D, kCatFigure);
        put(UiNames::Action::ChartAddErrorBar, kCatFigure);
        put(UiNames::Action::ChartAddBoxPlot, kCatFigure);
        put(UiNames::Action::ChartAddBar, kCatFigure);
        put(UiNames::Action::ChartAddMultiBar, kCatFigure);
        put(UiNames::Action::ChartAddHistogramBar, kCatFigure);
        put(UiNames::Action::ChartAddContourMap, kCatFigure);
        put(UiNames::Action::ChartAddCloudMap, kCatFigure);
        put(UiNames::Action::ChartAddVectorfield, kCatFigure);
        put(UiNames::Action::ChartAdd3DSurface, kCatFigure);
        put(UiNames::Action::ChartAdd3DBar, kCatFigure);
        put(UiNames::Action::ChartAdd3DLine, kCatFigure);
        put(UiNames::Action::StatsHistplot, kCatFigure);
        put(UiNames::Action::StatsKdeplot1d, kCatFigure);
        put(UiNames::Action::StatsKdeplot2d, kCatFigure);
        put(UiNames::Action::StatsBoxplot, kCatFigure);
        put(UiNames::Action::StatsHeatmap, kCatFigure);
        put(UiNames::Action::StatsScatterplot, kCatFigure);
        put(UiNames::Action::StatsBarplot, kCatFigure);
        put(UiNames::Action::StatsRegplot, kCatFigure);
        put(UiNames::Action::StatsECDFplot, kCatFigure);
        // ---- 图表操作 ----
        put(UiNames::Action::ChartEnableGrid, kCatChart);
        put(UiNames::Action::ChartEnableGridX, kCatChart);
        put(UiNames::Action::ChartEnableGridY, kCatChart);
        put(UiNames::Action::ChartEnableGridXMin, kCatChart);
        put(UiNames::Action::ChartEnableGridYMin, kCatChart);
        put(UiNames::Action::ChartEnableZoom, kCatChart);
        put(UiNames::Action::ChartZoomIn, kCatChart);
        put(UiNames::Action::ChartZoomOut, kCatChart);
        put(UiNames::Action::ChartZoomAll, kCatChart);
        put(UiNames::Action::ChartEnablePan, kCatChart);
        put(UiNames::Action::ChartDisableZoomX, kCatChart);
        put(UiNames::Action::ChartDisableZoomY, kCatChart);
        put(UiNames::Action::ChartEnablePickerCross, kCatChart);
        put(UiNames::Action::ChartEnablePickerXY, kCatChart);
        put(UiNames::Action::ChartEnablePickerY, kCatChart);
        put(UiNames::Action::LinkAllPicker, kCatChart);
        put(UiNames::Action::ChartPickerTextAtLeftTop, kCatChart);
        put(UiNames::Action::ChartPickerTextAtLeftBottom, kCatChart);
        put(UiNames::Action::ChartPickerTextAtRightTop, kCatChart);
        put(UiNames::Action::ChartPickerTextAtRightBottom, kCatChart);
        put(UiNames::Action::ChartPickerTextFollowMouse, kCatChart);
        put(UiNames::Action::ChartYPickerShowXValueEnabled, kCatChart);
        put(UiNames::Action::ChartEnableLegend, kCatChart);
        put(UiNames::Action::CopyFigureInClipboard, kCatChart);
        put(UiNames::Action::ChartLegendAtTop, kCatChart);
        put(UiNames::Action::ChartLegendAtBottom, kCatChart);
        put(UiNames::Action::ChartLegendAtLeft, kCatChart);
        put(UiNames::Action::ChartLegendAtRight, kCatChart);
        put(UiNames::Action::GroupChartLegendPosition, kCatChart);
        put(UiNames::Action::ChartEditorResizeSubChart, kCatChart);
        put(UiNames::Action::ChartEditorPointerSelector, kCatChart);
        put(UiNames::Action::ChartEditorRectSelector, kCatChart);
        put(UiNames::Action::ChartEditorEllipseSelector, kCatChart);
        put(UiNames::Action::ChartEditorPolygonSelector, kCatChart);
        put(UiNames::Action::ChartEditorAddCrossMarker, kCatChart);
        put(UiNames::Action::ChartEditorAddHLineMarker, kCatChart);
        put(UiNames::Action::ChartEditorAddVLineMarker, kCatChart);
        put(UiNames::Action::ChartEditorAddArrowMarker, kCatChart);
        put(UiNames::Action::ChartEditorAddTextMarker, kCatChart);
        put(UiNames::Action::ChartEditorDataProbe, kCatChart);
        put(UiNames::Action::ChartDataPickerSetting, kCatChart);
        // ---- 视图 ----
        put(UiNames::Action::ShowWorkFlowArea, kCatView);
        put(UiNames::Action::ShowWorkFlowManagerArea, kCatView);
        put(UiNames::Action::ShowChartArea, kCatView);
        put(UiNames::Action::ShowChartManagerArea, kCatView);
        put(UiNames::Action::ShowDataArea, kCatView);
        put(UiNames::Action::ShowDataManagerArea, kCatView);
        put(UiNames::Action::ShowMessageLogView, kCatView);
        put(UiNames::Action::ShowSettingWidget, kCatView);
        put(UiNames::Action::ShowLeftSideBar, kCatView);
        put(UiNames::Action::ShowRightSideBar, kCatView);
        put(UiNames::Action::ShowAgentArea, kCatView);
        put(UiNames::Action::ResetDefaultLayout, kCatView);
        put(UiNames::Action::ManageLayouts, kCatView);
        // ---- 工作流 ----
        put(UiNames::Action::WorkflowNew, kCatWorkflow);
        put(UiNames::Action::WorkflowEnableItemLinkageMove, kCatWorkflow);
        put(UiNames::Action::ItemSetGroup, kCatWorkflow);
        put(UiNames::Action::ItemCancelGroup, kCatWorkflow);
        put(UiNames::Action::WorkflowLinkEnable, kCatWorkflow);
        put(UiNames::Action::StartDrawRect, kCatWorkflow);
        put(UiNames::Action::StartDrawText, kCatWorkflow);
        put(UiNames::Action::AddBackgroundPixmap, kCatWorkflow);
        put(UiNames::Action::LockBackgroundPixmap, kCatWorkflow);
        put(UiNames::Action::EnableItemMoveWithBackground, kCatWorkflow);
        put(UiNames::Action::WorkflowShowGrid, kCatWorkflow);
        put(UiNames::Action::WorkflowViewLock, kCatWorkflow);
        put(UiNames::Action::WorkflowViewMarker, kCatWorkflow);
        put(UiNames::Action::WorkflowRun, kCatWorkflow);
        put(UiNames::Action::WorkflowTerminate, kCatWorkflow);
        put(UiNames::Action::ExportWorkflowSceneToImage, kCatWorkflow);
        put(UiNames::Action::ExportWorkflowSceneToPNG, kCatWorkflow);
        return t;
    }();
    return table;
}
}  // namespace

/**
 * @brief 把主程序与工作流编辑器的可配置命令注册进快捷键管理器
 * @param mgr 快捷键管理器
 * @param actions 主程序 action 管理器（可为 nullptr）
 * @param wfo 工作流操作窗口（可为 nullptr）
 * @return 成功注册的命令数量
 */
int setupAppShortcutCommands(DAShortcutManager* mgr, DAAppActions* actions, DAPyWorkFlowOperateWidget* wfo)
{
    if (nullptr == mgr) {
        return 0;
    }
    int count = 0;
    if (actions) {
        const QList< QAction* > all = actions->actions();
        const QHash< QString, QString >& table = actionCategoryTable();
        for (QAction* act : all) {
            if (nullptr == act) {
                continue;
            }
            const QString cat = table.value(act->objectName(), QString::fromUtf8(kCatOther));
            if (mgr->attachAction(act, cat)) {
                ++count;
            }
        }
    }
    if (wfo) {
        count += mgr->attachActions(wfo->configurableActions(), QString::fromUtf8(kCatWorkflowEditor));
    }
    return count;
}
}  // namespace DA
