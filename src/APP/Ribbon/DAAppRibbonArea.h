#ifndef DAAPPRIBBONAREA_H
#define DAAPPRIBBONAREA_H
#include <QObject>
#include <QAction>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QList>
#include <QPen>
#include <QString>
#include "DARibbonAreaInterface.h"

#define DAAPPRIBBONAREA_COMMON_SETTING_H(MiddleName)                                                                   \
public:                                                                                                                \
    QPen get##MiddleName##Pen() const;                                                                                 \
    QBrush get##MiddleName##Brush() const;                                                                             \
    QFont get##MiddleName##Font() const;                                                                               \
    QColor get##MiddleName##FontColor() const;                                                                         \
public Q_SLOTS:                                                                                                        \
    void set##MiddleName##Pen(const QPen& v);                                                                          \
    void set##MiddleName##Brush(const QBrush& v);                                                                      \
    void set##MiddleName##Font(const QFont& v);                                                                        \
    void set##MiddleName##FontColor(const QColor& v);

// Qt 前置声明（本类只以指针持有）
class QMenu;
// SA Ribbon 前置声明
class SARibbonBar;
class SARibbonCategory;
class SARibbonPanel;
class SARibbonContextCategory;
class SARibbonLineWidgetContainer;
class SARibbonButtonGroupWidget;
class SARibbonGallery;
class SARibbonGalleryGroup;
class SARibbonColorToolButton;

namespace DA
{
class AppMainWindow;
class DAAppDockingArea;
class DAAppActions;
class DAAppCommand;
class DAAppApplicationWidget;
class DAPyDType;
class DAPyDTypeComboBox;
class DATableDisplayFormatComboBox;
// 窗口
class DAFontEditPannelWidget;
class DATextAlignmentEditWidget;
class DAShapeEditPannelWidget;
class DAFigureWidget;
class DAChartWidget;
class DAPyWorkFlowOperateWidget;
class DADataStatsWidget;
/**
 * @brief App的Ribbon区域接口，负责ribbon层的管理和调度
 *
 * @todo 后续抽象出DAAppRibbonAreaInterface，DAAppRibbonArea继承DAAppRibbonAreaInterface，并把可暴露的方法暴露出来，实现插件化
 *
 * 标签和panel的固定objectname契约统一定义在 @ref DAUiObjectNames.h（单一事实来源），
 * 本类构建时全部通过 DA::UiNames::Ribbon / DA::UiNames::Menu 常量赋值，
 * 插件侧查询请同样引用该头文件的常量，禁止手写字符串字面量。
 *
 * ## 实现文件组织
 *
 * 本类体量较大，但**刻意保持单一类**：界面文本全部由本类的 `tr()` 输出，而 Qt 的翻译
 * 上下文是「类名」，拆成多个类会让 `src/i18n/*.ts` 里已有的译文整体失效。因此按 Ribbon
 * 标签页拆分的是**实现文件**，不是类型：
 * - `DAAppRibbonArea.cpp`         协调者：装配顺序、菜单、主/数据/视图/绘图标签、上下文显隐、文本重置
 * - `DAAppRibbonAreaTable.cpp`    表格上下文：表格操作 / 数据操作 / 表格样式
 * - `DAAppRibbonAreaWorkflow.cpp` 工作流上下文：工作流视图 / 工作流编辑 / 工作流运行
 * - `DAAppRibbonAreaChart.cpp`    绘图上下文：绘图样式 / 图表编辑 + 图表状态同步
 * - `DAAppRibbonAreaStats.cpp`    统计上下文
 * - `DAAppRibbonAreaAgent.cpp`    AI 智能体标签页 + agent 画廊
 *
 * ## 依赖注入约定
 *
 * `build*` 函数**不读** `mContextXxx`、`mDockArea` 之类的隐式状态：父容器与页面专属的外部
 * 对象（`SARibbonBar*`、`SARibbonContextCategory*`、`DAPyWorkFlowOperateWidget*`、
 * `DADataStatsWidget*`、`Menus`）一律通过参数注入，函数只负责「把给定的父容器填满，并把
 * 本页产出的控件写回成员」。因此单个页面的构建可以脱离其余页面单独阅读与调整。
 */
class DAAppRibbonArea : public DARibbonAreaInterface
{
    Q_OBJECT
    friend class AppMainWindow;
    friend class DAAppUI;
    friend class DAAppController;
    friend class DAAppTableActionHandler;

public:
    /**
     * @brief 上下文类型
     */
    enum ContextCategoryType
    {
        ContextCategoryData,      ///< Data相关的上下文
        ContextCategoryWorkflow,  ///< Workflow相关的上下文
        ContextCategoryChart,     ///< Chart相关的上下文
        AllContextCategory        ///< 这个代表所有的上下文
    };
    Q_ENUM(ContextCategoryType)
public:
    explicit DAAppRibbonArea(DAUIInterface* u);
    virtual ~DAAppRibbonArea() override;

    // 发生语言变更时会触发此函数
    virtual void retranslateUi() override;
    // 获取app
    AppMainWindow* app() const;
    // 获取ribbon
    SARibbonBar* ribbonBar() const;
    // 获取主标签
    SARibbonCategory* getRibbonCategoryMain() const;
    // 通过DACommandInterface构建redo/undo的action
    void buildRedoUndo();
    // 显示上下文(会把其他上下文隐藏)
    void showContextCategory(ContextCategoryType type);
    // 隐藏上下文
    void hideContextCategory(ContextCategoryType type);
    // 显示/隐藏「统计」上下文标签页（只由统计窗口 dock 的可见性驱动，不参与上面的互斥切换）
    void setStatsContextCategoryVisible(bool on);

public:
    //===================================================
    // 更新操作（把当前对象状态回灌到 Ribbon 控件）
    //===================================================
    // 更新绘图相关的ribbon
    void updateFigureAboutRibbon(DAFigureWidget* fig);
    void updateChartAboutRibbon(DAChartWidget* chart);
    void updateChartGridAboutRibbon(DAChartWidget* chart);
    void updateChartZoomPanAboutRibbon(DAChartWidget* chart);
    void updateChartPickerAboutRibbon(DAChartWidget* chart);
    void updateChartLegendAboutRibbon(DAChartWidget* chart);
    //
    void updateWorkflowAboutRibbon(DAPyWorkFlowOperateWidget* wfo);
    // 重置文字
    void resetText();

public:
    // Python相关
    // 设置DataFrame的类型，【Context】 - 【dataframe】 DataFrame -> Type -> Type,此函数的调用忽略combox的currentindexchanged信号
    void setDataframeOperateCurrentDType(const DAPyDType& d);

private:
    /**
     * @brief 下拉菜单集合
     *
     * 这些菜单由 buildMenu() 创建，随后通过 action 的 setMenu() 挂到对应按钮上；
     * 打包成一个结构是为了让页面构建函数按需整体注入（表格页要用 6 个菜单），
     * 避免一长串 QMenu* 参数。
     */
    struct Menus
    {
        QMenu* insertRow { nullptr };                   ///< 插入行（挂在 actionInsertRow 下）
        QMenu* insertColumn { nullptr };                ///< 插入列（挂在 actionInsertColumnRight 下）
        QMenu* fill { nullptr };                        ///< 填充（挂在 actionFill 下）
        QMenu* note { nullptr };                        ///< 批注（挂在 actionNote 下）
        QMenu* freezeColumns { nullptr };               ///< 冻结列（挂在 actionFreezeColumns 下）
        QMenu* sort { nullptr };                        ///< 排序（挂在 actionSort 下）
        QMenu* exportData { nullptr };                  ///< 数据导出格式
        QMenu* exportWorkflowSceneToImage { nullptr };  ///< 工作流场景导出为图片
        QMenu* viewLineMarkers { nullptr };             ///< 工作流视图标记线
        QMenu* chartPickSetting { nullptr };            ///< 图表拾取设置
        QMenu* chartLegendPosition { nullptr };         ///< 图表图例位置（挂在 actionChartEnableLegend 下）
    };

    //===================================================
    // 构建
    //
    // 依赖注入约定：父容器一律入参，不读本类上下文成员。
    //===================================================
    // 构建所有的下拉菜单（先于 buildRibbon 调用，页面构建会引用这些菜单）
    void buildMenu();
    // 构建界面总入口（装配顺序见实现文件中的注释）
    void buildRibbon();
    // 构建主页标签（文件、剪贴板、创建、配置）
    void buildRibbonMainCategory(SARibbonBar* ribbon);
    // 构建数据标签（数据集、导出）
    void buildRibbonDataCategory(SARibbonBar* ribbon);
    // 构建视图标签（显示、布局）
    void buildRibbonViewCategory(SARibbonBar* ribbon);
    // 构建绘图标签（添加绘图、统计绘图）
    void buildRibbonFigureCategory(SARibbonBar* ribbon);
    // 构建快速响应栏（当前为空实现，保留为扩展点）
    void buildRibbonQuickAccessBar();
    //===================================================
    // Context - 表格（objectName: da-ribbon-category-dataframe*）
    //===================================================
    // 构建表格上下文标签，并把三张页面分派给下面的函数
    void buildContextCategoryTable(SARibbonBar* ribbon);
    // 构建「表格操作」页：行与列、列、类型与格式、编辑、视图
    void buildContextCategoryTableOperate(SARibbonContextCategory* contextTable, const Menus& menus);
    // 构建「数据操作」页：排序与筛选、计算列、数据变换、数据校验（插件加载后会向本页追加面板）
    void buildContextCategoryTableDataOperate(SARibbonContextCategory* contextTable, const Menus& menus);
    // 构建「表格样式」页：字体、底色、条件格式、清除
    void buildContextCategoryTableStyle(SARibbonContextCategory* contextTable);
    //===================================================
    // Context - 工作流（objectName: da-ribbon-category-workflow*）
    //===================================================
    // 构建工作流上下文标签
    // @note 工作流三页必须按 视图 -> 编辑 -> 运行 的顺序构建（与原有标签顺序一致）
    void buildContextCategoryWorkflow(SARibbonBar* ribbon);
    // 构建「工作流视图」页：视图、导出（wfo 提供缩放/标记线等内建 action）
    void buildContextCategoryWorkflowView(SARibbonContextCategory* contextWorkflow, DAPyWorkFlowOperateWidget* wfo);
    // 构建「工作流编辑」页：剪贴板、图元、文本、背景、分组
    void buildContextCategoryWorkflowEdit(SARibbonContextCategory* contextWorkflow, DAPyWorkFlowOperateWidget* wfo);
    // 构建「工作流运行」页：运行、停止
    void buildContextCategoryWorkflowRun(SARibbonContextCategory* contextWorkflow);
    //===================================================
    // Context - 绘图（objectName: da-ribbon-category-chart*）
    //===================================================
    // 构建绘图上下文标签（绘图样式、图表编辑两页）
    void buildContextCategoryChart(SARibbonBar* ribbon);
    // 构建「绘图样式」页：图表设置、绘图主题
    void buildContextCategoryChartStyle(SARibbonContextCategory* contextChart, const Menus& menus);
    // 构建「图表编辑」页：绘图窗口设置、选区工具、辅助工具
    void buildContextCategoryChartEdit(SARibbonContextCategory* contextChart);
    //===================================================
    // Context - 统计
    //===================================================
    // 构建「统计」上下文标签页（统计窗口 dock 显示时才出现）
    // @param stats 统计窗口，动作由它持有，本函数只做布局（可为 nullptr，此时不构建）
    void buildContextCategoryStats(SARibbonBar* ribbon, DADataStatsWidget* stats);
    //===================================================
    // 标签页 - AI 智能体
    //===================================================
    // 构建 AI分析 标签页（agent 提示词库 gallery + 管理/执行）
    void buildRibbonAgentCategory(SARibbonBar* ribbon);
    //===================================================
    // 其他
    //===================================================
    // 构建应用面板（应用按钮弹出的 Backstage 面板）
    void buildApplicationWidget();
    // 构建右侧按钮区
    void buildRightButtonBar(SARibbonBar* ribbon);
private Q_SLOTS:
    void onActionAgentManage();
    void onActionRunAgent();
    void onAgentGalleryTriggered(QAction* act);
    void onActionAgentNewSession();
    void onActionAgentSessionManager();

private:
    void populateAgentGallery();

    DAAPPRIBBONAREA_COMMON_SETTING_H(WorkFlowEdit)

Q_SIGNALS:
    /**
       @brief 画笔选中了

       这是一个通用的画笔信号
       @param p
     */
    void selectedWorkflowItemPen(const QPen& p);

    /**
       @brief 画刷选中了

        这是一个通用的画刷选中信号
       @param b
     */
    void selectedWorkflowItemBrush(const QBrush& b);

    /**
       @brief 字体选中了

        这是一个通用的字体选中信号
       @param f
     */
    void selectedWorkflowItemFont(const QFont& f);

    /**
       @brief 字体颜色选中了

        这是一个通用的字体颜色选中信号
       @param f
     */
    void selectedWorkflowItemFontColor(const QColor& c);

protected:
    // 设置dock区，有些pannel的action是依赖dock界面的，统一在这里设置
    void setDockingArea(DAAppDockingArea* dock);

private:
    ///////////////////////////////////////////
    /// 依赖（由构造函数或 setDockingArea 注入）
    ///////////////////////////////////////////
    DAAppActions* mActions { nullptr };       ///< 所有的action管理
    DAAppDockingArea* mDockArea { nullptr };  ///< 注意这个变量不能在构造函数中调用
    AppMainWindow* mApp { nullptr };          ///< 宿主主窗口
    DAAppCommand* mAppCmd { nullptr };        ///< 命令/撤销组入口

    //----------------------------------------------------
    // 主页标签（da-ribbon-category-main）
    //----------------------------------------------------
    SARibbonCategory* mCategoryMain { nullptr };      ///< 主页标签
    SARibbonPanel* mPannelMainFileOpt { nullptr };    ///< 文件操作（打开/保存/另存/追加工程/最近文件）
    SARibbonPanel* mPannelMainClipboard { nullptr };  ///< 剪贴板（撤销/重做/剪切/复制/粘贴/删除/全选，按焦点路由）
    SARibbonPanel* mPannelMainCreate { nullptr };     ///< 创建（添加数据/添加绘图/新建工作流）
    SARibbonPanel* mPannelSetting { nullptr };        ///< 设定

    //----------------------------------------------------
    // 数据标签（da-ribbon-category-data）
    //----------------------------------------------------
    SARibbonCategory* mCategoryData { nullptr };     ///< 数据标签
    SARibbonPanel* mPannelDataOperate { nullptr };   ///< 数据操作
    SARibbonPanel* mPannelDataExport { nullptr };    ///< 数据导出

    //----------------------------------------------------
    // 视图标签（da-ribbon-category-view）
    //----------------------------------------------------
    SARibbonCategory* mCategoryView { nullptr };     ///< 视图标签
    SARibbonPanel* mPannelViewMainView { nullptr };  ///< 主要视图操作
    SARibbonPanel* mPannelViewLayout { nullptr };    ///< 布局管理
    // 外观面板（Ribbon 主题）已删除：主题切换由 设置→常规 页提供

    //----------------------------------------------------
    // 绘图标签（da-ribbon-category-figure）
    //----------------------------------------------------
    SARibbonCategory* mCategoryFigure { nullptr };  ///< 绘图标签
    SARibbonPanel* mPannelChartAdd { nullptr };     ///< 添加绘图
    SARibbonPanel* mPannelStatsPlot { nullptr };    ///< 统计绘图面板

    //----------------------------------------------------
    // Context - 表格（DAAppRibbonAreaTable.cpp）
    //
    // 插件（DataAnalysis）在加载时按 objectName 找到 mCategoryDataframeDataOperate
    // 并追加清洗/过滤/统计面板，修改本段的 objectName 会破坏插件契约。
    //----------------------------------------------------
    SARibbonContextCategory* mContextDataFrame { nullptr };               ///< 对应dataframe的上下文
    SARibbonCategory* mCategoryDataframeTableOperate { nullptr };         ///< 表格操作category
    SARibbonPanel* mPannelDataframeOperateAxes { nullptr };               ///< 行与列（插入/删除行与列、列名管理、列转索引）
    SARibbonPanel* mPannelDataframeOperateColumn { nullptr };             ///< 列级操作（五件套，与表头右键共用）
    SARibbonPanel* mPannelDataframeOperateDType { nullptr };              ///< 类型与格式（列类型 + 显示格式）
    SARibbonLineWidgetContainer* mComboxColumnTypesContainer { nullptr };  ///< 列类型选择器的container
    DAPyDTypeComboBox* mComboxColumnTypes { nullptr };                     ///< 列类型选择器（Controller 连接其信号）
    SARibbonButtonGroupWidget* mCastActionsButtonGroup { nullptr };        ///< 管理强制转换的action的工具栏
    // 显示格式（已并入「类型与格式」面板，不再单独建面板）
    SARibbonLineWidgetContainer* mComboxDisplayFormatContainer { nullptr };  ///< 显示格式选择器container
    DATableDisplayFormatComboBox* mComboxDisplayFormat { nullptr };          ///< 列显示格式选择器（Controller 连接其信号）
    // 编辑 panel（002 T1.4：查找/替换；005 填充也进此面板；清空单元格也在此）
    SARibbonPanel* mPannelDataframeOperateEdit { nullptr };  ///< 编辑面板
    SARibbonPanel* mPannelDataframeOperateView { nullptr };  ///< 视图面板（006 T1.8：冻结/隐藏/显示所有列/列视图）
    // 数据操作 category（核心数据加工入口；插件的数据清洗/统计面板在加载时追加到本页）
    SARibbonCategory* mCategoryDataframeDataOperate { nullptr };         ///< 数据操作category
    SARibbonPanel* mPannelDataframeDataOperateSortFilter { nullptr };    ///< 排序与筛选面板（插件的数据过滤动作也挂此面板）
    SARibbonPanel* mPannelDataframeDataOperateComputed { nullptr };      ///< 计算列面板（计算列/全部重算）
    SARibbonPanel* mPannelDataframeDataOperateTransform { nullptr };     ///< 数据变换面板（分列/合列/分组/转置/melt）
    SARibbonPanel* mPannelDataframeDataOperateValidation { nullptr };    ///< 数据校验面板（校验规则/执行校验）
    // 表格样式 category
    SARibbonCategory* mCategoryDataframeStyle { nullptr };        ///< dataframe表格样式category
    SARibbonPanel* mPannelDataframeStyleFill { nullptr };         ///< 底色面板
    SARibbonPanel* mPannelDataframeStyleFont { nullptr };         ///< 字体面板
    SARibbonPanel* mPannelDataframeStyleAlign { nullptr };        ///< 对齐面板（A1）
    SARibbonPanel* mPannelDataframeStyleBorder { nullptr };       ///< 边框面板（A2）
    SARibbonPanel* mPannelDataframeStyleClear { nullptr };        ///< 清除面板
    SARibbonPanel* mPannelDataframeStyleConditional { nullptr };  ///< 条件格式面板（003 T1.5）
    SARibbonColorToolButton* mBtnTableFillColor { nullptr };      ///< 表格底色按钮（Controller 连接其信号）
    DAFontEditPannelWidget* mWidgetTableFont { nullptr };         ///< 表格字体编辑面板（Controller 连接其信号）
    DATextAlignmentEditWidget* mWidgetTableAlign { nullptr };     ///< 表格对齐按钮组（Controller 连接其信号，A1）

    //----------------------------------------------------
    // Context - 工作流（DAAppRibbonAreaWorkflow.cpp）
    //----------------------------------------------------
    SARibbonContextCategory* mContextWorkflow { nullptr };  ///< 对应workflow的上下文
    // 工作流-视图页
    SARibbonCategory* mCategoryWorkflowGraphicsView { nullptr };  ///< 工作流视图
    SARibbonPanel* mPannelWorkflowView { nullptr };               ///< 图元视图pannel
    SARibbonPanel* mPannelWorkflowExport { nullptr };             ///< 视图导出
    // 工作流-编辑页
    DAFontEditPannelWidget* mWorkflowFontEditPannel { nullptr };          ///< 工作流的字体编辑器
    DAShapeEditPannelWidget* mWorkflowShapeEditPannelWidget { nullptr };  ///< 图框编辑
    SARibbonCategory* mCategoryWorkflowGraphicsEdit { nullptr };          ///< 工作流绘图编辑
    SARibbonPanel* mPannelClipBoard { nullptr };                          ///< 剪切板
    SARibbonPanel* mPannelWorkflowItem { nullptr };                       ///< 图元编辑
    SARibbonPanel* mPannelWorkflowBackground { nullptr };                 ///< 背景编辑
    SARibbonPanel* mPannelWorkflowText { nullptr };                       ///< 文本编辑
    SARibbonPanel* mPannelWorkflowGroup { nullptr };                      ///< 图元分组相关pannel
    // 工作流-运行页
    SARibbonCategory* mCategoryWorkflowRun { nullptr };  ///< 工作流运行
    SARibbonPanel* mPannelWorkflowRun { nullptr };       ///< 运行视图pannel

    //----------------------------------------------------
    // Context - 绘图（DAAppRibbonAreaChart.cpp）
    //----------------------------------------------------
    SARibbonContextCategory* mContextChart { nullptr };                      ///< 对应Chart的上下文
    SARibbonCategory* mCategoryChartStyle { nullptr };                       ///< Chart样式标签
    SARibbonPanel* mPannelFigureSettingForContext { nullptr };               ///< 绘图的设置（位于Chart Edit标签）
    SARibbonPanel* mPannelChartSetting { nullptr };                          ///< 图表的设置
    SARibbonButtonGroupWidget* mChartGridDirActionsButtonGroup { nullptr };  ///< grid的方向
    SARibbonButtonGroupWidget* mChartGridMinActionsButtonGroup { nullptr };  ///< grid的min设置
    SARibbonPanel* mPanelFigureTheme { nullptr };                            ///< 绘图样式
    SARibbonGallery* mFigureThemeGallery { nullptr };                        ///< 绘图样式画廊
    SARibbonCategory* mCategoryChartEdit { nullptr };                        ///< Chart编辑标签
    SARibbonPanel* mPannelChartSelectTool { nullptr };                       ///< 图表选区
    SARibbonPanel* mPannelChartAssistTool { nullptr };                       ///< 辅助工具

    //----------------------------------------------------
    // Context - 统计（DAAppRibbonAreaStats.cpp，统计窗口 dock 可见时出现）
    //----------------------------------------------------
    SARibbonContextCategory* mContextStats { nullptr };  ///< 对应统计窗口的上下文
    SARibbonCategory* mCategoryStats { nullptr };        ///< 统计操作标签
    SARibbonPanel* mPannelStatsParams { nullptr };       ///< 统计参数面板
    SARibbonPanel* mPannelStatsTests { nullptr };        ///< 检验面板
    SARibbonPanel* mPannelContextStatsPlot { nullptr };  ///< 绘图面板

    //----------------------------------------------------
    // 标签页 - AI 智能体（DAAppRibbonAreaAgent.cpp）
    //----------------------------------------------------
    SARibbonCategory* mCategoryAgent { nullptr };          ///< AI分析标签
    SARibbonPanel* mPanelAgent { nullptr };                ///< AI分析面板
    SARibbonGallery* mAgentGallery { nullptr };            ///< agent gallery
    SARibbonGalleryGroup* mAgentGalleryGroup { nullptr };  ///< agent gallery 分组
    QList< QAction* > mAgentActions;                       ///< gallery 临时 action
    QAction* mActionAgentManage { nullptr };               ///< agent 管理
    QAction* mActionRunAgent { nullptr };                  ///< 执行 agent
    QAction* mActionAgentNewSession { nullptr };           ///< 新建会话（session-tabs）
    QAction* mActionAgentSessionManager { nullptr };       ///< 会话管理（session-tabs）
    QString mSelectedAgentTitle;                           ///< 当前选中的 agent 标题

    //----------------------------------------------------
    // 其他：下拉菜单与应用面板
    //----------------------------------------------------
    Menus mMenus;                                            ///< 下拉菜单集合（由 buildMenu 创建）
    DAAppApplicationWidget* mApplicationWidget { nullptr };  ///< 应用按钮弹出的 Backstage 面板
};
}  // namespace DA
#endif  // DAAPPRIBBONAREA_H
