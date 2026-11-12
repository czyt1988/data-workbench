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
class DASettingContextProvider;

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
    virtual ~DADockingAreaInterface() override;

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

    // 显示某个窗体对应的dockwidget（不会唤起到前台，仅恢复可见，与hideDockWidget对称）
    void showDockWidget(QWidget* w);

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

    // 注册 dock 内容的设置上下文提供者（插件用它接管设置面板/上下文标签）
    void registerSettingContextProvider(QWidget* dockContent, DASettingContextProvider* provider);
    // 注销 dock 内容的设置上下文提供者（注册方应在内容销毁前调用）
    void unregisterSettingContextProvider(QWidget* dockContent);
    // 激活 dock 内容对应的设置上下文：命中返回 true（先失活上一个并激活其撤销栈）；未命中返回 false 且不改变当前状态
    //（失活时机由调用方决定：APP 在焦点丢失或回到固定操作窗口时调用 deactivateSettingContextProvider()）
    bool activateSettingContextForWidget(QWidget* dockContent);
    // 失活当前的设置上下文提供者
    void deactivateSettingContextProvider();
    // 查询 dock 内容对应的设置上下文提供者（未注册返回 nullptr）
    DASettingContextProvider* settingContextProviderFor(QWidget* dockContent);
    // 广播 dock 内容被激活（框架内部在焦点变化/程序化 raise 路由前调用）
    void notifyDockWidgetActivated(QWidget* dockContent);

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

    // 在中央区按需创建 Markdown 查看 dock 并加载文件，加载成功返回 true
    virtual bool showMarkdownFile(const QString& filePath) = 0;

    // 工作流节点dock
    virtual ads::CDockWidget* getWorkflowNodeListDock() const = 0;

    // 信息窗口dock
    virtual ads::CDockWidget* getMessageLogDock() const = 0;

    // 设置窗口dock
    virtual ads::CDockWidget* getSettingContainerDock() const = 0;

    // 数据操作窗口dock
    virtual ads::CDockWidget* getDataOperateDock() const = 0;

    // 绘图操作窗口dock
    virtual ads::CDockWidget* getChartOperateDock() const = 0;

    // 工作流操作窗口dock
    virtual ads::CDockWidget* getWorkFlowOperateDock() const = 0;

    // 数据管理窗口dock
    virtual ads::CDockWidget* getDataManageDock() const = 0;

    // 图表管理窗口dock
    virtual ads::CDockWidget* getChartManageDock() const = 0;

    // 判断DataOperateWidget是否是在焦点
    bool isDataOperateWidgetDockOnFocus() const;

    // 判断DataManageWidget是否是在焦点
    bool isDataManageWidgetDockOnFocus() const;

Q_SIGNALS:
    // dock窗口被程序化唤起时发射，参数为dock内部维护的widget
    void dockWidgetRaised(QWidget* w);

    // 任意 dock 内容被激活时发射（焦点变化或程序化 raise，在上下文路由前发出）
    void dockWidgetActivated(QWidget* dockContent);

public:
    // 获取当前的场景
    DAPyWorkFlowGraphicsScene* getCurrentScene() const;

protected:
    // 创建中央dock窗体
    ads::CDockWidget* createCenterDockWidget(QWidget* w, const QString& widgetName);
};

}  // namespace DA
#endif  // DAAPPDOCKINGAREAINTERFACE_H
