#ifndef DAAPPCONTROLLER_H
#define DAAPPCONTROLLER_H
#include <QObject>
#include <QAction>
#include <QUndoStack>
#include <QScopedPointer>
#include "DADataManageWidget.h"
#include "DAWorkFlowGraphicsScene.h"
#ifdef DA_ENABLE_PYTHON
#include "numpy/DAPyDType.h"
#endif
// Qt
class QComboBox;
class QToolBar;
class QMenuBar;
class QFontComboBox;
class QUndoStack;
class QGraphicsItem;
// Qt-Advanced-Docking-System 前置申明
namespace ads
{
class CDockWidget;
}
namespace DA
{
class AppMainWindow;
class DAAppCore;
class DAAppRibbonArea;
class DAAppDockingArea;
class DAAppCommand;
class DAAppActions;
class DAAppDataManager;
class DADataOperateOfDataFrameWidget;
class DAWorkFlowOperateWidget;
class DADataOperateWidget;
class DAChartOperateWidget;
class DADataManageWidget;
class DAFigureWidget;
class DAChartWidget;
class DADataOperatePageWidget;
class DAAppSettingDialog;
class DAAppConfig;
class DAWorkFlowEditWidget;
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
public:
    DAAppController(QObject* par = nullptr);
    ~DAAppController();
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

public:
    // 获取当前dataframeOperateWidget,如果没有返回nullptr,此函数不返回nullptr的前提是
    DADataOperateOfDataFrameWidget* getCurrentDataFrameOperateWidget(bool checkDataOperateAreaFocused = true);
    // 获取工作流操作窗口
    DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const;
    // 获取数据操作窗口
    DADataOperateWidget* getDataOperateWidget() const;
    // 获取绘图操作窗口
    DAChartOperateWidget* getChartOperateWidget() const;
    // 获取数据管理窗口
    DADataManageWidget* getDataManageWidget() const;
    // 获取当前的绘图,如果没有回返回nullptr
    DAFigureWidget* getCurrentFigure();
    DAFigureWidget* gcf();
    // 获取绘图操作窗口,如果没有回返回nullptr
    DAChartWidget* getCurrentChart() const;
    DAChartWidget* gca() const;
    // 判断当前是否是在绘图操作模式，就算绘图操作不在焦点，但绘图操作在前端，此函数也返回true
    bool isLastFocusedOnChartOptWidget() const;
    bool isLastFocusedOnWorkflowOptWidget() const;
    bool isLastFocusedOnDataOptWidget() const;
    DAAppConfig* getConfig() const;
    void setConfig(DAAppConfig* config);

private slots:

    //===================================================
    // 主页标签 Main Category
    //===================================================
    // 打开文件
    void onActionOpenTriggered();

    void onActionAppendProjectTriggered();

    // 保存工程
    void onActionSaveTriggered();
    // 另存为
    void onActionSaveAsTriggered();

    // app设定
    void onActionSettingTriggered();
    // 插件管理对话框触发
    void onActionPluginManagerTriggered(bool on);
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
    // 添加绘图
    void onActionFigureResizeChartTriggered(bool on);
    // 新坐标系
    void onActionFigureNewXYAxisTriggered();
    // 新坐标系
    void onActionChartAddCurveTriggered();
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
    // 允许绘图拾取XY
    void onActionChartEnablePickerXYTriggered(bool on);
    // 允许绘图图例
    void onActionChartEnableLegendTriggered(bool on);
    // 绘图图例对齐的actiongroup
    void onActionGroupChartLegendAlignmentTriggered(QAction* a);
    // 绘图图例的最大列数、margin、spacing等发生改变
    void onChartLegendMaxColumnsValueChanged(int v);
    void onChartLegendMarginValueChanged(int v);
    void onChartLegendSpacingValueChanged(int v);
    void onChartLegendItemMarginValueChanged(int v);
    void onChartLegendItemSpacingValueChanged(int v);
    void onChartLegendBorderRadiusValueChanged(double v);
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
    // 创建数据描述
    void onActionCreateDataDescribeTriggered();
#ifdef DA_ENABLE_PYTHON
    // 列数据类型改变
    void onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt);
    void onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DA::DAPyDType dt);
#endif
    // 选中列转换为数值
    void onActionCastToNumTriggered();
    // 选中列转换为文字
    void onActionCastToStringTriggered();
    // 选中列转换为日期
    void onActionCastToDatetimeTriggered();
    // 选中列转换为索引
    void onActionChangeToIndexTriggered();

    //===================================================
    // 视图标签 View Category
    //===================================================
    // 显示工作流区域
    void onActionShowWorkFlowAreaTriggered();
    // 显示绘图区域
    void onActionShowChartAreaTriggered();
    // 显示数据区域
    void onActionShowDataAreaTriggered();
    // 显示信息区域
    void onActionShowMessageLogViewTriggered();
    // 显示设置区域
    void onActionSettingWidgetTriggered();

    //===================================================
    // workflow上下文
    //===================================================

    void onActionNewWorkflowTriggered();

    // 绘制矩形
    void onActionStartDrawRectTriggered(bool on);
    // 绘制文本框
    void onActionStartDrawTextTriggered(bool on);
    // 运行
    void onActionRunCurrentWorkflowTriggered();
    // 终止
    void onActionTerminateCurrentWorkflowTriggered();

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
    //===================================================
    // 其他
    //===================================================
    // 主题切换
    void onActionGroupRibbonThemeTriggered(QAction* a);
private slots:
    //===================================================
    // DAWorkFlowOperateWidget的槽
    //===================================================
    void onSelectionGraphicsItemChanged(QGraphicsItem* lastSelectItem);
    void onWorkflowStartExecute(DA::DAWorkFlowEditWidget* wfw);
    void onWorkflowFinished(DA::DAWorkFlowEditWidget* wfw, bool success);
    //===================================================
    // DAChartOperateWidget
    //===================================================
    // 绘图窗口有新窗口创建
    void onFigureCreated(DA::DAFigureWidget* f);
    // 绘图窗口当前窗口改变
    void onCurrentFigureChanged(DA::DAFigureWidget* f, int index);
    // 图表有新窗口创建
    void onChartAdded(DA::DAChartWidget* c);
    // 当前图表窗口改变
    void onCurrentChartChanged(DA::DAChartWidget* c);
    //===================================================
    // project
    //===================================================
    void onProjectSaved(const QString& path);
    void onProjectLoaded(const QString& path);

    //===================================================
    // DADataOperatePageWidget数据操作相关
    //===================================================
    // 数据操作窗口添加，需要绑定相关信号槽到ribbon的页面
    void onDataOperatePageAdded(DA::DADataOperatePageWidget* page);

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
    // DAWorkFlowGraphicsScene
    //===================================================
    // 鼠标动作结束
    void onWorkFlowGraphicsSceneMouseActionFinished(DAWorkFlowGraphicsScene::MouseActionFlag mf);

private:
    // 初始化信号槽
    void initConnection();
#ifdef DA_ENABLE_PYTHON
    // 初始化脚本信息
    void initScripts();
#endif
private:
    AppMainWindow* mMainWindow { nullptr };
    DAAppCore* mCore { nullptr };
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
    DAAppConfig* mConfig;                                   ///< 设置类
};
}

#endif  // DAAPPCONTROLLER_H
