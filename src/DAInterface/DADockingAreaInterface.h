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
    /**
     * @brief 定义了固定的dock窗口
     */
    enum DockingArea
    {
        DockingAreaWorkFlowOperate,
        DockingAreaDataOperate,
        DockingAreaChartOperate,
        DockingAreaWorkFlowManager,
        DockingAreaDataManager,
        DockingAreaChartManager,
        DockingAreaSetting,
        DockingAreaMessageLog
    };

public:
    DADockingAreaInterface(DAUIInterface* u);
    ~DADockingAreaInterface();

    /**
     * @brief 获取CDockManager
     * @return
     */
    ads::CDockManager* dockManager();
    /**
     * @brief 获取CDockManager
     * @return
     */
    const ads::CDockManager* dockManager() const;

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
    /**
     * @brief 创建一个浮动窗体
     * @param w 窗口
     * @param widgetName 窗体名称
     * @param pos 位置
     * @return
     */
    ads::CDockWidget* createFloatingDockWidget(QWidget* w, const QString& widgetName, const QPoint& pos);

    /**
     * @brief 创建一个tab dock
     * @param w 窗口
     * @param widgetName 窗体名称
     * @param dockAreaWidget 停靠区域
     * @return
     */
    ads::CDockWidget* createDockWidgetAsTab(QWidget* w, const QString& widgetName, ads::CDockAreaWidget* dockAreaWidget);

    /**
     * @brief 在中央停靠区添加一个dock窗口，作为标签页
     * @param w
     * @param widgetName
     * @return 如果没有中央停靠区，此函数返回nullptr
     *
     * 此函数是createDockWidgetAsTab的简单封装
     * @code
     * ads::CDockWidget* DADockingAreaInterface::createDockWidgetTabAtCenterDockArea(QWidget* w, const QString& widgetName)
     * {
     *    return createDockWidgetAsTab(w,widgetName,d_ptr->mCenterArea);
     * }
     * @endcode
     */
    ads::CDockWidget* createDockWidgetTabAtCenterDockArea(QWidget* w, const QString& widgetName);

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
     * @brief 枚举DockingArea对应的窗口指针
     * @param area
     * @return
     */
    ads::CDockWidget* dockingAreaToDockWidget(DockingArea area) const;

    /**
     * @brief 唤起一个widget对应的dock widget，如果窗口关闭了，也会唤起
     * @param w
     * @sa raiseDockingArea
     */
    void raiseDockByWidget(QWidget* w);

    /**
     * @brief 唤起一个dock widget，如果窗口关闭了，也会唤起
     * @param area
     * @sa raiseDockByWidget
     */
    void raiseDockingArea(DockingArea area);

    // 判断是否处于焦点
    bool isDockingAreaFocused(DockingArea area) const;

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

    /**
     * @brief 获取当前选中的数据
     *
     * @note DA中选中数据指代DataManageWidget窗口选中的数据，数据操作窗口称之为操作数据Operate Data
     * @return
     */
    virtual QList< DAData > getCurrentSelectDatas() const;

    /**
     * @brief 获取当前正在操作的数据
     *
     * @note DA中选中数据指代DataManageWidget窗口选中的数据，数据操作窗口正在操作的数据称之为Operate Data
     * @return
     */
    virtual DAData getCurrentOperateData() const;

    /**
     * @brief 工作流节点dock
     * @return
     */
    virtual ads::CDockWidget* getWorkflowNodeListDock() const = 0;

    /**
     * @brief 信息窗口dock
     * @return
     */
    virtual ads::CDockWidget* getMessageLogDock() const = 0;

    /**
     * @brief 设置窗口dock
     * @return
     */
    virtual ads::CDockWidget* getSettingContainerDock() const = 0;

    /**
     * @brief 数据操作窗口dock
     * @return
     */
    virtual ads::CDockWidget* getDataOperateDock() const = 0;

    /**
     * @brief 绘图操作窗口dock
     * @return
     */
    virtual ads::CDockWidget* getChartOperateDock() const = 0;

    /**
     * @brief 工作流操作窗口dock
     * @return
     */
    virtual ads::CDockWidget* getWorkFlowOperateDock() const = 0;

    /**
     * @brief 数据管理窗口dock
     * @return
     */
    virtual ads::CDockWidget* getDataManageDock() const = 0;

    /**
     * @brief 图表管理窗口dock
     * @return
     */
    virtual ads::CDockWidget* getChartManageDock() const = 0;

    /**
     * @brief 判断DataOperateWidget是否是在焦点
     * @return
     */
    bool isDataOperateWidgetDockOnFource() const;

    /**
     * @brief 判断DataManageWidget是否是在焦点
     * @return
     */
    bool isDataManageWidgetDockOnFource() const;

public:
    /**
     * @brief 获取当前的场景
     * @return
     */
    DAWorkFlowGraphicsScene* getCurrentScene() const;

protected:
    /**
     * @brief 创建中央dock窗体
     *
     * 此函数只能调用一次，正常用户不应该调用
     * @param w
     * @param widgetName
     * @return
     */
    ads::CDockWidget* createCenterDockWidget(QWidget* w, const QString& widgetName);
};

}  // namespace DA
#endif  // DAAPPDOCKINGAREAINTERFACE_H
