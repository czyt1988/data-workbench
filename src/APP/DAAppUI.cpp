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
// SARibbon
#include "SARibbonPanel.h"
// 以下两个头文件用于让编译器获知 DAPyWorkFlowOperateWidget/DAPyWorkFlowNodeListWidget
// 继承自 QWidget，从而可隐式转换为 hideDockWidget(QWidget*) 的参数。
#include "DAPyWorkFlowOperateWidget.h"
#include "DAPyWorkFlowNodeListWidget.h"

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
 * @brief 设置功能模块的整体UI可见性
 *
 * 一次调用隐藏/显示该功能关联的全部ribbon标签、panel、上下文标签、dock与action，
 * 插件裁剪主程序功能时使用此接口，替代逐个隐藏具体控件（具体控件清单是主程序的实现细节，
 * 后续扩展不应导致已发布的插件出现漏隐藏）。
 *
 * 这是UI级裁剪而非能力锁：被隐藏功能对应的action/command仍可能被插件或agent直接触发。
 *
 * @param feature 功能模块，当前仅支持 Workflow
 * @param on true显示，false隐藏
 */
void DAAppUI::setFeatureVisible(DAWorkbenchFeatureType feature, bool on)
{
    switch (feature) {
    case DAWorkbenchFeatureType::Workflow: {
        // 两个workflow相关的dock
        QWidget* wfOperateWidget = dockingArea->getWorkFlowOperateWidget();
        QWidget* wfNodeListWidget = dockingArea->getWorkflowNodeListWidget();
        if (on) {
            dockingArea->showDockWidget(wfOperateWidget);
            dockingArea->showDockWidget(wfNodeListWidget);
        } else {
            dockingArea->hideDockWidget(wfOperateWidget);
            dockingArea->hideDockWidget(wfNodeListWidget);
        }
        // 主页创建面板下的「新建工作流」按钮（工作流入口随功能显隐）
        actions->actionWorkflowNew->setVisible(on);
        // workflow上下文标签（编辑工作流时出现的浮动标签组），使用类型化枚举接口
        if (on) {
            ribbonArea->showContextCategory(DAAppRibbonArea::ContextCategoryWorkflow);
        } else {
            ribbonArea->hideContextCategory(DAAppRibbonArea::ContextCategoryWorkflow);
        }
        // 视图标签下的workflow相关action
        actions->actionShowWorkFlowArea->setVisible(on);
        actions->actionShowWorkFlowManagerArea->setVisible(on);
        break;
    }
    default:
        qWarning() << "DAAppUI::setFeatureVisible: feature" << static_cast< int >(feature)
                   << "is not supported yet, only Workflow is supported";  // cn:暂不支持该功能模块的整体显隐，当前仅支持Workflow
        break;
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
