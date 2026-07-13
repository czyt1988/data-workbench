#include "DAMessageLogSink.h"
#include "DAMessageLogQueue.h"
#include "DAMessageLogItem.h"
// spdlog
#include <spdlog/details/log_msg.h>
// Qt
#include <QDateTime>
#include <QString>
// stl
#include <chrono>
#include <utility>

namespace DA
{

/**
 * @brief spdlog level → QtMsgType 映射
 * @param l spdlog 日志级别
 * @return 对应的 QtMsgType
 */
inline QtMsgType mapSpdlogToQtMsgType(spdlog::level::level_enum l)
{
    switch (l) {
    case spdlog::level::trace:
        return QtDebugMsg;
    case spdlog::level::debug:
        return QtDebugMsg;
    case spdlog::level::info:
        return QtInfoMsg;
    case spdlog::level::warn:
        return QtWarningMsg;
    case spdlog::level::err:
        return QtCriticalMsg;
    case spdlog::level::critical:
        return QtCriticalMsg;
    case spdlog::level::off:
    default:
        return QtDebugMsg;
    }
}

DAMessageLogSink::DAMessageLogSink() : mQueue(DAMessageLogQueue::weakInstance())
{
}

DAMessageLogSink::~DAMessageLogSink()
{
}

/**
 * @brief 从 log_msg 重建 DAMessageLogItem 并推送到全局队列
 *
 * log_msg 携带的信息：
 * - msg.level → QtMsgType（通过 mapSpdlogToQtMsgType）
 * - msg.time → QDateTime（从 system_clock::time_point 转换）
 * - msg.source.filename / funcname / line → 文件名/函数名/行号
 * - msg.payload → 消息内容
 *
 * @note 通过 weak_ptr lock 检查 queue 存活：程序退出时若 queue 已析构，
 * lock() 返回 nullptr，安全跳过推送，避免 use-after-free。
 * @param msg
 */
void DAMessageLogSink::log(const spdlog::details::log_msg& msg)
{
    // weak_ptr lock：queue 已析构时返回 nullptr，安全跳过
    auto queue = mQueue.lock();
    if (!queue) {
        return;
    }

    QtMsgType qtType = mapSpdlogToQtMsgType(msg.level);

    // 从 system_clock::time_point 转换为 QDateTime
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(
        std::chrono::duration_cast< std::chrono::milliseconds >(msg.time.time_since_epoch()).count());

    // source_loc 字段可能为 nullptr
    QString fileName = msg.source.filename ? QString::fromUtf8(msg.source.filename) : "";
    QString funcName = msg.source.funcname ? QString::fromUtf8(msg.source.funcname) : "";

    // payload 是 string_view，需要构造 QString
    // spdlog 内部使用 UTF-8，直接 fromUtf8
    QString payload = QString::fromUtf8(msg.payload.data(), static_cast< int >(msg.payload.size()));

    DAMessageLogItem item(qtType, fileName, funcName, msg.source.line, payload, dt);
    queue->push(std::move(item));
}

/**
 * @brief flush（空实现，队列无需 flush）
 */
void DAMessageLogSink::flush()
{
    // 队列无需 flush
}

/**
 * @brief set_pattern（空实现，sink 不需要格式化）
 * @param pattern
 */
void DAMessageLogSink::set_pattern(const std::string& pattern)
{
    // sink 不需要格式化，直接使用 log_msg 原始字段
    Q_UNUSED(pattern);
}

/**
 * @brief set_formatter（空实现，sink 不需要格式化）
 * @param sink_formatter
 */
void DAMessageLogSink::set_formatter(std::unique_ptr< spdlog::formatter > sink_formatter)
{
    Q_UNUSED(sink_formatter);
}

}  // namespace DA
