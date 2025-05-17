#ifndef DAAPPACTIONS_H
#define DAAPPACTIONS_H
#include "DAActionsInterface.h"
class QActionGroup;
namespace DA
{
/**
 * @brief action管理
 */
class DAAppActions : public DAActionsInterface
{
	Q_OBJECT
public:
	DAAppActions(DAUIInterface* u);
	~DAAppActions();
	// 发生语言变更时会调用此函数
	void retranslateUi() override;

protected:
	void buildActions();
	// 构建主actions
	void buildMainAction();
	// 构建data相关的action
	void buildDataAction();
	// 构建Chart相关的action
	void buildChartAction();
	// 构建视图action
	void buildViewAction();
	// 构建workflow action
	void buildWorkflowAction();
	// 建立其他的actions
	void buildOtherActions();

public:
	//===================================================
	// 主页标签 Main Category
	//===================================================
	QAction* actionOpen;           ///< 打开
	QAction* actionSave;           ///< 保存
	QAction* actionSaveAs;         ///< 另存为
	QAction* actionAppendProject;  ///< 追加工程

	QAction* actionRedo;
	QAction* actionUndo;
	QAction* actionSetting;        ///< 设置
	QAction* actionPluginManager;  ///< 插件管理
	QAction* actionAbout;          ///< about
	//===================================================
	// 数据标签 Data Category
	//===================================================
	QAction* actionAddData;     ///< 添加数据
	QAction* actionRemoveData;  ///< 移除数据

	//===================================================
	// 数据操作的上下文标签 Data Operate Context Category
	//===================================================
	QAction* actionRemoveRow;                 ///< 移除 一行
	QAction* actionRemoveColumn;              ///< 移除 一列
	QAction* actionRemoveCell;                ///< 移除单元格（设置为nan）
	QAction* actionInsertRow;                 ///< 向下插入 一行
	QAction* actionInsertRowAbove;            ///< 向上插入 一行
	QAction* actionInsertColumnLeft;          ///< 在左边插入列
	QAction* actionInsertColumnRight;         ///< 在右边插入列
	QAction* actionRenameColumns;             ///< 更改列名
	QAction* actionCastToString;              ///< 数据转换为文本
	QAction* actionCastToNum;                 ///< 数据转换为数字
	QAction* actionCastToDatetime;            ///< 转换为日期
	QAction* actionCreateDataDescribe;        ///< 数据描述
	QAction* actionChangeToIndex;             ///< 把某列转换为index
	QAction* actionDataFrameDropNone;         ///< 删除缺失值
	QAction* actionDropDuplicates;            ///< 重复值处理
	QAction* actionDataFrameFillNone;         ///< 填充缺失值
	QAction* actionDataFrameFFillNone;        ///< 前向填充缺失值
	QAction* actionDataFrameBFillNone;        ///< 后向填充缺失值
	QAction* actionDataFrameFillInterpolate;  ///< 插值法填充缺失值
	QAction* actionNstdFilterOutlier;         ///< n倍标准差过滤异常值
	QAction* actionDataFrameClipOutlier;      ///< 替换界限外异常值
	QAction* actionDataFrameQueryDatas;       ///< 过滤给定条件外的数据
	QAction* actionDataFrameDataSelect;       ///< 过滤范围外的数据
	QAction* actionCreatePivotTable;          ///< 创建数据透视表
	//===================================================
	// workflow的上下文标签
	//===================================================
	QAction* actionWorkflowNew;                  ///< 新建工作流
	QActionGroup* actionGroupWorkflowStartEdit;  ///< Start**的actionGroup
	QAction* actionWorkflowStartDrawRect;        ///< 绘制矩形
	QAction* actionWorkflowStartDrawText;        ///< 绘制文本框
	// workflow的视图操作
	QAction* actionWorkflowShowGrid;    ///< 显示网格
	QAction* actionWorkflowViewMarker;  ///< 视图标记

	QAction* actionWorkflowViewReadOnly;        ///< 锁定视图
	QAction* actionExportWorkflowSceneToImage;  ///< 导出png格式
	QAction* actionExportWorkflowSceneToPNG;    ///< 导出png格式
	// workflow的建模操作
	QAction* actionWorkflowAddBackgroundPixmap;           ///< 添加背景图
	QAction* actionWorkflowLockBackgroundPixmap;          ///< 锁定背景图
	QAction* actionWorkflowEnableItemMoveWithBackground;  ///< 背景图跟随元件移动
	QAction* actionWorkflowEnableItemLinkageMove;  ///< 图元联动，所谓联动，就是随着一个图元的移动，所有和这个图元链接的图元跟随移动
	QAction* actionItemGrouping;        ///< 分组
	QAction* actionItemUngroup;         ///< 取消分组
	QAction* actionWorkflowLinkEnable;  ///< 允许连接
	// workflow的运行操作
	QAction* actionWorkflowRun;        ///< 运行工作流
	QAction* actionWorkflowTerminate;  ///< 停止工作流
	//===================================================
	// 绘图标签 Chart Category
	//===================================================
	QAction* actionAddFigure;             ///< 添加绘图
	QAction* actionFigureResizeChart;     ///< 改变fig的chart大小
	QAction* actionFigureNewXYAxis;       ///< 新增加一个2D绘图
	QAction* actionChartAddCurve;         ///< 添加曲线
	QAction* actionChartAddScatter2D;     ///< 添加散点图
	QAction* actionChartAddErrorBar;      ///< 添加误差棒图
	QAction* actionChartAddBoxPlot;       ///< 添加箱线图
	QAction* actionChartAddBar;           ///< 添加柱状图
	QAction* actionChartAddMultiBar;      ///< 添加多维柱状图
	QAction* actionChartAddHistogramBar;  ///< 添加统计图
	QAction* actionChartAddContourMap;    ///< 等高线图
	QAction* actionChartAddCloudMap;      ///< 云图
	QAction* actionChartAddVectorfield;   ///< 向量场图

	QAction* actionChartEnableGrid;         ///< 网格显示总开关
	QAction* actionChartEnableGridX;        ///< 网格显示X开关
	QAction* actionChartEnableGridY;        ///< 网格显示Y开关
	QAction* actionChartEnableGridXMin;     ///< 网格显示Xmin开关
	QAction* actionChartEnableGridYMin;     ///< 网格显示Ymin开关
	QAction* actionChartEnableZoom;         ///< 绘图允许缩放
	QAction* actionChartZoomIn;             ///< 绘图放大
	QAction* actionChartZoomOut;            ///< 绘图缩小
	QAction* actionChartZoomAll;            ///< 显示全部
	QAction* actionChartEnablePan;          ///< 绘图拖动
	QActionGroup* actionGroupChartPickers;  ///< Chart Picker的actiongroup
	QAction* actionChartEnablePickerCross;  ///< 十字标记
	QAction* actionChartEnablePickerY;      ///< y拾取器
	QAction* actionChartEnablePickerXY;     ///< xy拾取器
	QAction* actionChartEnableLegend;       ///< legend
	//===================================================
	// 视图标签 View Category
	//===================================================
	QAction* actionShowWorkFlowArea;         ///< 显示工作流区域
	QAction* actionShowWorkFlowManagerArea;  ///< 显示工作流管理区域
	QAction* actionShowChartArea;
	QAction* actionShowChartManagerArea;  ///< 显示绘图管理区域
	QAction* actionShowDataArea;
	QAction* actionShowDataManagerArea;  ///< 显示数据管理区域
	QAction* actionShowMessageLogView;
	QAction* actionShowSettingWidget;
	//===================================================
	// 主题
	//===================================================
	QAction* actionRibbonThemeOffice2013;      ///< office2013主题
	QAction* actionRibbonThemeOffice2016Blue;  ///< office2016主题
	QAction* actionRibbonThemeOffice2021Blue;  ///< office2021主题
	QAction* actionRibbonThemeDark;            ///< dark主题
	QActionGroup* actionGroupRibbonTheme;      ///< actionRibbonTheme* 的actionGroup
};
}  // namespace DA

#ifndef DA_APP_ACTIONS
/**
 * @def 获取@sa DAAppCore 实例
 * @note 使用此宏需要以下两个头文件：
 * -# DAAppCore.h
 * -# DAAppUI.h
 */
#define DA_APP_ACTIONS DA::DAAppCore::getInstance().getUi()->getActions()
#endif

#endif  // DAAPPACTIONS_H
