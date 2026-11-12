#include "DADockingAreaInterface.h"
#include "DASettingContextProviderRegistry.h"
#include <QMap>
#include <QDebug>
#include <QApplication>
#include <QScreen>
// DA
#include "DAUIInterface.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
#include "FloatingDockContainer.h"
// SARibbon
//  这个头文件需要存在，ui()->mainWindow()获取的窗口需要这个头文件，不要移除
#include "SARibbonMainWindow.h"
//
#include "DataManager/DADataManageWidget.h"
#include "Table/DADataOperateWidget.h"
#include "Workflow/DAPyWorkFlowOperateWidget.h"
#include "Workflow/DAPyWorkFlowGraphicsScene.h"
namespace DA
{
class DADockingAreaInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DADockingAreaInterface)
public:
    PrivateData(DADockingAreaInterface* p);
    void buildDock();

public:
    ads::CDockManager* mDockManager { nullptr };
    ads::CDockAreaWidget* mCenterArea { nullptr };
    DASettingContextProviderRegistry mSettingContextProviders;  ///< dock内容 -> 设置上下文提供者注册表
};

//===================================================
// DAAppDockingAreaInterfacePrivate
//===================================================

DADockingAreaInterface::PrivateData::PrivateData(DADockingAreaInterface* p) : q_ptr(p)
{
}

void DADockingAreaInterface::PrivateData::buildDock()
{
    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::XmlCompressionEnabled, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    mDockManager = new ads::CDockManager(q_ptr->ui()->mainWindow());
}
//===================================================
// DAAppDockingAreaInterface
//===================================================
DADockingAreaInterface::DADockingAreaInterface(DAUIInterface* u) : DAUIExtendInterface(u), DA_PIMPL_CONSTRUCT
{
    d_ptr->buildDock();
}

DADockingAreaInterface::~DADockingAreaInterface()
{
}

/**
 * @brief 获取CDockManager
 * @return
 */
ads::CDockManager* DADockingAreaInterface::dockManager()
{
    return d_ptr->mDockManager;
}

/**
 * @brief 获取CDockManager
 * @return
 */
const ads::CDockManager* DADockingAreaInterface::dockManager() const
{
    return d_ptr->mDockManager;
}

/**
 * @brief 通过窗口查找对应的CDockWidget
 * @note 注意此函数是O(n)复杂度
 * @param w 要查询的窗口
 * @return 如果没找到，返回nullptr
 */
ads::CDockWidget* DADockingAreaInterface::findDockWidget(QWidget* w) const
{
    if (nullptr == w) {
        return nullptr;
    }
    const auto allDockWidgets = d_ptr->mDockManager->dockWidgetsMap();
    for (auto i = allDockWidgets.begin(); i != allDockWidgets.end(); ++i) {
        if (i.value()->widget() == w) {
            return i.value();
        }
    }
    return nullptr;
}

/**
 * @brief 隐藏某个窗体对应的dockwidget
 * @param w 传入dock内部维护的widget或dockwidget都可以
 */
void DADockingAreaInterface::hideDockWidget(QWidget* w)
{
    ads::CDockWidget* d = findDockWidget(w);
    if (d) {
        d->toggleView(false);
        qDebug().noquote() << QString("dock widget \"%1\" was closed and hidden").arg(d->windowTitle());  // cn:停靠窗口"%1"隐藏并关闭
    } else {
        d = qobject_cast< ads::CDockWidget* >(w);
        if (d) {
            d->toggleView(false);
        } else {
            qDebug().noquote() << QString("cannot find widget or dock widget");  // cn:无法找到需要隐藏的dock 窗口
        }
    }
}

/**
 * @brief 显示某个窗体对应的dockwidget，与hideDockWidget对称
 * @param w 传入dock内部维护的widget或dockwidget都可以
 * @note 仅恢复可见，不会唤起到前台，需要唤起使用 @ref raiseDockByWidget
 */
void DADockingAreaInterface::showDockWidget(QWidget* w)
{
    ads::CDockWidget* d = findDockWidget(w);
    if (d) {
        d->toggleView(true);
    } else {
        d = qobject_cast< ads::CDockWidget* >(w);
        if (d) {
            d->toggleView(true);
        } else {
            qDebug().noquote() << QString("cannot find widget or dock widget to show");  // cn:无法找到需要显示的dock 窗口
        }
    }
}

/**
 * @brief 唤起一个widget对应的dock widget，如果窗口关闭了，也会唤起
 * @param w
 * @sa raiseDockingArea
 */
void DADockingAreaInterface::raiseDockByWidget(QWidget* w)
{
    if (ads::CDockWidget* dw = findDockWidget(w)) {
        if (dw->isClosed()) {
            dw->toggleView();
        }
        dw->raise();
        Q_EMIT dockWidgetRaised(w);
    }
}

/**
 * @brief 获取中心区域
 * @return
 */
ads::CDockAreaWidget* DADockingAreaInterface::getCenterArea() const
{
    return d_ptr->mCenterArea;
}

/**
 * @brief 获取中心窗口
 * @return
 */
ads::CDockWidget* DADockingAreaInterface::getCentralWidget() const
{
    return d_ptr->mDockManager->centralWidget();
}

/**
 * @brief 重置分割尺寸
 */
void DADockingAreaInterface::resetDefaultSplitterSizes()
{
    ads::CDockAreaWidget* centerArea = getCenterArea();
    if (!centerArea) {
        return;
    }
    QScreen* screen = QApplication::primaryScreen();
    int screenWidth  = screen ? screen->size().width() : 1920;
    int leftwidth   = screenWidth / 6;
    int rightwidth  = leftwidth;
    int centerwidth = screenWidth - leftwidth - rightwidth;
    dockManager()->setSplitterSizes(centerArea, { leftwidth, centerwidth, rightwidth });
}

/**
 * @brief 获取当前选中的数据
 *
 * @note DA中选中数据指代DataManageWidget窗口选中的数据，数据操作窗口称之为操作数据Operate Data
 * @return
 */
QList< DAData > DADockingAreaInterface::getCurrentSelectDatas() const
{
    auto dmw = getDataManageWidget();
    if (!dmw) {
        return QList< DAData >();
    }
    return dmw->getAllSelectDatas();
}

/**
 * @brief 获取当前正在操作的数据
 *
 * @note DA中选中数据指代DataManageWidget窗口选中的数据，数据操作窗口正在操作的数据称之为Operate Data
 * @return
 */
DAData DADockingAreaInterface::getCurrentOperateData() const
{
    auto dow = getDataOperateWidget();
    if (!dow) {
        return DAData();
    }
    return dow->getCurrentOperateData();
}

/**
 * @brief 判断DataOperateWidget是否是在焦点
 * @return
 */
bool DADockingAreaInterface::isDataOperateWidgetDockOnFocus() const
{
    ads::CDockWidget* currentFocus = dockManager()->focusedDockWidget();
    if (!currentFocus) {
        return false;
    }
    return (currentFocus->widget() == getDataOperateWidget());
}

/**
 * @brief 判断DataManageWidget是否是在焦点
 * @return
 */
bool DADockingAreaInterface::isDataManageWidgetDockOnFocus() const
{
    ads::CDockWidget* currentFocus = dockManager()->focusedDockWidget();
    if (!currentFocus) {
        return false;
    }
    return (currentFocus->widget() == getDataManageWidget());
}

/**
 * @brief 获取当前的场景
 * @return
 */
DAPyWorkFlowGraphicsScene* DADockingAreaInterface::getCurrentScene() const
{
    DAPyWorkFlowOperateWidget* ow = getWorkFlowOperateWidget();
    if (ow) {
        return ow->getCurrentWorkFlowScene();
    }
    return nullptr;
}

/**
 * @brief 枚举DockingArea对应的窗口指针
 * @param area
 * @return
 */
ads::CDockWidget* DADockingAreaInterface::dockingAreaToDockWidget(DockingArea area) const
{
    switch (area) {
    case DockingAreaChartManager:
        return getChartManageDock();
    case DockingAreaChartOperate:
        return getChartOperateDock();
    case DockingAreaDataManager:
        return getDataManageDock();
    case DockingAreaDataOperate:
        return getDataOperateDock();
    case DockingAreaMessageLog:
        return getMessageLogDock();
    case DockingAreaSetting:
        return getSettingContainerDock();
    case DockingAreaWorkFlowManager:
        return getWorkflowNodeListDock();
    case DockingAreaWorkFlowOperate:
        return getWorkFlowOperateDock();
    default:
        break;
    }
    return nullptr;
}

/**
 * @brief 唤起一个dock widget，如果窗口关闭了，也会唤起
 * @param area
 * @sa raiseDockByWidget
 */
void DADockingAreaInterface::raiseDockingArea(DockingArea area)
{
    ads::CDockWidget* dw = dockingAreaToDockWidget(area);
    if (dw) {
        if (dw->isClosed()) {
            dw->toggleView();
        }
        dw->raise();
        Q_EMIT dockWidgetRaised(dw->widget());
    }
}

/**
 * @brief 唤起一个feature对应的dock widget，如果窗口关闭了，也会唤起
 * @param type
 */
void DA::DADockingAreaInterface::raiseFeatureArea(DA::DAWorkbenchFeatureType type)
{
    switch (type) {
    case DA::DAWorkbenchFeatureType::Chart:
        raiseDockingArea(DockingAreaChartManager);
        raiseDockingArea(DockingAreaChartOperate);
        break;
    case DA::DAWorkbenchFeatureType::Data:
        raiseDockingArea(DockingAreaDataManager);
        raiseDockingArea(DockingAreaDataOperate);
        break;
    case DA::DAWorkbenchFeatureType::Workflow:
        raiseDockingArea(DockingAreaWorkFlowManager);
        raiseDockingArea(DockingAreaWorkFlowOperate);
        break;
    default:
        break;
    }
    // 设置窗口在所有功能域切换时都唤起，便于用户调整当前功能域的配置
    raiseDockingArea(DockingAreaSetting);
}

/**
 * @brief 判断是否处于焦点
 * @param area
 * @return
 */
bool DADockingAreaInterface::isDockingAreaFocused(DockingArea area) const
{
    ads::CDockWidget* dw = dockingAreaToDockWidget(area);
    ads::CDockWidget* fd = dockManager()->focusedDockWidget();
    if (dw) {
        return (dw == fd);
    }
    return false;
}

/**
 * @brief 注册 dock 内容的设置上下文提供者
 *
 * 注册后，该 dock 内容被激活时 APP 会优先调用 provider->activateSettingContext()，
 * 由提供者接管右侧设置页与 Ribbon 上下文标签，不再走固定的数据/绘图/工作流路由。
 * 注册方应在 dock 内容销毁前调用 unregisterSettingContextProvider()。
 * @param dockContent dock 内部维护的内容窗口
 * @param provider 设置上下文提供者（不接管所有权）
 */
void DADockingAreaInterface::registerSettingContextProvider(QWidget* dockContent, DASettingContextProvider* provider)
{
    d_ptr->mSettingContextProviders.registerProvider(dockContent, provider);
}

/**
 * @brief 注销 dock 内容的设置上下文提供者
 * @param dockContent dock 内部维护的内容窗口
 */
void DADockingAreaInterface::unregisterSettingContextProvider(QWidget* dockContent)
{
    d_ptr->mSettingContextProviders.unregisterProvider(dockContent);
}

/**
 * @brief 激活 dock 内容对应的设置上下文
 * @param dockContent dock 内部维护的内容窗口
 * @return 命中并接管返回 true；未命中返回 false（调用方继续走固定窗口路由）
 */
bool DADockingAreaInterface::activateSettingContextForWidget(QWidget* dockContent)
{
    return d_ptr->mSettingContextProviders.activateFor(dockContent);
}

/**
 * @brief 失活当前的设置上下文提供者
 */
void DADockingAreaInterface::deactivateSettingContextProvider()
{
    d_ptr->mSettingContextProviders.deactivateCurrent();
}

/**
 * @brief 查询 dock 内容对应的设置上下文提供者
 * @param dockContent dock 内部维护的内容窗口
 * @return 提供者指针，未注册返回 nullptr
 */
DASettingContextProvider* DADockingAreaInterface::settingContextProviderFor(QWidget* dockContent)
{
    return d_ptr->mSettingContextProviders.providerFor(dockContent);
}

/**
 * @brief 广播 dock 内容被激活
 *
 * 由 APP 路由层在焦点变化/程序化 raise 时、执行上下文路由之前调用；插件可据此
 * 同步自身状态（如刷新当前会话）。与 dockWidgetRaised 不同，它覆盖焦点切换路径。
 * @param dockContent 被激活的 dock 内容窗口（焦点离开全部 dock 时为 nullptr）
 */
void DADockingAreaInterface::notifyDockWidgetActivated(QWidget* dockContent)
{
    Q_EMIT dockWidgetActivated(dockContent);
}

/**
 * @brief 创建中央dock窗体
 *
 * 此函数只能调用一次，正常用户不应该调用
 * @param w
 * @param widgetName
 * @return
 */
ads::CDockWidget* DADockingAreaInterface::createCenterDockWidget(QWidget* w, const QString& widgetName)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(d_ptr->mDockManager, widgetName);
    dockWidget->setWidget(w);
    d_ptr->mCenterArea = d_ptr->mDockManager->setCentralWidget(dockWidget);
    return dockWidget;
}

/**
 * @brief 创建一个dock窗体
 * @param w
 * @param area
 * @param widgetName 注意，这里的是作为title同时作为objectname,但多语言应该单独设置title，因此在构造之后必须在设置单独的objname
 * @param dockAreaWidget
 * @return
 */
ads::CDockWidget* DADockingAreaInterface::createDockWidget(QWidget* w,
                                                           ads::DockWidgetArea area,
                                                           const QString& widgetName,
                                                           ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(d_ptr->mDockManager, widgetName);
    dockWidget->setWidget(w);
    d_ptr->mDockManager->addDockWidget(area, dockWidget, dockAreaWidget);
    return dockWidget;
}

/**
 * @brief 创建一个浮动窗体
 * @param w 窗口
 * @param widgetName 窗体名称
 * @param pos 位置
 * @return
 */
ads::CDockWidget* DADockingAreaInterface::createFloatingDockWidget(QWidget* w, const QString& widgetName, const QPoint& pos)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(d_ptr->mDockManager, widgetName);
    dockWidget->setWidget(w);
    ads::CFloatingDockContainer* fc = d_ptr->mDockManager->addDockWidgetFloating(dockWidget);
    fc->move(pos);
    return dockWidget;
}

/**
 * @brief 创建一个tab dock
 * @param w 窗口
 * @param widgetName 窗体名称
 * @param dockAreaWidget 停靠区域
 * @return
 */
ads::CDockWidget* DADockingAreaInterface::createDockWidgetAsTab(QWidget* w,
                                                                const QString& widgetName,
                                                                ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(d_ptr->mDockManager, widgetName);
    dockWidget->setWidget(w);
    dockWidget->setFeatures(ads::CDockWidget::DefaultDockWidgetFeatures);
    dockWidget->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromDockWidget);
    d_ptr->mDockManager->addDockWidgetTabToArea(dockWidget, dockAreaWidget);
    return dockWidget;
}

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
ads::CDockWidget* DADockingAreaInterface::createDockWidgetTabAtCenterDockArea(QWidget* w, const QString& widgetName)
{
    if (!d_ptr->mCenterArea) {
        return nullptr;
    }
    return createDockWidgetAsTab(w, widgetName, d_ptr->mCenterArea);
}
}  // namespace DA
