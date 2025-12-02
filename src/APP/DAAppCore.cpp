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
#include <pybind11/embed.h>  //注入python使用
#include "DAPyInterpreter.h"
#include "DAPyScripts.h"
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
DAAppCore::DAAppCore(QObject* p)
    : DACoreInterface(p), mAppCmd(nullptr), mAppUI(nullptr), mIsPythonInterpreterInitialized(false), mProject(nullptr)
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
#if DA_ENABLE_PYTHON
    // 初始化python环境
    qDebug() << "begin app core initialized";
    initializePythonEnv();
    qDebug() << "core have been initialized Python Env";
#endif
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

/**
 * @brief python内核是否初始化成功
 * @return
 */
bool DAAppCore::isPythonInterpreterInitialized() const
{
    return mIsPythonInterpreterInitialized;
}

DADataManagerInterface* DAAppCore::getDataManagerInterface() const
{
    return mDataManager;
}

/**
 * @brief 初始化python环境
 * @return
 */
bool DAAppCore::initializePythonEnv()
{
    mIsPythonInterpreterInitialized = false;
#if DA_ENABLE_PYTHON
    try {
        DA::DAPyInterpreter& python = DA::DAPyInterpreter::getInstance();
        // 初始化python环境
        QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
        qInfo() << tr("Python interpreter path is %1").arg(pypath);
        QFileInfo fi(pypath);
        python.setPythonHomePath(fi.absolutePath());
        python.initializePythonInterpreter();
        // 初始化环境成功后，加入脚本路径

        // 把脚本路径加载到系统路径下，这样才能引入库
        QString scriptPath = getPythonScriptsPath();
        qInfo() << tr("Python scripts path is %1").arg(scriptPath);
        DA::DAPyScripts::appendSysPath(scriptPath);

        // DA::DAPyScripts::appendSysPath必须在getInstance前执行
        DA::DAPyScripts& scripts = DA::DAPyScripts::getInstance();
        if (!scripts.isInitScripts()) {
            qCritical() << tr("Scripts initialize error");
            return false;
        }
    } catch (const std::exception& e) {
        qCritical() << tr("Initialize python environment error:%1").arg(e.what());
        return false;
    }
    mIsPythonInterpreterInitialized = true;
#endif
    return true;
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
