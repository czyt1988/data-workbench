#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include <QPointer>
#include <QApplication>
#include "DAProjectInterface.h"
#if DA_ENABLE_PYTHON
#include <memory>
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
    std::shared_ptr< pybind11::scoped_interpreter > interpreter;
    std::unique_ptr< DAPythonSignalHandler > pythonHandler;
#endif
};


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
    d_ptr->interpreter = nullptr;
#endif
}

#if DA_ENABLE_PYTHON
bool DACoreInterface::initializePythonEnv()
{
    DA_D(d);
    if (DAPyInterpreter::isPythonInitialized()) {
        qCritical() << tr("Python interpreter is already initialized");  // cn:python解释器已初始化
        d->interpreter = DAPyInterpreter::interpreter;
        return false;
    }
    try {

        QString pythonHomePath;
        QString pypath = DAPyInterpreter::getPythonInterpreterPath();
        if (!pypath.isEmpty()) {
            qInfo() << tr("Python interpreter path is %1").arg(pypath);
            QFileInfo fi(pypath);
            pythonHomePath = fi.absolutePath();
            qInfo() << tr("Python home path is %1").arg(pythonHomePath);
        }
        DAPyInterpreter::initializePythonInterpreter(pythonHomePath);
        // 把脚本路径加载到系统路径下，这样才能引入库
        QString scriptPath = getPythonScriptsPath();
        qInfo() << tr("Python scripts path is %1").arg(scriptPath);
        DAPyInterpreter::appendSysPath(scriptPath);

        // python环境初始化完成，初始化da脚本
        DAPyScripts::initScripts();

        // DA::DAPyScripts::appendSysPath必须在getInstance前执行
        if (!(DAPyScripts::isInitScripts())) {
            qCritical() << tr("Scripts initialize error");
            return false;
        }
        // 初始化python信号投递器
        d->pythonHandler = std::make_unique< DAPythonSignalHandler >();
    } catch (const std::exception& e) {
        qCritical() << tr("Initialize python environment error:%1").arg(e.what());
        return false;
    }
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
    return DAPyInterpreter::isPythonInitialized();
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
