#include "DAPyInterpreter.h"
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
//
#include "DADir.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
namespace DA
{


std::shared_ptr< pybind11::scoped_interpreter > DAPyInterpreter::interpreter = nullptr;
//===================================================
// DAPyInterpreter
//===================================================
DAPyInterpreter::DAPyInterpreter()
{
    if (interpreter == nullptr) {
        initializePythonInterpreter();
    }
}

DAPyInterpreter::~DAPyInterpreter()
{
}


/**
 * @brief 获取系统记录的python环境
 * @return
 */
QList< QFileInfo > DAPyInterpreter::wherePython()
{
    QList< QFileInfo > validFis;
    QProcess process;
    QString command = "where python";
    process.start(command);
    if (!process.waitForFinished()) {
        return QList< QFileInfo >();
    }
    QString res = process.readAll();
    qDebug() << res;
    const QList< QString > pys = res.split("\r\n");
    // 遍历所有环境，确认是否的确是ptython路径,where 有时候会返回一些不正确的路径
    for (QString p : pys) {
        QFileInfo fi(p);
        if (fi.isExecutable()) {
            // 说明是可执行文件，windows下就是pythhon.exe
            validFis.append(fi);
        }
    }

    return validFis;
}

QList< QFileInfo > DAPyInterpreter::wherePythonFromConfig()
{
    QList< QFileInfo > validFis;
    //! 先查看根目录下python-config.json
    QString cfgPath = getAppPythonConfigFile();
    QFile file(cfgPath);
    if (!file.exists()) {
        return validFis;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return validFis;
    }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    if (jsonDoc.isNull() || jsonDoc.isEmpty()) {
        return validFis;
    }
    if (!jsonDoc.isObject()) {
        return validFis;
    }
    QJsonObject root = jsonDoc.object();
    if (!root.contains("config")) {
        return validFis;
    }

    // json可替换变量
    const static QString cs_jsonkeywork_current_app_dir = QStringLiteral("${current-app-dir}");

    QJsonObject config  = root[ "config" ].toObject();
    QString interpreter = config[ "interpreter" ].toString();
    if (interpreter.contains(cs_jsonkeywork_current_app_dir, Qt::CaseInsensitive)) {
        // 如果存在${current-app-dir}，则替换为程序安装目录
        interpreter.replace(cs_jsonkeywork_current_app_dir, DADir::getExecutablePath(), Qt::CaseInsensitive);
    }
    QFileInfo fi(interpreter);
    if (!fi.exists()) {
        return validFis;
    }
    validFis.append(fi);
    return validFis;
}

/**
 * @brief 是否初始化了python环境
 *
 * @return true 已初始化
 * @return false 未初始化
 */
bool DAPyInterpreter::isPythonInitialized()
{
    return interpreter != nullptr;
}

/**
 * @brief 添加python环境路径
 *
 * 等同于：
 * @code
 * import sys
 * sys.path.append(xx)
 * @endcode
 *
 * @param path
 *
 * @note 此函数必须在环境初始化后使用
 */
void DAPyInterpreter::appendSysPath(const QString& path)
{
    if(!isPythonInitialized()){
      qCritical() << "Python Is Not Initialized";
        return;
    }
    pybind11::module_ sys = pybind11::module_::import("sys");
    pybind11::object obj_path_append = sys.attr("path").attr("append");
    obj_path_append(DA::PY::toPyObject(path));
}

/**
 * @brief 设置python的运行路径
 *
 * @param path
 */
void DAPyInterpreter::setPythonHomePath(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
    std::vector< wchar_t > wp((path.size() + 1) * 4, 0);

    path.toWCharArray(wp.data());
    try {
        Py_SetPythonHome(wp.data());
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
}

/**
 * @brief 初始化Python解释器
 *
 * 使用默认配置初始化Python解释器
 */
void DAPyInterpreter::initializePythonInterpreter()
{
    initializePythonInterpreter(QString());
}

/**
 * @brief 初始化Python解释器（带Python Home路径参数）
 *
 * 使用指定的Python Home路径初始化Python解释器。
 * 对于Python 3.8+，使用PyConfig API来正确设置Python Home路径。
 *
 * @param pythonHomePath Python Home路径，指向Python安装目录（如C:\Python311）
 */
void DAPyInterpreter::initializePythonInterpreter(const QString& pythonHomePath)
{
    try {
        qDebug() << "Python DLL version from header:" << PY_VERSION;
        qDebug() << "Python hex version:" << PY_VERSION_HEX;

#if PY_VERSION_HEX >= 0x03080000
        if (!pythonHomePath.isEmpty()) {
            PyConfig config;
            PyConfig_InitPythonConfig(&config);
            config.parse_argv = 0;

            std::vector< wchar_t > wp((pythonHomePath.size() + 1) * 4, 0);
            pythonHomePath.toWCharArray(wp.data());
            PyStatus status = PyConfig_SetString(&config, &config.home, wp.data());
            if (PyStatus_Exception(status) != 0) {
                qWarning() << "Failed to set Python home path:" << pythonHomePath;
            } else {
                qDebug() << "Python home path set to:" << pythonHomePath;
            }
            interpreter = std::make_shared< pybind11::scoped_interpreter >(&config);
        } else {
            interpreter = std::make_shared< pybind11::scoped_interpreter >();
        }
#else
        if (!pythonHomePath.isEmpty()) {
            setPythonHomePath(pythonHomePath);
        }
        interpreter = std::make_shared< pybind11::scoped_interpreter >();
#endif

        qDebug() << "Python runtime version:" << Py_GetVersion();
        qDebug() << "Python path:" << Py_GetPath();
        qDebug() << "Compiled against:" << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION << "." << PY_MICRO_VERSION;
    } catch (const std::exception& e) {
        qWarning() << e.what();
    }
}

void DAPyInterpreter::shutdown()
{
    if (interpreter == nullptr) {
        qDebug() << "Python Interpreter already shutdown";
        return;
    }
    // ===== 第一步：执行 Python 端的预清理 =====
    try {
        {
            // 在调用Python函数之前释放GIL
            pybind11::gil_scoped_release release;

            // 短暂休眠，让Python线程有机会获得GIL
            QThread::msleep(200);
        }
        pybind11::module daWorkbench;
        try {
            daWorkbench = pybind11::module::import("DAWorkbench");
        } catch (...) {
            qWarning() << "can not load DAWorkbench package";
        }

        // 2. 如果模块存在并提供了清理函数，则调用它
        if (daWorkbench && pybind11::hasattr(daWorkbench, "stop_all_background_tasks")) {
            qDebug() << "begin run stop_all_background_tasks";
            daWorkbench.attr("stop_all_background_tasks")();
        } else {
            qDebug() << "can not find stop_all_background_tasks in DAWorkbench";
        }

        // 3. 强制进行一轮垃圾回收，释放可能存在的循环引用
        pybind11::module gc = pybind11::module::import("gc");
        gc.attr("collect")();

    } catch (const pybind11::error_already_set& e) {
        qWarning() << "run python error:" << e.what();
    }
    // ===== 第二步：等待非主线程结束 (带超时和更智能的判断) =====
    try {
        pybind11::module threading = pybind11::module::import("threading");
        pybind11::list all_threads = threading.attr("enumerate")();

        int waitCount = 0;
        for (auto thread : all_threads) {
            std::string thread_name = thread.attr("name").cast< std::string >();
            bool is_daemon          = thread.attr("daemon").cast< bool >();

            if (thread_name != "MainThread") {
                qDebug() << "find none main thread:" << thread_name.c_str() << (is_daemon ? "is_daemon" : "not_daemon");
                waitCount++;

                // 对于守护线程，通常不需要 join，解释器退出时会强制结束。
                // 我们只对明确知道的、需要清理的线程进行 join。
                // 由于 loguru 线程是守护线程，我们已经在第一步尝试停止了它，
                // 这里可以跳过 join，避免阻塞。
                if (!is_daemon) {
                    qDebug() << "  正在等待非守护线程结束...";
                    thread.attr("join")(0.5);  // 等待时间可以更短
                }
            }
        }
        if (waitCount == 0) {
            qDebug() << "no other thead need to wait。";
        }
    } catch (const pybind11::error_already_set& e) {
        qWarning() << "wait to kill thread occ error:" << e.what();
    }

    // ===== 第三步：清理静态缓存 =====
    DAPyModule::cleanupStaticCache();
    DAPyObjectWrapper::cleanupStaticCache();

    // ===== 第四步：最终释放解释器 =====
    qDebug() << "Shutdow Python Interpreter...";
    interpreter = nullptr;  // 这会触发 pybind11::finalize_interpreter
    qDebug() << "Python Interpreter Shutdowed";
}

void DAPyInterpreter::ensureShutdown()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() { DAPyInterpreter::shutdown(); });
}

/**
   @brief 获取python配置文件

   @return $RETURN
 */
QString DAPyInterpreter::getAppPythonConfigFile()
{
    QString appDir = QCoreApplication::applicationDirPath();
    return QDir::toNativeSeparators(appDir + "/python-config.json");
}

/**
   @brief 获取python的路径

   @param $PARAMS
   @return $RETURN
 */
QString DAPyInterpreter::getPythonInterpreterPath()
{
    QList< QFileInfo > validFis;
    //! 先查看根目录下python-config.json
    validFis = wherePythonFromConfig();
    if (!validFis.empty()) {
        return validFis.back().absoluteFilePath();
    }
    //! 如果没有，就看看wherepython
    validFis = wherePython();
    if (!validFis.empty()) {
        return validFis.back().absoluteFilePath();
    }
    return QString();
}

}  // end DA
