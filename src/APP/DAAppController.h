#ifndef DAAPPCONTROLLER_H
#define DAAPPCONTROLLER_H
#include <QObject>
#include <functional>
#include <QAction>
#include <QUndoStack>
#include <QScopedPointer>
#include <memory>
#include <QJsonObject>
#include "DataManager/DADataManageWidget.h"
#include "Workflow/DAPyWorkFlowGraphicsScene.h"
#include "DAFigureElementSelection.h"
#include "DAFigureWidget.h"
#include "numpy/DAPyDType.h"
#include "Table/Metadata/DATableDisplayFormat.h"
#include "Table/Views/DADataHeaderView.h"
#include "Table/DATableExporter.h"
#include "DAChart3DWidget.h"
#include "qwt3d_plotitem.h"
#include "DAAgentLinkDispatcher.h"
#include "Controller/DAAppActionHandlerDeps.h"
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
class QwtText;
// Qt-Advanced-Docking-System 前置申明
namespace ads
{
class CDockWidget;
}
namespace DA
{
class DATableCellStyle;
class DATableDisplayFormat;
class DADialogTableDisplayFormat;
class AppMainWindow;
class DAAppCore;
class DAProjectInterface;
class DAAppRibbonArea;
class DAAppDockingArea;
class DAAppCommand;
class DAAppActions;
class DAAppDataManager;
class DAAppTableActionHandler;
class DAAppChartActionHandler;
class DASettingContainerWidget;
class DADataOperateOfDataFrameWidget;
class DAAbstractOperateWidget;
class DAPyWorkFlowOperateWidget;
class DADataOperateWidget;
class DAAppChartOperateWidget;
class DADialogStatsChartGuide;
class DAStatsPlotRequest;
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
    // 新建工程（当前工程有未保存修改时先提示保存），返回 true 表示已创建
    bool newProject();
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
    // 重命名数据集（树视图选中项进入编辑）
    void onActionRenameDataTriggered();
    // 导出数据（大按钮，format 为空则按保存对话框后缀判定）
    void onActionExportDataTriggered();
    // 按指定格式导出（csv/excel/pickle/parquet）
    void onActionExportDataAsTriggered();
    //===================================================
    // 绘图标签 Chart Category
    //===================================================
    //===================================================
    // 统计绘图 Stats Plot
    //===================================================
    //===================================================
    // 绘图标签 Chart Context Category
    //===================================================
    //===================================================
    // 图例位置菜单（挂在actionChartEnableLegend下）
    //===================================================
    //===================================================
    // 数据操作的上下文标签 Data Operate Context Category
    //===================================================
    // 撤销历史面板（005 T1.21，Ribbon「视图」页；入口选 (a) 点击即 raise，与现有交互一致）
    void onActionShowUndoHistoryTriggered();
    // 历史面板调整撤销深度：写回配置（ui.undo-stack-limit）并施加到全部已打开数据页
    void onUndoHistoryLimitChangeRequested(int limit);

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
    // 显示标记线 - 此action有个menu，menu的action选中会设置当前action的图标，具体实现放在DAAppRibbonArea::buildContextCategoryWorkflowView函数中
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

    //===================================================
    // 主页剪贴板（按焦点路由：工作流→inner action；表格→复制/粘贴；图表→复制绘图）
    //===================================================
    void onActionCutTriggered();
    void onActionCopyTriggered();
    void onActionPasteTriggered();
    void onActionDeleteTriggered();
    void onActionSelectAllTriggered();

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

    // 显示 Markdown 查看器（懒创建 dock：已创建时 raise，未打开过文件时弹文件选择框）
    void onActionShowMarkdownViewTriggered();
    // 恢复默认布局（触发前弹确认框）
    void onActionResetDefaultLayoutTriggered();
    // 打开布局管理对话框（保存/应用/删除布局方案）
    void onActionManageLayoutsTriggered();
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
    // 程序化raise dock窗口时激活context category
    void onDockWidgetRaised(QWidget* w);

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
    // Agent 本地跳转超链接点击：da-<kind>: 协议链接 → DAAgentLinkDispatcher 按协议分发
    void onLinkActivated(const QString& href);

private:
    // 初始化信号槽
    void initConnection();
    // 初始化脚本信息
    void initScripts();
    // 初始化Python工作流信号槽
    void initPyWorkflowConnections();
    // 执行导出：取选中数据集 → 保存对话框 → exportToFile → daInfo/daWarning 反馈
    // format 非空时直接使用（子菜单路径），为空时由文件后缀判定（大按钮路径）
    void exportSelectedData(const QString& format);
public:
    // 把选中的 dataframe 数据集复制到剪贴板（数据管理树右键「复制为」与表头右键
    // 「复制选中列为」共用）：fmt 决定目标形态（Tab 文本/Markdown/HTML），经
    // DATableExporter 构建；超大表先确认（isLargeForCopy 阈值），构建期间进度
    // 对话框可取消。columns 非空时只复制这些列（表头右键入口），空 = 整表；
    // sourcePage 非空时列子集取数固定用该页面（表头右键菜单是独立窗口，
    // 触发时停靠区焦点尚未归还，不能走焦点检查取当前页）
    void copySelectedDataAsTable(DA::DATableExporter::Format fmt,
                                 const QList< int >& columns = QList< int >(),
                                 DADataOperateOfDataFrameWidget* sourcePage = nullptr);
    // 导出选中数据集为 Markdown/HTML（右键导出子菜单、主导出按钮与表头右键
    // 「导出选中列」共用）：取数/渲染与复制共用 DATableExporter，增量写入文件；
    // path 非空时跳过保存对话框（主导出按钮已选好路径）。columns/sourcePage 语义同上
    void exportSelectedDataAsDocument(DA::DATableExporter::Format fmt,
                                      const QString& path = QString(),
                                      const QList< int >& columns = QList< int >(),
                                      DADataOperateOfDataFrameWidget* sourcePage = nullptr);
private:
    // 取要操作的数据集（优先数据管理树选中，回退当前操作数据）与对应表元数据束，
    // 成功构造 DATableExporter 返回 true（exporter/dataOut 为出参；束可为空=无样式）；
    // columns 非空时构造列子集导出器（表头右键「按选中列」入口）；
    // sourcePage 非空时列子集取数固定用该页面（空 = 焦点检查取当前页）
    bool makeExporterForSelection(std::unique_ptr< DA::DATableExporter >* exporter,
                                  DA::DAData* dataOut = nullptr,
                                  const QList< int >& columns = QList< int >(),
                                  DADataOperateOfDataFrameWidget* sourcePage = nullptr) ;
    // 表头右键「复制选中列为/导出选中列」信号处理器：page 固定为发送信号表头
    // 所在页面（菜单窗口抢焦点，不能经焦点检查取当前页），从页面解析选中列
    // （无选中回退 fallbackColumn），映射格式枚举后走共用复制/导出链路
    void onHeaderColumnDataCopyRequested(DADataOperateOfDataFrameWidget* page,
                                          int fallbackColumn,
                                          DA::DADataHeaderView::ColumnDataFormat format);
    void onHeaderColumnDataExportRequested(DADataOperateOfDataFrameWidget* page,
                                           int fallbackColumn,
                                           DA::DADataHeaderView::ColumnDataFormat format);
    // 根据widget激活对应的context category
    void activateContextCategoryForWidget(QWidget* widget);
    // 激活操作窗口当前的undo栈（焦点切到数据操作/绘图窗口时同步全局undo/redo action状态）
    void activateUndoStackForWidget(DAAbstractOperateWidget* w);
    // 注册 Agent 聊天窗口本地跳转链接处理器（da-figure:/da-data: 等协议，见 DAAgentLinkDispatcher）
    void setupAgentLinkHandlers();
    // da-figure: 协议处理：定位目标 figure 并 raise 绘图区（由 setupAgentLinkHandlers 注册）
    void handleFigureLink(const QString& href);
    // da-data: 协议处理：定位目标数据集并打开对应数据表（由 setupAgentLinkHandlers 注册）
    void handleDataLink(const QString& href);

private:
    AppMainWindow* mMainWindow { nullptr };
    DAAppCore* mCore { nullptr };
    DAProjectInterface* mProject { nullptr };
    DAAppRibbonArea* mRibbon { nullptr };
    DAAppDockingArea* mDock { nullptr };
    DAAppCommand* mCommand { nullptr };
    DAAppActions* mActions { nullptr };
    DAAppDataManager* mDatas { nullptr };
    DAAppTableActionHandler* mTableHandler { nullptr };  ///< 表格域动作分发器（随控制器销毁）
    DAAppChartActionHandler* mChartHandler { nullptr };  ///< 图表域动作分发器（随控制器销毁）
    // 组装分发器依赖（各分发器共用）
    DAAppActionHandlerDeps makeActionHandlerDeps();

    QStringList mFileReadFilters;  ///< 包含支持的文件[Images (*.png *.xpm *.jpg)] [Text files (*.txt)]
    //
    LastFocusedOpertateWidgets mLastFocusedOpertateWidget;  ///< 最后获取焦点的操作窗口
                                                            //
    DAAppSettingDialog* mSettingDialog { nullptr };         ///< 设置窗口
    DAAppConfig* mConfig { nullptr };                                   ///< 设置类
    DAAgentLinkDispatcher mAgentLinkDispatcher;  ///< Agent 本地跳转链接分发器（da-<kind>: 协议注册表）
};
}

#endif  // DAAPPCONTROLLER_H
