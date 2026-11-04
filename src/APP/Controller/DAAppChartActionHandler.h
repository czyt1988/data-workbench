#ifndef DAAPPCHARTACTIONHANDLER_H
#define DAAPPCHARTACTIONHANDLER_H
#include <QObject>
#include <QList>
#include <memory>
#include "DAAppActionHandlerDeps.h"
#include "Chart/DAStatsPlotRequest.h"
#include "DAFigureElementSelection.h"
#include "DAFigureWidget.h"  // onChartEditorStatusChanged 用嵌套枚举 ChartEditorStatus

class QAction;
class QMenu;

namespace DA
{
class DADialogStatsChartGuide;
class DAChartWidget;
class DAFigureWidget;
class DAChartManageWidget;
class DAFigureElementSelection;

/**
 * @brief 图表域动作分发器：添加绘图/坐标系/各类图表与统计图、图表属性开关、图表生命周期
 *
 * 从 DAAppController 抽出的分发器，只负责把动作转成对当前活动对象的调用；
 * 翻译上下文沿用 DA::DAAppController（.ts 译文已存在，拆类不改上下文）。
 */
class DAAppChartActionHandler : public QObject
{
    Q_OBJECT
public:
    explicit DAAppChartActionHandler(const DAAppActionHandlerDeps& deps, QObject* par = nullptr);
    // 建立本域的动作与信号绑定
    void initConnections();

public Q_SLOTS:
    // 绘图元素选中，信号由DAChartManageWidget发出
    void onFigureElementClicked(const DAFigureElementSelection& selection);
    void onFigureElementDbClicked(const DAFigureElementSelection& selection);
    // 绘图窗口有新窗口创建
    void onFigureCreated(DA::DAFigureWidget* f);
    // 绘图窗口当前窗口改变
    void onCurrentFigureChanged(DA::DAFigureWidget* f, int index);
    // 当前图表窗口改变
    void onCurrentChartChanged(DA::DAChartWidget* c);
    // 图表编辑器状态改变
    void onChartEditorStatusChanged(DA::DAFigureWidget::ChartEditorStatus status);
    // 绘图项创建完成（DADialogChartGuide 确认后），提升绘图 dock 显示新绘图
    void onPlotItemCreated(DA::DAFigureWidget* f, DA::DAChartWidget* plot, QwtPlotItem* item);
    // 统计窗口「转入绘图区」：把当前统计图转成绘图区里的一个新绘图
    void onStatsTransferToFigureRequested();
    // 复制到剪切板
    void onActionCopyFigureToClipboardTriggered();

private Q_SLOTS:
    // 添加绘图
    void onActionAddFigureTriggered();
    // 新坐标系
    void onActionFigureNewXYAxisTriggered();
    // 添加曲线
    void onActionChartAddCurveTriggered();
    // 添加散点图
    void onActionChartAddScatterTriggered();
    // 添加柱状图
    void onActionChartAddBarTriggered();
    // 添加误差棒图
    void onActionactionChartAddErrorBarTriggered();
    // 添加箱型图
    void onActionChartAddBoxPlotTriggered();
    // 添加谱图
    void onActionChartAddCloudMapTriggered();
    // 添加多重柱状图
    void onActionChartAddMultiBarTriggered();
    // 添加直方图
    void onActionChartAddHistogramTriggered();
    // 添加等高线图
    void onActionChartAddContourMapTriggered();
    // 添加向量场图
    void onActionChartAddVectorfieldTriggered();
    // 添加3D曲面图
    void onActionChartAdd3DSurfaceTriggered();
    // 添加3D柱状图
    void onActionChartAdd3DBarTriggered();
    // 添加3D线图
    void onActionChartAdd3DLineTriggered();
    void onActionStatsHistplotTriggered();
    void onActionStatsKdeplot1dTriggered();
    void onActionStatsKdeplot2dTriggered();
    void onActionStatsBoxplotTriggered();
    void onActionStatsHeatmapTriggered();
    void onActionStatsScatterplotTriggered();
    void onActionStatsBarplotTriggered();
    void onActionStatsRegplotTriggered();
    void onActionStatsECDFplotTriggered();
    // 确保当前 Figure 和 Chart 存在（不存在则创建）
    bool ensureFigureChart(DA::DAFigureWidget*& fig, DA::DAChartWidget*& chart);
    // 显示统计绘图引导对话框并预选指定类型
    void showStatsChartGuide(DA::DAChartTypes type);
    // 允许网格
    void onActionChartEnableGridTriggered(bool on);
    // 允许网格X
    void onActionChartEnableGridXTriggered(bool on);
    // 允许网格Y
    void onActionChartEnableGridYTriggered(bool on);
    // 允许网格XMin
    void onActionChartEnableGridXMinEnableTriggered(bool on);
    // 允许网格YMin
    void onActionChartEnableGridYMinTriggered(bool on);
    // 允许缩放
    void onActionChartEnableZoomTriggered(bool on);
    // 当前图表放大
    void onActionChartZoomInTriggered();
    // 当前图表缩小
    void onActionChartZoomOutTriggered();
    // 当前图表全部显示
    void onActionChartZoomAllTriggered();
    // 允许绘图拖动
    void onActionChartEnablePanTriggered(bool on);
    // 禁止水平缩放
    void onActionChartDisableZoomXTriggered(bool on);
    // 禁止垂直缩放
    void onActionChartDisableZoomYTriggered(bool on);
    // 允许绘图拾取
    void onActionChartEnablePickerCrossTriggered(bool on);
    // 允许绘图拾取Y
    void onActionChartEnablePickerYTriggered(bool on);
    // 绘图样式选择
    void onActionGroupChartPickerTextRegionTriggered(QAction* act);
    // 允许绘图拾取XY
    void onActionChartEnablePickerXYTriggered(bool on);
    // 连接所有picker
    void onActionChartLinkAllPickerEnabledTriggered(bool on);
    // 是否在ypicker的时候显示x值
    void onActionChartYPickerShowXValueEnabledTriggered(bool on);
    // 允许绘图图例
    void onActionChartEnableLegendTriggered(bool on);
    // 绘图样式选择
    void onActionGroupFigureThemeTriggered(QAction* act);
    // 图例位置菜单切换
    void onActionGroupChartLegendPositionTriggered(QAction* act);
    // 数据拾取设置
    void onActionChartDataPickerSettingTriggered();
    // 绘图编辑器的切换
    void onActionGroupChartEditorTriggered(QAction* a);
    // 统计绘图引导对话框确认槽：类型化请求随信号一起传入，执行编排并提升绘图区
    void onStatsGuideAccepted(const DA::DAStatsPlotRequest& req);

private:
    DAAppActionHandlerDeps mDeps;
    DADialogStatsChartGuide* mStatsChartGuideDlg { nullptr };  ///< 统计绘图引导对话框
};
}  // namespace DA

#endif  // DAAPPCHARTACTIONHANDLER_H
