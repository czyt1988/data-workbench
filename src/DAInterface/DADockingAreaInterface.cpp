#include "DADockingAreaInterface.h"
#include <QMap>
#include <QDebug>
// DA
#include "DAUIInterface.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
// SARibbon
//  这个头文件需要存在，ui()->mainWindow()获取的窗口需要这个头文件，不要移除
#include "SARibbonMainWindow.h"

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

void DADockingAreaInterface::hideDockWidget(QWidget* w)
{
    ads::CDockWidget* d = findDockWidget(w);
    if (d) {
        d->toggleView(false);
        qDebug().noquote() << tr("dock widget \"%1\" was closed and hide").arg(d->windowTitle());  // cn:停靠窗口“%1”隐藏并关闭
    } else {
        d = qobject_cast< ads::CDockWidget* >(w);
        if (d) {
            d->toggleView(false);
        } else {
            qDebug().noquote() << tr("can not find widget or dock widget ");  // cn:无法找到需要隐藏的dock 窗口
        }
    }
}

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
ads::CDockWidget* DADockingAreaInterface::createCenterDockWidget(QWidget* w, const QString& widgetName)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    d_ptr->mDockManager->setCentralWidget(dockWidget);
    return dockWidget;
}

ads::CDockWidget* DADockingAreaInterface::createDockWidget(QWidget* w,
                                                           ads::DockWidgetArea area,
                                                           const QString& widgetName,
                                                           ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    d_ptr->mDockManager->addDockWidget(area, dockWidget, dockAreaWidget);
    return dockWidget;
}

ads::CDockWidget* DADockingAreaInterface::createDockWidgetAsTab(QWidget* w,
                                                                const QString& widgetName,
                                                                ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    dockWidget->setFeatures(ads::CDockWidget::DefaultDockWidgetFeatures);
    dockWidget->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromDockWidget);
    d_ptr->mDockManager->addDockWidgetTabToArea(dockWidget, dockAreaWidget);
    return dockWidget;
}
}  // namespace DA
