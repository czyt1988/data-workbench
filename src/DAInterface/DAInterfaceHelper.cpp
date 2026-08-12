#include "DAInterfaceHelper.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
#include "DARibbonAreaInterface.h"
#include "DAStatusBarInterface.h"
#include "SARibbonMainWindow.h"
#include "DACommandInterface.h"
namespace DA
{
/**
 * @brief 构造函数
 */
DAInterfaceHelper::DAInterfaceHelper()
{
}

/**
 * @brief 析构函数
 */
DAInterfaceHelper::~DAInterfaceHelper()
{
}

/**
 * @brief 初始化，通过核心接口获取所有子接口
 * @param core 核心接口指针
 */
void DAInterfaceHelper::initialize(DACoreInterface* core)
{
    mCore = core;
    if (!core) {
        return;
    }
    mUi            = core->getUiInterface();
    mDataManager   = core->getDataManagerInterface();
    mProject       = core->getProjectInterface();
    if (mUi) {
        mDockArea      = mUi->getDockingArea();
        mRibbonArea    = mUi->getRibbonArea();
        mStatusBarArea = mUi->getStatusBar();
        mCmd           = mUi->getCommandInterface();
    }
}

/**
 * @brief 获取核心接口
 * @return 核心接口指针
 */
DACoreInterface* DAInterfaceHelper::core() const
{
    return mCore;
}

/**
 * @brief 获取UI接口
 * @return UI接口指针
 */
DAUIInterface* DAInterfaceHelper::uiInterface() const
{
    return mUi;
}

/**
 * @brief 获取停靠区域接口
 * @return 停靠区域接口指针
 */
DADockingAreaInterface* DAInterfaceHelper::dockAreaInterface() const
{
    return mDockArea;
}

/**
 * @brief 获取Ribbon区域接口
 * @return Ribbon区域接口指针
 */
DARibbonAreaInterface* DAInterfaceHelper::ribbonAreaInterface() const
{
    return mRibbonArea;
}

/**
 * @brief 获取状态栏区域接口
 * @return 状态栏区域接口指针
 */
DAStatusBarInterface* DAInterfaceHelper::statusBarAreaInterface() const
{
    return mStatusBarArea;
}

/**
 * @brief 获取数据管理接口
 * @return 数据管理接口指针
 */
DADataManagerInterface* DAInterfaceHelper::dataManagerInterface() const
{
    return mDataManager;
}

/**
 * @brief 获取命令接口
 * @return 命令接口指针
 */
DACommandInterface* DAInterfaceHelper::commandInterface() const
{
    return mCmd;
}

/**
 * @brief 获取工程接口
 * @return 工程接口指针
 */
DAProjectInterface* DAInterfaceHelper::projectInterface() const
{
    return mProject;
}

/**
 * @brief 获取主窗口
 * @return 主窗口指针
 */
QMainWindow* DAInterfaceHelper::mainWindow() const
{
    return mUi->mainWindow();
}
}
