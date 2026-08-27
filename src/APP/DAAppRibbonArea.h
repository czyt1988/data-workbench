#ifndef DAAPPRIBBONAREA_H
#define DAAPPRIBBONAREA_H
#include <QObject>
#include <QAction>
#include <QUndoStack>
#include <QSpinBox>
#include <QWidgetAction>
#include "DARibbonAreaInterface.h"
#include "DADataManageWidget.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include "SARibbonColorToolButton.h"
#include "DAFontEditPannelWidget.h"
// Py
#include "numpy/DAPyDType.h"

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

// Qt
class QComboBox;
class QToolBar;
class QMenuBar;
class QFontComboBox;
class QUndoStack;
// Qt-Advanced-Docking-System 前置申明
namespace ads
{
class CDockWidget;
}
// SA Ribbon
class SARibbonBar;
class SARibbonCategory;
class SARibbonPanel;
class SARibbonContextCategory;
class SARibbonLineWidgetContainer;
class SARibbonButtonGroupWidget;
class SARibbonCtrlContainer;
class SARibbonGallery;
class SARibbonGalleryGroup;
namespace DA
{
class AppMainWindow;
class DAAppDockingArea;
class DAAppActions;
class DAAppCommand;
class DAPyDTypeComboBox;
class DATableDisplayFormatComboBox;
class DACommandInterface;
class DAAppDataManager;
class DADataOperateOfDataFrameWidget;
class DAAppProject;
class DAAppRibbonApplicationMenu;
//
class DADataOperatePageWidget;
// 窗口
class DAFontEditPannelWidget;
class DAShapeEditPannelWidget;
class DAColorPickerButton;
class DAFigureWidget;
class DAChartOperateWidget;
class DAChartWidget;
class DADataOperateWidget;
class DAPyWorkFlowEditWidget;
class DAPyWorkFlowOperateWidget;
/**
 * @brief App的Ribbon区域接口，负责ribbon层的管理和调度
 *
 * @todo  后续抽象出DAAppRibbonAreaInterface，DAAppRibbonArea继承DAAppRibbonAreaInterface，并把可暴露的方法暴露出来，实现插件化
 *
 *```
 * 标签的固定objectname
 * 主页标签:da-ribbon-category-main
 *     通用pannel:da-ribbon-pannel-main.common
 *     数据操作pannel：da-pannel-main.data-opt
 *     绘图操作pannel：da-pannel-main.chart-opt
 *     工作流pannel：da-pannel-main.workflow
 *     设置pannel：da-pannel-main.setting
 * 数据标签:da-ribbon-category-data
 *     数据操作pannel:da-pannel-data.data-opt
 *     数据文件夹操作pannel:da-pannel-data.folder-opt
 * 视图标签:da-ribbon-category-view
 *     视图pannel:da-pannel-view.main
 * 编辑标签:da-ribbon-category-edit
 *     工作流编辑pannel:da-pannel-edit.workflow
 * 绘图标签：da-ribbon-category-figure
 *     绘图编辑：da-pannel-figure.fig_setting
 *     添加绘图：da-pannel-figure.chart-add
 * 工作流编辑上下文标签：da-ribbon-contextcategory-workflow
 *  工作流视图category：da-ribbon-category-workflow.view
 *     视图pannel：da-pannel-context.workflow.view
 *     导出pannel：da-pannel-context.workflow.export
 *  工作流编辑category：da-ribbon-category-workflow.edit
 *     条目pannel：da-pannel-context.workflow.item
 *     文本pannel：da-pannel-context.workflow.text
 *     背景pannel：da-pannel-context.workflow.background
 *     分组pannel：da-pannel-context.workflow.group
 *  工作流运行category：da-ribbon-category-workflow.run
 *     运行pannel：da-pannel-context.workflow.run
 *
 * DataFrame上下文标签:da-ribbon-contextcategory-dataframe
 *   DataFrame操作category:da-ribbon-category-dataframe.operate
 *      坐标设置pannel:da-pannel-dataframe.operate.axes
 *      类型设置pannel:da-pannel-dataframe.operate.type
 * Chart上下文标签：da-ribbon-contextcategory-chart
 *  Chart操作category：da-ribbon-category-chart.opt
 *     绘图窗口设置pannel:da-pannel-context-chartedit.fig_setting
 *     图表设置pannel:da-pannel-context-chartedit.chart_setting
 *  Chart编辑category：da-ribbon-category-chart.edit
 * ```
 */
class DAAppRibbonArea : public DARibbonAreaInterface
{
    Q_OBJECT
    friend class AppMainWindow;
    friend class DAAppUI;
    friend class DAAppController;

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
    // 更新ActionLockBackgroundPixmap的check statue
    void updateActionLockBackgroundPixmapCheckStatue(bool c);
    // 显示上下文(会把其他上下文隐藏)
    void showContextCategory(ContextCategoryType type);
    // 隐藏上下文
    void hideContextCategory(ContextCategoryType type);

public:
    //===================================================
    // 更新操作
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
    // 构建所有的action
    void buildMenu();
    // 构建界面
    void buildRibbon();
    // 构建主页
    void buildRibbonMainCategory();
    // 构建数据标签
    void buildRibbonDataCategory();
    // 构建主页
    void buildRibbonViewCategory();
    // 构建编辑标签，编辑标签是通用的编辑功能，例如添加文字，添加形状等
    void buildRibbonEditCategory();
    // 构建绘图标签
    void buildRibbonFigureCategory();
    // 构建快速响应栏
    void buildRibbonQuickAccessBar();
    // 构建DataFrame上下文标签
    void buildContextCategoryDataFrame();
    // 构建Workflow的上下文标签，注意buildContextCategoryWorkflowEdit和buildContextCategoryWorkflowRun必须在此函数之后调用
    void buildContextCategoryWorkflow();
    // 构建Workflow-编辑的上下文标签
    void buildContextCategoryWorkflowEdit_();
    // 构建Workflow-视图的上下文标签
    void buildContextCategoryWorkflowView_();
    // 构建workflow-运行的上下文标签
    void buildContextCategoryWorkflowRun_();
    // 构建chart上下文
    void buildContextCategoryChartEdit();
    // 构建ApplicationMenu
    void buildApplicationMenu();
    // 构建右工具栏
    void buildRightButtonBar();
    // 构建 AI分析 标签页（agent 提示词库 gallery + 管理/执行）
    void buildRibbonAgentCategory();
private Q_SLOTS:
    void onActionAgentManage();
    void onActionRunAgent();
    void onAgentGalleryTriggered(QAction* act);

private:
    void populateAgentGallery();

    DAAPPRIBBONAREA_COMMON_SETTING_H(Edit)
    DAAPPRIBBONAREA_COMMON_SETTING_H(WorkFlowEdit)

Q_SIGNALS:
    /**
       @fn selectedPen
       @brief 画笔选中了

       这是一个通用的画笔信号
       @param p
     */
    void selectedPen(const QPen& p);

    /**
       @fn selectedBrush
       @brief 画刷选中了

        这是一个通用的画刷选中信号
       @param b
     */
    void selectedBrush(const QBrush& b);

    /**
       @fn selectedFont
       @brief 字体选中了

       这是一个通用的字体选中信号
       @param f
     */
    void selectedFont(const QFont& f);

    /**
       @fn selectedFontColor
       @brief 字体颜色选中了

       这是一个通用的字体颜色选中信号
       @param f
     */
    void selectedFontColor(const QColor& c);

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
    /// ribbon
    ///////////////////////////////////////////
    // pass

    DAAppActions* mActions { nullptr };       ///< 所有的action管理
    DAAppDockingArea* mDockArea { nullptr };  ///< 注意这个变量不能在构造函数中调用
    AppMainWindow* mApp { nullptr };
    DAAppCommand* mAppCmd { nullptr };                  ///< cmd
                                                        //----------------------------------------------------
                                                        // main
                                                        //----------------------------------------------------
    SARibbonCategory* mCategoryMain { nullptr };        ///< 主页标签
    SARibbonPanel* mPannelMainFileOpt { nullptr };      ///< 文件操作
    SARibbonPanel* mPannelMainDataOpt { nullptr };      ///< 数据操作
    SARibbonPanel* mPannelMainChartOpt { nullptr };     ///< 数据操作
    SARibbonPanel* mPannelMainWorkflowOpt { nullptr };  ///< 工作流在main的pannel
    SARibbonPanel* mPannelSetting { nullptr };          ///< 设定
                                                        //----------------------------------------------------
                                                        // data
                                                        //----------------------------------------------------
    SARibbonCategory* mCategoryData { nullptr };        ///< 数据标签
    SARibbonPanel* mPannelDataOperate { nullptr };      ///< 数据操作
                                                        //----------------------------------------------------
                                                        // view
                                                        //----------------------------------------------------
    SARibbonCategory* mCategoryView { nullptr };        ///< 视图标签
    SARibbonPanel* mPannelViewMainView { nullptr };     ///< 主要视图操作
    //----------------------------------------------------
    // edit
    //----------------------------------------------------
    //--widget
    DAFontEditPannelWidget* mEditFontEditPannel { nullptr };          ///< 工作流的字体编辑器
    DAShapeEditPannelWidget* mEditShapeEditPannelWidget { nullptr };  ///< 图框编辑
    SARibbonCategory* mCategoryEdit { nullptr };                      ///< 编辑标签
    SARibbonPanel* mPannelEditWorkflow { nullptr };                   ///< 主要编辑操作

    //----------------------------------------------------
    // figure
    //----------------------------------------------------
    SARibbonCategory* mCategoryFigure { nullptr };    ///< 绘图标签
    SARibbonPanel* mPannelFigureSetting { nullptr };  ///< 绘图的设置
    SARibbonPanel* mPannelChartAdd { nullptr };       ///< 添加绘图
    SARibbonPanel* mPannelStatsPlot { nullptr };      ///< 统计绘图面板

    //----------------------------------------------------
    // Context - dataframe
    //----------------------------------------------------
    SARibbonContextCategory* mContextDataFrame { nullptr };                ///< 对应dataframe的上下文
    SARibbonCategory* mCategoryDataframeOperate { nullptr };               ///< dataframe对应的category
    SARibbonPanel* mPannelDataframeOperateAxes { nullptr };                ///< 数据信息的编辑
    SARibbonPanel* mPannelDataframeOperateDType { nullptr };               ///< 数据类型的编辑
    SARibbonLineWidgetContainer* mComboxColumnTypesContainer { nullptr };  ///< 列类型选择器的container
    DAPyDTypeComboBox* mComboxColumnTypes { nullptr };                     ///< 列类型选择器
    SARibbonButtonGroupWidget* mCastActionsButtonGroup { nullptr };        ///< 管理强制转换的action的工具栏
    // 显示格式 panel
    SARibbonPanel* mPannelDataframeOperateFormat { nullptr };                ///< 显示格式面板
    SARibbonLineWidgetContainer* mComboxDisplayFormatContainer { nullptr };  ///< 显示格式选择器container
    DATableDisplayFormatComboBox* mComboxDisplayFormat { nullptr };          ///< 列显示格式选择器
    // 表格样式 category
    SARibbonCategory* mCategoryDataframeStyle { nullptr };    ///< dataframe表格样式category
    SARibbonPanel* mPannelDataframeStyleFill { nullptr };     ///< 底色面板
    SARibbonPanel* mPannelDataframeStyleFont { nullptr };     ///< 字体面板
    SARibbonPanel* mPannelDataframeStyleClear { nullptr };    ///< 清除面板
    SARibbonColorToolButton* mBtnTableFillColor { nullptr };  ///< 表格底色按钮
    DAFontEditPannelWidget* mWidgetTableFont { nullptr };     ///< 表格字体编辑面板
    //----------------------------------------------------
    // Context - workflow
    //----------------------------------------------------
    SARibbonContextCategory* mContextWorkflow { nullptr };  ///< 对应workflow的上下文
    //----------------------------------------------------
    // Context - workflow-view
    //----------------------------------------------------
    SARibbonCategory* mCategoryWorkflowGraphicsView { nullptr };  ///< 工作流视图
    SARibbonPanel* mPannelWorkflowView { nullptr };               ///< 图元视图pannel

    SARibbonPanel* mPannelWorkflowExport { nullptr };  ///< 视图导出
    //----------------------------------------------------
    // Context - workflow-edit
    //----------------------------------------------------
    DAFontEditPannelWidget* mWorkflowFontEditPannel { nullptr };          ///< 工作流的字体编辑器
    DAShapeEditPannelWidget* mWorkflowShapeEditPannelWidget { nullptr };  ///< 图框编辑

    SARibbonCategory* mCategoryWorkflowGraphicsEdit { nullptr };  ///< 工作流绘图编辑
    SARibbonPanel* mPannelClipBoard { nullptr };                  ///< 剪切板
    SARibbonPanel* mPannelWorkflowItem { nullptr };               ///< 图元编辑
    SARibbonPanel* mPannelWorkflowBackground { nullptr };         ///< 背景编辑
    SARibbonPanel* mPannelWorkflowText { nullptr };               ///< 文本编辑
    SARibbonPanel* mPannelWorkflowGroup { nullptr };              ///< 图元分组相关pannel

    //----------------------------------------------------
    // Context - workflow-run
    //----------------------------------------------------
    SARibbonCategory* mCategoryWorkflowRun { nullptr };  ///< 工作流运行
    SARibbonPanel* mPannelWorkflowRun { nullptr };       ///< 运行视图pannel
    //----------------------------------------------------
    // Context - chart
    //----------------------------------------------------
    SARibbonContextCategory* mContextChart { nullptr };                      ///< 对应Chart的上下文
    SARibbonCategory* mCategoryChartStyle { nullptr };                       ///< Chart样式标签
    SARibbonPanel* mPannelFigureSettingForContext { nullptr };               ///< 绘图的设置
    SARibbonPanel* mPannelChartSetting { nullptr };                          ///< 图表的设置
    SARibbonButtonGroupWidget* mChartGridDirActionsButtonGroup { nullptr };  ///< grid的方向
    SARibbonButtonGroupWidget* mChartGridMinActionsButtonGroup { nullptr };  ///< grid的min设置
    SARibbonPanel* mPanelFigureTheme { nullptr };                            ///< 绘图样式
    SARibbonGallery* mFigureThemeGallery { nullptr };                        ///< 绘图样式
    SARibbonCategory* mCategoryChartEdit { nullptr };                        ///< Chart编辑标签
    SARibbonPanel* mPannelChartSelectTool { nullptr };                       ///< 图表选区
    SARibbonPanel* mPannelChartSelectOpt { nullptr };                        ///< 图表选区操作
    SARibbonPanel* mPannelChartAssistTool { nullptr };                       ///< 辅助工具
    //----------------------------------------------------
    // AI分析（agent 提示词库）
    //----------------------------------------------------
    SARibbonCategory* mCategoryAgent { nullptr };          ///< AI分析标签
    SARibbonPanel* mPanelAgent { nullptr };                ///< AI分析面板
    SARibbonGallery* mAgentGallery { nullptr };            ///< agent gallery
    SARibbonGalleryGroup* mAgentGalleryGroup { nullptr };  ///< agent gallery 分组
    QList< QAction* > mAgentActions;                       ///< gallery 临时 action
    QAction* mActionAgentManage { nullptr };               ///< agent 管理
    QAction* mActionRunAgent { nullptr };                  ///< 执行 agent
    QString mSelectedAgentTitle;                           ///< 当前选中的 agent 标题
    //----------------------------------------------------
    //
    //----------------------------------------------------
    // ApplicationMenu
    DAAppRibbonApplicationMenu* mApplicationMenu { nullptr };  ///< ribbon-app menu
                                                               // 菜单相关
    QMenu* mExportWorkflowSceneToImageMenu { nullptr };        ///< scene导出为图片菜单
    QMenu* mMenuViewLineMarkers { nullptr };                   ///< 视图标记线
    QMenu* mMenuInsertRow { nullptr };                         ///< 针对insertrow的action menu
    QMenu* mMenuInsertColumn { nullptr };                      ///< 这对insertcol的action menu
    QMenu* mMenuTheme { nullptr };                             ///< 主题菜单
    QMenu* mMenuChartPickSetting { nullptr };                  ///< chart的picker设置
};
}  // namespace DA
#endif  // DAAPPRIBBONAREA_H
