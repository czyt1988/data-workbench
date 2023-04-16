#include "DAAppUI.h"
#include "DAAppDockingArea.h"
#include "DAAppRibbonArea.h"
#include "DAAppActions.h"
#include "DAAppCommand.h"
#include "DAAppCore.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAAppDataManager.h"
#include "AppMainWindow.h"
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

void DAAppUI::createUi()
{
    createCmd();      // cmd必须先创建，因为Actions会用到cmd的
    createActions();  // Actions第二个创建
    createDockingArea();
    createRibbonArea();
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

void DAAppUI::createActions()
{
    m_actions = new DAAppActions(this);
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
