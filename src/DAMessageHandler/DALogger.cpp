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
#include <vector>

namespace DA
{
class DALogger::PrivateData
{
    DA_DECLARE_PUBLIC(DALogger)
public:
    PrivateData(DALogger* p);
    ~PrivateData();

    std::shared_ptr< spdlog::logger > mLogger;
    std::shared_ptr< DAMessageLogSink > mUiSink;   ///< 持有 UI sink 引用，用于 setQueueLevel
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
 */
static std::shared_ptr< spdlog::logger > buildAsyncLogger(
    const std::vector< spdlog::sink_ptr >& sinks,
    DAOverflowPolicy policy)
{
    spdlog::init_thread_pool(8192, 1);
    return std::make_shared< spdlog::async_logger >("da_global",
                                                     sinks.begin(),
                                                     sinks.end(),
                                                     spdlog::thread_pool(),
                                                     mapOverflowPolicy(policy));
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
void DALogger::setupRotatingFile(const QString& filename,
                                 int maxSize,
                                 int maxFiles,
                                 bool outputStdout,
                                 DAOverflowPolicy policy)
{
    DA_D(d);
    // 转换文件路径（处理 Windows 宽字符路径）
#ifdef SPDLOG_WCHAR_FILENAMES
    spdlog::filename_t path = qstringToSystemWString(filename);
#else
    std::string path(filename.toLocal8Bit().constData());
#endif

    std::vector< spdlog::sink_ptr > sinks;
    if (outputStdout) {
        sinks.emplace_back(std::make_shared< spdlog::sinks::stdout_color_sink_mt >());
    }
    sinks.emplace_back(std::make_shared< spdlog::sinks::rotating_file_sink_mt >(path, maxSize, maxFiles));

    // UI sink
    d->mUiSink = std::make_shared< DAMessageLogSink >();
    d->mUiSink->set_level(spdlog::level::warn);  // 默认 UI 只显示 warn 及以上
    sinks.emplace_back(d->mUiSink);

    d->mLogger = buildAsyncLogger(sinks, policy);
    d->mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    d->mLogger->set_level(spdlog::level::trace);
    d->mLogger->flush_on(spdlog::level::warn);
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
void DALogger::setupDailyFile(const QString& filename,
                              int maxFiles,
                              bool outputStdout,
                              DAOverflowPolicy policy)
{
    DA_D(d);
#ifdef SPDLOG_WCHAR_FILENAMES
    spdlog::filename_t path = qstringToSystemWString(filename);
#else
    std::string path(filename.toLocal8Bit().constData());
#endif

    std::vector< spdlog::sink_ptr > sinks;
    if (outputStdout) {
        sinks.emplace_back(std::make_shared< spdlog::sinks::stdout_color_sink_mt >());
    }
    // daily_file_sink_mt: rotation_hour=0, rotation_minute=0（每天午夜轮转）
    sinks.emplace_back(std::make_shared< spdlog::sinks::daily_file_sink_mt >(path, 0, 0, false, maxFiles));

    // UI sink
    d->mUiSink = std::make_shared< DAMessageLogSink >();
    d->mUiSink->set_level(spdlog::level::warn);
    sinks.emplace_back(d->mUiSink);

    d->mLogger = buildAsyncLogger(sinks, policy);
    d->mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    d->mLogger->set_level(spdlog::level::trace);
    d->mLogger->flush_on(spdlog::level::warn);
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
    std::vector< spdlog::sink_ptr > sinks;
    sinks.emplace_back(std::make_shared< spdlog::sinks::stdout_color_sink_mt >());

    // UI sink
    d->mUiSink = std::make_shared< DAMessageLogSink >();
    d->mUiSink->set_level(spdlog::level::warn);
    sinks.emplace_back(d->mUiSink);

    d->mLogger = buildAsyncLogger(sinks, policy);
    d->mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    d->mLogger->set_level(spdlog::level::trace);
    d->mLogger->flush_on(spdlog::level::info);
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
}

/**
 * @brief 设置 UI 队列的日志级别
 * @param level
 */
void DALogger::setQueueLevel(DALogLevel level)
{
    DA_D(d);
    if (d->mUiSink) {
        d->mUiSink->set_level(mapDALogLevelToSpdlog(level));
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
        // 通过设置 sink level 为 off 来禁用
        d->mUiSink->set_level(on ? spdlog::level::warn : spdlog::level::off);
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
 * 将 Qt 消息转发到 spdlog，spdlog 通过多 sink 分发到文件、控制台、UI 队列。
 * @param type
 * @param context
 * @param msg
 */
void DALogger::daMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    auto level = mapQtMsgTypeToSpdlog(type);
    // 通过 source_loc 传递 file/line/function，null-safe
    spdlog::source_loc loc{ context.file, context.line, context.function };
    // 使用 toUtf8 确保跨平台 UTF-8 编码（Windows 下 toStdString 使用系统 locale 编码会导致乱码）
    spdlog::default_logger()->log(loc, level, "{}", msg.toUtf8().toStdString());

    if (type == QtFatalMsg) {
        // Fatal 消息需要同步 flush 确保落盘，Qt 会在 handler 返回后自动 abort
        spdlog::default_logger()->flush();
    }
}

}  // namespace DA
