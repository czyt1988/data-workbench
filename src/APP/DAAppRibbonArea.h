#ifndef DAAPPRIBBONAREA_H
#define DAAPPRIBBONAREA_H
#include <QObject>
#include <QAction>
#include <QUndoStack>
#include <QSpinBox>
#include <QWidgetAction>
#include "DARibbonAreaInterface.h"
#include "numpy/DAPyDType.h"
#include "DADataManageWidget.h"
#include "DAWorkFlowGraphicsScene.h"
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
class SARibbonPannel;
class SARibbonContextCategory;
class SARibbonLineWidgetContainer;
class SARibbonButtonGroupWidget;
class SARibbonCtrlContainer;
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
//
class DADataOperatePageWidget;
//窗口
class DAFontEditPannelWidget;
class DAShapeEditPannelWidget;
class DAColorPickerButton;
class DAFigureWidget;
class DAChartOperateWidget;
class DAChartWidget;
class DADataOperateWidget;
class DAWorkFlowOperateWidget;
/**
 * @brief App的Ribbon区域接口，负责ribbon层的管理和调度
 *
 * @todo 后续抽象出DAAppRibbonAreaInterface，DAAppRibbonArea继承DAAppRibbonAreaInterface，并把可暴露的方法暴露出来，实现插件化
 *
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
 *  工作流编辑category：da-ribbon-category-workflow.edit
 *     条目pannel：da-pannel-context.workflow.item
 *     文本pannel：da-pannel-context.workflow.text
 *     背景pannel：da-pannel-context.workflow.background
 *     视图pannel：da-pannel-context.workflow.view
 *     运行pannel：da-pannel-context.workflow.run
 * DataFrame上下文标签:da-ribbon-contextcategory-dataframe
 * Chart上下文标签：da-ribbon-contextcategory-chart
 *  Chart编辑category：da-ribbon-category-chart.edit
 *     绘图窗口设置pannel:da-pannel-context-chartedit.fig_setting
 *     图表设置pannel:da-pannel-context-chartedit.chart_setting
 */
class DAAppRibbonArea : public DARibbonAreaInterface
{
    friend class AppMainWindow;
    Q_OBJECT
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
    DAAppRibbonArea(DAUIInterface* u);
    ~DAAppRibbonArea();

    //发生语言变更时会触发此函数
    virtual void retranslateUi() override;
    //获取app
    AppMainWindow* app() const;
    //获取ribbon
    SARibbonBar* ribbonBar() const;
    //获取主标签
    SARibbonCategory* getRibbonCategoryMain() const;
    //通过DACommandInterface构建redo/undo的action
    void buildRedoUndo();
    //更新ActionLockBackgroundPixmap的check statue
    void updateActionLockBackgroundPixmapCheckStatue(bool c);
    //显示上下文(会把其他上下文隐藏)
    void showContextCategory(ContextCategoryType type);
    //隐藏上下文
    void hideContextCategory(ContextCategoryType type);

public:
    //===================================================
    // 更新操作
    //===================================================
    //更新绘图相关的ribbon
    void updateFigureAboutRibbon(DAFigureWidget* fig);
    void updateChartAboutRibbon(DAChartWidget* chart);
    void updateChartGridAboutRibbon(DAChartWidget* chart);
    void updateChartZoomPanAboutRibbon(DAChartWidget* chart);
    void updateChartPickerAboutRibbon(DAChartWidget* chart);
    void updateChartLegendAboutRibbon(DAChartWidget* chart);
    //更新workflow编辑
    void updateWorkflowItemAboutRibbon(QGraphicsItem* lastSelectItem);

public:
    //设置DataFrame的类型，【Context】 - 【dataframe】 DataFrame -> Type -> Type,此函数的调用忽略combox的currentindexchanged信号
    void setDataframeOperateCurrentDType(const DAPyDType& d);

private:
    //构建所有的action
    void buildMenu();
    //构建界面
    void buildRibbon();
    //构建主页
    void buildRibbonMainCategory();
    //构建数据标签
    void buildRibbonDataCategory();
    //构建主页
    void buildRibbonViewCategory();
    //构建编辑标签，编辑标签是通用的编辑功能，例如添加文字，添加形状等
    void buildRibbonEditCategory();
    //构建绘图标签
    void buildRibbonFigureCategory();
    //构建快速响应栏
    void buildRibbonQuickAccessBar();
    //构建DataFrame上下文标签
    void buildContextCategoryDataFrame();
    //构建Workflow上下文标签
    void buildContextCategoryWorkflowEdit();
    //构建chart上下文
    void buildContextCategoryChart();

public:
    void resetText();

public:
    ///////////////////////////////////////////
    /// ribbon
    ///////////////////////////////////////////
    // pass

    // widgets
    DAColorPickerButton* m_textColorButton;
    QAction* m_textColorButtonActoin;
    QFontComboBox* m_textFontComboBox;
    QAction* m_textFontComboBoxActoin;
    QComboBox* m_textSizeComboBox;
    QAction* m_textSizeComboBoxActoin;

    DAFontEditPannelWidget* m_workflowFontEditPannel;          ///< 工作流的字体编辑器
    DAShapeEditPannelWidget* m_workflowShapeEditPannelWidget;  ///< 图框编辑

    DAAppActions* m_actions;       ///< 所有的action管理
    DAAppDockingArea* m_dockArea;  ///< 注意这个变量不能在构造函数中调用
    AppMainWindow* m_app;
    DAAppCommand* m_appCmd;  ///< cmd
    // main
    SARibbonCategory* m_categoryMain;         ///< 主页标签
    SARibbonPannel* m_pannelMainFileOpt;      ///< 文件操作
    SARibbonPannel* m_pannelMainDataOpt;      ///< 数据操作
    SARibbonPannel* m_pannelMainChartOpt;     ///< 数据操作
    SARibbonPannel* m_pannelMainWorkflowOpt;  ///< 工作流在main的pannel
    SARibbonPannel* m_pannelSetting;          ///< 设定
    // data
    SARibbonCategory* m_categoryData;           ///< 数据标签
    SARibbonPannel* m_pannelDataOperate;        ///< 数据操作
    SARibbonPannel* m_pannelDataFolderOperate;  ///< 数据文件夹操作
    // view
    SARibbonCategory* m_categoryView;      ///< 视图标签
    SARibbonPannel* m_pannelViewMainView;  ///< 主要视图操作
    // figure
    SARibbonCategory* m_categoryFigure;     ///< 绘图标签
    SARibbonPannel* m_pannelFigureSetting;  ///< 绘图的设置
    SARibbonPannel* m_pannelChartAdd;       ///< 添加绘图
    // Context - dataframe
    SARibbonContextCategory* m_contextDataFrame;                ///< 对应dataframe的上下文
    SARibbonCategory* m_categoryDataframeOperate;               ///< dataframe对应的category
    SARibbonPannel* m_pannelDataframeOperateAxes;               ///< 数据信息的编辑
    SARibbonPannel* m_pannelDataframeOperateDType;              ///< 数据类型的编辑
    SARibbonLineWidgetContainer* m_comboxColumnTypesContainer;  ///<列类型选择器的container
    SARibbonButtonGroupWidget* m_castActionsButtonGroup;        ///< 管理强制转换的action的工具栏
    DAPyDTypeComboBox* m_comboxColumnTypes;                     ///< 列类型选择器
    SARibbonPannel* m_pannelDataframeOperateStatistic;          ///< 统计相关操作
    // Context - workflow

    SARibbonCategory* m_categoryEdit;      ///< 编辑标签
    SARibbonPannel* m_pannelEditWorkflow;  ///< 主要编辑操作

    SARibbonContextCategory* m_contextWorkflow;        ///< 对应workflow的上下文
    SARibbonCategory* m_categoryWorkflowGraphicsEdit;  ///< 工作流绘图编辑
    SARibbonPannel* m_pannelWorkflowItem;              ///< 图元编辑
    SARibbonPannel* m_pannelWorkflowBackground;        ///< 背景编辑
    SARibbonPannel* m_pannelWorkflowText;              ///< 文本编辑
    SARibbonPannel* m_pannelWorkflowView;              ///< 图元视图pannel
    SARibbonPannel* m_pannelWorkflowRun;               ///< 运行视图pannel
    // Context - chart
    SARibbonContextCategory* m_contextChart;                      ///< 对应Chart的上下文
    SARibbonCategory* m_categoryChartEdit;                        ///< Chart编辑
    SARibbonPannel* m_pannelFigureSettingForContext;              ///< 绘图的设置
    SARibbonPannel* m_pannelChartSetting;                         ///< 图表的设置
    SARibbonButtonGroupWidget* m_chartGridDirActionsButtonGroup;  ///< grid的方向
    SARibbonButtonGroupWidget* m_chartGridMinActionsButtonGroup;  ///< grid的min设置
    QMenu* m_menuChartLegendProperty;                             ///< legend的属性设置
    QAction* m_actionOfMenuChartLegendAlignmentSection;           ///< m_menuChartLegendProperty对应的Section
    QSpinBox* m_spinboxChartLegendMaxColumns;                     ///< legend setMaxColumns
    SARibbonCtrlContainer* m_ctrlContainerChartLegendMaxColumns;
    QSpinBox* m_spinboxChartLegendMargin;  ///< legend setMargin
    SARibbonCtrlContainer* m_ctrlContainerChartLegendMargin;
    QSpinBox* m_spinboxChartLegendSpacing;  ///< legend setSpacing
    SARibbonCtrlContainer* m_ctrlContainerChartLegendSpacing;
    QSpinBox* m_spinboxChartLegendItemMargin;  ///< legend setItemMargin
    SARibbonCtrlContainer* m_ctrlContainerChartLegendItemMargin;
    QSpinBox* m_spinboxChartLegendItemSpacing;  ///< legend setItemSpacing
    SARibbonCtrlContainer* m_ctrlContainerChartLegendItemSpacing;
    QDoubleSpinBox* m_spinboxChartLegendBorderRadius;  ///< legend setBorderRadius
    SARibbonCtrlContainer* m_ctrlContainerChartLegendBorderRadius;
    //
    QStringList m_fileReadFilters;
    QMenu* m_menuInsertRow;     ///< 针对insertrow的action menu
    QMenu* m_menuInsertColumn;  ///< 这对insertcol的action menu
};
}  // namespace DA
#endif  // DAAPPRIBBONAREA_H
