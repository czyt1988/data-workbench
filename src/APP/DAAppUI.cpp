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
#include "DALog.h"
#include "DAPropertyFormDialog.h"

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

DAAppUI::~DAAppUI()
{
    qDeleteAll(cachePropertyDialog);
    cachePropertyDialog.clear();
}

QMainWindow* DAAppUI::getMainWindow() const
{
    return static_cast< QMainWindow* >(ribbonArea->app());
}

DADockingAreaInterface* DAAppUI::getDockingArea()
{
    return dockingArea;
}

DARibbonAreaInterface* DAAppUI::getRibbonArea()
{
    return ribbonArea;
}

DAStatusBarInterface* DAAppUI::getStatusBar()
{
    return statusBar;
}

/**
 * @brief 执行统一的表单配置窗口，来获取设置信息
 *
 * 输入为 v2 表单 schema（JSON 字符串），由 @ref DAPropertyFormDialog 渲染。
 * 当传入 cacheKey 时，首次构建的对话框会被缓存，后续相同 cacheKey 的调用直接 exec 已缓存的对话框。
 * @param jsonConfig v2 表单 schema 的 JSON 字符串
 * @param parent 父窗口
 * @param cacheKey 缓存关键字，非空时启用对话框缓存
 * @return 用户确认时返回字段值组成的 QJsonObject，取消或加载失败时返回空对象
 */
QJsonObject DAAppUI::getConfigValues(const QString& jsonConfig, QWidget* parent, const QString& cacheKey)
{
    if (!cacheKey.isEmpty()) {
        DAPropertyFormDialog* dialog = cachePropertyDialog.value(cacheKey, nullptr);
        if (!dialog) {
            // 缓存的对话框使用 mainWindow() 作为 parent，确保稳定的生命周期管理
            // 不使用调用方传入的 parent，因为 parent 可能为 nullptr 或临时窗口
            dialog = new DAPropertyFormDialog(mainWindow());
            if (!dialog->loadFromJson(jsonConfig)) {
                qWarning() << QString("Failed to load form config for settings dialog");  // cn:无法加载设置对话框的表单配置
                delete dialog;
                return QJsonObject();
            }
            cachePropertyDialog[ cacheKey ] = dialog;
        }

        if (QDialog::Accepted == dialog->exec()) {
            return QJsonObject::fromVariantMap(dialog->values());
        }
        return QJsonObject();
    }
    // 非缓存路径，使用调用方传入的 parent
    return DAPropertyFormDialog::showSettingsDialog(jsonConfig, parent);
}

void DAAppUI::createUi()
{
    createCmd();      // cmd必须先创建，因为Actions会用到cmd的
    createActions();  // Actions第二个创建
    createDockingArea();
    createRibbonArea();
    ribbonArea->setDockingArea(dockingArea);
    createStatusBar();
    statusBar->setAppDockingArea(dockingArea);
    statusBar->setAppActions(actions);
}

void DAAppUI::addInfoLogMessage(const QString& msg, bool showInStatusBar)
{
    daInfo.noquote() << msg;
    if (showInStatusBar) {
        statusBar->showMessage(msg);
    }
}

void DAAppUI::addWarningLogMessage(const QString& msg, bool showInStatusBar)
{
    daWarning.noquote() << msg;
    if (showInStatusBar) {
        statusBar->showMessage(msg);
    }
}

void DAAppUI::addCriticalLogMessage(const QString& msg, bool showInStatusBar)
{
    daCritical.noquote() << msg;
    if (showInStatusBar) {
        statusBar->showMessage(msg);
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
    return actions;
}

DAAppCommand* DAAppUI::getAppCmd()
{
    return cmd;
}

DAAppDockingArea* DAAppUI::getAppDockingArea()
{
    return dockingArea;
}

DAAppRibbonArea* DAAppUI::getAppRibbonArea()
{
    return ribbonArea;
}

DAAppStatusBar* DAAppUI::getAppStatusBar()
{
    return statusBar;
}

void DAAppUI::createActions()
{
    actions = new DAAppActions(this);
    actions->retranslateUi();  // 显示调用文字翻译
    registeAction(actions);
}

void DAAppUI::createCmd()
{
    cmd = new DAAppCommand(this);
    registeCommand(cmd);
}

void DAAppUI::createDockingArea()
{
    dockingArea = new DAAppDockingArea(this);
    registeExtend(dockingArea);
}

void DAAppUI::createRibbonArea()
{
    ribbonArea = new DAAppRibbonArea(this);
    registeExtend(ribbonArea);
}

void DAAppUI::createStatusBar()
{

    statusBar = new DAAppStatusBar(this);
    registeExtend(statusBar);
}
