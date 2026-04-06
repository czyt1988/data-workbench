#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include <QPointer>
#include <QApplication>
#include "DAProjectInterface.h"
#if DA_ENABLE_PYTHON
// DA Python
#include "DAPyInterpreter.h"
#include "DAPyScripts.h"
#include "DAPythonSignalHandler.h"
#else
#include <QProcess>
#include <QList>
#include <QFileInfo>
#endif
namespace DA
{

class DACoreInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DACoreInterface)
public:
    PrivateData(DACoreInterface* p);

public:
    QTemporaryDir mTempDir;
#if DA_ENABLE_PYTHON
    bool isPythonInterpreterInitialized;
    std::unique_ptr< DA::DAPyInterpreter > pythonInterpreter;
    std::unique_ptr< DAPythonSignalHandler > pythonHandler;
    std::unique_ptr< DAPyScripts > daScripts;
    static DAPyScripts* s_daScripts;
#endif
};
DAPyScripts* DACoreInterface::PrivateData::s_daScripts = nullptr;

DACoreInterface::PrivateData::PrivateData(DACoreInterface* p) : q_ptr(p)
{
}

DACoreInterface::DACoreInterface(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->mTempDir.setAutoRemove(true);
#if DA_ENABLE_PYTHON
    initializePythonEnv();
#endif
}

DACoreInterface::~DACoreInterface()
{
#if DA_ENABLE_PYTHON
    // 析构过程需要先析构脚本，再析构解释器，否则会导致异常
    d_ptr->daScripts.reset();
    d_ptr->pythonInterpreter.reset();
#endif
}

#if DA_ENABLE_PYTHON
bool DACoreInterface::initializePythonEnv()
{
    DA_D(d);
    d->isPythonInterpreterInitialized = false;
    try {
        d->pythonInterpreter = std::make_unique< DAPyInterpreter >();
        // 初始化python环境
        QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
        qInfo() << tr("Python interpreter path is %1").arg(pypath);
        QFileInfo fi(pypath);
        d->pythonInterpreter->setPythonHomePath(fi.absolutePath());
        d->pythonInterpreter->initializePythonInterpreter();
        // 初始化da脚本
        d->daScripts = std::make_unique< DAPyScripts >();
        // 初始化环境成功后，加入脚本路径

        // 把脚本路径加载到系统路径下，这样才能引入库
        QString scriptPath = getPythonScriptsPath();
        qInfo() << tr("Python scripts path is %1").arg(scriptPath);
        d->daScripts->appendSysPath(scriptPath);

        // DA::DAPyScripts::appendSysPath必须在getInstance前执行
        if (!(d->daScripts->isInitScripts())) {
            qCritical() << tr("Scripts initialize error");
            return false;
        }
        // 把da脚本赋值给静态变量
        PrivateData::s_daScripts = d->daScripts.get();
        // 初始化python信号投递器
        d->pythonHandler = std::make_unique< DAPythonSignalHandler >();
    } catch (const std::exception& e) {
        qCritical() << tr("Initialize python environment error:%1").arg(e.what());
        return false;
    }
    d->isPythonInterpreterInitialized = true;
    return true;
}

DAPythonSignalHandler* DACoreInterface::getPythonSignalHandler() const
{
    return d_ptr->pythonHandler.get();
}

/**
 * @brief 获取python脚本的位置
 *
 * 默认情况下，在初始化python环境后，脚本位置会被加入到python的系统目录中
 * @return
 */
QString DACoreInterface::getPythonScriptsPath()
{
    QString appabsPath = QApplication::applicationDirPath();
    return QDir::toNativeSeparators(appabsPath + "/PyScripts");
}

bool DACoreInterface::isPythonInterpreterInitialized()
{
    return d_ptr->isPythonInterpreterInitialized;
}

DAPyScripts* DACoreInterface::getDAScripts()
{
    return PrivateData::s_daScripts;
}


#endif  // end DA_ENABLE_PYTHON


bool DACoreInterface::isProjectDirty() const
{
    DAProjectInterface* pi = getProjectInterface();
    if (pi) {
        return pi->isDirty();
    }
    return false;
}

void DACoreInterface::setProjectDirty(bool on)
{
    DAProjectInterface* pi = getProjectInterface();
    if (pi) {
        pi->setModified(on);
    }
}

/**
 * @brief 获取本程序的临时路径
 * @return
 */
QDir DACoreInterface::getTempDir() const
{
    return QDir(d_ptr->mTempDir.path());
}

}  // namespace DA
