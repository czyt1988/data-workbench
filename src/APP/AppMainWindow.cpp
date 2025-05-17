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
// 对话框

// 节点相关
#include "DANodeMetaData.h"

//
#include "DAGraphicsItemFactory.h"
#include "DAWorkFlowNodeListWidget.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAWorkFlowOperateWidget.h"
//
#include "DAAppSettingDialog.h"
#include "SettingPages/DAAppConfig.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
namespace DA
{

//===================================================
// AppMainWindow
//===================================================
AppMainWindow::AppMainWindow(QWidget* parent) : SARibbonMainWindow(parent)
{
	// 标签可高亮
	ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
	// 让dock可以最小化到一个标签
	ads::CDockManager::setAutoHideConfigFlags({ ads::CDockManager::DefaultAutoHideConfig });
	// 建立ribbonArea，此函数的构造函数会生成界面
	QIcon icon(QStringLiteral(":/app/bright/Icon/icon.svg"));
	setWindowIcon(icon);
	DAAppCore& core = DAAppCore::getInstance();
	// 创建界面
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
		qInfo().noquote() << tr("Restore UI State");  // cn:加载界面状态信息
	}
	// 首次调用此函数会加载插件，可放置在main函数中调用
	init();
	retranslateUi();  // 非必要可以验证调用是否正常
	if (!hasUIStateFile) {
		ribbonBar()->setRibbonStyle(SARibbonBar::RibbonStyleCompactTwoRow);
		showMaximized();
	}
}

AppMainWindow::~AppMainWindow()
{
	//    delete ui;
}

void AppMainWindow::retranslateUi()
{
	// [*]为改变标记占位符
	setWindowTitle(tr("Data WorkFlow [*]"));
	mUI->retranslateUi();
}

void AppMainWindow::changeEvent(QEvent* e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		qDebug() << tr("LanguageChange");
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
	// 判断是否需要保存
	if (mController->isDirty()) {
		// 是否保存
		auto btn = QMessageBox::question(this,
		                                 tr("Question"),                          // cn:疑问
		                                 tr("Do you need to save the project?"),  // cn:是否需要保存工程？
		                                 QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No
		                                     | QMessageBox::StandardButton::Cancel,
		                                 QMessageBox::StandardButton::Yes);
		if (QMessageBox::StandardButton::Yes == btn) {
			mController->save();
		} else if (QMessageBox::StandardButton::Cancel == btn) {
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
			qDebug() << tr("success save ui state to %1").arg(uistateFile);
		} else {
			qDebug() << tr("can not open %1,because:%2").arg(uistateFile, file.errorString());
		}
	} else {
		// 不保存要删除
		if (QFile::exists(uistateFile)) {
			if (!QFile::remove(uistateFile)) {
				qDebug() << tr("can not remove %1").arg(uistateFile);
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
	// 提取所有的元数据
	QList< DANodeMetaData > nodeMetaDatas = pluginmgr.getAllNodeMetaDatas();
	// 把数据写入toolbox
	mDockArea->getWorkflowNodeListWidget()->addItems(nodeMetaDatas);
	// 此时才创建第一个workflow，这个workflow创建时，插件已经加载好
	mDockArea->getWorkFlowOperateWidget()->appendWorkflow(tr("untitle"));

	// 执行一些必要的回调
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
		QMessageBox::information(this, tr("infomation"), tr("Topology execution completed"));  // 拓扑执行完成
	} else {
		QMessageBox::critical(this, tr("infomation"), tr("Topology execution failed"));  // 拓扑执行失败
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

/**
 * @brief 保存所有状态
 *
 * 包含了QMainWindow::saveGeometry,QMainWindow::saveState,ads::CDockManager::saveState
 * @return
 */
QByteArray AppMainWindow::saveUIState() const
{
	QVector< QByteArray > uiStateArr;
	uiStateArr << saveGeometry() << saveGeometry();
	if (mDockArea) {
		uiStateArr << mDockArea->dockManager()->saveState();
	}
	QByteArray res;
	QBuffer buffer(&res);
	buffer.open(QIODevice::WriteOnly);
	QDataStream st(&buffer);
	st << uiStateArr;
	return res;
}

/**
 * @brief 恢复状态
 * @param v
 * @return
 */
bool AppMainWindow::restoreUIState(const QByteArray& v)
{
	QVector< QByteArray > uiStateArr;

	try {
		QDataStream st(v);
		st >> uiStateArr;
		if (1 <= uiStateArr.size()) {
			restoreGeometry(uiStateArr.at(0));
		}
		if (2 <= uiStateArr.size()) {
			restoreState(uiStateArr.at(1));
		}
		if (3 <= uiStateArr.size()) {
			if (mDockArea) {
				mDockArea->dockManager()->restoreState(uiStateArr.at(2));
			}
		}
	} catch (const std::exception& e) {
		qCritical() << tr("restore UI state error:%1").arg(e.what());  // 恢复状态过程中出错:%1
	}
	return true;
}

bool AppMainWindow::restoreUIState()
{
	QString uistateFile = getUIStateSettingFilePath();
	QFile file(uistateFile);
	if (!file.open(QIODevice::ReadOnly)) {
		qCritical() << tr("can not read ui state file %1,because %2").arg(uistateFile, file.errorString());
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
