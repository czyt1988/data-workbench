#ifndef DADOCKINGAREAINTERFACE_H
#define DADOCKINGAREAINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAGlobals.h"
#include "DAUIExtendInterface.h"
#include "DAData.h"
#include <QList>
#include <QPair>
#include "ads_globals.h"

#include "DAWorkFlowOperateWidget.h"

class SARibbonMainWindow;
namespace ads
{
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}

namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DAChartManageWidget;
class DAChartOperateWidget;
class DADataManageWidget;
class DADataOperateWidget;
class DAMessageLogViewWidget;
class DAWorkFlowNodeListWidget;
class DAWorkFlowOperateWidget;
class DASettingContainerWidget;

/**
 * @brief 此接口负责整个app的dock区域
 */
class DAINTERFACE_API DADockingAreaInterface : public DAUIExtendInterface
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DADockingAreaInterface)
public:
	DADockingAreaInterface(DAUIInterface* u);
	~DADockingAreaInterface();
	// 获取CDockManager
	ads::CDockManager* dockManager();
	const ads::CDockManager* dockManager() const;

	// 创建中央dock窗体
	ads::CDockWidget* createCenterDockWidget(QWidget* w, const QString& widgetName);

	/**
	 * @brief 创建一个dock窗体
	 * @param w
	 * @param area
	 * @param widgetName 注意，这里的是作为title同时作为objectname,但多语言应该单独设置title，因此在构造之后必须在设置单独的objname
	 * @param dockAreaWidget
	 * @return
	 */
	ads::CDockWidget* createDockWidget(QWidget* w,
									   ads::DockWidgetArea area,
									   const QString& widgetName,
									   ads::CDockAreaWidget* dockAreaWidget = nullptr);
	ads::CDockWidget* createDockWidgetAsTab(QWidget* w, const QString& widgetName, ads::CDockAreaWidget* dockAreaWidget);

	/**
	 * @brief 通过窗口查找对应的CDockWidget
	 * @note 注意此函数是O(n)复杂度
	 * @param w 要查询的窗口
	 * @return 如果没找到，返回nullptr
	 */
	ads::CDockWidget* findDockWidget(QWidget* w) const;

	/**
	 * @brief 隐藏某个窗体对应的dockwidget
	 * @param w 传入dock内部维护的widget或dockwidget都可以
	 */
	void hideDockWidget(QWidget* w);

	/**
	 * @brief 唤起一个widget对应的dock widget，如果窗口关闭了，也会唤起
	 * @param w
	 */
	void raiseDockByWidget(QWidget* w);

	/**
	 * @brief 获取中心区域
	 * @return
	 */
	ads::CDockAreaWidget* getCenterArea() const;

	/**
	 * @brief 获取中心窗口
	 * @return
	 */
	ads::CDockWidget* getCentralWidget() const;

	/**
	 * @brief 重置分割尺寸
	 */
	void resetDefaultSplitterSizes();

public:
	/*
	 * 接口:
	 */

	// 获取工作节点管理窗口
	virtual DAWorkFlowNodeListWidget* getWorkflowNodeListWidget() const = 0;

	// 获取workflow操作窗口
	virtual DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const = 0;

	// 绘图管理窗口
	virtual DAChartManageWidget* getChartManageWidget() const = 0;

	// 绘图操作窗口
	virtual DAChartOperateWidget* getChartOperateWidget() const = 0;

	// 数据管理窗口
	virtual DADataManageWidget* getDataManageWidget() const = 0;

	// 数据操作窗口
	virtual DADataOperateWidget* getDataOperateWidget() const = 0;

	// 获取日志显示窗口
	virtual DAMessageLogViewWidget* getMessageLogViewWidget() const = 0;

	// 获取设置窗口,设置容器可以放置多个设置窗口
	virtual DASettingContainerWidget* getSettingContainerWidget() const = 0;

	// 获取当前选中的数据，此函数会根据界面的焦点，获取当前选中的数据
	virtual QList< DAData > getCurrentSelectDatas() const;
#if DA_ENABLE_PYTHON
	// 获取当前选中的Dataframe,如果用户在选中了列，返回选中的列索引
	virtual std::pair< DAPyDataFrame, QList< int > > getCurrentSelectDataFrame() const;
#endif
	// 判断DataOperateWidget是否是在焦点
	bool isDataOperateWidgetDockOnFource() const;

	// 判断DataManageWidget是否是在焦点
	bool isDataManageWidgetDockOnFource() const;

public:
	// 基于接口的快速方法
	DAWorkFlowGraphicsScene* getCurrentScene() const;
};
}  // namespace DA
#endif  // DAAPPDOCKINGAREAINTERFACE_H
