#ifndef DAAPPRIBBONAREA_H
#define DAAPPRIBBONAREA_H
#include <QObject>
#include <QAction>
#include <QUndoStack>
#include <QSpinBox>
#include <QWidgetAction>
#include "DAAppRibbonAreaInterface.h"
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
class DAProject;
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
class DAAppRibbonArea : public DAAppRibbonAreaInterface
{
    friend class AppMainWindow;
    Q_OBJECT
public:
    /**
     * @brief 针对Operate窗口的最后焦点枚举，用于识别最后用户所在的操作窗口
     */
    enum LastFocusedOpertateWidget
    {
        LastFocusedNoneOptWidget = 0x00,
        LastFocusedOnWorkflowOpt = 0x01,  ///< 最后焦点在工作流操作窗口
        LastFocusedOnDataOpt     = 0x02,  ///<最后焦点在数据操作窗口
        LastFocusedOnChartOpt    = 0x04   ///< 最后焦点在绘图操作窗口
    };
    Q_DECLARE_FLAGS(LastFocusedOpertateWidgets, LastFocusedOpertateWidget)
    Q_FLAG(LastFocusedOpertateWidget)
public:
    DAAppRibbonArea(DAAppUIInterface* u);
    ~DAAppRibbonArea();

    //发生语言变更时会触发此函数
    virtual void retranslateUi() override;
    //获取app
    AppMainWindow* app() const;
    //获取ribbon
    SARibbonBar* ribbonBar() const;
    //获取主标签
    SARibbonCategory* getRibbonCategoryMain() const;
    //返回一个list，包含支持的文件[Images (*.png *.xpm *.jpg)] [Text files (*.txt)]
    QStringList getFileReadFilters() const;
    //设置dockarea的指针 note 注意这个要在构建完成后设置进去
    void setDockAreaInterface(DAAppDockingArea* d);
    //通过DACommandInterface构建redo/undo的action
    void buildRedoUndo();
    //更新ActionLockBackgroundPixmap的check statue
    void updateActionLockBackgroundPixmapCheckStatue(bool c);
    //获取当前dataframeOperateWidget,如果没有返回nullptr,此函数不返回nullptr的前提是
    DADataOperateOfDataFrameWidget* getCurrentDataFrameOperateWidget(bool checkDataOperateAreaFocused = true);
    //获取工作流操作窗口
    DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const;
    //获取数据操作窗口
    DADataOperateWidget* getDataOperateWidget() const;
    //获取绘图操作窗口
    DAChartOperateWidget* getChartOperateWidget() const;
    //获取数据管理窗口
    DADataManageWidget* getDataManageWidget() const;
    //获取当前的绘图,如果没有回返回nullptr
    DAFigureWidget* getCurrentFigure();
    //获取绘图操作窗口,如果没有回返回nullptr
    DAChartWidget* getCurrentChart() const;
    //更新绘图相关的ribbon
    void updateFigureAboutRibbon(DAFigureWidget* fig);
    void updateChartAboutRibbon(DAChartWidget* chart);
    void updateChartGridAboutRibbon(DAChartWidget* chart);
    void updateChartZoomPanAboutRibbon(DAChartWidget* chart);
    void updateChartPickerAboutRibbon(DAChartWidget* chart);

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
    //构建编辑标签
    void buildRibbonEditCategory();
    //构建快速响应栏
    void buildRibbonQuickAccessBar();
    //构建DataFrame上下文标签
    void buildContextCategoryDataFrame();
    //构建Workflow上下文标签
    void buildContextCategoryWorkflowEdit();
    //构建chart上下文
    void buildContextCategoryChart();
    //初始化信号槽
    void initConnection();
    //初始化脚本信息
    void initScripts();
    //判断当前是否是在绘图操作模式，就算绘图操作不在焦点，但绘图操作在前端，此函数也返回true
    bool isLastFocusedOnChartOptWidget() const;
    bool isLastFocusedOnWorkflowOptWidget() const;
    bool isLastFocusedOnDataOptWidget() const;

private slots:

    //===================================================
    // 主页标签 Main Category
    //===================================================
    //打开文件
    void onActionOpenTriggered();

    void onActionAppendProjectTriggered();

    //保存工程
    void onActionSaveTriggered();
    //另存为
    void onActionSaveAsTriggered();

    // app设定
    void onActionSettingTriggered();
    //插件管理对话框触发
    void onActionPluginManagerTriggered(bool on);
    //===================================================
    // 数据标签 Data Category
    //===================================================
    //添加数据
    void onActionAddDataTriggered();
    //移除数据
    void onActionRemoveDataTriggered();
    //添加数据文件夹
    void onActionAddDataFolderTriggered();
    //===================================================
    // 绘图标签 Chart Category
    //===================================================
    //添加绘图
    void onActionAddFigureTriggered();
    //添加绘图
    void onActionFigureResizeChartTriggered(bool on);
    //新坐标系
    void onActionFigureNewXYAxisTriggered();
    //允许网格
    void onActionChartEnableGridTriggered(bool on);
    //允许网格X
    void onActionChartEnableGridXTriggered(bool on);
    //允许网格Y
    void onActionChartEnableGridYTriggered(bool on);
    //允许网格XMin
    void onActionChartEnableGridXMinEnableTriggered(bool on);
    //允许网格YMin
    void onActionChartEnableGridYMinTriggered(bool on);
    //允许缩放
    void onActionChartEnableZoomTriggered(bool on);
    //当前图表放大
    void onActionChartZoomInTriggered();
    //当前图表缩小
    void onActionChartZoomOutTriggered();
    //当前图表全部显示
    void onActionChartZoomAllTriggered();
    //允许绘图拖动
    void onActionChartEnablePanTriggered(bool on);
    //允许绘图拾取
    void onActionChartEnablePickerCrossTriggered(bool on);
    //允许绘图拾取Y
    void onActionChartEnablePickerYTriggered(bool on);
    //允许绘图拾取XY
    void onActionChartEnablePickerXYTriggered(bool on);
    //允许绘图图例
    void onActionChartEnableLegendTriggered(bool on);
    //绘图图例对齐的actiongroup
    void onActionGroupChartLegendAlignmentTriggered(QAction* a);
    //===================================================
    // 数据操作的上下文标签 Data Operate Context Category
    //===================================================
    //移除选中行
    void onActionRemoveRowTriggered();
    //移除选中列
    void onActionRemoveColumnTriggered();
    //移除单元格内容
    void onActionRemoveCellTriggered();
    //插入一行
    void onActionInsertRowTriggered();
    //在选中位置上面插入一行
    void onActionInsertRowAboveTriggered();
    //在选中位置右边插入一列
    void onActionInsertColumnRightTriggered();
    //在选中位置左边插入一列
    void onActionInsertColumnLeftTriggered();
    // dataframe列重命名
    void onActionRenameColumnsTriggered();
    //创建数据描述
    void onActionCreateDataDescribeTriggered();
    //列数据类型改变
    void onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt);
    //选中列转换为数值
    void onActionCastToNumTriggered();
    //选中列转换为文字
    void onActionCastToStringTriggered();
    //选中列转换为日期
    void onActionCastToDatetimeTriggered();
    //选中列转换为索引
    void onActionChangeToIndexTriggered();

    //===================================================
    // 视图标签 View Category
    //===================================================
    //显示工作流区域
    void onActionShowWorkFlowAreaTriggered();
    //显示绘图区域
    void onActionShowChartAreaTriggered();
    //显示数据区域
    void onActionShowDataAreaTriggered();
    //显示信息区域
    void onActionShowMessageLogViewTriggered();
    //显示设置区域
    void onActionSettingWidgetTriggered();

    //===================================================
    // workflow上下文
    //===================================================

    void onActionNewWorkflowTriggered();

    //绘制矩形
    void onActionStartDrawRectTriggered(bool on);
    //绘制文本框
    void onActionStartDrawTextTriggered(bool on);
    //运行
    void onActionRunCurrentWorkflowTriggered();
    //当前工作流的字体变更
    void onCurrentWorkflowFontChanged(const QFont& f);
    void onCurrentWorkflowFontColorChanged(const QColor& c);
    //图元的背景和框线变更
    void onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b);
    void onCurrentWorkflowShapeBorderPenChanged(const QPen& p);
    //添加背景图
    void onActionAddBackgroundPixmapTriggered();
    //锁定背景图
    void onActionLockBackgroundPixmapTriggered(bool on);
    //跟随背景图
    void onActionEnableItemMoveWithBackgroundTriggered(bool on);
private slots:
    //===================================================
    // DAWorkFlowOperateWidget的槽
    //===================================================
    void onSelectionItemChanged(QGraphicsItem* lastSelectItem);

    //===================================================
    // DAChartOperateWidget
    //===================================================
    //绘图窗口有新窗口创建
    void onFigureCreated(DA::DAFigureWidget* f);
    //绘图窗口当前窗口改变
    void onCurrentFigureChanged(DA::DAFigureWidget* f, int index);
    //图表有新窗口创建
    void onChartAdded(DA::DAChartWidget* c);
    //当前图表窗口改变
    void onCurrentChartChanged(DA::DAChartWidget* c);
    //===================================================
    // project
    //===================================================
    void onProjectSaved(const QString& path);
    void onProjectLoaded(const QString& path);

    //===================================================
    // DADataOperatePageWidget数据操作相关
    //===================================================
    //数据操作窗口添加，需要绑定相关信号槽到ribbon的页面
    void onDataOperatePageAdded(DA::DADataOperatePageWidget* page);
    void onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DA::DAPyDType dt);

    //==========================================
    // Qt-Advanced-Docking-System
    //===================================================
    // DockWidget的焦点变化
    void onFocusedDockWidgetChanged(ads::CDockWidget* old, ads::CDockWidget* now);

    //===================================================
    // DADataManageWidget
    //===================================================

    //一些界面的联动槽在此
    // DADataManageWidget查看数据的模式改变
    void onDataManageWidgetDataViewModeChanged(DADataManageWidget::DataViewMode v);

    //===================================================
    // DAWorkFlowGraphicsScene
    //===================================================
    //鼠标动作结束
    void onWorkFlowGraphicsSceneMouseActionFinished(DAWorkFlowGraphicsScene::MouseActionFlag mf);

private:
    void resetText();

private:
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
private:
    DAAppDataManager* m_datas;     ///< 数据管理区
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
    SARibbonPannel* m_pannelFigureSetting;                        ///< 绘图的设置
    SARibbonPannel* m_pannelChartSetting;                         ///< 图表的设置
    SARibbonButtonGroupWidget* m_chartGridDirActionsButtonGroup;  ///< grid的方向
    SARibbonButtonGroupWidget* m_chartGridMinActionsButtonGroup;  ///< grid的min设置
    QMenu* m_menuChartLegendProperty;                             ///< legend的属性设置
    QAction* m_actionOfMenuChartLegendAlignmentSection;           ///< m_menuChartLegendProperty对应的Section
    QSpinBox* m_spinboxChartLegendMaxColumns;                     ///< legend setMaxColumns
    //
    QStringList m_fileReadFilters;
    QMenu* m_menuInsertRow;     ///< 针对insertrow的action menu
    QMenu* m_menuInsertColumn;  ///< 这对insertcol的action menu
    //
    LastFocusedOpertateWidgets m_lastFocusedOpertateWidget;  ///< 最后获取焦点的操作窗口
};
}  // namespace DA
#endif  // DAAPPRIBBONAREA_H
