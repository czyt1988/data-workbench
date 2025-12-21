#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
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
namespace DA
{

class DAPyInterpreter::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyInterpreter)
public:
    PrivateData(DAPyInterpreter* p);

public:
    std::unique_ptr< pybind11::scoped_interpreter > interpreter;
};

DAPyInterpreter::PrivateData::PrivateData(DAPyInterpreter* p) : q_ptr(p)
{
}
//===================================================
// DAPyInterpreter
//===================================================
DAPyInterpreter::DAPyInterpreter() : DA_PIMPL_CONSTRUCT
{
}

DAPyInterpreter::~DAPyInterpreter()
{
    ensureShutdown();
}

DAPyInterpreter& DAPyInterpreter::getInstance()
{
    static DAPyInterpreter s_python;
    return s_python;
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

QList< QFileInfo > DA::DAPyInterpreter::wherePythonFromConfig()
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

void DAPyInterpreter::initializePythonInterpreter()
{
    try {
        d_ptr->interpreter = std::make_unique< pybind11::scoped_interpreter >();
    } catch (const std::exception& e) {
        qWarning() << e.what();
    }
}

void DAPyInterpreter::registerFinalizeCallback(DAPyInterpreter::callback_finalize fp)
{
    mFinalizeCallbacks.push_back(fp);
}

void DAPyInterpreter::shutdown()
{
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

    // ===== 第三步：最终释放解释器 =====
    qDebug() << "Shutdow Python Interpreter...";
    d_ptr->interpreter.reset();  // 这会触发 pybind11::finalize_interpreter
    qDebug() << "Python Interpreter Shutdowed";
}

void DAPyInterpreter::ensureShutdown()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [ this ]() { shutdown(); });
}

/**
   @brief 获取python配置文件

   @return $RETURN
 */
QString DA::DAPyInterpreter::getAppPythonConfigFile()
{
    QString appDir = QCoreApplication::applicationDirPath();
    return QDir::toNativeSeparators(appDir + "/python-config.json");
}

/**
   @brief 获取python的路径

   @param $PARAMS
   @return $RETURN
 */
QString DA::DAPyInterpreter::getPythonInterpreterPath()
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

}
