#ifndef DALOGGER_H
#define DALOGGER_H
#include "DAMessageHandlerGlobal.h"
#include <QString>

namespace DA
{
/**
 * @brief 日志级别枚举
 *
 * 与 spdlog::level::level_enum 一一对应，但不直接暴露 spdlog 类型给消费者
 */
enum class DALogLevel
{
    Trace,     ///< 详细跟踪
    Debug,     ///< 调试信息
    Info,      ///< 一般信息
    Warn,      ///< 警告
    Error,     ///< 错误
    Critical,  ///< 严重错误
    Off        ///< 关闭日志
};

/**
 * @brief 异步队列溢出策略
 */
enum class DAOverflowPolicy
{
    Block,         ///< 队列满时阻塞调用线程
    OverrunOldest  ///< 丢弃最旧的消息（推荐）
};

/**
 * @brief RAII 日志管理器（单例）
 *
 * 整合 Qt qInstallMessageHandler 与 spdlog，提供统一的日志分发：
 *
 * - qDebug/qInfo/qWarning/qCritical → daMessageHandler → 按 category 分流
 * - 业务日志（context.category 以 "da." 开头）→ mLogger（带 UI sink）→ DAMessageLogQueue → UI
 * - 系统日志（Qt 自身、第三方库）→ mSystemLogger（仅文件+控制台，不进 UI 队列）
 * - spdlog 通过多 sink 分发到文件、控制台、UI 队列
 * - DAMessageLogSink 从 log_msg 重建 DAMessageLogItem，推送到 DAMessageLogQueue
 * - DAMessageLogsModel 订阅 DAMessageLogQueue 信号，更新 UI
 *
 * 业务代码通过 daInfo/daWarning/daCritical/daDebug 便捷宏（见 DALogCategory.h）
 * 打印日志，category 为 "da.user"，会自动进入 UI 队列；
 * 未声明 category 的 qDebug/qInfo 等（含 Qt 自身、第三方库）只写文件和控制台，不会污染 UI。
 *
 * @note RAII：析构时自动 qInstallMessageHandler(nullptr) → spdlog::shutdown()，
 *       无需手动调用注销函数。
 *
 * @code
 * // 典型用法（main.cpp）
 * DA::DALogger::instance().setupRotatingFile(logFilePath);
 * // ... QApplication app(argc, argv); app.exec();
 * // 无需手动注销，程序退出时 DALogger 单例析构自动清理
 * @endcode
 *
 * @code
 * // 业务模块打印日志（引入 DALogCategory.h 即可）
 * daInfo << "kernel initialized, version =" << DA_VERSION;
 * daWarning << "config file not found:" << path;
 * @endcode
 *
 * @see DAMessageLogQueue
 * @see DAMessageLogSink
 * @see DALogCategory.h  (daInfo/daWarning/daCritical/daDebug 便捷宏)
 */
class DAMESSAGEHANDLER_API DALogger
{
    DA_DECLARE_PRIVATE(DALogger)
public:
    // 获取全局单例
    static DALogger& instance();

    // 析构，自动注销 message handler 并关闭 spdlog
    ~DALogger();

    // 设置旋转文件日志（文件达到 maxSize 后自动旋转，保留最多 maxFiles 个历史文件）
    void setupRotatingFile(const QString& filename,
                           int maxSize             = 10 * 1024 * 1024,
                           int maxFiles            = 5,
                           bool outputStdout       = true,
                           DAOverflowPolicy policy = DAOverflowPolicy::OverrunOldest);

    // 设置按日期分割的文件日志（每天生成一个新文件，保留最多 maxFiles 个历史文件）
    void setupDailyFile(const QString& filename,
                        int maxFiles            = 5,
                        bool outputStdout       = true,
                        DAOverflowPolicy policy = DAOverflowPolicy::OverrunOldest);

    // 设置控制台日志（无文件输出）
    void setupConsole(DAOverflowPolicy policy = DAOverflowPolicy::OverrunOldest);

    // 设置日志级别（影响所有 sink）
    void setLevel(DALogLevel level);

    // 设置 spdlog 原生 pattern
    void setPattern(const QString& pattern);

    // 设置 UI 队列的日志级别（per-sink level）
    void setQueueLevel(DALogLevel level);

    // 设置 flush 间隔（秒）
    void setFlushInterval(int seconds);

    // 启用/禁用 UI 队列捕获
    void setQueueCaptureEnabled(bool on);

    // 判断 UI 队列捕获是否启用
    bool isQueueCaptureEnabled() const;

private:
    DALogger();
    Q_DISABLE_COPY(DALogger)
    void installMessageHandler();
    // 按 category 分流：da.* 开头走业务 logger（带 UI sink），其他走系统 logger
    void dispatchMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static void daMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);
};

}  // namespace DA
#endif  // DALOGGER_H
