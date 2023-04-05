#include "AppMainWindow.h"
#include "ui_AppMainWindow.h"
// Qt 相关
#include <QMessageBox>
//
#include "SARibbonBar.h"
//插件相关
#include "DAAppPluginManager.h"
#include "DAPluginManager.h"
#include "DAAbstractPlugin.h"
#include "DAAbstractNodePlugin.h"
//界面相关
#include "DAAppController.h"
#include "DAAppCore.h"
#include "DAAppUI.h"
#include "DAAppDockingArea.h"
#include "DAAppRibbonArea.h"
#include "DAAppCommand.h"
#include "DAAppActions.h"
#include "DAAppDataManager.h"
//对话框
#include "DAPluginManagerDialog.h"

//节点相关
#include "DANodeMetaData.h"
#include "DAAbstractNodeFactory.h"
#include "DAAbstractNodeWidget.h"

//
#include "DAGraphicsItemFactory.h"
#include "DAWorkFlowNodeListWidget.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAChartOperateWidget.h"
#include "DAChartManageWidget.h"
#include "DADataManageWidget.h"
#include "DADataOperateWidget.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAMessageLogViewWidget.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// AppMainWindow
//===================================================
AppMainWindow::AppMainWindow(QWidget* parent) : SARibbonMainWindow(parent)

{
    //建立ribbonArea，此函数的构造函数会生成界面
    DAAppCore& core = DAAppCore::getInstance();
    core.createUi(this);
    m_ui        = qobject_cast< DAAppUI* >(core.getUiInterface());
    m_dockArea  = m_ui->getAppDockingArea();
    _controller = new DAAppController(this);
    _controller
            ->setAppMainWindow(this)                       // app
            .setAppCore(&core)                             // core
            .setAppActions(m_ui->getAppActions())          // action
            .setAppCommand(m_ui->getAppCmd())              // cmd
            .setAppDataManager(core.getAppDatas())         // data
            .setAppDockingArea(m_ui->getAppDockingArea())  // dock
            .setAppRibbonArea(m_ui->getAppRibbonArea())    // ribbon
            ;
    _controller->initialize();
    //首次调用此函数会加载插件，可放置在main函数中调用
    init();
    DAGraphicsItemFactory::initialization();
    retranslateUi();
    ribbonBar()->setRibbonStyle(SARibbonBar::WpsLiteStyleTwoRow);
    showMaximized();
}

AppMainWindow::~AppMainWindow()
{
    //    delete ui;
}

void AppMainWindow::retranslateUi()
{
    m_ui->retranslateUi();
}

void AppMainWindow::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        retranslateUi();
        break;

    default:
        break;
    }
}

void AppMainWindow::setupNodeListWidget()
{
    DAAppPluginManager& plugin               = DAAppPluginManager::instance();
    QList< DAAbstractNodeFactory* > factorys = plugin.getNodeFactorys();
}

void AppMainWindow::init()
{
    initPlugins();
    initWorkflowNodes();
}

void AppMainWindow::initPlugins()
{
    DAAppPluginManager& pluginmgr = DAAppPluginManager::instance();
    DAAppCore& core               = DAAppCore::getInstance();
    pluginmgr.initLoadPlugins(&core);
}

void AppMainWindow::initWorkflowNodes()
{
    DAAppPluginManager& pluginmgr = DAAppPluginManager::instance();
    //提取所有的元数据
    QList< DANodeMetaData > nodeMetaDatas = pluginmgr.getAllNodeMetaDatas();
    //把数据写入toolbox
    m_dockArea->getWorkflowNodeListWidget()->addItems(nodeMetaDatas);
    //此时才创建第一个workflow，这个workflow创建时，插件已经加载好
    m_dockArea->getWorkFlowOperateWidget()->appendWorkflow(tr("untitle"));

    //执行一些必要的回调
    QList< DAAbstractNodePlugin* > nodeplugins = pluginmgr.getNodePlugins();
    for (DAAbstractNodePlugin* plugin : nodeplugins) {
        plugin->afterLoadedNodes();
    }
}

void AppMainWindow::onWorkflowFinished(bool success)
{
    if (success) {
        QMessageBox::information(this, tr("infomation"), tr("Topology execution completed"));  //拓扑执行完成
    } else {
        QMessageBox::critical(this, tr("infomation"), tr("Topology execution failed"));  //拓扑执行失败
    }
}
