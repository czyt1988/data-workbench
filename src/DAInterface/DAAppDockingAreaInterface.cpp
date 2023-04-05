#include "DAAppDockingAreaInterface.h"
#include "DAAppUIInterface.h"
#include "SARibbonMainWindow.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include <QMap>
#include <QDebug>
namespace DA
{
class DAAppDockingAreaInterfacePrivate
{
    DA_IMPL_PUBLIC(DAAppDockingAreaInterface)
public:
    DAAppDockingAreaInterfacePrivate(DAAppDockingAreaInterface* p, DAAppUIInterface* u);
    void buildDock();

public:
    DAAppUIInterface* _ui;
    ads::CDockManager* _dockmgr;
    QMap< QWidget*, ads::CDockWidget* > _widgetToDockWidget;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppDockingAreaInterfacePrivate
//===================================================

DAAppDockingAreaInterfacePrivate::DAAppDockingAreaInterfacePrivate(DAAppDockingAreaInterface* p, DAAppUIInterface* u)
    : q_ptr(p), _ui(u)
{
}

void DAAppDockingAreaInterfacePrivate::buildDock()
{
    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::XmlCompressionEnabled, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    _dockmgr = new ads::CDockManager(q_ptr->ui()->mainWindow());
}
//===================================================
// DAAppDockingAreaInterface
//===================================================
DAAppDockingAreaInterface::DAAppDockingAreaInterface(DAAppUIInterface* u)
    : DAAppUIExtendInterface(u), d_ptr(new DAAppDockingAreaInterfacePrivate(this, u))
{
    d_ptr->buildDock();
}

DAAppDockingAreaInterface::~DAAppDockingAreaInterface()
{
}

ads::CDockManager* DAAppDockingAreaInterface::dockManager()
{
    return d_ptr->_dockmgr;
}

const ads::CDockManager* DAAppDockingAreaInterface::dockManager() const
{
    return d_ptr->_dockmgr;
}

ads::CDockWidget* DAAppDockingAreaInterface::findDockWidget(QWidget* w) const
{
    return d_ptr->_widgetToDockWidget.value(w, nullptr);
}

/**
 * @brief 隐藏某个窗体对应的dockwidget
 * @param w 传入dock内部维护的widget或dockwidget都可以
 */
void DAAppDockingAreaInterface::hideDockWidget(QWidget* w)
{
    ads::CDockWidget* d = findDockWidget(w);
    if (d) {
        d->closeDockWidget();
        qInfo().noquote() << tr("dock widget \"%1\" was closed and hide").arg(d->windowTitle());  // cn:停靠窗口“%1”隐藏并关闭
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
void DAAppDockingAreaInterface::raiseDockByWidget(QWidget* w)
{
    if (ads::CDockWidget* dw = findDockWidget(w)) {
        if (dw->isClosed()) {
            dw->toggleView();
        }
        dw->raise();
    }
}

/**
 * @brief 创建中央dock窗体
 * @param w
 * @param widgetName
 * @return
 */
QPair< ads::CDockWidget*, ads::CDockAreaWidget* > DAAppDockingAreaInterface::createCenterDockWidget(QWidget* w, const QString& widgetName)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    ads::CDockAreaWidget* areaWidget = d_ptr->_dockmgr->setCentralWidget(dockWidget);
    d_ptr->_widgetToDockWidget[ w ]  = dockWidget;
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
QPair< ads::CDockWidget*, ads::CDockAreaWidget* > DAAppDockingAreaInterface::createDockWidget(QWidget* w,
                                                                                              ads::DockWidgetArea area,
                                                                                              const QString& widgetName,
                                                                                              ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    ads::CDockAreaWidget* areaWidget = d_ptr->_dockmgr->addDockWidget(area, dockWidget, dockAreaWidget);
    d_ptr->_widgetToDockWidget[ w ]  = dockWidget;
    return qMakePair(dockWidget, areaWidget);
}

QPair< ads::CDockWidget*, ads::CDockAreaWidget* > DAAppDockingAreaInterface::createDockWidgetAsTab(QWidget* w,
                                                                                                   const QString& widgetName,
                                                                                                   ads::CDockAreaWidget* dockAreaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    ads::CDockAreaWidget* areaWidget = d_ptr->_dockmgr->addDockWidgetTabToArea(dockWidget, dockAreaWidget);
    d_ptr->_widgetToDockWidget[ w ]  = dockWidget;
    return qMakePair(dockWidget, areaWidget);
}
