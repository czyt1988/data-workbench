#include "DAPyScriptRunner.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <QObject>
#include <QVariant>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <pybind11/eval.h>
#include "DAPyGILGuard.h"
#include "DAPyInterpreter.h"
#include "DAPybind11QtCaster.hpp"
#include "DALogCategory.h"

namespace DA
{
namespace
{
///< 重入防护标志：脚本可自行泵 Qt 事件（processEvents/模态对话框等），导致
///< QTimer::singleShot(0) 投递的工具调用嵌套进未结束的 runCode/runScript
std::atomic< bool > s_isRunning { false };
///< 脚本执行超时（秒），0=禁用，默认300秒
std::atomic< int > s_timeoutSec { 300 };
///< 结果序列化后的最大字符数
std::atomic< int > s_resultMaxChars { 10000 };

///< 基线键集合（重置命名空间时保留）
const char* const c_baselineKeys[] = { "__builtins__", "__name__", "da_app", "da_interface", "da_data" };

/**
 * @brief 可取消的超时看门狗
 *
 * 脚本在超时前结束时，stop() 置 cancelled 并 notify 立即唤醒看门狗线程，
 * join 瞬间返回（不阻塞整个超时周期）；真正超时时置 timedOut 并调用
 * PyErr_SetInterrupt()（线程安全）注入 KeyboardInterrupt。
 */
class TimeoutWatchdog
{
public:
    ~TimeoutWatchdog()
    {
        stop();
    }

    void start(int timeoutSec)
    {
        mWorker = std::thread([ this, timeoutSec ]() {
            std::unique_lock< std::mutex > lk(mMtx);
            // 等待：要么超时到期，要么被取消
            mCv.wait_for(lk, std::chrono::seconds(timeoutSec), [ this ]() { return mCancelled.load(); });
            if (!mCancelled.load()) {
                mTimedOut.store(true);
                PyErr_SetInterrupt();  // 线程安全，注入 KeyboardInterrupt
            }
        });
    }

    void stop()
    {
        {
            std::lock_guard< std::mutex > lk(mMtx);
            mCancelled.store(true);
        }
        mCv.notify_all();  // 立即唤醒，无需等满超时周期
        if (mWorker.joinable()) {
            mWorker.join();  // 线程已醒，join 立即返回
        }
    }

    bool isTimedOut() const
    {
        return mTimedOut.load();
    }

private:
    std::mutex mMtx;
    std::condition_variable mCv;
    std::atomic< bool > mCancelled { false };  ///< 脚本结束→置 true
    std::atomic< bool > mTimedOut { false };   ///< 看门狗触发→置 true
    std::thread mWorker;
};

/**
 * @brief 执行期间 CWD 切换的 RAII 守卫（执行后恢复原 CWD）
 */
class CwdGuard
{
public:
    explicit CwdGuard(const QString& newCwd)
    {
        mOldCwd = QDir::currentPath();
        if (!newCwd.isEmpty() && QDir(newCwd).exists()) {
            mChanged = QDir::setCurrent(newCwd);
        }
    }

    ~CwdGuard()
    {
        if (mChanged) {
            QDir::setCurrent(mOldCwd);
        }
    }

private:
    QString mOldCwd;
    bool mChanged { false };
};

/**
 * @brief sys.stdout/sys.stderr 重定向到 io.StringIO 的 RAII 守卫
 *
 * 构造时保存原 stdout/stderr 并重定向，析构时无条件恢复——即便脚本执行
 * 抛异常进入 catch，stdout/stderr 也必然恢复。
 * @note io.StringIO 无 fileno()，调用 sys.stdout.fileno() 的库会抛
 * UnsupportedOperation，属已知限制
 */
class StdoutRedirectGuard
{
public:
    StdoutRedirectGuard()
    {
        pybind11::module_ sys = pybind11::module_::import("sys");
        pybind11::module_ io  = pybind11::module_::import("io");
        mOldOut               = pybind11::getattr(sys, "stdout");
        mOldErr               = pybind11::getattr(sys, "stderr");
        pybind11::object stringIo = pybind11::getattr(io, "StringIO");
        mBufOut                   = stringIo();
        mBufErr                   = stringIo();
        sys.attr("stdout")        = mBufOut;
        sys.attr("stderr")        = mBufErr;
    }

    ~StdoutRedirectGuard()
    {
        try {
            pybind11::module_ sys = pybind11::module_::import("sys");
            sys.attr("stdout")    = mOldOut;
            sys.attr("stderr")    = mOldErr;
        } catch (...) {
            // 恢复失败（如解释器状态异常）不能再抛出
        }
    }

    QString takeStdout() const
    {
        return QString::fromStdString(pybind11::str(mBufOut.attr("getvalue")()).cast< std::string >());
    }

    QString takeStderr() const
    {
        return QString::fromStdString(pybind11::str(mBufErr.attr("getvalue")()).cast< std::string >());
    }

private:
    pybind11::object mOldOut;
    pybind11::object mOldErr;
    pybind11::object mBufOut;
    pybind11::object mBufErr;
};

/**
 * @brief 类型感知序列化 __result__（不触发 pandas 导入）
 *
 * - pandas DataFrame/Series → head(100).to_string() 限行 + shape 摘要
 * - builtins 的 dict/list/标量 → 尝试 json.dumps，失败回退 repr()
 * - 其他对象 → repr()
 * 序列化结果超过 maxChars 时截断并追加 "... (truncated)"
 * @param result 命名空间中的 __result__ 值
 * @param maxChars 最大字符数
 * @param isNull 输出：result 为 None 时置 true
 * @return 序列化文本
 */
QString serializeResult(const pybind11::object& result, int maxChars, bool* isNull)
{
    if (isNull) {
        *isNull = result.is_none();
    }
    if (result.is_none()) {
        return QString();
    }
    QString text;
    try {
        pybind11::object typeObj = pybind11::type::of(result);
        std::string moduleName   = pybind11::str(typeObj.attr("__module__")).cast< std::string >();
        std::string typeName     = pybind11::str(typeObj.attr("__name__")).cast< std::string >();
        const bool isPandas      = (moduleName.rfind("pandas", 0) == 0);
        if (isPandas && (typeName == "DataFrame" || typeName == "Series")) {
            // 先限行再转字符串，避免对全量数据 str()
            try {
                pybind11::object head = result.attr("head")(100);
                std::string headStr   = pybind11::str(head.attr("to_string")()).cast< std::string >();
                std::string shapeStr  = pybind11::str(result.attr("shape")).cast< std::string >();
                text                  = QStringLiteral("%1 shape=%2, showing head(100):\n%3")
                                       .arg(QString::fromStdString(typeName),
                                            QString::fromStdString(shapeStr),
                                            QString::fromStdString(headStr));
            } catch (...) {
                text = QString::fromStdString(pybind11::str(pybind11::repr(result)).cast< std::string >());
            }
        } else if (moduleName == "builtins") {
            // dict/list/标量等尝试 json.dumps 便于 agent 解析，失败回退 repr()
            try {
                pybind11::module_ json  = pybind11::module_::import("json");
                pybind11::object dumped = json.attr("dumps")(result, pybind11::arg("ensure_ascii") = false);
                text                    = QString::fromStdString(dumped.cast< std::string >());
            } catch (...) {
                text = QString::fromStdString(pybind11::str(pybind11::repr(result)).cast< std::string >());
            }
        } else {
            text = QString::fromStdString(pybind11::str(pybind11::repr(result)).cast< std::string >());
        }
    } catch (...) {
        text = QStringLiteral("<unserializable result object>");
    }
    if (maxChars > 0 && text.length() > maxChars) {
        text = text.left(maxChars) + QStringLiteral("... (truncated)");
    }
    return text;
}
}  // namespace

/**
 * @brief 内部数据
 *
 * 与 DAPyScripts::InnerModules 同理：持有解释器引用计数，保证命名空间 dict
 * 不会在解释器销毁后才析构（否则 UAF 崩溃）
 */
class DAPyScriptRunner::InnerData
{
public:
    InnerData()
    {
        interpreter = DAPyInterpreter::interpreter;
        if (!interpreter) {
            throw std::runtime_error(
                "DAPyInterpreter is not initialized before DAPyScriptRunner::InnerData construction");
        }
    }

public:
    pybind11::dict ns;  ///< 持久命名空间
    std::shared_ptr< pybind11::scoped_interpreter > interpreter;  ///< 解析器，增加引用计数，避免python环境析构了此类还存在
    QString workspaceRoot;  ///< 当前工作区根目录
};

std::unique_ptr< DAPyScriptRunner::InnerData > DAPyScriptRunner::s_data = nullptr;

/**
 * @brief 初始化共享命名空间
 *
 * 构建持久命名空间并预导入基线模块 {da_app, da_interface, da_data}。
 * 三个 import 逐一 try-catch，任一失败则返回 false 并记录 daCritical。
 * @note da_app 由 APP 层 PYBIND11_EMBEDDED_MODULE 定义（L2→L5 运行时依赖，
 * 经权衡接受）；da_interface/da_data 分别来自 L4/L2
 * @return true 初始化成功
 */
bool DAPyScriptRunner::init()
{
    if (s_data) {
        return true;
    }
    if (!DAPyInterpreter::isPythonInitialized()) {
        daCritical << QObject::tr(
            "Python interpreter is not initialized, script runner cannot start");  // cn:Python 解释器未初始化，脚本执行引擎无法启动
        return false;
    }
    try {
        DAPyGILGuard gil;
        s_data = std::make_unique< InnerData >();
        pybind11::dict ns;
        ns["__name__"]     = "__main__";
        ns["__builtins__"] = pybind11::module_::import("builtins");
        // 预导入基线模块（链式数据访问需三个模块全部就绪）
        static const char* baselineModules[] = { "da_app", "da_interface", "da_data" };
        for (const char* m : baselineModules) {
            try {
                ns[ m ] = pybind11::module_::import(m);
            } catch (const pybind11::error_already_set& e) {
                daCritical << QObject::tr("Script runner failed to import baseline module %1: %2")
                                      .arg(QString(m), QString::fromUtf8(e.what()));  // cn:脚本执行引擎导入基线模块 %1 失败：%2
                s_data.reset();
                return false;
            }
        }
        ns["__result__"] = pybind11::none();
        ns["args"]       = pybind11::dict();
        s_data->ns       = ns;
    } catch (const std::exception& e) {
        daCritical << QObject::tr("Failed to initialize script runner: %1")
                              .arg(QString::fromUtf8(e.what()));  // cn:初始化脚本执行引擎失败：%1
        s_data.reset();
        return false;
    }
    return true;
}

/**
 * @brief 判断命名空间引擎是否已初始化
 * @return 已初始化返回 true
 */
bool DAPyScriptRunner::isInit()
{
    return s_data != nullptr;
}

/**
 * @brief 清理命名空间引擎
 *
 * 必须在 Python 解释器关闭前调用（由 ~DACoreInterface 保证）。
 * 析构 py::dict 必须持 GIL，否则崩溃。
 */
void DAPyScriptRunner::cleanup()
{
    if (!s_data) {
        return;
    }
    if (DAPyInterpreter::isPythonInitialized()) {
        DAPyGILGuard gil;
        s_data.reset();
    } else {
        s_data.reset();
    }
}

/**
 * @brief 设置脚本工作区根目录
 *
 * 管理 sys.path：移除旧工作区路径、把新路径追加到末尾（非 insert(0)，
 * 降低 workspace 内模块 shadow 标准库/第三方库的风险）。
 * 未初始化或已有脚本在跑时直接返回。
 * @param dir 工作区根目录，空串表示清除
 */
void DAPyScriptRunner::setWorkspaceRoot(const QString& dir)
{
    if (!s_data) {
        return;
    }
    if (s_isRunning.load()) {
        // 执行期间不允许变更命名空间相关状态
        return;
    }
    const QString newRoot = dir.isEmpty() ? QString() : QDir::cleanPath(dir);
    try {
        DAPyGILGuard gil;
        if (!s_data) {
            return;
        }
        const QString oldRoot = s_data->workspaceRoot;
        pybind11::module_ sys = pybind11::module_::import("sys");
        pybind11::object path = sys.attr("path");
        if (!oldRoot.isEmpty()) {
            try {
                path.attr("remove")(pybind11::str(oldRoot.toUtf8().constData()));
            } catch (const pybind11::error_already_set&) {
                // sys.path 中不存在该路径（ValueError），忽略
            }
        }
        if (!newRoot.isEmpty()) {
            path.attr("append")(pybind11::str(newRoot.toUtf8().constData()));
        }
        s_data->workspaceRoot = newRoot;
    } catch (const std::exception& e) {
        qWarning() << "DAPyScriptRunner::setWorkspaceRoot failed:" << e.what();
    }
}

/**
 * @brief 按绝对路径执行 .py 文件
 *
 * 在持久命名空间中执行（py::eval_file），执行期间 CWD 切到工作区根目录，
 * stdout/stderr 重定向捕获，超时注入 KeyboardInterrupt。
 * @param absPath 脚本绝对路径（调用方负责解析与边界校验）
 * @param args 注入命名空间的 args 全局变量
 * @return {success, result, stdout, stderr, error}
 */
QJsonObject DAPyScriptRunner::runScript(const QString& absPath, const QJsonObject& args)
{
    if (!s_data) {
        QJsonObject resp;
        resp[ "success" ] = false;
        resp[ "error" ]   = QStringLiteral("script runner not initialized");
        return resp;
    }
    QFileInfo fi(absPath);
    if (!fi.isFile()) {
        QJsonObject resp;
        resp[ "success" ] = false;
        resp[ "error" ]   = QStringLiteral("script file not found: ") + absPath;
        return resp;
    }
    if (!absPath.endsWith(QStringLiteral(".py"), Qt::CaseInsensitive)) {
        QJsonObject resp;
        resp[ "success" ] = false;
        resp[ "error" ]   = QStringLiteral("not a python script: ") + absPath;
        return resp;
    }
    return runInternal(true, absPath, args);
}

/**
 * @brief 执行内联代码字符串（agent run_code 用）
 *
 * 在同一持久命名空间执行，多步分析可累积状态。
 * @param code Python 代码
 * @param args 注入命名空间的 args 全局变量
 * @return {success, result, stdout, stderr, error}
 */
QJsonObject DAPyScriptRunner::runCode(const QString& code, const QJsonObject& args)
{
    if (!s_data) {
        QJsonObject resp;
        resp[ "success" ] = false;
        resp[ "error" ]   = QStringLiteral("script runner not initialized");
        return resp;
    }
    if (code.trimmed().isEmpty()) {
        QJsonObject resp;
        resp[ "success" ] = false;
        resp[ "error" ]   = QStringLiteral("code is empty");
        return resp;
    }
    return runInternal(false, code, args);
}

/**
 * @brief runScript/runCode 的公共执行流程
 *
 * 持 GIL → 重入防护（RAII）→ 注入 args 与 __result__ → stdout/stderr
 * 重定向（RAII）→ CWD 切工作区（RAII）→ 可取消超时看门狗 → 执行 →
 * 消费残留中断标志 → 读取输出与 __result__（类型感知序列化）。
 * 异常后命名空间保留到异常前状态（与 Jupyter 一致），部分输出仍返回。
 * @param isFile true 时 codeOrPath 为 .py 文件绝对路径（eval_file），否则为代码字符串（exec）
 * @param codeOrPath 代码或文件路径
 * @param args 注入命名空间的 args 全局变量
 * @return {success, result, stdout, stderr, error}
 */
QJsonObject DAPyScriptRunner::runInternal(bool isFile, const QString& codeOrPath, const QJsonObject& args)
{
    // 重入防护：脚本可自行泵 Qt 事件，导致嵌套工具调用进入本函数
    bool expected = false;
    if (!s_isRunning.compare_exchange_strong(expected, true)) {
        QJsonObject resp;
        resp[ "success" ] = false;
        resp[ "error" ]   = QStringLiteral("another script is running");
        return resp;
    }
    QJsonObject resp;
    try {
        DAPyGILGuard gil;
        pybind11::dict ns = s_data->ns;
        // 注入 args 全局变量（QJsonObject→py dict 经 DAPybind11QtCaster 的 QVariant 转换器）
        QVariant argsVariant = args.toVariantMap();
        ns[ "args" ]         = pybind11::cast(argsVariant);
        ns[ "__result__" ]   = pybind11::none();

        StdoutRedirectGuard stdoutGuard;
        CwdGuard cwdGuard(s_data->workspaceRoot);

        const int timeoutSec = s_timeoutSec.load();
        TimeoutWatchdog watchdog;
        bool hasError      = false;
        QString errorText;
        bool isInterrupted = false;
        if (timeoutSec > 0) {
            watchdog.start(timeoutSec);
        }
        try {
            if (isFile) {
                // eval_file 形参为 pybind11::str，直接 C++ 调用不走类型转换器，需显式构造
                pybind11::eval_file(pybind11::str(codeOrPath.toUtf8().constData()), ns);
            } else {
                pybind11::exec(pybind11::str(codeOrPath.toUtf8().constData()), ns);
            }
        } catch (const pybind11::error_already_set& e) {
            hasError      = true;
            isInterrupted = e.matches(PyExc_KeyboardInterrupt);
            errorText     = QString::fromUtf8(e.what());
        }
        // 消费可能残留的中断标志（脚本恰在超时触发瞬间结束的竞态），避免污染下一次执行
        PyErr_CheckSignals();
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
        watchdog.stop();

        const QString outStr = stdoutGuard.takeStdout();
        const QString errStr = stdoutGuard.takeStderr();

        if (hasError) {
            if (watchdog.isTimedOut() && isInterrupted) {
                errorText = QStringLiteral("script execution timed out after %1 seconds").arg(timeoutSec);
            } else if (watchdog.isTimedOut()) {
                errorText = QStringLiteral("timeout fired but script swallowed KeyboardInterrupt: ") + errorText;
            }
            resp[ "success" ] = false;
            resp[ "error" ]   = errorText;
        } else {
            bool resultIsNull = true;
            pybind11::object result;
            try {
                result = ns.contains("__result__") ? pybind11::object(ns[ "__result__" ]) : pybind11::none();
            } catch (...) {
                result = pybind11::none();
            }
            QString resultText = serializeResult(result, s_resultMaxChars.load(), &resultIsNull);
            resp[ "success" ]  = true;
            resp[ "result" ]   = resultIsNull ? QJsonValue(QJsonValue::Null) : QJsonValue(resultText);
            // 脚本吞掉 KeyboardInterrupt 后正常跑完：超时确实发生过，仍记录在 error 字段
            if (watchdog.isTimedOut()) {
                resp[ "error" ] = QStringLiteral("timeout fired but script swallowed KeyboardInterrupt");
            }
        }
        resp[ "stdout" ] = outStr;
        resp[ "stderr" ] = errStr;
    } catch (const std::exception& e) {
        resp[ "success" ] = false;
        resp[ "error" ]   = QStringLiteral("script runner internal error: ") + QString::fromUtf8(e.what());
    }
    s_isRunning.store(false);
    return resp;
}

/**
 * @brief 清理用户符号回基线
 *
 * 清除命名空间中除基线键以外的所有键（含用户 import 的第三方库别名），
 * 并从 sys.modules 移除所有来源于当前工作区目录的自定义模块（通过
 * __file__ 判断），强制下次 import 重新执行，避免跨工程模块级状态残留。
 * 第三方库（site-packages 来源）保留不清。未初始化或已有脚本在跑时直接返回。
 */
void DAPyScriptRunner::resetNamespace()
{
    if (!s_data) {
        return;
    }
    if (s_isRunning.load()) {
        return;
    }
    try {
        DAPyGILGuard gil;
        if (!s_data) {
            return;
        }
        pybind11::dict ns = s_data->ns;
        // 收集键后删除（不能边迭代边删）
        pybind11::list keys(ns.attr("keys")());
        for (auto k : keys) {
            try {
                std::string name = pybind11::str(k).cast< std::string >();
                bool isBaseline  = false;
                for (const char* b : c_baselineKeys) {
                    if (name == b) {
                        isBaseline = true;
                        break;
                    }
                }
                if (!isBaseline) {
                    ns.attr("pop")(k, pybind11::none());
                }
            } catch (...) {
                continue;
            }
        }
        // 从 sys.modules 移除来源于工作区目录的模块
        const QString wsRoot = s_data->workspaceRoot;
        if (!wsRoot.isEmpty()) {
            pybind11::object modules = pybind11::module_::import("sys").attr("modules");
            pybind11::list moduleNames(modules.attr("keys")());
            const QString wsPrefix = QDir::cleanPath(wsRoot);
            for (auto nameObj : moduleNames) {
                try {
                    pybind11::object mod = modules[ nameObj ];
                    if (!pybind11::hasattr(mod, "__file__")) {
                        continue;
                    }
                    pybind11::object f = mod.attr("__file__");
                    if (f.is_none()) {
                        continue;
                    }
                    QString filePath = QString::fromStdString(pybind11::str(f).cast< std::string >());
                    filePath.replace('\\', '/');
                    if (filePath.startsWith(wsPrefix, Qt::CaseInsensitive)) {
                        modules.attr("pop")(nameObj, pybind11::none());
                    }
                } catch (...) {
                    continue;
                }
            }
        }
        ns["__result__"] = pybind11::none();
        ns["args"]       = pybind11::dict();
    } catch (const std::exception& e) {
        qWarning() << "DAPyScriptRunner::resetNamespace failed:" << e.what();
    }
}

/**
 * @brief 获取命名空间 dict
 *
 * @warning 唯一不内部持 GIL 的方法：返回 Python 对象引用，要求调用方自行持 GIL
 * @return 命名空间 dict，未初始化时返回空 dict
 */
pybind11::dict DAPyScriptRunner::getNamespace()
{
    if (!s_data) {
        return pybind11::dict();
    }
    return s_data->ns;
}

/**
 * @brief 设置脚本执行超时（秒）
 * @param seconds 0=禁用
 */
void DAPyScriptRunner::setScriptTimeout(int seconds)
{
    s_timeoutSec.store(seconds < 0 ? 0 : seconds);
}

/**
 * @brief 获取脚本执行超时（秒）
 * @return 超时秒数，0=禁用
 */
int DAPyScriptRunner::getScriptTimeout()
{
    return s_timeoutSec.load();
}

/**
 * @brief 设置结果序列化后的最大字符数
 * @param maxChars 最大字符数，<=0 表示不截断
 */
void DAPyScriptRunner::setResultMaxChars(int maxChars)
{
    s_resultMaxChars.store(maxChars);
}

/**
 * @brief 获取结果最大字符数
 * @return 最大字符数
 */
int DAPyScriptRunner::getResultMaxChars()
{
    return s_resultMaxChars.load();
}

}  // namespace DA
