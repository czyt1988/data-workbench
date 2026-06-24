#ifndef DAMESSAGELOGSINK_H
#define DAMESSAGELOGSINK_H
#include "DAMessageHandlerGlobal.h"
#include <spdlog/sinks/sink.h>

namespace DA
{
/**
 * @brief spdlog 自定义 sink，将日志推送到 DAMessageLogQueue 供 UI 订阅
 *
 * 此 sink 在 spdlog 后台线程（async_logger 模式下）被调用，
 * 从 spdlog::details::log_msg 重建 DAMessageLogItem 并推送到全局队列。
 *
 * @note 继承 spdlog::sinks::sink（非 base_sink），因为 async_logger 已在
 * pool thread 序列化调用 sink，无需额外锁保护。
 *
 * @see DAMessageLogQueue
 * @see DALogger
 */
class DAMESSAGEHANDLER_API DAMessageLogSink : public spdlog::sinks::sink
{
public:
    DAMessageLogSink();
    ~DAMessageLogSink() override;

    /**
     * @brief 从 log_msg 重建 DAMessageLogItem 并推送到队列
     * @param msg spdlog 日志消息
     */
    void log(const spdlog::details::log_msg& msg) override;

    /**
     * @brief flush（空实现，队列无需 flush）
     */
    void flush() override;

    /**
     * @brief set_pattern（空实现，sink 不需要格式化）
     */
    void set_pattern(const std::string& pattern) override;

    /**
     * @brief set_formatter（空实现，sink 不需要格式化）
     */
    void set_formatter(std::unique_ptr< spdlog::formatter > sink_formatter) override;
};

}  // namespace DA
#endif  // DAMESSAGELOGSINK_H
