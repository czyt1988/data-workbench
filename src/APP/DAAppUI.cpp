#include "DAAppUI.h"
#include <QDebug>
#include "DAAppDockingArea.h"
#include "DAAppRibbonArea.h"
#include "DAAppActions.h"
#include "DAAppCommand.h"
#include "DAAppCore.h"
#include "DAAppDataManager.h"
#include "AppMainWindow.h"
#include "DAAppStatusBar.h"
#include "DACommonPropertySettingDialog.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppUI
//===================================================
DAAppUI::DAAppUI(SARibbonMainWindow* m, DACoreInterface* c) : DAUIInterface(m, c)
{
	//! 这里不进行createUi的调用，因为很多地方的窗口的构建需要DAAppActions，
	//! 而DAAppActions又依赖DAAppUI，在DAAppCore构建DAAppUI时，如果在DAAppUI的构造函数中调用createUi
	//! 那么会导致DAAppUI构造过程中调用createDockingArea，而createDockingArea是创建窗口的主要函数，
	//! 很多窗口的创建又依赖DAAppActions，虽然DAAppActions已经创建，但如果把createUi放到DAAppUI构造函数中，
	//! 此时DAAppUI还未构造完成，DAAppUI未构造完成就导致DAAppCore还无法持有DAAppUI指针，
	//! 那么createDockingArea构造各种窗口时就无法通过DA_APP_UI_ACTIONS宏（DA::DAAppCore::getInstance().getUi()->getActions()）来获取action
	//!
	//! 因此createUi要等DAAppCore持有DAAppUI指针后再调用
	//!
}

QMainWindow* DAAppUI::getMainWindow() const
{
	return static_cast< QMainWindow* >(m_ribbonArea->app());
}

DADockingAreaInterface* DAAppUI::getDockingArea()
{
	return m_dockingArea;
}

DARibbonAreaInterface* DAAppUI::getRibbonArea()
{
	return m_ribbonArea;
}

DAStatusBarInterface* DAAppUI::getStatusBar()
{
	return m_statusBar;
}

/**
 * @brief 执行一个通用的设置窗口，来获取设置信息，传入内容为构建窗口的设置信息
 *
 * 具体json的设置见@ref DACommonPropertySettingDialog
 * @param jsonConfig json设置文件内容
 * @param parent 父窗口
 * @param cacheKey 缓存关键字，如果缓存关键字有值，优先匹配缓存的窗口，如果匹配中
 * @param defaultTitle
 * @return
 */
QJsonObject DAAppUI::getConfigValues(const QString& jsonConfig, QWidget* parent, const QString& cacheKey)
{
	if (!cacheKey.isEmpty()) {
		DACommonPropertySettingDialog* dialog = m_cachePropertyDialog.value(cacheKey, nullptr);
		if (!dialog) {
			dialog = new DACommonPropertySettingDialog(parent);
			if (!dialog->loadFromJson(jsonConfig)) {
				qDebug() << tr("Failed to load JSON config for settings dialog");
				return QJsonObject();
			}
			m_cachePropertyDialog[ cacheKey ] = dialog;
		}

		if (QDialog::Accepted == dialog->exec()) {
			return dialog->getCurrentValues();
		}
		return QJsonObject();
	}
	// 创建一个配置对话框
	return DACommonPropertySettingDialog::showSettingsDialog(jsonConfig, parent);
}

void DAAppUI::createUi()
{
	createCmd();      // cmd必须先创建，因为Actions会用到cmd的
	createActions();  // Actions第二个创建
	createDockingArea();
	createRibbonArea();
	m_ribbonArea->setDockingArea(m_dockingArea);
	createStatusBar();
	m_statusBar->setAppDockingArea(m_dockingArea);
	m_statusBar->setAppActions(m_actions);
}

void DAAppUI::addInfoLogMessage(const QString& msg, bool showInStatusBar)
{
	qInfo().noquote() << msg;
	if (showInStatusBar) {
		m_statusBar->showMessage(msg);
	}
}

void DAAppUI::addWarningLogMessage(const QString& msg, bool showInStatusBar)
{
	qWarning().noquote() << msg;
	if (showInStatusBar) {
		m_statusBar->showMessage(msg);
	}
}

void DAAppUI::addCriticalLogMessage(const QString& msg, bool showInStatusBar)
{
	qCritical().noquote() << msg;
	if (showInStatusBar) {
		m_statusBar->showMessage(msg);
	}
}

void DAAppUI::setDirty(bool on)
{
	QMainWindow* mw = mainWindow();
	if (mw) {
		mw->setWindowModified(on);
	}
}

/**
 * @brief 获取app core
 * @return
 */
DAAppCore* DAAppUI::getAppCore()
{
	return qobject_cast< DAAppCore* >(core());
}

DAAppActions* DAAppUI::getAppActions()
{
	return m_actions;
}

DAAppCommand* DAAppUI::getAppCmd()
{
	return m_cmd;
}

DAAppDockingArea* DAAppUI::getAppDockingArea()
{
	return m_dockingArea;
}

DAAppRibbonArea* DAAppUI::getAppRibbonArea()
{
	return m_ribbonArea;
}

DAAppStatusBar* DAAppUI::getAppStatusBar()
{
	return m_statusBar;
}

void DAAppUI::createActions()
{
	m_actions = new DAAppActions(this);
	m_actions->retranslateUi();  // 显示调用文字翻译
	registeAction(m_actions);
}

void DAAppUI::createCmd()
{
	m_cmd = new DAAppCommand(this);
	registeCommand(m_cmd);
}

void DAAppUI::createDockingArea()
{
	m_dockingArea = new DAAppDockingArea(this);
	registeExtend(m_dockingArea);
}

void DAAppUI::createRibbonArea()
{
	m_ribbonArea = new DAAppRibbonArea(this);
	registeExtend(m_ribbonArea);
}

void DAAppUI::createStatusBar()
{

	m_statusBar = new DAAppStatusBar(this);
	registeExtend(m_statusBar);
}
