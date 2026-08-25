#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include <QPointer>
#include <QApplication>
#include <QTemporaryDir>
#include "DAProjectInterface.h"
#include "DALogCategory.h"
#include <memory>
// DA Python
#include "DAPyInterpreter.h"
#include "DAPyScripts.h"
#include "DAPyScriptRunner.h"
#include "DAPythonSignalHandler.h"
namespace DA
{

class DACoreInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DACoreInterface)
public:
    PrivateData(DACoreInterface* p);

public:
    QTemporaryDir mTempDir;
    std::shared_ptr< pybind11::scoped_interpreter > interpreter;
    std::unique_ptr< DAPythonSignalHandler > pythonHandler;
};

DACoreInterface::PrivateData::PrivateData(DACoreInterface* p) : q_ptr(p)
{
}

DACoreInterface::DACoreInterface(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->mTempDir.setAutoRemove(true);
    initializePythonScripts();
}

DACoreInterface::~DACoreInterface()
{
    d_ptr->pythonHandler.reset();
    // 必须在 DAPyScripts::cleanup() 与解释器关闭之前清理命名空间引擎（析构 py::dict 需持 GIL）
    DAPyScriptRunner::cleanup();
    DAPyScripts::cleanup();
    d_ptr->interpreter = nullptr;
    DAPyInterpreter::ensureShutdown();
}

/**
 * @brief 初始化python环境,加载默认脚本
 *
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool DACoreInterface::initializePythonScripts()
{
    DA_D(d);
    if (!DAPyInterpreter::isPythonInitialized()) {
        daCritical << tr("Python interpreter is not initialized");  // cn:Python 解释器未初始化
        return false;
    }
    d->interpreter = DAPyInterpreter::interpreter;
    try {
        // 把脚本路径加载到系统路径下，这样才能引入库
        QString scriptPath = getPythonScriptsPath();
        daInfo << tr("Python scripts path is %1").arg(scriptPath);  // cn:Python 脚本路径为 %1
        DAPyInterpreter::appendSysPath(scriptPath);

        // python环境初始化完成，初始化da脚本
        DAPyScripts::initScripts();

        // DA::DAPyScripts::appendSysPath必须在getInstance前执行
        if (!(DAPyScripts::isInitScripts())) {
            daCritical << tr("Failed to initialize scripts");  // cn:脚本初始化失败
            return false;
        }
        // 脚本初始化成功后，初始化共享命名空间脚本执行引擎（失败不阻断整个 Python 环境，
        // 仅 runScript/runCode 返回 "script runner not initialized" 错误）
        if (!DAPyScriptRunner::init()) {
            daCritical << tr("Failed to initialize script runner, script execution will be unavailable");  // cn:脚本执行引擎初始化失败，脚本执行功能将不可用
        }
        // 初始化python信号投递器
        d->pythonHandler = std::make_unique< DAPythonSignalHandler >();
    } catch (const std::exception& e) {
        daCritical << tr("Failed to initialize Python environment: %1").arg(e.what());  // cn:初始化 Python 环境失败：%1
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
