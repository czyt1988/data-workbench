#include "DALogger.h"
#include "DAMessageLogSink.h"
#include "DAMessageLogQueue.h"
#include "DAMessageLogItem.h"
// Qt
#include <QByteArray>
#include <QString>
// DAUtils
#include "DAStringUtil.h"
// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// stl
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace DA
{
class DALogger::PrivateData
{
    DA_DECLARE_PUBLIC(DALogger)
public:
    PrivateData(DALogger* p);
    ~PrivateData();

    std::shared_ptr< spdlog::logger > mLogger;        ///< 业务日志 logger（带 UI sink，收 da.* category）
    std::shared_ptr< spdlog::logger > mSystemLogger;  ///< 系统日志 logger（无 UI sink，收非 da.* category）
    std::shared_ptr< DAMessageLogSink > mUiSink;      ///< 持有 UI sink 引用，用于 setQueueLevel
    spdlog::level::level_enum mQueueLevel { spdlog::level::info };  ///< UI sink 当前级别（用于 setQueueCaptureEnabled 恢复）
    std::atomic_bool mQueueCaptureEnabled { true };
    std::atomic_bool mInstalled { false };
};

DALogger::PrivateData::PrivateData(DALogger* p) : q_ptr(p)
{
}

DALogger::PrivateData::~PrivateData()
{
}

/**
 * @brief QtMsgType → spdlog::level 映射
 */
inline spdlog::level::level_enum mapQtMsgTypeToSpdlog(QtMsgType t)
{
    switch (t) {
    case QtDebugMsg:
        return spdlog::level::debug;
    case QtInfoMsg:
        return spdlog::level::info;
    case QtWarningMsg:
        return spdlog::level::warn;
    case QtCriticalMsg:
        return spdlog::level::critical;
    case QtFatalMsg:
        return spdlog::level::critical;
    default:
        return spdlog::level::info;
    }
}

/**
 * @brief DALogLevel → spdlog::level 映射
 */
inline spdlog::level::level_enum mapDALogLevelToSpdlog(DALogLevel l)
{
    switch (l) {
    case DALogLevel::Trace:
        return spdlog::level::trace;
    case DALogLevel::Debug:
        return spdlog::level::debug;
    case DALogLevel::Info:
        return spdlog::level::info;
    case DALogLevel::Warn:
        return spdlog::level::warn;
    case DALogLevel::Error:
        return spdlog::level::err;
    case DALogLevel::Critical:
        return spdlog::level::critical;
    case DALogLevel::Off:
        return spdlog::level::off;
    default:
        return spdlog::level::info;
    }
}

/**
 * @brief DAOverflowPolicy → spdlog::async_overflow_policy 映射
 */
inline spdlog::async_overflow_policy mapOverflowPolicy(DAOverflowPolicy p)
{
    return (p == DAOverflowPolicy::Block) ? spdlog::async_overflow_policy::block
                                          : spdlog::async_overflow_policy::overrun_oldest;
}

/**
 * @brief 构建 async_logger
 *
 * 多次调用时 thread_pool 只初始化一次（std::call_once），使业务 logger 和系统 logger
 * 共享同一个 spdlog 后台线程池。
 */
static std::shared_ptr< spdlog::logger >
buildAsyncLogger(const std::string& name, const std::vector< spdlog::sink_ptr >& sinks, DAOverflowPolicy policy)
{
    static std::once_flag s_initFlag;
    std::call_once(s_initFlag, []() { spdlog::init_thread_pool(8192, 1); });
    return std::make_shared< spdlog::async_logger >(
        name, sinks.begin(), sinks.end(), spdlog::thread_pool(), mapOverflowPolicy(policy));
}

//===================================================
// DALogger
//===================================================

DALogger::DALogger() : d_ptr(std::make_unique< PrivateData >(this))
{
    // 预先引用 DAMessageLogQueue，确保其在主线程构造（QTimer 必须在主线程创建）
    // 否则首次 push（来自 spdlog pool thread）会触发单例构造，导致 QTimer 跨线程警告
    DAMessageLogQueue::instance();
}

DALogger::~DALogger()
{
    DA_D(d);
    if (d->mInstalled.load()) {
        // 先切断入口，避免其他线程在 shutdown 后进入 daMessageHandler
        qInstallMessageHandler(nullptr);
    }
    if (d->mLogger) {
        d->mLogger->flush();
    }
    if (d->mSystemLogger) {
        d->mSystemLogger->flush();
    }
    spdlog::drop_all();
    spdlog::shutdown();
}

DALogger& DALogger::instance()
{
    static DALogger s_logger;
    return s_logger;
}

/**
 * @brief 设置旋转文件日志
 * @param filename
 * @param maxSize
 * @param maxFiles
 * @param outputStdout
 * @param policy
 */
void DALogger::setupRotatingFile(const QString& filename, int maxSize, int maxFiles, bool outputStdout, DAOverflowPolicy policy)
{
    DA_D(d);
    // 转换文件路径（处理 Windows 宽字符路径）
#ifdef SPDLOG_WCHAR_FILENAMES
    spdlog::filename_t path = qstringToSystemWString(filename);
#else
    std::string path(filename.toLocal8Bit().constData());
#endif

    // 基础 sinks：控制台 + 文件（业务 logger 和系统 logger 共享）
    std::vector< spdlog::sink_ptr > baseSinks;
    if (outputStdout) {
        baseSinks.emplace_back(std::make_shared< spdlog::sinks::stdout_color_sink_mt >());
    }
    baseSinks.emplace_back(std::make_shared< spdlog::sinks::rotating_file_sink_mt >(path, maxSize, maxFiles));

    // UI sink（仅业务 logger 使用）——默认 info，让 qInfo 进入 UI 队列
    d->mUiSink = std::make_shared< DAMessageLogSink >();
    d->mUiSink->set_level(d->mQueueLevel);

    // 业务 logger：base sinks + UI sink
    std::vector< spdlog::sink_ptr > businessSinks = baseSinks;
    businessSinks.emplace_back(d->mUiSink);
    d->mLogger = buildAsyncLogger("da_business", businessSinks, policy);

    // 系统 logger：只有 base sinks（不进 UI 队列）
    d->mSystemLogger = buildAsyncLogger("da_system", baseSinks, policy);

    // 两个 logger 共享 pattern 和 level
    d->mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    d->mSystemLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    d->mLogger->set_level(spdlog::level::trace);
    d->mSystemLogger->set_level(spdlog::level::trace);
    d->mLogger->flush_on(spdlog::level::warn);
    d->mSystemLogger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(d->mLogger);
    spdlog::flush_every(std::chrono::seconds(15));

    installMessageHandler();
}

/**
 * @brief 设置按日期分割的文件日志
 * @param filename
 * @param maxFiles
 * @param outputStdout
 * @param policy
 */
void DALogger::setupDailyFile(const QString& filename, int maxFiles, bool outputStdout, DAOverflowPolicy policy)
{
    DA_D(d);
#ifdef SPDLOG_WCHAR_FILENAMES
    spdlog::filename_t path = qstringToSystemWString(filename);
#else
    std::string path(filename.toLocal8Bit().constData());
#endif

    // 基础 sinks：控制台 + 文件（业务 logger 和系统 logger 共享）
    std::vector< spdlog::sink_ptr > baseSinks;
    if (outputStdout) {
        baseSinks.emplace_back(std::make_shared< spdlog::sinks::stdout_color_sink_mt >());
    }
    // daily_file_sink_mt: rotation_hour=0, rotation_minute=0（每天午夜轮转）
    baseSinks.emplace_back(std::make_shared< spdlog::sinks::daily_file_sink_mt >(path, 0, 0, false, maxFiles));

    // UI sink（仅业务 logger 使用）
    d->mUiSink = std::make_shared< DAMessageLogSink >();
    d->mUiSink->set_level(d->mQueueLevel);

    // 业务 logger：base sinks + UI sink
    std::vector< spdlog::sink_ptr > businessSinks = baseSinks;
    businessSinks.emplace_back(d->mUiSink);
    d->mLogger = buildAsyncLogger("da_business", businessSinks, policy);

    // 系统 logger：只有 base sinks（不进 UI 队列）
    d->mSystemLogger = buildAsyncLogger("da_system", baseSinks, policy);

    d->mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    d->mSystemLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    d->mLogger->set_level(spdlog::level::trace);
    d->mSystemLogger->set_level(spdlog::level::trace);
    d->mLogger->flush_on(spdlog::level::warn);
    d->mSystemLogger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(d->mLogger);
    spdlog::flush_every(std::chrono::seconds(15));

    installMessageHandler();
}

/**
 * @brief 设置控制台日志
 * @param policy
 */
void DALogger::setupConsole(DAOverflowPolicy policy)
{
    DA_D(d);
    // 基础 sink：控制台（业务 logger 和系统 logger 共享）
    std::vector< spdlog::sink_ptr > baseSinks;
    baseSinks.emplace_back(std::make_shared< spdlog::sinks::stdout_color_sink_mt >());

    // UI sink（仅业务 logger 使用）
    d->mUiSink = std::make_shared< DAMessageLogSink >();
    d->mUiSink->set_level(d->mQueueLevel);

    // 业务 logger：base sink + UI sink
    std::vector< spdlog::sink_ptr > businessSinks = baseSinks;
    businessSinks.emplace_back(d->mUiSink);
    d->mLogger = buildAsyncLogger("da_business", businessSinks, policy);

    // 系统 logger：只有 base sink（不进 UI 队列）
    d->mSystemLogger = buildAsyncLogger("da_system", baseSinks, policy);

    d->mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    d->mSystemLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    d->mLogger->set_level(spdlog::level::trace);
    d->mSystemLogger->set_level(spdlog::level::trace);
    d->mLogger->flush_on(spdlog::level::info);
    d->mSystemLogger->flush_on(spdlog::level::info);
    spdlog::set_default_logger(d->mLogger);
    spdlog::flush_every(std::chrono::seconds(1));

    installMessageHandler();
}

/**
 * @brief 设置日志级别（影响所有 sink）
 * @param level
 */
void DALogger::setLevel(DALogLevel level)
{
    DA_D(d);
    auto spdLevel = mapDALogLevelToSpdlog(level);
    if (d->mLogger) {
        d->mLogger->set_level(spdLevel);
    }
    if (d->mSystemLogger) {
        d->mSystemLogger->set_level(spdLevel);
    }
}

/**
 * @brief 设置 spdlog 原生 pattern
 * @param pattern
 */
void DALogger::setPattern(const QString& pattern)
{
    DA_D(d);
    if (d->mLogger) {
        d->mLogger->set_pattern(pattern.toStdString());
    }
    if (d->mSystemLogger) {
        d->mSystemLogger->set_pattern(pattern.toStdString());
    }
}

/**
 * @brief 设置 UI 队列的日志级别
 * @param level
 */
void DALogger::setQueueLevel(DALogLevel level)
{
    DA_D(d);
    auto spdLevel  = mapDALogLevelToSpdlog(level);
    d->mQueueLevel = spdLevel;
    if (d->mUiSink && d->mQueueCaptureEnabled.load()) {
        d->mUiSink->set_level(spdLevel);
    }
}

/**
 * @brief 设置 flush 间隔
 * @param seconds
 */
void DALogger::setFlushInterval(int seconds)
{
    spdlog::flush_every(std::chrono::seconds(seconds));
}

/**
 * @brief 启用/禁用 UI 队列捕获
 * @param on
 */
void DALogger::setQueueCaptureEnabled(bool on)
{
    DA_D(d);
    d->mQueueCaptureEnabled.store(on);
    if (d->mUiSink) {
        // 通过设置 sink level 来禁用/恢复（恢复时用上次 setQueueLevel 设置的级别）
        d->mUiSink->set_level(on ? d->mQueueLevel : spdlog::level::off);
    }
}

/**
 * @brief 判断 UI 队列捕获是否启用
 * @return
 */
bool DALogger::isQueueCaptureEnabled() const
{
    DA_DC(dc);
    return dc->mQueueCaptureEnabled.load();
}

/**
 * @brief 安装 qInstallMessageHandler
 */
void DALogger::installMessageHandler()
{
    DA_D(d);
    d->mInstalled.store(true);
    qInstallMessageHandler(daMessageHandler);
}

/**
 * @brief Qt 消息回调
 *
 * 转发到 dispatchMessage 按 category 分流。
 * @param type
 * @param context
 * @param msg
 */
void DALogger::daMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    instance().dispatchMessage(type, context, msg);
}

/**
 * @brief 按 category 分流日志
 *
 * - context.category 以 "da." 开头 → 业务日志，走 mLogger（带 UI sink，进 DAMessageLogQueue → UI）
 * - 其他（Qt 自身、第三方库、未声明 category 的 qDebug 等）→ 系统日志，走 mSystemLogger（仅文件+控制台）
 *
 * 这样 UI 日志窗口只显示业务日志，避免第三方库噪音污染。
 * @param type Qt 消息类型
 * @param context Qt 消息上下文（含 category）
 * @param msg 消息内容
 */
void DALogger::dispatchMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    DA_D(d);
    auto level = mapQtMsgTypeToSpdlog(type);
    // 通过 source_loc 传递 file/line/function，null-safe
    spdlog::source_loc loc { context.file, context.line, context.function };
    // 按 category 分流：da.* 开头的业务日志走带 UI sink 的 logger
    bool isBusinessLog = context.category && QByteArray(context.category).startsWith("da.");
    auto& logger       = isBusinessLog ? d->mLogger : d->mSystemLogger;
    if (logger) {
        // 使用 toUtf8 确保跨平台 UTF-8 编码（Windows 下 toStdString 使用系统 locale 编码会导致乱码）
        logger->log(loc, level, "{}", msg.toUtf8().toStdString());
    }
    if (type == QtFatalMsg) {
        // Fatal 消息需要同步 flush 确保落盘，Qt 会在 handler 返回后自动 abort
        if (logger) {
            logger->flush();
        }
    }
}

}  // namespace DA
