#include "AppMainWindow.h"
// Qt 相关
#include <QMessageBox>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QVector>
#include <QDataStream>
#include <QCloseEvent>
#include <QFile>
#include <QBuffer>
#include <QSplashScreen>
#include <QApplication>
//
#include "SARibbonBar.h"
// 插件相关
#include "DAAppPluginManager.h"
#include "DAAbstractPlugin.h"
#include "DAAbstractNodePlugin.h"
// 界面相关
#include "DAAppController.h"
#include "DAAppCore.h"
#include "DAAppUI.h"
#include "DAAppDockingArea.h"
#include "DAAppRibbonArea.h"
#include "DAAppWorkFlowOperateWidget.h"
#include "DAAppProject.h"
// 对话框

// 节点相关
#include "DAPyNodeFactory.h"

//
#include "DAGraphicsItemFactory.h"
#include "DAPyWorkFlowNodeListWidget.h"
#include "DAPyWorkFlowOperateWidget.h"
//
#include "DAAppSettingDialog.h"
#include "SettingPages/DAAppConfig.h"
#include "DAAppProjectActionPolicy.h"
#include "DAAppWindowStateSerializer.h"
#include "DALogCategory.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"

namespace DA
{

//===================================================
// AppMainWindow
//===================================================
AppMainWindow::AppMainWindow(QWidget* parent) : SARibbonMainWindow(parent)
{
    // 查找活动的启动画面并更新消息的辅助函数
    auto updateSplash = [](const QString& msg) {
        const QWidgetList widgets = QApplication::topLevelWidgets();
        for (QWidget* w : widgets) {
            QSplashScreen* sp = qobject_cast< QSplashScreen* >(w);
            if (sp && sp->isVisible()) {
                sp->showMessage(msg, Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
                QApplication::processEvents();
                return;
            }
        }
    };

    // 标签可高亮
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    // 让dock可以最小化到一个标签
    ads::CDockManager::setAutoHideConfigFlags({ ads::CDockManager::DefaultAutoHideConfig });
    // 建立ribbonArea，此函数的构造函数会生成界面
    QIcon icon(QStringLiteral(":/app/bright/Icon/icon.svg"));
    setWindowIcon(icon);
    updateSplash(tr("Initializing core interface..."));  // cn:正在初始化核心接口...
    DAAppCore& core = DAAppCore::getInstance();
    // 创建界面
    updateSplash(tr("Creating user interface..."));  // cn:正在创建用户界面...
    core.createUi(this);
    mCore     = &core;
    mUI       = qobject_cast< DAAppUI* >(core.getUiInterface());
    mDockArea = mUI->getAppDockingArea();
    // 创建controller
    mController = new DAAppController(this);
    (*mController)
        .setAppMainWindow(this)                       // app
        .setAppCore(&core)                            // core
        .setAppActions(mUI->getAppActions())          // action
        .setAppCommand(mUI->getAppCmd())              // cmd
        .setAppDataManager(core.getAppDatas())        // data
        .setAppDockingArea(mUI->getAppDockingArea())  // dock
        .setAppRibbonArea(mUI->getAppRibbonArea())    // ribbon
        ;
    mController->initialize();
    ribbonBar()->setContentsMargins(3, 0, 3, 0);
    // 界面状态的加载要在init之前，因为inti的插件会改变界面，如果在之后就永远改变不了界面了
    bool hasUIStateFile = isHaveStateSettingFile();
    if (hasUIStateFile) {
        restoreUIState();
        daInfo.noquote() << tr("Restore UI state");  // cn:加载界面状态信息
    }
    // 首次调用此函数会加载插件，可放置在main函数中调用
    updateSplash(tr("Loading plugins..."));  // cn:正在加载插件...
    init();
    updateSplash(tr("Preparing interface..."));  // cn:正在准备界面...
    retranslateUi();
    setContentsMargins(3, 0, 3, 1);
    if (!hasUIStateFile) {
        ribbonBar()->setRibbonStyle(SARibbonBar::RibbonStyleCompactTwoRow);
        showMaximized();
    }
}

AppMainWindow::~AppMainWindow()
{
    mPluginMgr->unloadAllPlugins();
}

void AppMainWindow::retranslateUi()
{
    // [*]为改变标记占位符
    mController->updateWindowTitle();
    mUI->retranslateUi();
}

void AppMainWindow::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        qDebug() << tr("LanguageChange");  // cn:语言变更
        retranslateUi();
        break;

    default:
        break;
    }
}

/**
 * @brief 程序关闭事件
 * @param event
 */
void AppMainWindow::closeEvent(QCloseEvent* e)
{
    DAAppCloseAction closeAction = DAAppCloseAction::CloseDirectly;
    if (mController->isDirty()) {
        // 是否保存
        auto btn    = QMessageBox::question(this,
                                            tr("Question"),                          // cn:疑问
                                            tr("Do you need to save the project?"),  // cn:是否需要保存工程？
                                            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No
                                                | QMessageBox::StandardButton::Cancel,
                                            QMessageBox::StandardButton::Yes);
        closeAction = resolveAppCloseAction(true, toAppSavePromptChoice(btn));
    }
    if (DAAppCloseAction::CancelClosing == closeAction) {
        e->ignore();
        return;
    }
    if (DAAppCloseAction::SaveThenClose == closeAction) {
        if (!mController->save()) {
            e->ignore();
            return;
        }
    }
    QString uistateFile = getUIStateSettingFilePath();
    if (mIsSaveUIStateOnClose) {
        QFile file(uistateFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QDataStream st(&file);
            st << saveUIState();
            qDebug() << tr("Successfully saved UI state to %1").arg(uistateFile);  // cn:成功保存界面状态到%1
        } else {
            qDebug() << tr("Cannot open %1, because: %2").arg(uistateFile, file.errorString());  // cn:无法打开%1，原因：%2
        }
    } else {
        // 不保存要删除
        if (QFile::exists(uistateFile)) {
            if (!QFile::remove(uistateFile)) {
                qDebug() << tr("Cannot remove %1").arg(uistateFile);  // cn:无法删除%1
            }
        }
    }
    // Delete dock manager here to delete all floating widgets. This ensures
    // that all top level windows of the dock manager are properly closed
    mDockArea->dockManager()->deleteLater();
    SARibbonMainWindow::closeEvent(e);
}

void AppMainWindow::init()
{
    // 初始化配置文件，这个要在所有之前
    initConfig();
    // 初始化图元工厂
    DAGraphicsItemFactory::initialization();
    // 先初始化插件
    initPlugins();
    // 初始化工作流的节点
    initWorkflowNodes();
    // 应用所有配置
    mConfig->loadConfig();
    mConfig->apply();
    // 给project接口设置插件管理器
    mCore->getAppProject()->setPluginMgr(mPluginMgr);
}

void AppMainWindow::initPlugins()
{
    mPluginMgr      = new DAAppPluginManager(this);
    DAAppCore& core = DAAppCore::getInstance();
    mPluginMgr->loadAllPlugins(&core);
    // 加载完成后，把DAAppPluginManager赋值给DAAppWorkFlowOperateWidget
    DAAppWorkFlowOperateWidget* appWFO = qobject_cast< DAAppWorkFlowOperateWidget* >(mDockArea->getWorkFlowOperateWidget());
    if (appWFO) {
        appWFO->setPluginManager(mPluginMgr);
    }
}

void AppMainWindow::initWorkflowNodes()
{
    // 提取所有的元数据
    QList< DAPyNodeMetaData > nodeMetaDatas = mPluginMgr->getAllNodeMetaDatas();
    // 把数据写入toolbox
    mDockArea->getWorkflowNodeListWidget()->addItems(nodeMetaDatas);
    // 此时才创建第一个workflow，这个workflow创建时，插件已经加载好
    mDockArea->getWorkFlowOperateWidget()->appendWorkflow(tr("Untitled"));  // cn:未命名

    // 执行一些必要的回调
    QList< DAAbstractNodePlugin* > nodeplugins = mPluginMgr->getNodePlugins();
    for (DAAbstractNodePlugin* plugin : std::as_const(nodeplugins)) {
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
        QMessageBox::information(this, tr("Information"), tr("Topology execution completed"));  // cn:信息,cn:拓扑执行完成
    } else {
        QMessageBox::critical(this, tr("Information"), tr("Topology execution failed"));  // cn:信息,cn:拓扑执行失败
    }
}

void AppMainWindow::onConfigNeedSave()
{
    mConfig->saveConfig();
}

bool AppMainWindow::isSaveUIStateOnClose() const
{
    return mIsSaveUIStateOnClose;
}

void AppMainWindow::setSaveUIStateOnClose(bool v)
{
    mIsSaveUIStateOnClose = v;
}

QString AppMainWindow::getUIStateSettingFilePath()
{
    return QDir::toNativeSeparators(QString("%1/.dawork-ui-state").arg(DAAbstractSettingPage::getConfigFileSavePath()));
}

/**
 * @brief 判断是否存在状态设置文件
 * @return
 */
bool AppMainWindow::isHaveStateSettingFile()
{
    return QFileInfo::exists(getUIStateSettingFilePath());
}

/**
 * @brief 把保存的窗口状态保存文件删除
 * @return 成功删除了文件返回true，如果文件不存在，也返回true
 */
bool AppMainWindow::removeStateSettingFile()
{
    QString path = getUIStateSettingFilePath();
    QFile f(path);
    if (!f.exists()) {
        return true;
    }
    return f.remove();
}

/**
 * @brief 打开已有工程
 *
 * @param 工程路径
 * @return 成功打开返回true，否则返回false
 */
bool AppMainWindow::openProject(const QString& projectFilePath)
{
    return mController->openProjectFile(projectFilePath);
}

/**
 * @brief 针对import-data命令
 * @param filePath
 * @param args
 * @return
 */
bool AppMainWindow::importData(const QString& filePath, const QVariantMap& args)
{
    return mController->importData(filePath, args);
}

/**
 * @brief 返回插件管理器
 * @return
 */
DAAppPluginManager* AppMainWindow::getPluginManager() const
{
    return mPluginMgr;
}

DAAppConfig* AppMainWindow::getAppConfig() const
{
    return mConfig.get();
}

/**
 * @brief 显示设置对话框
 */
void AppMainWindow::showSettingDialog()
{
    if (nullptr == mSettingDialog) {
        // 创建设置窗口
        mSettingDialog = new DAAppSettingDialog(this);
        connect(mSettingDialog, &DAAppSettingDialog::needSave, this, &AppMainWindow::onConfigNeedSave);
        mSettingDialog->buildUI(getAppConfig());

        QList< DAAbstractPlugin* > plugins = mPluginMgr->getAllPlugins();
        for (DAAbstractPlugin* p : std::as_const(plugins)) {
            DAAbstractSettingPage* page = p->createSettingPage();
            if (page) {
                mSettingDialog->settingWidget()->addPage(page);
            }
        }
    }
    mSettingDialog->exec();
}

/**
 * @brief 保存所有状态
 *
 * 包含了QMainWindow::saveGeometry,QMainWindow::saveState,ads::CDockManager::saveState
 * @return
 */
QByteArray AppMainWindow::saveUIState() const
{
    DAAppWindowStateSnapshot snapshot;
    snapshot.geometry        = saveGeometry();
    snapshot.mainWindowState = saveState();
    if (mDockArea) {
        snapshot.dockingState = mDockArea->dockManager()->saveState();
    }
    return serializeAppWindowState(snapshot);
}

/**
 * @brief 恢复状态
 * @param v
 * @return
 */
bool AppMainWindow::restoreUIState(const QByteArray& v)
{
    DAAppWindowStateSnapshot snapshot;
    if (!deserializeAppWindowState(v, &snapshot)) {
        daCritical << tr("failed to restore UI state");  // cn:恢复界面状态过程中出错
        return false;
    }
    if (!snapshot.geometry.isEmpty()) {
        restoreGeometry(snapshot.geometry);
    }
    if (!snapshot.mainWindowState.isEmpty()) {
        restoreState(snapshot.mainWindowState);
    }
    if (!snapshot.dockingState.isEmpty() && mDockArea) {
        mDockArea->dockManager()->restoreState(snapshot.dockingState);
    }
    return true;
}

bool AppMainWindow::restoreUIState()
{
    QString uistateFile = getUIStateSettingFilePath();
    QFile file(uistateFile);
    if (!file.open(QIODevice::ReadOnly)) {
        daCritical << tr("cannot read UI state file %1: %2").arg(uistateFile, file.errorString());  // cn:无法读取界面状态文件%1，原因：%2
        return false;
    }
    QByteArray res;
    QDataStream st(&file);
    st >> res;
    return restoreUIState(res);
}

void AppMainWindow::resetUIState()
{
    // TODO:重置ui
}

}  // end DA namespace
