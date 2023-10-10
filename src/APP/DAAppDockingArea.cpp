#include "DAAppDockingArea.h"
#include <QApplication>
#include <QScreen>
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
#include "DAChartManageWidget.h"
#include "DAChartOperateWidget.h"
#include "DAAppFigureFactory.h"
#include "DAAppChartOperateWidget.h"
// Data相关
#include "DADataOperateWidget.h"
#include "DADataManageWidget.h"
// message相关
#include "DAMessageLogViewWidget.h"
// workflow相关
#include "DAAbstractNodeGraphicsItem.h"
#include "DAWorkFlowNodeListWidget.h"
#include "DAWorkFlowGraphicsView.h"
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
 * @brief 枚举DockingArea对应的窗口指针
 * @param area
 * @return
 */
ads::CDockWidget* DAAppDockingArea::dockingAreaToDockWidget(DAAppDockingArea::DockingArea area) const
{
    switch (area) {
    case DockingAreaChartManager:
        return mChartManageDock;
    case DockingAreaChartOperate:
        return mChartOperateDock;
    case DockingAreaDataManager:
        return mDataManageDock;
    case DockingAreaDataOperate:
        return mDataOperateDock;
    case DockingAreaMessageLog:
        return mMessageLogDock;
    case DockingAreaSetting:
        return mSettingContainerDock;
    case DockingAreaWorkFlowManager:
        return mWorkflowNodeListDock;
    case DockingAreaWorkFlowOperate:
        return mWorkFlowOperateDock;
    default:
        break;
    }
    return nullptr;
}

/**
 * @brief 唤起一个dock widget，如果窗口关闭了，也会唤起
 * @param area
 */
void DAAppDockingArea::raiseDockingArea(DAAppDockingArea::DockingArea area)
{
    ads::CDockWidget* dw = dockingAreaToDockWidget(area);
    if (dw) {
        if (dw->isClosed()) {
            dw->toggleView();
        }
        dw->raise();
    }
}

/**
 * @brief 判断是否处于焦点
 * @param area
 * @return
 */
bool DAAppDockingArea::isDockingAreaFocused(DAAppDockingArea::DockingArea area) const
{
    ads::CDockWidget* dw = dockingAreaToDockWidget(area);
    ads::CDockWidget* fd = dockManager()->focusedDockWidget();
    if (dw) {
        return (dw == fd);
    }
    return false;
}

/**
 * @brief 显示数据
 * @param data
 */
void DAAppDockingArea::showDataOperateWidget(const DA::DAData& data)
{
    mDataOperateWidget->showData(data);
    //把表格窗口唤起
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
    //中央操作区
    buildWorkflowAboutWidgets();
    buildChartAboutWidgets();
    buildDataAboutWidgets();
    buildOtherWidgets();
    auto wfoa = createCenterDockWidget(mWorkFlowOperateWidget, QStringLiteral("da_workFlowOperateWidgetDock"));
    mWorkFlowOperateDock = wfoa.first;
    mWorkFlowOperateDock->setIcon(QIcon(":/Icon/Icon/showWorkFlow.svg"));
    mWorkFlowOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wcoa = createDockWidgetAsTab(mChartOperateWidget, QStringLiteral("da_chartOperateWidgetDock"), wfoa.second);
    mChartOperateDock = wcoa.first;
    mChartOperateDock->setIcon(QIcon(":/Icon/Icon/showChart.svg"));
    mChartOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wdoa = createDockWidgetAsTab(mDataOperateWidget, QStringLiteral("da_dataOperateWidgetDock"), wfoa.second);
    mDataOperateDock = wdoa.first;
    mDataOperateDock->setIcon(QIcon(":/Icon/Icon/showTable.svg"));
    mDataOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    mDataOperateDock->raise();

    //左侧管理区 - 工作流节点窗口
    auto wfna = createDockWidget(mWorkflowNodeListWidget, ads::LeftDockWidgetArea, QStringLiteral("da_workflowNodeListWidgetDock"));
    mWorkflowNodeListDock = wfna.first;
    mWorkflowNodeListDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wcma = createDockWidgetAsTab(mChartManageWidget, QStringLiteral("da_chartManageWidgetDock"), wfna.second);
    mChartManageDock = wcma.first;
    mChartManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wdma       = createDockWidgetAsTab(mDataManageWidget, QStringLiteral("da_dataManageWidgetDock"), wfna.second);
    mDataManageDock = wdma.first;
    mDataManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    mDataManageDock->raise();

    //右侧附属区 - 添加设置视图
    auto sca = createDockWidget(mSettingContainerWidget, ads::RightDockWidgetArea, QStringLiteral("da_settingDock"));
    mSettingContainerDock = sca.first;
    mSettingContainerDock->setIcon(QIcon(":/Icon/Icon/showSettingWidget.svg"));
    //日志窗口
    auto ica        = createDockWidget(mMessageLogViewWidget, ads::BottomDockWidgetArea, QStringLiteral("da_messageLogViewWidgetDock"), sca.second);
    mMessageLogDock = ica.first;
    mMessageLogDock->setIcon(QIcon(":/Icon/Icon/showInfomation.svg"));

    //设置dock的区域大小,默认为左1：中间4：右：1
    QScreen* screen = QApplication::primaryScreen();
    int leftwidth   = screen->size().width() / 6;
    int rightwidth  = leftwidth;
    int centerwidth = screen->size().width() - leftwidth - rightwidth;
    mDockmgr->setSplitterSizes(wfna.second, { leftwidth, centerwidth, rightwidth });
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
    //把工作流操作窗口设置到工程中
    DAAppProject* project = DA_APP_CORE.getAppProject();
    project->setWorkFlowOperateWidget(mWorkFlowOperateWidget);
}

void DAAppDockingArea::buildChartAboutWidgets()
{
    mChartOperateWidget = new DAAppChartOperateWidget(mApp);
    mChartOperateWidget->setObjectName(QStringLiteral("da_chartOperateWidget"));

    mChartManageWidget = new DAChartManageWidget(mApp);
    mChartManageWidget->setObjectName(QStringLiteral("da_chartManageWidget"));

    DADataManager* dmgr = mDataMgr->dataManager();
    mChartManageWidget->setChartOperateWidget(mChartOperateWidget);
    DAAppFigureFactory* factory = new DAAppFigureFactory();
    factory->setDataManager(dmgr);
    mChartOperateWidget->setupFigureFactory(factory);
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
    //右侧附属区 - 添加设置视图
    mSettingContainerWidget = new DASettingContainerWidget(mApp);
    mSettingContainerWidget->setObjectName(QStringLiteral("da_settingContainerWidget"));
    //日志窗口
    mMessageLogViewWidget = new DAMessageLogViewWidget(mApp);
    mMessageLogViewWidget->setObjectName(QStringLiteral("da_messageLogViewWidget"));
}

void DAAppDockingArea::initConnection()
{
    connect(mWorkFlowOperateWidget, &DAWorkFlowOperateWidget::selectNodeItemChanged, this, &DAAppDockingArea::onSelectNodeItemChanged);
    connect(mWorkFlowOperateWidget, &DAWorkFlowOperateWidget::workflowCreated, this, &DAAppDockingArea::onWorkFlowOperateWidgetWorkflowCreated);
    // DADataManageWidget的数据双击，在DADataOperateWidget中显示
    connect(mDataManageWidget, &DADataManageWidget::dataDbClicked, this, &DAAppDockingArea::onDataManageWidgetDataDbClicked);
    //设置窗口的绑定
    mSettingContainerWidget->getWorkFlowNodeItemSettingWidget()->setWorkFlowOperateWidget(mWorkFlowOperateWidget);
}

/**
 * @brief 节点选择发生了变化
 * @param i
 */
void DAAppDockingArea::onSelectNodeItemChanged(DAAbstractNodeGraphicsItem* i)
{
    Q_UNUSED(i);
    //    DAWorkFlowNodeItemSettingWidget* nodesetting = mSettingContainerWidget->getWorkFlowNodeItemSettingWidget();
    //    if (nullptr == nodesetting) {
    //        return;
    //    }
    //    //! 节点有个虚函数getNodeWidget是可以针对不同节点获取不同的设置窗口,这个窗口是一个个性化设置窗口，需要在节点切换时单独加载
    //    if (nullptr == i) {
    //        if (nullptr != mLastSetNodeWidget) {
    //            mSettingContainerWidget->getWorkFlowNodeItemSettingWidget()->removeWidget(mLastSetNodeWidget);
    //        }
    //        return;
    //    }
    //    DAAbstractNodeWidget* w = i->getNodeWidget();

    //    if (nullptr == w) {
    //        if (nullptr != mLastSetNodeWidget) {
    //            mSettingContainerWidget->getWorkFlowNodeItemSettingWidget()->removeWidget(mLastSetNodeWidget);
    //        }
    //        return;
    //    }
    //    if (mLastSetNodeWidget != w) {
    //        mSettingContainerWidget->getWorkFlowNodeItemSettingWidget()->removeWidget(mLastSetNodeWidget);
    //        mSettingContainerWidget->getWorkFlowNodeItemSettingWidget()->addWidget(w, QIcon(":/Icon/Icon/node-settting.svg"), tr("property"));
    //    }
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
