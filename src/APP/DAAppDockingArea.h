#ifndef DAAPPDOCKINGAREA_H
#define DAAPPDOCKINGAREA_H
#include "DADockingAreaInterface.h"
//
#include "DAData.h"
// CDockArea
namespace ads
{
class CDockManager;
class CDockAreaWidget;
}
// SARibbon
class SARibbonMainWindow;

namespace DA
{
class AppMainWindow;
class DACoreInterface;
class DAAppDataManager;
class DAAppCommand;
// 管理窗口
class DAWorkFlowNodeListWidget;
class DAChartManageWidget;
class DADataManageWidget;
// 操作窗口
class DAWorkFlowOperateWidget;
class DAWorkFlowEditWidget;
class DAChartOperateWidget;
class DADataOperateWidget;
// 设置窗口
class DASettingContainerWidget;
// 日志窗口
class DAMessageLogViewWidget;
//
class DAAbstractNodeGraphicsItem;
class DAAbstractNodeWidget;

/**
 * @brief 负责docking窗口区域的管理，APP分两大区域-RibbonArea和DockArea
 * DockArea包含所有的窗口
 */
class DAAppDockingArea : public DADockingAreaInterface
{
    Q_OBJECT
public:
public:
	DAAppDockingArea(DAUIInterface* u);
	~DAAppDockingArea();

	// 翻译
	void retranslateUi() override;
	// 设置文本
	void resetText();

public:
	// 获取工作节点管理窗口
	virtual DAWorkFlowNodeListWidget* getWorkflowNodeListWidget() const override;

	// 工作流操作窗口
	virtual DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const override;

	// 绘图管理窗口
	virtual DAChartManageWidget* getChartManageWidget() const override;

	// 绘图操作窗口
	virtual DAChartOperateWidget* getChartOperateWidget() const override;

	// 数据管理窗口
	virtual DADataManageWidget* getDataManageWidget() const override;

	// 数据操作窗口
	virtual DADataOperateWidget* getDataOperateWidget() const override;

	// 获取日志显示窗口
	virtual DAMessageLogViewWidget* getMessageLogViewWidget() const override;

	// 获取设置窗口,设置容器可以放置多个设置窗口
	virtual DASettingContainerWidget* getSettingContainerWidget() const override;

	/**
	 * @brief 工作流节点dock
	 * @return
	 */
	ads::CDockWidget* getWorkflowNodeListDock() const override;

	/**
	 * @brief 信息窗口dock
	 * @return
	 */
	ads::CDockWidget* getMessageLogDock() const override;

	/**
	 * @brief 设置窗口dock
	 * @return
	 */
	ads::CDockWidget* getSettingContainerDock() const override;

	/**
	 * @brief 数据操作窗口dock
	 * @return
	 */
	ads::CDockWidget* getDataOperateDock() const override;

	/**
	 * @brief 绘图操作窗口dock
	 * @return
	 */
	ads::CDockWidget* getChartOperateDock() const override;

	/**
	 * @brief 工作流操作窗口dock
	 * @return
	 */
	ads::CDockWidget* getWorkFlowOperateDock() const override;

	/**
	 * @brief 数据管理窗口dock
	 * @return
	 */
	ads::CDockWidget* getDataManageDock() const override;

	/**
	 * @brief 图表管理窗口dock
	 * @return
	 */
	ads::CDockWidget* getChartManageDock() const override;

public slots:
	// 显示数据
	void showDataOperateWidget(const DA::DAData& data);

private:
	// 构建界面
	void buildDockingArea();
	// 创建各个相关的窗口
	void buildWorkflowAboutWidgets();
	void buildChartAboutWidgets();
	void buildDataAboutWidgets();
	void buildOtherWidgets();
	// 初始化信号槽
	void initConnection();
private slots:
	void onDataManageWidgetDataDbClicked(const DA::DAData& data);

private:
	AppMainWindow* mApp;
	DAAppCommand* mAppCmd;  ///< cmd
	DAAppDataManager* mDataMgr;

	// 管理窗口不允许关闭
	//  管理窗口
	DAWorkFlowNodeListWidget* mWorkflowNodeListWidget;  ///< 工作流节点窗口
	ads::CDockWidget* mWorkflowNodeListDock;            ///< m_workflowNodeListWidget对应的dock
	DAChartManageWidget* mChartManageWidget;            ///< 绘图管理窗口
	ads::CDockWidget* mChartManageDock;                 ///< m_chartManageWidget对应的dock
	DADataManageWidget* mDataManageWidget;              ///< 数据窗口
	ads::CDockWidget* mDataManageDock;                  ///< m_dataManageWidget对应的dock
	// 操作窗口不允许关闭
	//  操作窗口
	DAWorkFlowOperateWidget* mWorkFlowOperateWidget;  ///< 工作流操作窗口
	ads::CDockWidget* mWorkFlowOperateDock;           ///< m_workFlowOperateWidget对应的dock
	DAChartOperateWidget* mChartOperateWidget;        ///< 绘图操作窗口
	ads::CDockWidget* mChartOperateDock;              ///< m_chartOperateWidget对应的dock
	DADataOperateWidget* mDataOperateWidget;          ///< 数据操作窗口
	ads::CDockWidget* mDataOperateDock;               ///< m_dataOperateWidget对应的dock

	// 设置窗口
	DASettingContainerWidget* mSettingContainerWidget;  ///< 设置窗口容器
	ads::CDockWidget* mSettingContainerDock;
	// 日志窗口
	DAMessageLogViewWidget* mMessageLogViewWidget;  ///< 日志窗口
	ads::CDockWidget* mMessageLogDock;
	//
	DAAbstractNodeWidget* mLastSetNodeWidget;
};
}  // namespace DA
#endif  // DAAPPDOCKINGAREA_H
