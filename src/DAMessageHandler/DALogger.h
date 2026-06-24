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
    Block,          ///< 队列满时阻塞调用线程
    OverrunOldest   ///< 丢弃最旧的消息（推荐）
};

/**
 * @brief RAII 日志管理器（单例）
 *
 * 整合 Qt qInstallMessageHandler 与 spdlog，提供统一的日志分发：
 *
 * - qDebug/qInfo/qWarning/qCritical → daMessageHandler → spdlog::async_logger
 * - spdlog 通过多 sink 分发到文件、控制台、UI 队列
 * - DAMessageLogSink 从 log_msg 重建 DAMessageLogItem，推送到 DAMessageLogQueue
 * - DAMessageLogsModel 订阅 DAMessageLogQueue 信号，更新 UI
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
 * @see DAMessageLogQueue
 * @see DAMessageLogSink
 */
class DAMESSAGEHANDLER_API DALogger
{
    DA_DECLARE_PRIVATE(DALogger)
public:
    /**
     * @brief 获取全局单例
     * @return
     */
    static DALogger& instance();

    /**
     * @brief 析构，自动注销 message handler 并关闭 spdlog
     */
    ~DALogger();

    /**
     * @brief 设置旋转文件日志
     *
     * 文件达到 maxSize 后自动旋转，保留最多 maxFiles 个历史文件。
     * @param filename 日志文件路径
     * @param maxSize 单个文件最大字节数（默认 10MB）
     * @param maxFiles 最大文件数量（默认 5）
     * @param outputStdout 是否同时输出到控制台
     * @param policy 异步队列溢出策略（默认 OverrunOldest）
     */
    void setupRotatingFile(const QString& filename,
                           int maxSize           = 10 * 1048576,
                           int maxFiles          = 5,
                           bool outputStdout     = true,
                           DAOverflowPolicy policy = DAOverflowPolicy::OverrunOldest);

    /**
     * @brief 设置按日期分割的文件日志
     *
     * 每天生成一个新文件，保留最多 maxFiles 个历史文件。
     * @param filename 日志文件路径基名
     * @param maxFiles 最大文件数量（默认 5）
     * @param outputStdout 是否同时输出到控制台
     * @param policy 异步队列溢出策略（默认 OverrunOldest）
     */
    void setupDailyFile(const QString& filename,
                        int maxFiles          = 5,
                        bool outputStdout     = true,
                        DAOverflowPolicy policy = DAOverflowPolicy::OverrunOldest);

    /**
     * @brief 设置控制台日志（无文件输出）
     * @param policy 异步队列溢出策略（默认 OverrunOldest）
     */
    void setupConsole(DAOverflowPolicy policy = DAOverflowPolicy::OverrunOldest);

    /**
     * @brief 设置日志级别（影响所有 sink）
     * @param level
     */
    void setLevel(DALogLevel level);

    /**
     * @brief 设置 spdlog 原生 pattern
     *
     * 常用占位符：
     * - %l 级别名 (debug/info/warning/error/critical)
     * - %L 级别首字母 (D/I/W/E/C)
     * - %v 消息内容
     * - %s 文件名
     * - %# 行号
     * - %! 函数名
     * - %Y-%m-%d %H:%M:%S.%e 时间戳
     * @param pattern spdlog pattern 字符串
     */
    void setPattern(const QString& pattern);

    /**
     * @brief 设置 UI 队列的日志级别（per-sink level）
     *
     * 例如设为 Warn，则只有 Warning 及以上消息会推送到 UI 队列，
     * 文件和控制台仍记录所有级别。
     * @param level
     */
    void setQueueLevel(DALogLevel level);

    /**
     * @brief 设置 flush 间隔（秒）
     * @param seconds
     */
    void setFlushInterval(int seconds);

    /**
     * @brief 启用/禁用 UI 队列捕获
     * @param on
     */
    void setQueueCaptureEnabled(bool on);

    /**
     * @brief 判断 UI 队列捕获是否启用
     * @return
     */
    bool isQueueCaptureEnabled() const;

private:
    DALogger();
    Q_DISABLE_COPY(DALogger)
    void installMessageHandler();
    static void daMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);
};

}  // namespace DA
#endif  // DALOGGER_H
