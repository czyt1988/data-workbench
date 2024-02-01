#include "DAAppCore.h"
#include <QFileInfo>
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
// API
#include "DAAppUI.h"
#include "DAAppRibbonArea.h"
#include "DAAppDataManager.h"
#include "DACommandInterface.h"
#include "DAAppProject.h"
#include "DAAppCommand.h"
#ifdef DA_ENABLE_PYTHON
// DA Python
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

using namespace DA;

//===================================================
// DAAppCore
//===================================================
DAAppCore::DAAppCore(QObject* p)
    : DACoreInterface(p), mAppUI(nullptr), mAppCmd(nullptr), mIsPythonInterpreterInitialized(false), mProject(nullptr)
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
#ifdef DA_ENABLE_PYTHON
    // 初始化python环境
    qDebug() << "begin app core initialized";
    initializePythonEnv();
    qDebug() << "core have been initialized Python Env";
#endif
    // 初始化数据
    mDataManager = new DAAppDataManager(this, this);
    qDebug() << "core have been initialized App Data Manager";
    mProject = new DAAppProject(this, this);
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
#ifdef DA_ENABLE_PYTHON
    try {
        DA::DAPyInterpreter& python = DA::DAPyInterpreter::getInstance();
        // 初始化python环境
        QString pypath = getPythonInterpreterPath();
        qInfo() << tr("Python interpreter path is %1").arg(pypath);
        python.setPythonHomePath(pypath);
        python.initializePythonInterpreter();
        // 初始化环境成功后，加入脚本路径

        DA::DAPyScripts& scripts = DA::DAPyScripts::getInstance();
        QString scriptPath       = getPythonScriptsPath();
        qInfo() << tr("Python scripts path is %1").arg(scriptPath);
        scripts.appendSysPath(scriptPath);
        if (!scripts.initScripts()) {
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

/**
 * @brief 获取Python环境路径
 * @return
 */
QString DAAppCore::getPythonInterpreterPath()
{
    QString appabsPath = QApplication::applicationDirPath();
#ifdef DA_ENABLE_PYTHON
    QList< QFileInfo > paths = DAPyInterpreter::wherePython();
    if (paths.empty()) {
        return QDir::toNativeSeparators(appabsPath + "/Python");
    }
    return paths.first().absolutePath();
#else
    QProcess process;
    QString command = "where python";
    process.start(command);
    if (!process.waitForFinished()) {
        return QString();
    }
    QString res = process.readAll();
    qDebug() << res;
    const QList< QString > pys = res.split("\r\n");
    QList< QFileInfo > validFis;
    // 遍历所有环境，确认是否的确是ptython路径,where 有时候会返回一些不正确的路径
    for (const QString& p : pys) {
        QFileInfo fi(p);
        if (fi.isExecutable()) {
            // 说明是可执行文件，windows下就是pythhon.exe
            validFis.append(fi);
        }
    }
    if (validFis.empty()) {
        return QString();
    }
    return validFis.first().absolutePath();
#endif
}

/**
 * @brief 获取python脚本的位置
 * @return
 */
QString DAAppCore::getPythonScriptsPath()
{
    QString appabsPath = QApplication::applicationDirPath();
    return QDir::toNativeSeparators(appabsPath + "/PyScripts");
}
