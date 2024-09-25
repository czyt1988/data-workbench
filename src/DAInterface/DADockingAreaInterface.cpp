#include "DADockingAreaInterface.h"
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
#include "DADataManageWidget.h"
#include "DADataOperateWidget.h"
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
	ads::CDockAreaWidget* mCenterArea { nullptr };
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

ads::CDockAreaWidget* DADockingAreaInterface::getCenterArea() const
{
	return d_ptr->mCenterArea;
}

ads::CDockWidget* DADockingAreaInterface::getCentralWidget() const
{
	return d_ptr->mDockManager->centralWidget();
}

void DADockingAreaInterface::resetDefaultSplitterSizes()
{
	QScreen* screen = QApplication::primaryScreen();
	int leftwidth   = screen->size().width() / 6;
	int rightwidth  = leftwidth;
	int centerwidth = screen->size().width() - leftwidth - rightwidth;
	dockManager()->setSplitterSizes(getCenterArea(), { leftwidth, centerwidth, rightwidth });
}

/**
 * @brief 获取当前选中的数据
 *
 * 如果用户当前焦点实在表格操作界面
 *
 * 如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，
 * 如果用户选中了多列，那么每列作为一个DAData，最后组成一个QList<DAData>返回,如果用户打开了表格，但没选择任何列，这个函数返回这个表作为Data（pd.DataFrame）
 *
 * 如果用户没有选择列，但选中了单元格，那么相当于选中了单元格对应的列
 *
 * 如果什么都没选中，那么返回一个空的list
 *
 *
 * 如果用户实在数据管理界面
 *
 * 返回选中的dataframe
 * @return
 */
QList< DAData > DADockingAreaInterface::getCurrentSelectDatas() const
{
	QList< DAData > res;
	// 获取当前的焦点
	ads::CDockWidget* currentFource = dockManager()->focusedDockWidget();
	if (currentFource == nullptr) {
		// 说明没有焦点的窗口，返回空
		return res;
	}
	// 查看是否是DataOperateWidget
	auto dow = getDataOperateWidget();
	auto dmw = getDataManageWidget();

	if (currentFource->widget() == dow) {
		// 数据表窗口，调用getCurrentSelectDatas
		return dow->getCurrentSelectDatas();
	} else if (currentFource->widget() == dmw) {
		// 数据管理窗口，调用getSelectDatas
		return dmw->getCurrentSelectDatas();
	}

	return res;
}
#if DA_ENABLE_PYTHON
/**
 * @brief 获取当前选中的dataframe
 *
 * @return pair:first 选中的dataframe，pair:second 选中的列索引，对于当前选中的是DataManageWidget，第二项返回空
 */
std::pair< DAPyDataFrame, QList< int > > DADockingAreaInterface::getCurrentSelectDataFrame() const
{
	std::pair< DAPyDataFrame, QList< int > > res;
	// 获取当前的焦点
	ads::CDockWidget* currentFource = dockManager()->focusedDockWidget();
	if (currentFource == nullptr) {
		// 说明没有焦点的窗口，返回空
		return res;
	}
	// 查看是否是DataOperateWidget
	auto dow = getDataOperateWidget();
	auto dmw = getDataManageWidget();

	if (currentFource->widget() == dow) {
		// 数据表窗口，调用getCurrentSelectDatas
		return dow->getCurrentSelectDataFrame();
	} else if (currentFource->widget() == dmw) {
		// 数据管理窗口，调用getSelectDatas
		auto sd = dmw->getOneSelectData();
		if (sd.isDataFrame()) {
			res.first = sd.toDataFrame();
		}
	}
	return res;
}
#endif
/**
 * @brief 判断DataOperateWidget是否是在焦点
 * @return
 */
bool DADockingAreaInterface::isDataOperateWidgetDockOnFource() const
{
	ads::CDockWidget* currentFource = dockManager()->focusedDockWidget();
	return (currentFource->widget() == getDataOperateWidget());
}

/**
 * @brief 判断DataManageWidget是否是在焦点
 * @return
 */
bool DADockingAreaInterface::isDataManageWidgetDockOnFource() const
{
	ads::CDockWidget* currentFource = dockManager()->focusedDockWidget();
	return (currentFource->widget() == getDataManageWidget());
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
	d_ptr->mCenterArea = d_ptr->mDockManager->setCentralWidget(dockWidget);
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

ads::CDockWidget* DADockingAreaInterface::createFloatingDockWidget(QWidget* w, const QString& widgetName, const QPoint& pos)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(widgetName);
    dockWidget->setWidget(w);
    ads::CFloatingDockContainer* fc = d_ptr->mDockManager->addDockWidgetFloating(dockWidget);

    fc->move(pos);
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
