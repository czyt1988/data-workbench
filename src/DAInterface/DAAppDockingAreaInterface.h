#ifndef DAAPPDOCKINGAREAINTERFACE_H
#define DAAPPDOCKINGAREAINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAGlobals.h"
#include "DAAppUIExtendInterface.h"
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
class DAAppUIInterface;
class DAChartManageWidget;
class DAChartOperateWidget;
class DADataManageWidget;
class DADataOperateWidget;
class DAMessageLogViewWidget;
class DAWorkFlowNodeListWidget;
class DAWorkFlowOperateWidget;

DA_IMPL_FORWARD_DECL(DAAppDockingAreaInterface)

/**
 * @brief 此接口负责整个app的dock区域
 */
class DAINTERFACE_API DAAppDockingAreaInterface : public DAAppUIExtendInterface
{
    Q_OBJECT
    DA_IMPL(DAAppDockingAreaInterface)
public:
    DAAppDockingAreaInterface(DAAppUIInterface* u);
    ~DAAppDockingAreaInterface();
    //获取CDockManager
    ads::CDockManager* dockManager();
    const ads::CDockManager* dockManager() const;

    //创建中央dock窗体
    QPair< ads::CDockWidget*, ads::CDockAreaWidget* > createCenterDockWidget(QWidget* w, const QString& widgetName);

    //创建dock窗体
    QPair< ads::CDockWidget*, ads::CDockAreaWidget* > createDockWidget(QWidget* w,
                                                                       ads::DockWidgetArea area,
                                                                       const QString& widgetName,
                                                                       ads::CDockAreaWidget* dockAreaWidget = nullptr);
    QPair< ads::CDockWidget*, ads::CDockAreaWidget* > createDockWidgetAsTab(QWidget* w,
                                                                            const QString& widgetName,
                                                                            ads::CDockAreaWidget* dockAreaWidget);

    //通过窗口查找对应的CDockWidget
    ads::CDockWidget* findDockWidget(QWidget* w) const;

    //隐藏窗口对应的dockwidget
    void hideDockWidget(QWidget* w);

    //让某个窗口显示出来
    void raiseDockByWidget(QWidget* w);

public:
    /*
     * 接口:
     */

    //获取工作节点管理窗口
    virtual DAWorkFlowNodeListWidget* getWorkflowNodeListWidget() const = 0;

    //获取workflow操作窗口
    virtual DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const = 0;

    //绘图管理窗口
    virtual DAChartManageWidget* getChartManageWidget() const = 0;

    //绘图操作窗口
    virtual DAChartOperateWidget* getChartOperateWidget() const = 0;

    //数据管理窗口
    virtual DADataManageWidget* getDataManageWidget() const = 0;

    //数据操作窗口
    virtual DADataOperateWidget* getDataOperateWidget() const = 0;

    //获取日志显示窗口
    virtual DAMessageLogViewWidget* getMessageLogViewWidget() const = 0;
};
}  // namespace DA
#endif  // DAAPPDOCKINGAREAINTERFACE_H
