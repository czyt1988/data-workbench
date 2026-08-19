#ifndef DAAPPCONTROLLER_H
#define DAAPPCONTROLLER_H
#include <QObject>
#include <functional>
#include <QAction>
#include <QUndoStack>
#include <QScopedPointer>
#include <QJsonObject>
#include "DADataManageWidget.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include "DAFigureElementSelection.h"
#include "DAFigureWidget.h"
#include "numpy/DAPyDType.h"
#include "DAChart3DWidget.h"
#include "qwt3d_plotitem.h"
// Qt
class QComboBox;
class QToolBar;
class QMenuBar;
class QFontComboBox;
class QUndoStack;
class QGraphicsItem;
class QMenu;
// qwt
class QwtPlotItem;
// Qt-Advanced-Docking-System 前置申明
namespace ads
{
class CDockWidget;
}
namespace DA
{
class DATableCellStyle;
class AppMainWindow;
class DAAppCore;
class DAProjectInterface;
class DAAppRibbonArea;
class DAAppDockingArea;
class DAAppCommand;
class DAAppActions;
class DAAppDataManager;
class DASettingContainerWidget;
class DADataOperateOfDataFrameWidget;
class DAPyWorkFlowOperateWidget;
class DADataOperateWidget;
class DAAppChartOperateWidget;
class DADialogStatsChartGuide;
class DADataManageWidget;
class DAChartWidget;
class DADataOperatePageWidget;
class DAAppSettingDialog;
class DAAppConfig;
class DAPyWorkFlowEditWidget;
/**
 * @brief 控制层负责逻辑的对接
 */
class DAAppController : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 针对Operate窗口的最后焦点枚举，用于识别最后用户所在的操作窗口
     */
    enum LastFocusedOpertateWidget
    {
        LastFocusedNoneOptWidget = 0x00,
        LastFocusedOnWorkflowOpt = 0x01,  ///< 最后焦点在工作流操作窗口
        LastFocusedOnDataOpt     = 0x02,  ///< 最后焦点在数据操作窗口
        LastFocusedOnChartOpt    = 0x04   ///< 最后焦点在绘图操作窗口
    };
    Q_DECLARE_FLAGS(LastFocusedOpertateWidgets, LastFocusedOpertateWidget)
    Q_FLAG(LastFocusedOpertateWidget)
    /**
     * @brief 批量应用绘图的函数指针
     *
     * 返回false，将停止应用
     */
    using FpChartWidgetApply = std::function< bool(DAChartWidget*) >;

public:
    DAAppController(QObject* par = nullptr);
    ~DAAppController() override;
    // 设置AppMainWindow
    DAAppController& setAppMainWindow(AppMainWindow* mainWindow);
    // 设置core
    DAAppController& setAppCore(DAAppCore* core);
    // 设置ribbon
    DAAppController& setAppRibbonArea(DAAppRibbonArea* ribbon);
    // 设置dock
    DAAppController& setAppDockingArea(DAAppDockingArea* dock);
    // 设置AppCommand
    DAAppController& setAppCommand(DAAppCommand* cmd);
    // 设置AppActions
    DAAppController& setAppActions(DAAppActions* act);
    // 设置AppDataManager
    DAAppController& setAppDataManager(DAAppDataManager* d);
    // 获取app
    AppMainWindow* app() const;
    // 初始化--必须初始化才能生效
    void initialize();
    // 插件加载后调用：推送模型选择 + 预启动 agent（确保工具已注册）
    void postPluginInit();

public:
    // 获取当前dataframeOperateWidget,如果没有返回nullptr,此函数不返回nullptr的前提是
    DADataOperateOfDataFrameWidget* getCurrentDataFrameOperateWidget(bool checkDataOperateAreaFocused = true,
                                                                     bool isShowMessage               = true);
    // 获取工作流操作窗口
    DAPyWorkFlowOperateWidget* getWorkFlowOperateWidget() const;
    // 获取数据操作窗口
    DADataOperateWidget* getDataOperateWidget() const;
    // 获取绘图操作窗口
    DAAppChartOperateWidget* getChartOperateWidget() const;
    // 获取数据管理窗口
    DADataManageWidget* getDataManageWidget() const;
    // 获取当前的绘图,如果没有回返回nullptr
    DAFigureWidget* getCurrentFigure();
    DAFigureWidget* gcf();
    // 获取绘图操作窗口,如果没有回返回nullptr
    DAChartWidget* getCurrentChart() const;
    DAChartWidget* gca() const;
    QList< DAChartWidget* > getAllCharts() const;
    QList< DAChartWidget* > gcas() const;
    bool applyToCharts(const FpChartWidgetApply& fp);
    // 获取需要操作的绘图
    QList< DAChartWidget* > needOperateCharts() const;
    // 获取设置窗口
    DASettingContainerWidget* getSettingContainerWidget() const;
    // 判断当前是否是在绘图操作模式，就算绘图操作不在焦点，但绘图操作在前端，此函数也返回true
    bool isLastFocusedOnChartOptWidget() const;
    bool isLastFocusedOnWorkflowOptWidget() const;
    bool isLastFocusedOnDataOptWidget() const;
    DAAppConfig* getConfig() const;
    void setConfig(DAAppConfig* config);
    // 设置工程为dirty
    void setDirty(bool on = true);
    bool isDirty() const;
    // 导入数据
    bool importData(const QString& filePath, const QVariantMap& args, QString* err = nullptr);
    // 更新窗口标题
    void updateWindowTitle();
    // 生成窗口标题
    static QString makeWindowTitle();
    static QString makeWindowTitle(DAProjectInterface* proj);
    // 是否应用到所有绘图
    bool isApplyToAllCharts() const;
public Q_SLOTS:
    // 保存
    bool save();
    // 另存为
    void saveAs();
    //   打开 文件前的检查，  返回true说明可以打开
    bool openCheck();
    // 打开文件
    void open();
    // 打开工程文件
    bool openProjectFile(const QString& projectFilePath);
private Q_SLOTS:
    // 工程的胀状态改变槽
    void onProjectDirtyStateChanged(bool isdirty);
    //===================================================
    // 主页标签 Main Category
    //===================================================
    void onActionAppendProjectTriggered();
    // 打开 Markdown 文件，在中央区 dock 中显示
    void onActionOpenMarkdownTriggered();

    // app设定
    void onActionSettingTriggered();
    // 插件管理对话框触发
    void onActionPluginManagerTriggered(bool on);
    // about
    void onActionAboutTriggered();
    //===================================================
    // 数据标签 Data Category
    //===================================================
    // 添加数据
    void onActionAddDataTriggered();
    // 移除数据
    void onActionRemoveDataTriggered();
    //===================================================
    // 绘图标签 Chart Category
    //===================================================
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
    //===================================================
    // 统计绘图 Stats Plot
    //===================================================
    void onActionStatsHistplotTriggered();
    void onActionStatsKdeplot1dTriggered();
    void onActionStatsKdeplot2dTriggered();
    void onActionStatsBoxplotTriggered();
    void onActionStatsHeatmapTriggered();
    void onActionStatsScatterplotTriggered();
    void onActionStatsBarplotTriggered();
    void onActionStatsRegplotTriggered();
    void onActionStatsECDFplotTriggered();
    //===================================================
    // 绘图标签 Chart Context Category
    //===================================================
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
    // 复制到剪切板
    void onActionCopyFigureToClipboardTriggered();
    // 数据拾取设置
    void onActionChartDataPickerSettingTriggered();
    // 绘图编辑器的切换
    void onActionGroupChartEditorTriggered(QAction* a);
    //===================================================
    // 数据操作的上下文标签 Data Operate Context Category
    //===================================================
    // 移除选中行
    void onActionRemoveRowTriggered();
    // 移除选中列
    void onActionRemoveColumnTriggered();
    // 移除单元格内容
    void onActionRemoveCellTriggered();
    // 插入一行
    void onActionInsertRowTriggered();
    // 在选中位置上面插入一行
    void onActionInsertRowAboveTriggered();
    // 在选中位置右边插入一列
    void onActionInsertColumnRightTriggered();
    // 在选中位置左边插入一列
    void onActionInsertColumnLeftTriggered();
    // dataframe列重命名
    void onActionRenameColumnsTriggered();
    // dataframe单列重命名（表头右键）
    void onActionRenameColumnTriggered();
    // 复制列名到剪贴板（表头右键）
    void onActionCopyColumnNameTriggered();
    // 跳转到最大值（表头右键）
    void onActionGotoMaxTriggered();
    // 跳转到最小值（表头右键）
    void onActionGotoMinTriggered();
    // 显示列统计信息（表头右键）
    void onActionShowColumnDescribeTriggered();
    // 列数据类型改变
    void onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt);
    void onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DA::DAPyDType dt);
    // 选中列转换为数值
    void onActionCastToNumTriggered();
    // 选中列转换为文字
    void onActionCastToStringTriggered();
    // 选中列转换为日期
    void onActionCastToDatetimeTriggered();
    // 选中列转换为索引
    void onActionChangeToIndexTriggered();
    // 表格样式
    void onTableStyleFillColorChanged(const QColor& c);
    void onTableStyleFontChanged(const QFont& f);
    void onTableStyleFontColorChanged(const QColor& c);
    void onActionClearStyleSelectedTriggered();
    void onActionClearStyleAllTriggered();
    // 选中区样式反向同步 ribbon 控件
    void onTableStyleCurrentChanged(const DATableCellStyle& style);

    //===================================================
    // 视图标签 View Category
    //===================================================
    // 显示工作流区域
    void onActionShowWorkFlowAreaTriggered();
    void onActionShowWorkFlowManagerAreaTriggered();
    // 显示绘图区域
    void onActionShowChartAreaTriggered();
    void onActionShowChartManagerAreaTriggered();
    // 显示数据区域
    void onActionShowDataAreaTriggered();
    void onActionShowDataManagerAreaTriggered();
    // 显示信息区域
    void onActionShowMessageLogViewTriggered();
    // 显示设置区域
    void onActionSettingWidgetTriggered();
    // 显示标记线 - 此action有个menu，menu的action选中会设置当前action的图标，具体实现放在DAAppRibbonArea::buildContextCategoryWorkflowView_函数中
    void onActionWorkflowViewMarkerTriggered(bool on);
    // 显示右侧边栏
    void onActionShowRightSideBarTriggered(bool on);
    // 显示左侧边栏
    void onActionShowLeftSideBarTriggered(bool on);
    //===================================================
    // workflow上下文
    //===================================================

    void onActionNewWorkflowTriggered();

    // 绘制矩形
    void onActionStartDrawRectTriggered(bool on);
    // 绘制文本框
    void onActionStartDrawTextTriggered(bool on);
    // 允许连线
    void onActionWorkflowLinkEnableTriggered(bool on);

    // 通用的字体变更
    void onEditFontChanged(const QFont& f);
    void onEditFontColorChanged(const QColor& c);
    // 通用的背景和框线变更
    void onEditBrushChanged(const QBrush& b);
    void onEditPenChanged(const QPen& p);

    // 当前工作流的字体变更
    void onCurrentWorkflowFontChanged(const QFont& f);
    void onCurrentWorkflowFontColorChanged(const QColor& c);
    // 图元的背景和框线变更
    void onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b);
    void onCurrentWorkflowShapeBorderPenChanged(const QPen& p);
    // 添加背景图
    void onActionAddBackgroundPixmapTriggered();
    // 锁定背景图
    void onActionLockBackgroundPixmapTriggered(bool on);
    // 跟随背景图
    void onActionEnableItemMoveWithBackgroundTriggered(bool on);
    // 允许移动图元时，其它和此图元链接起来的图元跟随移动
    void onActionWorkflowEnableItemLinkageMoveTriggered(bool on);
    // 分组
    void onActionItemGroupingTriggered();
    // 取消分组
    void onActionItemUngroupTriggered();

    /////////---视图------
    // 导出png
    void onActionExportWorkflowScenePNGTriggered();
    // 锁定视图
    void onActionWorkflowViewReadOnlyTriggered(bool on);
    /////////---运行------
    // 运行
    void onActionRunCurrentWorkflowTriggered();
    // 终止
    void onActionTerminateCurrentWorkflowTriggered();
    //===================================================
    // 其他
    //===================================================
    // 主题切换
    void onActionGroupRibbonThemeTriggered(QAction* a);
private Q_SLOTS:
    //===================================================
    // DAPyWorkFlowOperateWidget的槽
    //===================================================
    void onWorkflowSceneSelectionItemChanged(QGraphicsItem* lastSelectItem);
    void onWorkflowStartExecute(DA::DAPyWorkFlowEditWidget* wfw);
    void onWorkflowFinished(DA::DAPyWorkFlowEditWidget* wfw, bool success);
    void onWorkflowSceneitemsAdded(DA::DAGraphicsScene* sc, const QList< QGraphicsItem* >& its);
    void onWorkflowSceneitemsRemoved(DA::DAGraphicsScene* sc, const QList< QGraphicsItem* >& its);
    void onCurrentWorkflowWidgetChanged(DA::DAPyWorkFlowEditWidget* wfw);
    //===================================================
    // DAChartOperateWidget
    //===================================================
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
    // 3D绘图项创建完成，提升绘图 dock 显示新3D绘图
    void onPlot3DItemCreated(DA::DAFigureWidget* f, DA::DAChart3DWidget* plot, Qwt3DPlotItem* item);
    // 统计绘图请求槽：DAAbstractStatsChartAddWidget::plotRequested -> 调用 Python plot()
    void onStatsPlotRequested(const QJsonObject& params, DA::DAFigureWidget* fig, DA::DAChartWidget* chart);
    // 统计绘图引导对话框确认槽（直接从对话框获取 widget，不依赖 sender()）
    void onStatsGuideAccepted(const QJsonObject& params, DA::DAFigureWidget* fig, DA::DAChartWidget* chart);
    //===================================================
    // project
    //===================================================
    void onProjectSaved(const QString& path);
    void onProjectLoaded(const QString& path);

    //===================================================
    // DADataOperatePageWidget数据操作相关
    //===================================================
    // 数据操作窗口添加，需要绑定相关信号槽到ribbon的页面
    void onDataOperatePageCreated(DA::DADataOperatePageWidget* page);
    //==========================================
    // Qt-Advanced-Docking-System
    //===================================================
    // DockWidget的焦点变化
    void onFocusedDockWidgetChanged(ads::CDockWidget* old, ads::CDockWidget* now);

    //===================================================
    // DADataManageWidget
    //===================================================

    // 一些界面的联动槽在此

    //===================================================
    // DAPyWorkFlowGraphicsScene
    //===================================================
    // 鼠标动作结束
    void onWorkFlowGraphicsSceneActionDeactive(DA::DAAbstractGraphicsSceneAction* scAction);
    // 工作流页面创建槽
    void onWorkflowCreated(DA::DAPyWorkFlowEditWidget* wfw);

    //===================================================
    //   其它
    //===================================================
    void onRecentFileSelected(const QString& filePath);
    // 绘图元素选中，信号由DAChartManageWidget发出
    void onFigureElementClicked(const DAFigureElementSelection& selection);
    void onFigureElementDbClicked(const DAFigureElementSelection& selection);
    // Agent 绘图引用超链接点击：da-figure: 协议链接 → 解析定位 → raise 绘图区
    void onFigureLinkRequested(const QString& href);

private:
    // 初始化信号槽
    void initConnection();
    // 初始化脚本信息
    void initScripts();
    // 初始化Python工作流信号槽
    void initPyWorkflowConnections();
    // 为 DataFrame 操作窗口的表头注入右键菜单
    void setupDataFrameHeaderContextMenu(DADataOperateOfDataFrameWidget* w);
    // 为数据管理树的 series 节点注入右键菜单（与表头右键共用 action）
    void setupDataManagerTreeSeriesContextMenu(DADataManagerTreeWidget* w);
    // 填充共用的列右键菜单项（表头右键与树右键共用）
    void populateColumnContextMenu(QMenu& menu);
    // 在 DataFrame 操作窗口中选中指定列（replace 语义，供表头/树右键共用）
    void selectColumnInDataFrameWidget(DADataOperateOfDataFrameWidget* w, int col);
    // 显示统计绘图引导对话框并预选指定类型
    void showStatsChartGuide(DA::DAChartTypes type);
    // 执行 Python 统计绘图的公共逻辑
    void executeStatsPlot(const QJsonObject& params, DA::DAFigureWidget* fig, DA::DAChartWidget* chart, const DAData& data);
    // 确保当前 Figure 和 Chart 存在（不存在则创建）
    bool ensureFigureChart(DA::DAFigureWidget*& fig, DA::DAChartWidget*& chart);

private:
    AppMainWindow* mMainWindow { nullptr };
    DAAppCore* mCore { nullptr };
    DAProjectInterface* mProject { nullptr };
    DAAppRibbonArea* mRibbon { nullptr };
    DAAppDockingArea* mDock { nullptr };
    DAAppCommand* mCommand { nullptr };
    DAAppActions* mActions { nullptr };
    DAAppDataManager* mDatas { nullptr };

    QStringList mFileReadFilters;  ///< 包含支持的文件[Images (*.png *.xpm *.jpg)] [Text files (*.txt)]
    //
    LastFocusedOpertateWidgets mLastFocusedOpertateWidget;  ///< 最后获取焦点的操作窗口
                                                            //
    DAAppSettingDialog* mSettingDialog { nullptr };         ///< 设置窗口
    DAAppConfig* mConfig { nullptr };                                   ///< 设置类
    DADialogStatsChartGuide* mStatsChartGuideDlg { nullptr };  ///< 统计绘图引导对话框
};
}

#endif  // DAAPPCONTROLLER_H
