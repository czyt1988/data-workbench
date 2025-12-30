#ifndef DAAPPRIBBONAREA_H
#define DAAPPRIBBONAREA_H
#include <QObject>
#include <QAction>
#include <QUndoStack>
#include <QSpinBox>
#include <QWidgetAction>
#include "DARibbonAreaInterface.h"
#include "DADataManageWidget.h"
#include "DAWorkFlowGraphicsScene.h"
#if DA_ENABLE_PYTHON
// Py
#include "numpy/DAPyDType.h"
#endif

#define DAAPPRIBBONAREA_COMMON_SETTING_H(MiddleName)                                                                   \
public:                                                                                                                \
    QPen get##MiddleName##Pen() const;                                                                                 \
    QBrush get##MiddleName##Brush() const;                                                                             \
    QFont get##MiddleName##Font() const;                                                                               \
    QColor get##MiddleName##FontColor() const;                                                                         \
public slots:                                                                                                          \
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
namespace DA
{
class AppMainWindow;
class DAAppDockingArea;
class DAAppActions;
class DAAppCommand;
class DAPyDTypeComboBox;
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
class DAWorkFlowEditWidget;
class DAWorkFlowOperateWidget;
/**
 * @brief App的Ribbon区域接口，负责ribbon层的管理和调度
 *
 * @todo 后续抽象出DAAppRibbonAreaInterface，DAAppRibbonArea继承DAAppRibbonAreaInterface，并把可暴露的方法暴露出来，实现插件化
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
 *  Chart编辑category：da-ribbon-category-chart.edit
 *     绘图窗口设置pannel:da-pannel-context-chartedit.fig_setting
 *     图表设置pannel:da-pannel-context-chartedit.chart_setting
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
    ~DAAppRibbonArea();

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
    void updateWorkflowAboutRibbon(DAWorkFlowOperateWidget* wfo);
    // 重置文字
    void resetText();

public:
    // Python相关
#if DA_ENABLE_PYTHON
    // 设置DataFrame的类型，【Context】 - 【dataframe】 DataFrame -> Type -> Type,此函数的调用忽略combox的currentindexchanged信号
    void setDataframeOperateCurrentDType(const DAPyDType& d);
#endif
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
    void buildContextCategoryChart();
    // 构建ApplicationMenu
    void buildApplicationMenu();
    // 构建右工具栏
    void buildRightButtonBar();

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
    /**
     * @brief 设置dock区，有些pannel的action是依赖dock界面的，统一在这里设置
     * @param dock
     */
    void setDockingArea(DAAppDockingArea* dock);

private:
    ///////////////////////////////////////////
    /// ribbon
    ///////////////////////////////////////////
    // pass

    DAAppActions* m_actions;       ///< 所有的action管理
    DAAppDockingArea* m_dockArea;  ///< 注意这个变量不能在构造函数中调用
    AppMainWindow* m_app;
    DAAppCommand* m_appCmd;                  ///< cmd
                                             //----------------------------------------------------
                                             // main
                                             //----------------------------------------------------
    SARibbonCategory* m_categoryMain;        ///< 主页标签
    SARibbonPanel* m_pannelMainFileOpt;      ///< 文件操作
    SARibbonPanel* m_pannelMainDataOpt;      ///< 数据操作
    SARibbonPanel* m_pannelMainChartOpt;     ///< 数据操作
    SARibbonPanel* m_pannelMainWorkflowOpt;  ///< 工作流在main的pannel
    SARibbonPanel* m_pannelSetting;          ///< 设定
                                             //----------------------------------------------------
                                             // data
                                             //----------------------------------------------------
    SARibbonCategory* m_categoryData;        ///< 数据标签
    SARibbonPanel* m_pannelDataOperate;      ///< 数据操作
                                             //----------------------------------------------------
                                             // view
                                             //----------------------------------------------------
    SARibbonCategory* m_categoryView;        ///< 视图标签
    SARibbonPanel* m_pannelViewMainView;     ///< 主要视图操作
    //----------------------------------------------------
    // edit
    //----------------------------------------------------
    //--widget
    DAFontEditPannelWidget* m_editFontEditPannel;          ///< 工作流的字体编辑器
    DAShapeEditPannelWidget* m_editShapeEditPannelWidget;  ///< 图框编辑
    SARibbonCategory* m_categoryEdit;                      ///< 编辑标签
    SARibbonPanel* m_pannelEditWorkflow;                   ///< 主要编辑操作

    //----------------------------------------------------
    // figure
    //----------------------------------------------------
    SARibbonCategory* m_categoryFigure;    ///< 绘图标签
    SARibbonPanel* m_pannelFigureSetting;  ///< 绘图的设置
    SARibbonPanel* m_pannelChartAdd;       ///< 添加绘图

    //----------------------------------------------------
    // Context - dataframe
    //----------------------------------------------------
    SARibbonContextCategory* m_contextDataFrame;   ///< 对应dataframe的上下文
    SARibbonCategory* m_categoryDataframeOperate;  ///< dataframe对应的category
    SARibbonPanel* m_pannelDataframeOperateAxes;   ///< 数据信息的编辑
    SARibbonPanel* m_pannelDataframeOperateDType;  ///< 数据类型的编辑
#if DA_ENABLE_PYTHON
    SARibbonLineWidgetContainer* m_comboxColumnTypesContainer;  ///< 列类型选择器的container
    DAPyDTypeComboBox* m_comboxColumnTypes;                     ///< 列类型选择器
#endif
    SARibbonButtonGroupWidget* m_castActionsButtonGroup;  ///< 管理强制转换的action的工具栏
    //----------------------------------------------------
    // Context - workflow
    //----------------------------------------------------
    SARibbonContextCategory* m_contextWorkflow;  ///< 对应workflow的上下文
    //----------------------------------------------------
    // Context - workflow-view
    //----------------------------------------------------
    SARibbonCategory* m_categoryWorkflowGraphicsView;  ///< 工作流视图
    SARibbonPanel* m_pannelWorkflowView;               ///< 图元视图pannel

    SARibbonPanel* m_pannelWorkflowExport;  ///< 视图导出
    //----------------------------------------------------
    // Context - workflow-edit
    //----------------------------------------------------
    DAFontEditPannelWidget* m_workflowFontEditPannel;          ///< 工作流的字体编辑器
    DAShapeEditPannelWidget* m_workflowShapeEditPannelWidget;  ///< 图框编辑

    SARibbonCategory* m_categoryWorkflowGraphicsEdit;  ///< 工作流绘图编辑
    SARibbonPanel* m_pannelClipBoard;                  ///< 剪切板
    SARibbonPanel* m_pannelWorkflowItem;               ///< 图元编辑
    SARibbonPanel* m_pannelWorkflowBackground;         ///< 背景编辑
    SARibbonPanel* m_pannelWorkflowText;               ///< 文本编辑
    SARibbonPanel* m_pannelWorkflowGroup;              ///< 图元分组相关pannel

    //----------------------------------------------------
    // Context - workflow-run
    //----------------------------------------------------
    SARibbonCategory* m_categoryWorkflowRun;  ///< 工作流运行
    SARibbonPanel* m_pannelWorkflowRun;       ///< 运行视图pannel
    //----------------------------------------------------
    // Context - chart
    //----------------------------------------------------
    SARibbonContextCategory* m_contextChart;                      ///< 对应Chart的上下文
    SARibbonCategory* m_categoryChartEdit;                        ///< Chart编辑
    SARibbonPanel* m_pannelFigureSettingForContext;               ///< 绘图的设置
    SARibbonPanel* m_pannelChartSetting;                          ///< 图表的设置
    SARibbonButtonGroupWidget* m_chartGridDirActionsButtonGroup;  ///< grid的方向
    SARibbonButtonGroupWidget* m_chartGridMinActionsButtonGroup;  ///< grid的min设置
    SARibbonPanel* m_panelFigureTheme;                            ///< 绘图样式
    SARibbonGallery* m_figureThemeGallery;                        ///< 绘图样式
    // ApplicationMenu
    DAAppRibbonApplicationMenu* mApplicationMenu;        ///< ribbon-app menu
                                                         // 菜单相关
    QMenu* mExportWorkflowSceneToImageMenu { nullptr };  ///< scene导出为图片菜单
    QMenu* m_menuViewLineMarkers { nullptr };            ///< 视图标记线
    QMenu* m_menuInsertRow { nullptr };                  ///< 针对insertrow的action menu
    QMenu* m_menuInsertColumn { nullptr };               ///< 这对insertcol的action menu
    QMenu* m_menuTheme { nullptr };                      ///< 主题菜单
    QMenu* m_menuChartPickSetting { nullptr };           ///< chart的picker设置
};
}  // namespace DA
#endif  // DAAPPRIBBONAREA_H
