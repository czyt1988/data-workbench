#include "DAAppActions.h"
#include "DAUiObjectNames.h"
#include <QActionGroup>
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
}

void DAAppActions::buildDataAction()
{
    // Data Category
    actionAddData    = createAction(UiNames::Action::AddData, ":/app/bright/Icon/addData.svg");
    actionRemoveData = createAction(UiNames::Action::RemoveData, ":/app/bright/Icon/removeData.svg");
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
}

void DAAppActions::buildChartAction()
{
    // 绘图标签 Chart Category
    actionAddFigure       = createAction(UiNames::Action::AddFigure, ":/app/bright/Icon/addFigure.svg");
    actionFigureNewXYAxis = createAction(UiNames::Action::FigureNewXYAxis, ":/app/bright/Icon/newAxis.svg");
    actionChartAddCurve   = createAction(UiNames::Action::ChartAddCurve, ":/app/chart-type/Icon/chart-type/chart-curve.svg");
    actionChartAddScatter2D =
        createAction(UiNames::Action::ChartAddScatter2D, ":/app/chart-type/Icon/chart-type/chart-scatter-2d.svg");
    actionChartAddErrorBar =
        createAction(UiNames::Action::ChartAddErrorBar, ":/app/chart-type/Icon/chart-type/chart-intervalcurve.svg");
    actionChartAddBoxPlot = createAction(UiNames::Action::ChartAddBoxPlot, ":/app/chart-type/Icon/chart-type/chart-OHLC.svg");
    actionChartAddBar     = createAction(UiNames::Action::ChartAddBar, ":/app/chart-type/Icon/chart-type/chart-bar.svg");
    actionChartAddMultiBar = createAction(UiNames::Action::ChartAddMultiBar, ":/app/chart-type/Icon/chart-type/chart-multibar.svg");
    actionChartAddHistogramBar =
        createAction(UiNames::Action::ChartAddHistogramBar, ":/app/chart-type/Icon/chart-type/chart-histogram.svg");
    actionChartAddContourMap =
        createAction(UiNames::Action::ChartAddContourMap, ":/app/chart-type/Icon/chart-type/chart-spectrocurve.svg");
    actionChartAddCloudMap =
        createAction(UiNames::Action::ChartAddCloudMap, ":/app/chart-type/Icon/chart-type/chart-spectrogram.svg");
    actionChartAddVectorfield =
        createAction(UiNames::Action::ChartAddVectorfield, ":/app/chart-type/Icon/chart-type/chart-vectorfield.svg");
    actionChartAdd3DSurface = createAction(UiNames::Action::ChartAdd3DSurface, ":/app/chart-type/Icon/chart-type/chart-surface3d.svg");
    actionChartAdd3DBar     = createAction(UiNames::Action::ChartAdd3DBar, ":/app/chart-type/Icon/chart-type/chart-bar3d.svg");
    actionChartAdd3DLine    = createAction(UiNames::Action::ChartAdd3DLine, ":/app/chart-type/Icon/chart-type/chart-line3d.svg");

    // 统计绘图 Stats Plot
    actionStatsHistplot     = createAction(UiNames::Action::StatsHistplot, ":/app/chart-type/Icon/chart-type/stats-histplot.svg");
    actionStatsKdeplot1d    = createAction(UiNames::Action::StatsKdeplot1d, ":/app/chart-type/Icon/chart-type/stats-kdeplot.svg");
    actionStatsKdeplot2d    = createAction(UiNames::Action::StatsKdeplot2d, ":/app/chart-type/Icon/chart-type/stats-kdeplot-2d.svg");
    actionStatsBoxplot      = createAction(UiNames::Action::StatsBoxplot, ":/app/chart-type/Icon/chart-type/stats-boxplot.svg");
    actionStatsHeatmap      = createAction(UiNames::Action::StatsHeatmap, ":/app/chart-type/Icon/chart-type/stats-heatmap.svg");
    actionStatsScatterplot  = createAction(UiNames::Action::StatsScatterplot, ":/app/chart-type/Icon/chart-type/stats-scatterplot.svg");
    actionStatsBarplot      = createAction(UiNames::Action::StatsBarplot, ":/app/chart-type/Icon/chart-type/stats-barplot.svg");
    actionStatsRegplot      = createAction(UiNames::Action::StatsRegplot, ":/app/chart-type/Icon/chart-type/stats-regplot.svg");
    actionStatsECDFplot     = createAction(UiNames::Action::StatsECDFplot, ":/app/chart-type/Icon/chart-type/stats-ecdfplot.svg");

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

    actionGroupChartEditor = new QActionGroup(this);
    actionGroupChartEditor->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  // 允许所有都不选择
    actionChartEditorResizeSubChart = createAction(
        UiNames::Action::ChartEditorResizeSubChart, ":/app/bright/Icon/figureResizeChart.svg", true, false, actionGroupChartEditor);
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
    actionAddHorizontalPlotProbeMarker = createAction(
        UiNames::Action::AddHorizontalPlotProbeMarker, ":/app/bright/Icon/arrow.svg", true, false, actionGroupChartEditor);
    actionAddVerticalPlotProbeMarker = createAction(
        UiNames::Action::AddVerticalPlotProbeMarker, ":/app/bright/Icon/plot-probe.svg", true, false, actionGroupChartEditor);
    actionChartDataPickerSetting   = createAction(UiNames::Action::ChartDataPickerSetting, ":/DAGui/icon/data-picker-setting.svg");
    actionChartEditorResizeSubChart->setData(static_cast< int >(DAFigureWidget::SubChartEditor));
    actionChartEditorRectSelector->setData(static_cast< int >(DAFigureWidget::RectSelectEditor));
    actionChartEditorEllipseSelector->setData(static_cast< int >(DAFigureWidget::EllipseSelectEditor));
    actionChartEditorPolygonSelector->setData(static_cast< int >(DAFigureWidget::PolygonSelectEditor));
    actionChartEditorAddHLineMarker->setData(static_cast< int >(DAFigureWidget::HLineMarker));
    actionChartEditorAddVLineMarker->setData(static_cast< int >(DAFigureWidget::VLineMarker));
    actionChartEditorAddCrossMarker->setData(static_cast< int >(DAFigureWidget::CrossMarker));
    actionChartEditorAddArrowMarker->setData(static_cast< int >(DAFigureWidget::ArrowMarker));
    actionAddHorizontalPlotProbeMarker->setData(static_cast< int >(DAFigureWidget::VerticalDataProbe));
    actionAddVerticalPlotProbeMarker->setData(static_cast< int >(DAFigureWidget::HorizontalDataProbe));
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
    actionShowSettingWidget    = createAction(UiNames::Action::ShowSettingWidget, ":/app/bright/Icon/showSettingWidget.svg");
    actionShowLeftSideBar = createAction(UiNames::Action::ShowLeftSideBar, ":/app/bright/Icon/left-sider-bar.svg", true, true);
    actionShowRightSideBar = createAction(UiNames::Action::ShowRightSideBar, ":/app/bright/Icon/right-sider-bar.svg", true, true);
    actionShowAgentArea = createAction(UiNames::Action::ShowAgentArea, ":/app/bright/Icon/showAgent.svg");
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
}

void DAAppActions::buildOtherActions()
{
    actionGroupRibbonTheme = new QActionGroup(this);
    actionGroupRibbonTheme->setObjectName(QString::fromUtf8(UiNames::Action::GroupRibbonTheme));
    actionRibbonThemeOffice2013 = createAction(UiNames::Action::RibbonThemeOffice2013, true, true, actionGroupRibbonTheme);
    actionRibbonThemeOffice2016Blue = createAction(UiNames::Action::RibbonThemeOffice2016Blue, true, false, actionGroupRibbonTheme);
    actionRibbonThemeOffice2021Blue = createAction(UiNames::Action::RibbonThemeOffice2021Blue, true, false, actionGroupRibbonTheme);
    actionRibbonThemeDark = createAction(UiNames::Action::RibbonThemeDark, true, false, actionGroupRibbonTheme);
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
    actionOpen->setText(tr("Open"));                     // cn:打开
    actionOpen->setToolTip(tr("Open file or project"));  // cn:打开文件或项目
    actionSave->setText(tr("Save"));                     // cn:保存
    actionSave->setToolTip(tr("Save file or project"));                                // cn:保存文件或项目
    actionSaveAs->setText(tr("Save As"));                                              // cn:另存为
    actionSaveAs->setToolTip(tr("Save file or project as"));                           // cn:保存文件或项目为
    actionAppendProject->setText(tr("Append To Project"));                             // cn:附加到项目
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
    actionAddData->setText(tr("Add \nData"));                                     // cn:添加\n数据
    actionAddData->setToolTip(tr("Add data to the table"));                       // cn:添加数据到表格
    actionRemoveData->setText(tr("Remove \nData"));                               // cn:移除\n数据
    actionRemoveData->setToolTip(tr("Remove data from the table"));               // cn:从表格中移除数据
    //-----------------------------------------------------
    // Chart Category
    //-----------------------------------------------------
    actionAddFigure->setText(tr("Add \nFigure"));                                  // cn:添加\n绘图
    actionAddFigure->setToolTip(tr("Add a figure to the workspace"));              // cn:添加绘图到工作区
    actionChartEditorResizeSubChart->setText(tr("Resize \nChart"));                // cn:绘图\n尺寸
    actionChartEditorResizeSubChart->setToolTip(tr("Resize the sub-chart"));       // cn:调整子图尺寸
    actionFigureNewXYAxis->setText(tr("New \nXY Axis"));                           // cn:新建\n坐标系
    actionFigureNewXYAxis->setToolTip(tr("Add a new XY axis to the figure"));      // cn:新建坐标系
    actionChartAddCurve->setText(tr("Add \nCurve"));                               // cn:折线图
    actionChartAddCurve->setToolTip(tr("Add a curve to the chart"));               // cn:添加折线图
    actionChartAddScatter2D->setText(tr("Add \nScatter"));                         // cn:散点图
    actionChartAddScatter2D->setToolTip(tr("Add a scatter plot to the chart"));    // cn:添加散点图
    actionChartAddErrorBar->setText(tr("Add \nError Bar"));                        // cn:误差棒图
    actionChartAddErrorBar->setToolTip(tr("Add an error bar to the chart"));       // cn:添加误差棒图
    actionChartAddBoxPlot->setText(tr("Add \nBox Plot"));                          // cn:箱线图
    actionChartAddBoxPlot->setToolTip(tr("Add a box plot to the chart"));          // cn:添加箱线图
    actionChartAddBar->setText(tr("Add \nBar"));                                   // cn:柱状图
    actionChartAddBar->setToolTip(tr("Add a bar chart to the chart"));             // cn:添加柱状图
    actionChartAddMultiBar->setText(tr("Add \nMultiBar"));                         // cn:多重柱状图
    actionChartAddMultiBar->setToolTip(tr("Add a multi-bar chart to the chart"));  // cn:添加多重柱状图
    actionChartAddHistogramBar->setText(tr("Add \nHistogram"));                    // cn:分布图
    actionChartAddHistogramBar->setToolTip(tr("Add a histogram to the chart"));    // cn:添加分布图
    actionChartAddContourMap->setText(tr("Add \nContour Map"));                    // cn:等高线图
    actionChartAddContourMap->setToolTip(tr("Add a contour map to the chart"));    // cn:添加等高线图
    actionChartAddCloudMap->setText(tr("Add \nCloud Map"));                        // cn:云图
    actionChartAddCloudMap->setToolTip(tr("Add a cloud map to the chart"));        // cn:添加云图
    actionChartAddVectorfield->setText(tr("Add \nVector Field"));                  // cn:向量场图
    actionChartAddVectorfield->setToolTip(tr("Add a vector field to the chart"));  // cn:添加向量场图
    actionChartAdd3DSurface->setText(tr("Add \nSurface 3D"));                        // cn:3D曲面图
    actionChartAdd3DSurface->setToolTip(tr("Add a 3D surface plot to the chart"));   // cn:添加3D曲面图
    actionChartAdd3DBar->setText(tr("Add \nBar 3D"));                                 // cn:3D柱状图
    actionChartAdd3DBar->setToolTip(tr("Add a 3D bar chart to the chart"));           // cn:添加3D柱状图
    actionChartAdd3DLine->setText(tr("Add \nLine 3D"));                              // cn:3D线图
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
    actionStatsECDFplot->setText(tr("ECDF"));                                           // cn:累积分布
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
    actionChartZoomAll->setText(tr("Show \nAll"));                                          // cn:显示\n全部
    actionChartZoomAll->setToolTip(tr("Zoom to show all data in the chart"));               // cn:缩放以显示所有数据
    actionChartEnablePan->setText(tr("Pan"));                                               // cn:拖动
    actionChartEnablePan->setToolTip(tr("Enable or disable pan in the chart"));             // cn:启用或禁用图表中的拖动
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
    actionChartLinkAllPickerEnabled->setText(tr("Link All Picker"));                        // cn:联动\n拾取
    actionChartLinkAllPickerEnabled->setToolTip(tr("Enable or disable all picker linked"));  // cn:启用或禁用所有拾取联动
    actionChartEnableLegend->setText(tr("Legend"));                                          // cn:图例
    actionChartEnableLegend->setToolTip(tr("Enable or disable legend in the chart"));  // cn:启用或禁用图表中的图例
    actionCopyFigureInClipboard->setText(tr("Copy To Clipboard"));                     // cn:复制到剪切板
    actionCopyFigureInClipboard->setToolTip(tr("Copy the figure to the clipboard"));   // cn:将绘图复制到剪切板

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
    actionAddHorizontalPlotProbeMarker->setText(tr("Add Horizontal Probe"));               // cn:添加水平数据探针标记
    actionAddHorizontalPlotProbeMarker->setToolTip(tr("Add a horizontal plot probe marker to the chart"));  // cn:水平数据探针
    actionAddVerticalPlotProbeMarker->setText(tr("Add Vertical Probe"));  // cn:垂直数据探针
    actionAddVerticalPlotProbeMarker->setToolTip(tr("Add a vertical plot probe marker to the chart"));  // cn:添加垂直数据探针标记
    actionChartDataPickerSetting->setText(tr("Data Picker Setting"));                     // cn:数据拾取设置
    actionChartDataPickerSetting->setToolTip(tr("Configure data picker properties"));     // cn:配置数据拾取器属性
    //-----------------------------------------------------
    // 数据操作的上下文标签 Data Operate Context Category
    //-----------------------------------------------------
    actionRemoveRow->setText(tr("Remove Row"));                                  // cn:删除行
    actionRemoveRow->setToolTip(tr("Remove a row from the table"));              // cn:从表中删除行
    actionRemoveColumn->setText(tr("Remove Column"));                            // cn:删除列
    actionRemoveColumn->setToolTip(tr("Remove a column from the table"));        // cn:从表中删除列
    actionRemoveCell->setText(tr("Remove Cell"));                                // cn:移除单元格
    actionRemoveCell->setToolTip(tr("Remove a cell from the table"));            // cn:从表中移除单元格
    actionInsertRow->setText(tr("Insert \nRow"));                                // cn:插入行
    actionInsertRow->setToolTip(tr("Insert a row into the table"));              // cn:插入行
    actionInsertRowAbove->setText(tr("Insert Row(Above)"));                      // cn:插入行(上)
    actionInsertRowAbove->setToolTip(tr("Insert a row above the current row"));  // cn:插入行(上)
    actionInsertColumnRight->setText(tr("Insert \nColumn"));                     // cn:插入\n列(右)
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
    //-----------------------------------------------------
    // workflow 编辑
    //-----------------------------------------------------
    actionWorkflowNew->setText(tr("New \nWorkflow"));                    // cn:新建\n工作流
    actionWorkflowNew->setToolTip(tr("Create a new workflow"));          // cn:创建新工作流
    actionWorkflowEnableItemLinkageMove->setText(tr("Linkage \nMove"));  // cn:联动
    actionWorkflowEnableItemLinkageMove->setToolTip(
        tr("When moving elements, other elements linked to this element follow the movement"));  // cn:允许移动图元时，其它和此图元链接起来的图元跟随移动
    actionItemGrouping->setText(tr("Grouping"));                                                 // cn:分组
    actionItemGrouping->setToolTip(tr("Group selected elements"));   // cn:对选中的元素进行分组
    actionItemUngroup->setText(tr("Ungroup"));                       // cn:取消分组
    actionItemUngroup->setToolTip(tr("Ungroup selected elements"));  // cn:对选中的元素进行取消分组
    actionWorkflowStartDrawRect->setText(tr("Draw \nRect"));         // cn:绘制\n矩形
    actionWorkflowStartDrawRect->setToolTip(tr("Draw a rectangle on the workflow scene"));  // cn:在工作流场景中绘制矩形
    actionWorkflowStartDrawText->setText(tr("Draw \nText"));                                // cn:绘制\n文本
    actionWorkflowStartDrawText->setToolTip(tr("Draw text on the workflow scene"));         // cn:在工作流场景中绘制文本
    actionWorkflowShowGrid->setText(tr("Show \nGrid"));                                     // cn:显示\n网格
    actionWorkflowShowGrid->setToolTip(tr("Show grid on the workflow scene"));              // cn:在工作流场景中显示网格
    actionWorkflowViewReadOnly->setText(tr("Lock \nView"));                                 // cn:锁定\n视图
    actionWorkflowViewReadOnly->setToolTip(tr("Lock the workflow view"));                   // cn:锁定工作流视图
    actionWorkflowRun->setText(tr("Run \nWorkflow"));                                       // cn:运行\n工作流
    actionWorkflowRun->setToolTip(tr("Run the workflow"));                                  // cn:运行工作流
    actionWorkflowTerminate->setText(tr("Terminate \nWorkflow"));                           // cn:停止\n工作流
    actionWorkflowTerminate->setToolTip(tr("Terminate the workflow"));                      // cn:停止工作流
    actionWorkflowLinkEnable->setText(tr("Link"));                                          // cn:连线
    actionWorkflowLinkEnable->setToolTip(tr("Enable or disable link between elements"));    // cn:启用或禁用元素之间连线
    actionWorkflowAddBackgroundPixmap->setText(tr("Add \nBackground"));                     // cn:添加\n背景
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
    actionExportWorkflowSceneToPNG->setText(tr("Export To PNG"));                               // cn:导出为PNG
    actionExportWorkflowSceneToPNG->setToolTip(tr("Export the workflow scene to a PNG image"));  // cn:导出工作流场景为PNG图片
    actionWorkflowViewMarker->setText(tr("Show Marker"));                                        // cn:显示标记
    actionWorkflowViewMarker->setToolTip(tr("Show marker on the workflow scene"));  // cn:在工作流场景中显示标记
    //-----------------------------------------------------
    // View Category
    //-----------------------------------------------------
    actionShowWorkFlowArea->setText(tr("Show \nWorkflow Area"));                      // cn:工作流\n区域
    actionShowWorkFlowArea->setToolTip(tr("Show the workflow area"));                 // cn:显示工作流区域
    actionShowWorkFlowManagerArea->setText(tr("Show \nWorkflow Manager"));            // cn:工作流\n管理
    actionShowWorkFlowManagerArea->setToolTip(tr("Show the workflow manager area"));  // cn:显示工作流管理区域
    actionShowChartArea->setText(tr("Show \nChart Area"));                            // cn:绘图\n区域
    actionShowChartArea->setToolTip(tr("Show the chart area"));                       // cn:显示绘图区域
    actionShowChartManagerArea->setText(tr("Show \nChart Manager"));                  // cn:绘图\n管理
    actionShowChartManagerArea->setToolTip(tr("Show the chart manager area"));        // cn:显示绘图管理区域
    actionShowDataArea->setText(tr("Show \nTable Area"));                             // cn:表格\n区域
    actionShowDataArea->setToolTip(tr("Show the data area"));                         // cn:显示数据区域
    actionShowDataManagerArea->setText(tr("Show \nData Manager"));                    // cn:数据\n管理
    actionShowDataManagerArea->setToolTip(tr("Show the data manager area"));          // cn:显示数据管理区域
    actionShowMessageLogView->setText(tr("Show Information Window"));                  // cn:信息窗口
    actionShowMessageLogView->setToolTip(tr("Show the message log window"));          // cn:显示信息窗口
    actionShowSettingWidget->setText(tr("Show Setting Window"));                      // cn:设置窗口
    actionShowSettingWidget->setToolTip(tr("Show the setting window"));               // cn:显示设置窗口
    actionShowLeftSideBar->setText(tr("Show Left \nSide Bar"));                       // cn:显示左侧边栏
    actionShowLeftSideBar->setToolTip(tr("Show the left side bar"));  // cn:显示左侧边栏
    actionShowRightSideBar->setText(tr("Show Right \nSide Bar"));       // cn:显示右侧边栏
    actionShowRightSideBar->setToolTip(tr("Show the right side bar"));  // cn:显示右侧边栏
    actionShowAgentArea->setText(tr("Show \nAgent Area"));  // cn:Agent\n助手
    actionShowAgentArea->setToolTip(tr("Show the agent assistant area"));  // cn:显示 Agent 助手区域
    // Config Category
    actionPluginManager->setText(tr("Plugin \nConfig"));             // cn:插件\n设置
    actionPluginManager->setToolTip(tr("Show the plugin manager"));  // cn:显示插件管理器
    // Other
    actionRibbonThemeOffice2013->setText(tr("Office 2013 Theme"));      // cn:Office 2013 主题
    actionRibbonThemeOffice2016Blue->setText(tr("Office 2016 Blue Theme"));  // cn:Office 2016 蓝色主题
    actionRibbonThemeOffice2021Blue->setText(tr("Office 2021 Blue Theme"));  // cn:Office 2021 蓝色主题
    actionRibbonThemeDark->setText(tr("Dark Theme"));                  // cn:深色主题

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
