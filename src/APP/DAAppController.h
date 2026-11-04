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
class DAAppWorkflowActionHandler;
class DAAppDataActionHandler;
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
    // 导入对话框的文件过滤器（由 initScripts 从 Python 侧读取）
    const QStringList& fileReadFilters() const;
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
public:
    // 数据导出/复制：转发到数据域分发器（主导出按钮、数据管理树右键与表头右键共用；
    // 表格域分发器亦经此门面调用，避免分发器之间互相持有）
    void copySelectedDataAsTable(DA::DATableExporter::Format fmt,
                                 const QList< int >& columns = QList< int >(),
                                 DADataOperateOfDataFrameWidget* sourcePage = nullptr);
    void exportSelectedDataAsDocument(DA::DATableExporter::Format fmt,
                                      const QString& path = QString(),
                                      const QList< int >& columns = QList< int >(),
                                      DADataOperateOfDataFrameWidget* sourcePage = nullptr);
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
    // 显示右侧边栏
    void onActionShowRightSideBarTriggered(bool on);
    // 显示左侧边栏
    void onActionShowLeftSideBarTriggered(bool on);
    //===================================================
    // workflow上下文
    //===================================================



    //===================================================
    // 主页剪贴板（按焦点路由：工作流→inner action；表格→复制/粘贴；图表→复制绘图）
    //===================================================
    void onActionCutTriggered();
    void onActionCopyTriggered();
    void onActionPasteTriggered();
    void onActionDeleteTriggered();
    void onActionSelectAllTriggered();


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
    DAAppWorkflowActionHandler* mWorkflowHandler { nullptr };  ///< 工作流域动作分发器（随控制器销毁）
    DAAppDataActionHandler* mDataHandler { nullptr };          ///< 数据域动作分发器（随控制器销毁）
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
