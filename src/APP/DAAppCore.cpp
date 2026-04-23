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
#include "DAPyInterpreter.h"
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
#if DA_ENABLE_PYTHON
    // 初始化Python环境路径（在插件加载之前完成，确保sys.path包含PyScripts）
    initPythonEnvPaths();
#endif
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

#if DA_ENABLE_PYTHON
/**
 * @brief 初始化Python环境路径
 *
 * 将DAPyWorkFlow的PyScripts源码路径添加到Python sys.path中，
 * 确保在开发环境下DAWorkFlowPy模块可被正确导入。
 * 此方法在initialized()中调用，在插件加载之前完成路径配置。
 *
 * 添加的路径：
 * - 源码路径：用于开发环境，指向src/DAPyWorkFlow/PyScripts目录
 * - 安装路径：用于发布环境，指向applicationDirPath/PyScripts（由initPyNodeFactory添加）
 *
 * @note 此函数仅添加源码路径，安装路径由DAAppPluginManager::initPyNodeFactory()负责
 */
void DAAppCore::initPythonEnvPaths()
{
    if (!DAPyInterpreter::isPythonInitialized()) {
        qWarning() << tr("Python interpreter not initialized, skip Python env paths setup");
        return;
    }
    // 添加源码路径（开发环境使用）
    QString sourcePyScriptsPath = QApplication::applicationDirPath() + "/../src/DAPyWorkFlow/PyScripts";
    QDir sourceDir(sourcePyScriptsPath);
    if (sourceDir.exists()) {
        DAPyInterpreter::appendSysPath(sourceDir.absolutePath());
        qInfo() << tr("Added Python source path: %1").arg(sourceDir.absolutePath());
    }
}
#endif

}  // end namespace DA
