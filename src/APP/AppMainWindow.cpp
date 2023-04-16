#include "AppMainWindow.h"
#include "ui_AppMainWindow.h"
// Qt 相关
#include <QMessageBox>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
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
//
#include "DAAppSettingDialog.h"
#include "SettingPages/DAAppConfig.h"
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
    mCore       = &core;
    mUI         = qobject_cast< DAAppUI* >(core.getUiInterface());
    mDockArea   = mUI->getAppDockingArea();
    mController = new DAAppController(this);
    mController
            ->setAppMainWindow(this)                      // app
            .setAppCore(&core)                            // core
            .setAppActions(mUI->getAppActions())          // action
            .setAppCommand(mUI->getAppCmd())              // cmd
            .setAppDataManager(core.getAppDatas())        // data
            .setAppDockingArea(mUI->getAppDockingArea())  // dock
            .setAppRibbonArea(mUI->getAppRibbonArea())    // ribbon
            ;
    mController->initialize();
    //首次调用此函数会加载插件，可放置在main函数中调用
    init();
    DAGraphicsItemFactory::initialization();
    mConfig->loadConfig();
    mConfig->apply();
    retranslateUi();
    //    ribbonBar()->setRibbonStyle(SARibbonBar::WpsLiteStyleTwoRow);
    showMaximized();
}

AppMainWindow::~AppMainWindow()
{
    //    delete ui;
}

void AppMainWindow::retranslateUi()
{
    mUI->retranslateUi();
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

void AppMainWindow::init()
{
    //初始化配置文件，这个要在所有之前
    initConfig();
    //先初始化插件
    initPlugins();
    //初始化工作流的节点
    initWorkflowNodes();
    //应用所有配置
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
    mDockArea->getWorkflowNodeListWidget()->addItems(nodeMetaDatas);
    //此时才创建第一个workflow，这个workflow创建时，插件已经加载好
    mDockArea->getWorkFlowOperateWidget()->appendWorkflow(tr("untitle"));

    //执行一些必要的回调
    QList< DAAbstractNodePlugin* > nodeplugins = pluginmgr.getNodePlugins();
    for (DAAbstractNodePlugin* plugin : qAsConst(nodeplugins)) {
        plugin->afterLoadedNodes();
    }
}

void AppMainWindow::initConfig()
{
    mConfig = std::make_unique< DAAppConfig >();
    mConfig->setCore(mCore);
}

void AppMainWindow::onWorkflowFinished(bool success)
{
    if (success) {
        QMessageBox::information(this, tr("infomation"), tr("Topology execution completed"));  //拓扑执行完成
    } else {
        QMessageBox::critical(this, tr("infomation"), tr("Topology execution failed"));  //拓扑执行失败
    }
}

void AppMainWindow::onConfigNeedSave()
{
    mConfig->saveConfig();
}

DAAppConfig* AppMainWindow::getAppConfig() const
{
    return mConfig.get();
}

void AppMainWindow::showSettingDialog()
{
    if (nullptr == mSettingDialog) {
        //创建设置窗口
        mSettingDialog = new DAAppSettingDialog(this);
        connect(mSettingDialog, &DAAppSettingDialog::needSave, this, &AppMainWindow::onConfigNeedSave);
        mSettingDialog->buildUI(getAppConfig());
        DAAppPluginManager& pluginmgr      = DAAppPluginManager::instance();
        QList< DAAbstractPlugin* > plugins = pluginmgr.getAllPlugins();
        for (DAAbstractPlugin* p : qAsConst(plugins)) {
            DAAbstractSettingPage* page = p->createSettingPage();
            if (page) {
                mSettingDialog->settingWidget()->addPage(page);
            }
        }
    }
    mSettingDialog->exec();
}
