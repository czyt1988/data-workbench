#ifndef DADOCKINGAREAINTERFACE_H
#define DADOCKINGAREAINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAGlobals.h"
#include "DAUIExtendInterface.h"
#include "DAData.h"
#include <QList>
#include <QPair>
#include "ads_globals.h"

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
class DAPyWorkFlowNodeListWidget;
class DAPyWorkFlowOperateWidget;
class DAPyWorkFlowGraphicsScene;
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

    // 获取CDockManager
    ads::CDockManager* dockManager();
    // 获取CDockManager
    const ads::CDockManager* dockManager() const;

    // 创建一个dock窗体
    ads::CDockWidget* createDockWidget(QWidget* w,
                                       ads::DockWidgetArea area,
                                       const QString& widgetName,
                                       ads::CDockAreaWidget* dockAreaWidget = nullptr);
    // 创建一个浮动窗体
    ads::CDockWidget* createFloatingDockWidget(QWidget* w, const QString& widgetName, const QPoint& pos);

    // 创建一个tab dock
    ads::CDockWidget* createDockWidgetAsTab(QWidget* w, const QString& widgetName, ads::CDockAreaWidget* dockAreaWidget);

    // 在中央停靠区添加一个dock窗口，作为标签页
    ads::CDockWidget* createDockWidgetTabAtCenterDockArea(QWidget* w, const QString& widgetName);

    // 通过窗口查找对应的CDockWidget
    ads::CDockWidget* findDockWidget(QWidget* w) const;

    // 隐藏某个窗体对应的dockwidget
    void hideDockWidget(QWidget* w);

    // 枚举DockingArea对应的窗口指针
    ads::CDockWidget* dockingAreaToDockWidget(DockingArea area) const;

    // 唤起一个widget对应的dock widget，如果窗口关闭了，也会唤起
    void raiseDockByWidget(QWidget* w);

    // 唤起一个dock widget，如果窗口关闭了，也会唤起
    void raiseDockingArea(DockingArea area);

    // 唤起一个feature对应的dock widget，如果窗口关闭了，也会唤起
    void raiseFeatureArea(DA::DAWorkbenchFeatureType type);

    // 判断是否处于焦点
    bool isDockingAreaFocused(DockingArea area) const;

    // 获取中心区域
    ads::CDockAreaWidget* getCenterArea() const;

    // 获取中心窗口
    ads::CDockWidget* getCentralWidget() const;

    // 重置分割尺寸
    void resetDefaultSplitterSizes();

public:
    /*
     * 接口:
     */

    // 获取工作节点管理窗口
    virtual DAPyWorkFlowNodeListWidget* getWorkflowNodeListWidget() const = 0;

    // 获取workflow操作窗口
    virtual DAPyWorkFlowOperateWidget* getWorkFlowOperateWidget() const = 0;

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

    // 获取当前选中的数据
    virtual QList< DAData > getCurrentSelectDatas() const;

    // 获取当前正在操作的数据
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

    // 判断DataOperateWidget是否是在焦点
    bool isDataOperateWidgetDockOnFocus() const;

    // 判断DataManageWidget是否是在焦点
    bool isDataManageWidgetDockOnFocus() const;

public:
    // 获取当前的场景
    DAPyWorkFlowGraphicsScene* getCurrentScene() const;

protected:
    // 创建中央dock窗体
    ads::CDockWidget* createCenterDockWidget(QWidget* w, const QString& widgetName);
};

}  // namespace DA
#endif  // DAAPPDOCKINGAREAINTERFACE_H
