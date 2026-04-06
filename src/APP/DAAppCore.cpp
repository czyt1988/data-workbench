#include "DAAppCore.h"
#include <QFileInfo>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
// API
#include "DAAppUI.h"
#include "DAAppRibbonArea.h"
#include "DAAppDataManager.h"
#include "DACommandInterface.h"
#include "DAAppProject.h"
#include "DAAppCommand.h"
#if DA_ENABLE_PYTHON
// DA Python
#else
#include <QProcess>
#include <QList>
#include <QFileInfo>
#endif
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAAppCore
//===================================================
DAAppCore::DAAppCore(QObject* p) : DACoreInterface(p), mAppCmd(nullptr), mAppUI(nullptr), mProject(nullptr)
{
    mDataManager = nullptr;
}

DAAppCore& DAAppCore::getInstance()
{
    static DAAppCore s_core;
    return (s_core);
}

bool DAAppCore::initialized()
{
    // 初始化数据
    mDataManager = new DAAppDataManager(this, this);
    qDebug() << "core have been initialized App Data Manager";
    mProject = new DAAppProject(this, this);
    mProject->setDataManagerInterface(mDataManager);
    return true;
}

DAUIInterface* DAAppCore::getUiInterface() const
{
    return mAppUI;
}

DAProjectInterface* DAAppCore::getProjectInterface() const
{
    return mProject;
}

void DAAppCore::createUi(SARibbonMainWindow* mainwindow)
{
    mAppUI = new DAAppUI(mainwindow, this);
    mAppUI->createUi();
    mAppCmd = mAppUI->getAppCmd();
    if (mDataManager) {
        // 把dataManager的undo stack 注册
        if (mAppCmd) {
            mAppCmd->setDataManagerStack(mDataManager->getUndoStack());
        }
    }
}


DADataManagerInterface* DAAppCore::getDataManagerInterface() const
{
    return mDataManager;
}


/**
 * @brief 获取DAAppUI，省去qobject_cast
 * @return
 */
DAAppUI* DAAppCore::getAppUi()
{
    return mAppUI;
}
/**
 * @brief 获取工程
 * @return
 */
DAAppProject* DAAppCore::getAppProject()
{
    return mProject;
}
/**
 * @brief 直接获取数据
 * @return
 */
DAAppDataManager* DAAppCore::getAppDatas()
{
    return mDataManager;
}
/**
 * @brief 直接获取DAAppCommand
 *
 * @note 此函数必须在@ref createUi 之后调用才有实际结果
 * @return
 */
DAAppCommand* DAAppCore::getAppCmd()
{
    return mAppCmd;
}

DACoreInterface* getAppCorePtr()
{
    return &(DAAppCore::getInstance());
}

}  // end namespace DA
