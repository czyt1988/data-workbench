#include "DAAppDockingArea.h"
#include <QApplication>
#include <QScreen>
#include "AppMainWindow.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// API相关
#include "DAAppCore.h"
#include "DAProject.h"
#include "DAAppUIInterface.h"
#include "DACommandInterface.h"
#include "DAAppCommand.h"
#include "DAAppDataManager.h"
// chart相关
#include "DAChartManageWidget.h"
#include "DAChartOperateWidget.h"
#include "DAAppFigureFactory.h"
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
DAAppDockingArea::DAAppDockingArea(DAAppUIInterface* u) : DAAppDockingAreaInterface(u)
{
    m_app     = qobject_cast< AppMainWindow* >(u->mainWindow());
    m_appCmd  = qobject_cast< DAAppCommand* >(u->getCommandInterface());
    m_dataMgr = qobject_cast< DAAppDataManager* >(core()->getDataManagerInterface());
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
    m_workflowNodeListDock->setWindowTitle(tr("workflow node"));    // cn:节点
    m_chartManageDock->setWindowTitle(tr("charts manager"));        // cn:绘图管理
    m_dataManageDock->setWindowTitle(tr("datas manager"));          // cn:数据管理
    m_workFlowOperateDock->setWindowTitle(tr("workflow operate"));  // cn:工作流操作
    m_chartOperateDock->setWindowTitle(tr("chart operate"));        // cn:绘图操作
    m_dataOperateDock->setWindowTitle(tr("data operate"));          // cn:数据操作
    m_settingContainerDock->setWindowTitle(tr("setting"));          // cn:设置
    m_messageLogDock->setWindowTitle(tr("log"));                    // cn:消息
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAWorkFlowNodeListWidget* DAAppDockingArea::getWorkflowNodeListWidget() const
{
    return m_workflowNodeListWidget;
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAWorkFlowOperateWidget* DAAppDockingArea::getWorkFlowOperateWidget() const
{
    return m_workFlowOperateWidget;
}

/**
 * @brief 获取绘图管理窗口
 * @return
 */
DAChartManageWidget* DAAppDockingArea::getChartManageWidget() const
{
    return m_chartManageWidget;
}

/**
 * @brief 获取绘图操作窗口
 * @return
 */
DAChartOperateWidget* DAAppDockingArea::getChartOperateWidget() const
{
    return m_chartOperateWidget;
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataManageWidget* DAAppDockingArea::getDataManageWidget() const
{
    return m_dataManageWidget;
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataOperateWidget* DAAppDockingArea::getDataOperateWidget() const
{
    return m_dataOperateWidget;
}

/**
 * @brief 获取日志显示窗口
 * @return
 */
DAMessageLogViewWidget* DAAppDockingArea::getMessageLogViewWidget() const
{
    return m_messageLogViewWidget;
}

/**
 * @brief 获取设置窗口
 * @return
 */
DASettingContainerWidget* DAAppDockingArea::getSettingContainerWidget() const
{
    return m_settingContainerWidget;
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
        return m_chartManageDock;
    case DockingAreaChartOperate:
        return m_chartOperateDock;
    case DockingAreaDataManager:
        return m_dataManageDock;
    case DockingAreaDataOperate:
        return m_dataOperateDock;
    case DockingAreaMessageLog:
        return m_messageLogDock;
    case DockingAreaSetting:
        return m_settingContainerDock;
    case DockingAreaWorkFlowManager:
        return m_workflowNodeListDock;
    case DockingAreaWorkFlowOperate:
        return m_workFlowOperateDock;
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
    m_dataOperateWidget->showData(data);
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
    auto wfoa = createCenterDockWidget(m_workFlowOperateWidget, QStringLiteral("da_workFlowOperateWidgetDock"));
    m_workFlowOperateDock = wfoa.first;
    m_workFlowOperateDock->setIcon(QIcon(":/Icon/Icon/showWorkFlow.svg"));
    m_workFlowOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wcoa = createDockWidgetAsTab(m_chartOperateWidget, QStringLiteral("da_chartOperateWidgetDock"), wfoa.second);
    m_chartOperateDock = wcoa.first;
    m_chartOperateDock->setIcon(QIcon(":/Icon/Icon/showChart.svg"));
    m_chartOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wdoa = createDockWidgetAsTab(m_dataOperateWidget, QStringLiteral("da_dataOperateWidgetDock"), wfoa.second);
    m_dataOperateDock = wdoa.first;
    m_dataOperateDock->setIcon(QIcon(":/Icon/Icon/showTable.svg"));
    m_dataOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    m_dataOperateDock->raise();

    //左侧管理区 - 工作流节点窗口
    auto wfna = createDockWidget(m_workflowNodeListWidget, ads::LeftDockWidgetArea, QStringLiteral("da_workflowNodeListWidgetDock"));
    m_workflowNodeListDock = wfna.first;
    m_workflowNodeListDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wcma = createDockWidgetAsTab(m_chartManageWidget, QStringLiteral("da_chartManageWidgetDock"), wfna.second);
    m_chartManageDock = wcma.first;
    m_chartManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    auto wdma = createDockWidgetAsTab(m_dataManageWidget, QStringLiteral("da_dataManageWidgetDock"), wfna.second);
    m_dataManageDock = wdma.first;
    m_dataManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    m_dataManageDock->raise();

    //右侧附属区 - 添加设置视图
    auto sca = createDockWidget(m_settingContainerWidget, ads::RightDockWidgetArea, QStringLiteral("da_settingDock"));
    m_settingContainerDock = sca.first;
    m_settingContainerDock->setIcon(QIcon(":/Icon/Icon/showSettingWidget.svg"));
    //日志窗口
    auto ica         = createDockWidget(m_messageLogViewWidget, ads::BottomDockWidgetArea, QStringLiteral("da_messageLogViewWidgetDock"), sca.second);
    m_messageLogDock = ica.first;
    m_messageLogDock->setIcon(QIcon(":/Icon/Icon/showInfomation.svg"));

    //设置dock的区域大小,默认为左1：中间4：右：1
    QScreen* screen = QApplication::primaryScreen();
    int leftwidth   = screen->size().width() / 6;
    int rightwidth  = leftwidth;
    int centerwidth = screen->size().width() - leftwidth - rightwidth;
    m_dockmgr->setSplitterSizes(wfna.second, { leftwidth, centerwidth, rightwidth });
    //

    initConnection();
    resetText();
}

/**
 * @brief 创建workflow相关的窗口
 */
void DAAppDockingArea::buildWorkflowAboutWidgets()
{
    m_workFlowOperateWidget = new DAAppWorkFlowOperateWidget(m_app);
    m_workFlowOperateWidget->setObjectName(QStringLiteral("da_workFlowOperateWidget"));
    m_workflowNodeListWidget = new DAWorkFlowNodeListWidget(m_app);
    m_workflowNodeListWidget->setObjectName(QStringLiteral("da_workflowNodeListWidget"));
    //把工作流操作窗口设置到工程中
    DAProject* project = DA_APP_CORE.getAppProject();
    project->setWorkFlowOperateWidget(m_workFlowOperateWidget);
}

void DAAppDockingArea::buildChartAboutWidgets()
{
    m_chartOperateWidget = new DAChartOperateWidget(m_app);
    m_chartOperateWidget->setObjectName(QStringLiteral("da_chartOperateWidget"));

    m_chartManageWidget = new DAChartManageWidget(m_app);
    m_chartManageWidget->setObjectName(QStringLiteral("da_chartManageWidget"));

    DADataManager* dmgr = m_dataMgr->dataManager();
    m_chartManageWidget->setChartOperateWidget(m_chartOperateWidget);
    DAAppFigureFactory* factory = new DAAppFigureFactory();
    factory->setDataManager(dmgr);
    m_chartOperateWidget->setupFigureFactory(factory);
}

void DAAppDockingArea::buildDataAboutWidgets()
{
    DADataManager* dmgr = m_dataMgr->dataManager();
    m_dataOperateWidget = new DADataOperateWidget(dmgr, m_app);
    m_dataOperateWidget->setObjectName(QStringLiteral("da_dataOperateWidget"));

    m_dataManageWidget = new DADataManageWidget(m_app);
    m_dataManageWidget->setObjectName(QStringLiteral("da_dataManageWidget"));
    m_dataManageWidget->setDataManager(dmgr);
}

void DAAppDockingArea::buildOtherWidgets()
{
    //右侧附属区 - 添加设置视图
    m_settingContainerWidget = new DASettingContainerWidget(m_app);
    m_settingContainerWidget->setObjectName(QStringLiteral("da_settingContainerWidget"));
    //日志窗口
    m_messageLogViewWidget = new DAMessageLogViewWidget(m_app);
    m_messageLogViewWidget->setObjectName(QStringLiteral("da_messageLogViewWidget"));
}

void DAAppDockingArea::initConnection()
{
    connect(m_workFlowOperateWidget, &DAWorkFlowOperateWidget::selectNodeItemChanged, this, &DAAppDockingArea::onSelectNodeItemChanged);
    connect(m_workFlowOperateWidget, &DAWorkFlowOperateWidget::workflowCreated, this, &DAAppDockingArea::onWorkFlowOperateWidgetWorkflowCreated);
    // DADataManageWidget的数据双击，在DADataOperateWidget中显示
    connect(m_dataManageWidget, &DADataManageWidget::dataDbClicked, this, &DAAppDockingArea::onDataManageWidgetDataDbClicked);
    //设置窗口的绑定
    m_settingContainerWidget->getWorkFlowNodeItemSettingWidget()->setWorkFlowOperateWidget(m_workFlowOperateWidget);
}

void DAAppDockingArea::onSelectNodeItemChanged(DAAbstractNodeGraphicsItem* i)
{
    if (nullptr == i) {
        if (nullptr != m_lastSetNodeWidget) {
            m_settingContainerWidget->getWorkFlowNodeItemSettingWidget()->removeWidget(m_lastSetNodeWidget);
        }
        return;
    }
    DAAbstractNodeWidget* w = i->getNodeWidget();

    if (nullptr == w) {
        if (nullptr != m_lastSetNodeWidget) {
            m_settingContainerWidget->getWorkFlowNodeItemSettingWidget()->removeWidget(m_lastSetNodeWidget);
        }
        return;
    }
    if (m_lastSetNodeWidget != w) {
        m_settingContainerWidget->getWorkFlowNodeItemSettingWidget()->removeWidget(m_lastSetNodeWidget);
        m_settingContainerWidget->getWorkFlowNodeItemSettingWidget()->addWidget(w, QIcon(":/Icon/Icon/node-settting.svg"), tr("property"));
    }
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
    if (m_appCmd) {
        // 新加的DAWorkFlowEditWidget，把undostack加入command
        m_appCmd->addStack(wfw->getUndoStack());
    }
}
