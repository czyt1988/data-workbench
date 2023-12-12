#include "DADockingAreaInterface.h"
#include "DAUIInterface.h"
#include "SARibbonMainWindow.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include <QMap>
#include <QDebug>
namespace DA
{
class DADockingAreaInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DADockingAreaInterface)
public:
    PrivateData(DADockingAreaInterface* p);
    void buildDock();

public:
    DAUIInterface* mUiInterface { nullptr };
    ads::CDockManager* mDockManager { nullptr };
    QMap< QWidget*, ads::CDockWidget* > mWidgetToDockWidget;
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
    d_ptr->mUiInterface = u;
    d_ptr->buildDock();
}

DADockingAreaInterface::~DADockingAreaInterface()
{
}

ads::CDockManager* DADockingAreaInterface::dockManager()
{
    return d_ptr->mDockManager;
}

const ads::CDockManager* DADockingAreaInterface::dockManager() const
{
    return d_ptr->mDockManager;
}

ads::CDockWidget* DADockingAreaInterface::findDockWidget(QWidget* w) const
{
    return d_ptr->mWidgetToDockWidget.value(w, nullptr);
}

/**
 * @brief 隐藏某个窗体对应的dockwidget
 * @param w 传入dock内部维护的widget或dockwidget都可以
 */
void DADockingAreaInterface::hideDockWidget(QWidget* w)
{
    ads::CDockWidget* d = findDockWidget(w);
    if (d) {
        d->closeDockWidget();
        qDebug().noquote() << tr("dock widget \"%1\" was closed and hide").arg(d->windowTitle());  // cn:停靠窗口“%1”隐藏并关闭
    } else {
        d = qobject_cast< ads::CDockWidget* >(w);
        if (d) {
            d->closeDockWidget();
        }
    }
}

/**
 * @brief 唤起一个widget对应的dock widget，如果窗口关闭了，也会唤起
 * @param w
 */
void DADockingAreaInterface::raiseDockByWidget(QWidget* w)
{
    if (ads::CDockWidget* dw = findDockWidget(w)) {
        if (dw->isClosed()) {
            dw->toggleView();
        }
        dw->raise();
    }
}

/**
 * @brief 获取当前的场景
 * @return
 */
DAWorkFlowGraphicsScene* DADockingAreaInterface::getCurrentScene() const
{
    DAWorkFlowOperateWidget* ow = getWorkFlowOperateWidget();
    if (ow) {
        return ow->getCurrentWorkFlowScene();
    }
    return nullptr;
}

/**
 * @brief 创建中央dock窗体
 * @param w
 * @param widgetName
 * @return
 */
QPair< ads::CDockWidget*, ads::CDockAreaWidget* > DADockingAreaInterface::createCenterDockWidget(QWidget* w, const QString& widgetName)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    ads::CDockAreaWidget* areaWidget = d_ptr->mDockManager->setCentralWidget(dockWidget);
    d_ptr->mWidgetToDockWidget[ w ]  = dockWidget;
    return qMakePair(dockWidget, areaWidget);
}

/**
 * @brief 创建一个dock窗体
 * @param w
 * @param area
 * @param widgetName 注意，这里的是作为title同时作为objectname,但多语言应该单独设置title，因此在构造之后必须在设置单独的objname
 * @param dockAreaWidget
 * @return
 */
QPair< ads::CDockWidget*, ads::CDockAreaWidget* > DADockingAreaInterface::createDockWidget(QWidget* w,
                                                                                           ads::DockWidgetArea area,
                                                                                           const QString& widgetName,
                                                                                           ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    ads::CDockAreaWidget* areaWidget = d_ptr->mDockManager->addDockWidget(area, dockWidget, dockAreaWidget);
    d_ptr->mWidgetToDockWidget[ w ]  = dockWidget;
    return qMakePair(dockWidget, areaWidget);
}

QPair< ads::CDockWidget*, ads::CDockAreaWidget* > DADockingAreaInterface::createDockWidgetAsTab(QWidget* w,
                                                                                                const QString& widgetName,
                                                                                                ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    dockWidget->setFeatures(ads::CDockWidget::DefaultDockWidgetFeatures);
    dockWidget->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromDockWidget);
    ads::CDockAreaWidget* areaWidget = d_ptr->mDockManager->addDockWidgetTabToArea(dockWidget, dockAreaWidget);
    d_ptr->mWidgetToDockWidget[ w ]  = dockWidget;
    return qMakePair(dockWidget, areaWidget);
}
}  // namespace DA
