#include "DAAppDockingArea.h"
#include <QApplication>
#include <QScreen>
#include <QLabel>
#include "AppMainWindow.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// API相关
#include "DAAppCore.h"
#include "DAAppProject.h"
#include "DAUIInterface.h"
#include "DACommandInterface.h"
#include "DAAppCommand.h"
#include "DAAppDataManager.h"
// chart相关
#include "DAChartOperateWidget.h"
#include "DAAppFigureFactory.h"
#include "DAAppChartOperateWidget.h"
#include "DAAppChartManageWidget.h"
#include "DAChartSettingWidget.h"
// Data相关
#include "DADataOperateWidget.h"
#include "DADataManageWidget.h"
// message相关
#include "DAMessageLogViewWidget.h"
// workflow相关
#include "DAAbstractNodeGraphicsItem.h"
#include "DAWorkFlowNodeListWidget.h"
#include "DAWorkFlowEditWidget.h"
#include "DASettingContainerWidget.h"
#include "DAWorkFlowNodeItemSettingWidget.h"
#include "DAAppWorkFlowOperateWidget.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppDockingArea
//===================================================
DAAppDockingArea::DAAppDockingArea(DAUIInterface* u) : DADockingAreaInterface(u)
{
	mApp     = qobject_cast< AppMainWindow* >(u->mainWindow());
	mAppCmd  = qobject_cast< DAAppCommand* >(u->getCommandInterface());
	mDataMgr = qobject_cast< DAAppDataManager* >(core()->getDataManagerInterface());
	buildDockingArea();
}

DAAppDockingArea::~DAAppDockingArea()
{
}

void DAAppDockingArea::retranslateUi()
{
	resetText();
}

void DAAppDockingArea::resetText()
{
	mWorkflowNodeListDock->setWindowTitle(tr("workflow node"));    // cn:节点
	mChartManageDock->setWindowTitle(tr("charts manager"));        // cn:绘图管理
	mDataManageDock->setWindowTitle(tr("datas manager"));          // cn:数据管理
	mWorkFlowOperateDock->setWindowTitle(tr("workflow operate"));  // cn:工作流操作
	mChartOperateDock->setWindowTitle(tr("chart operate"));        // cn:绘图操作
	mDataOperateDock->setWindowTitle(tr("data operate"));          // cn:数据操作
	mSettingContainerDock->setWindowTitle(tr("setting"));          // cn:设置
	mMessageLogDock->setWindowTitle(tr("log"));                    // cn:消息
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAWorkFlowNodeListWidget* DAAppDockingArea::getWorkflowNodeListWidget() const
{
    return mWorkflowNodeListWidget;
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAWorkFlowOperateWidget* DAAppDockingArea::getWorkFlowOperateWidget() const
{
    return mWorkFlowOperateWidget;
}

/**
 * @brief 获取绘图管理窗口
 * @return
 */
DAChartManageWidget* DAAppDockingArea::getChartManageWidget() const
{
    return mChartManageWidget;
}

/**
 * @brief 获取绘图操作窗口
 * @return
 */
DAChartOperateWidget* DAAppDockingArea::getChartOperateWidget() const
{
    return mChartOperateWidget;
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataManageWidget* DAAppDockingArea::getDataManageWidget() const
{
    return mDataManageWidget;
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataOperateWidget* DAAppDockingArea::getDataOperateWidget() const
{
    return mDataOperateWidget;
}

/**
 * @brief 获取日志显示窗口
 * @return
 */
DAMessageLogViewWidget* DAAppDockingArea::getMessageLogViewWidget() const
{
    return mMessageLogViewWidget;
}

/**
 * @brief 获取设置窗口
 * @return
 */
DASettingContainerWidget* DAAppDockingArea::getSettingContainerWidget() const
{
    return mSettingContainerWidget;
}

/**
 * @brief 显示数据
 * @param data
 */
void DAAppDockingArea::showDataOperateWidget(const DA::DAData& data)
{
	mDataOperateWidget->showData(data);
	// 把表格窗口唤起
	raiseDockByWidget((QWidget*)getDataOperateWidget());
}

/**
 * @brief 构建dockwidget
 *
 * 可以通过dockManager()->findDockWidget(objname:QString)函数来找到对应的dockwidget并进行操作
 *
 * 所有本APP相关的关键object name都以da打头:
 *
 * da_workflowNodeListWidgetDock
 * da_chartManageWidgetDock
 * da_dataManageWidgetDock
 * da_workFlowOperateWidgetDock
 * da_chartOperateWidgetDock
 * da_dataOperateWidgetDock
 * da_settingDock
 * da_messageLogViewWidgetDock
 *
 */
void DAAppDockingArea::buildDockingArea()
{
	// 中央操作区
	buildWorkflowAboutWidgets();
	buildChartAboutWidgets();
	buildDataAboutWidgets();
	buildOtherWidgets();

	// QLabel* centerLabel = new QLabel(mApp);
	// auto center         = createCenterDockWidget(centerLabel, QStringLiteral("centerLabel"));

	mWorkFlowOperateDock = createCenterDockWidget(mWorkFlowOperateWidget, QStringLiteral("da_workFlowOperateWidgetDock"));
	mWorkFlowOperateDock->setIcon(QIcon(":/app/bright/Icon/showWorkFlow.svg"));
	mWorkFlowOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

	mChartOperateDock = createDockWidgetAsTab(
		mChartOperateWidget, QStringLiteral("da_chartOperateWidgetDock"), mWorkFlowOperateDock->dockAreaWidget());
	mChartOperateDock->setIcon(QIcon(":/app/bright/Icon/showChart.svg"));
	mChartOperateDock->setToggleViewActionMode(ads::CDockWidget::ActionModeToggle);
	mChartOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

	mDataOperateDock = createDockWidgetAsTab(
		mDataOperateWidget, QStringLiteral("da_dataOperateWidgetDock"), mWorkFlowOperateDock->dockAreaWidget());
	mDataOperateDock->setIcon(QIcon(":/app/bright/Icon/showTable.svg"));
	mDataOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	mDataOperateDock->raise();

	// 左侧管理区 - 工作流节点窗口
	mWorkflowNodeListDock = createDockWidget(
		mWorkflowNodeListWidget, ads::LeftDockWidgetArea, QStringLiteral("da_workflowNodeListWidgetDock"));
	mWorkflowNodeListDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

	mChartManageDock = createDockWidgetAsTab(
		mChartManageWidget, QStringLiteral("da_chartManageWidgetDock"), mWorkflowNodeListDock->dockAreaWidget());
	mChartManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

	mDataManageDock = createDockWidgetAsTab(
		mDataManageWidget, QStringLiteral("da_dataManageWidgetDock"), mWorkflowNodeListDock->dockAreaWidget());
	mDataManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	mDataManageDock->raise();

	// 右侧附属区 - 添加设置视图
	mSettingContainerDock =
		createDockWidget(mSettingContainerWidget, ads::RightDockWidgetArea, QStringLiteral("da_settingDock"));
	mSettingContainerDock->setIcon(QIcon(":/app/bright/Icon/showSettingWidget.svg"));
	// 日志窗口
	mMessageLogDock = createDockWidget(mMessageLogViewWidget,
									   ads::BottomDockWidgetArea,
									   QStringLiteral("da_messageLogViewWidgetDock"),
									   mSettingContainerDock->dockAreaWidget());
	mMessageLogDock->setIcon(QIcon(":/app/bright/Icon/showInfomation.svg"));

	// 设置dock的区域大小,默认为左1：中间4：右：1
	resetDefaultSplitterSizes();
#if 0
	QScreen* screen = QApplication::primaryScreen();
	int leftwidth   = screen->size().width() / 6;
	int rightwidth  = leftwidth;
	int centerwidth = screen->size().width() - leftwidth - rightwidth;
	dockManager()->setSplitterSizes(mWorkflowNodeListDock->dockAreaWidget(), { leftwidth, centerwidth, rightwidth });
#endif
	//

	initConnection();
	resetText();
}

/**
 * @brief 创建workflow相关的窗口
 */
void DAAppDockingArea::buildWorkflowAboutWidgets()
{
	mWorkFlowOperateWidget = new DAAppWorkFlowOperateWidget(mApp);
	mWorkFlowOperateWidget->setObjectName(QStringLiteral("da_workFlowOperateWidget"));
	mWorkflowNodeListWidget = new DAWorkFlowNodeListWidget(mApp);
	mWorkflowNodeListWidget->setObjectName(QStringLiteral("da_workflowNodeListWidget"));
	// 把工作流操作窗口设置到工程中
	DAAppProject* project = DA_APP_CORE.getAppProject();
	project->setWorkFlowOperateWidget(mWorkFlowOperateWidget);
}

void DAAppDockingArea::buildChartAboutWidgets()
{
	DADataManager* dmgr = mDataMgr->dataManager();

	DAAppChartOperateWidget* appChartOptWidget = new DAAppChartOperateWidget(mApp);
	appChartOptWidget->setObjectName(QStringLiteral("da_chartOperateWidget"));
	appChartOptWidget->setDataManager(dmgr);
	appChartOptWidget->setupFigureFactory(new DAAppFigureFactory());
	mChartOperateWidget = appChartOptWidget;

	mChartManageWidget = new DAAppChartManageWidget(mApp);
	mChartManageWidget->setObjectName(QStringLiteral("da_chartManageWidget"));

	mChartManageWidget->setChartOperateWidget(mChartOperateWidget);
}

void DAAppDockingArea::buildDataAboutWidgets()
{
	DADataManager* dmgr = mDataMgr->dataManager();
	mDataOperateWidget  = new DADataOperateWidget(dmgr, mApp);
	mDataOperateWidget->setObjectName(QStringLiteral("da_dataOperateWidget"));

	mDataManageWidget = new DADataManageWidget(mApp);
	mDataManageWidget->setObjectName(QStringLiteral("da_dataManageWidget"));
	mDataManageWidget->setDataManager(dmgr);
}

void DAAppDockingArea::buildOtherWidgets()
{
	// 右侧附属区 - 添加设置视图
	mSettingContainerWidget = new DASettingContainerWidget(mApp);
	mSettingContainerWidget->setObjectName(QStringLiteral("da_settingContainerWidget"));
	// 日志窗口
	mMessageLogViewWidget = new DAMessageLogViewWidget(mApp);
	mMessageLogViewWidget->setObjectName(QStringLiteral("da_messageLogViewWidget"));
}

void DAAppDockingArea::initConnection()
{
	connect(mWorkFlowOperateWidget,
			&DAWorkFlowOperateWidget::workflowCreated,
			this,
			&DAAppDockingArea::onWorkFlowOperateWidgetWorkflowCreated);
	// DADataManageWidget的数据双击，在DADataOperateWidget中显示
	connect(mDataManageWidget, &DADataManageWidget::dataDbClicked, this, &DAAppDockingArea::onDataManageWidgetDataDbClicked);
	// 设置窗口的绑定
	mSettingContainerWidget->getWorkFlowNodeItemSettingWidget()->setWorkFlowOperateWidget(mWorkFlowOperateWidget);
	mSettingContainerWidget->getChartSettingWidget()->setChartOprateWidget(mChartOperateWidget);
}

void DAAppDockingArea::onDataManageWidgetDataDbClicked(const DA::DAData& data)
{
    showDataOperateWidget(data);
}

/**
 * @brief DAWorkFlowOperateWidget有新的工作流窗口创建会触发此槽
 * @param wfw
 */
void DAAppDockingArea::onWorkFlowOperateWidgetWorkflowCreated(DA::DAWorkFlowEditWidget* wfw)
{
	if (mAppCmd) {
		// 新加的DAWorkFlowEditWidget，把undostack加入command
		mAppCmd->addStack(wfw->getUndoStack());
	}
}

ads::CDockWidget* DAAppDockingArea::getChartManageDock() const
{
	return mChartManageDock;
}

ads::CDockWidget* DAAppDockingArea::getDataManageDock() const
{
	return mDataManageDock;
}

ads::CDockWidget* DAAppDockingArea::getWorkFlowOperateDock() const
{
	return mWorkFlowOperateDock;
}

ads::CDockWidget* DAAppDockingArea::getChartOperateDock() const
{
	return mChartOperateDock;
}

ads::CDockWidget* DAAppDockingArea::getDataOperateDock() const
{
	return mDataOperateDock;
}

ads::CDockWidget* DAAppDockingArea::getSettingContainerDock() const
{
	return mSettingContainerDock;
}

ads::CDockWidget* DAAppDockingArea::getMessageLogDock() const
{
	return mMessageLogDock;
}

ads::CDockWidget* DAAppDockingArea::getWorkflowNodeListDock() const
{
	return mWorkflowNodeListDock;
}
