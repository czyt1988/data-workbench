#include "DASlowLogWatchdog.h"
// DAMessageHandler
#include "DALog.h"
// Qt
#include <QString>
// stl
#include <chrono>

namespace DA
{

namespace
{

// __FILE__ 截成文件名（与 DASlowLog 输出字段口径一致）
const char* watchdogBaseFileName(const char* path)
{
    if (!path) {
        return "-";
    }
    const char* base = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

}  // namespace

DASlowLogWatchdog::DASlowLogWatchdog(std::atomic< quint64 >* heartbeat) : mHeartbeat(heartbeat)
{
}

DASlowLogWatchdog::~DASlowLogWatchdog()
{
    stop();
}

/**
 * @brief 启动监听线程（重复调用无效）
 */
void DASlowLogWatchdog::start()
{
    if (mRunning.load(std::memory_order_acquire)) {
        return;
    }
    mRunning.store(true, std::memory_order_release);
    mThread = std::thread([this]() { run(); });
}

/**
 * @brief 停止监听线程并 join（幂等，最长阻塞一个检查周期 100ms）
 */
void DASlowLogWatchdog::stop()
{
    if (!mRunning.exchange(false)) {
        return;
    }
    if (mThread.joinable()) {
        mThread.join();
    }
}

/**
 * @brief 监听循环：每 100ms 检查心跳，卡顿恢复时输出一条 mainloop.stall
 */
void DASlowLogWatchdog::run()
{
    using Clock = std::chrono::steady_clock;
    quint64 lastBeat       = mHeartbeat ? mHeartbeat->load(std::memory_order_relaxed) : 0;
    Clock::time_point lastChange = Clock::now();
    bool stalled             = false;
    DASlowLog::ScopeInfo lastScope;  // 进入卡顿状态时快照主线程栈顶作用域

    while (mRunning.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const quint64 beat       = mHeartbeat ? mHeartbeat->load(std::memory_order_relaxed) : 0;
        const Clock::time_point now = Clock::now();
        if (beat != lastBeat) {
            lastBeat = beat;
            if (stalled) {
                // 心跳恢复：计算本次卡顿时长并输出一条（含卡住时最后进入的作用域）
                const qint64 stallMs = std::chrono::duration_cast< std::chrono::milliseconds >(now - lastChange).count();
                QString detail;
                if (lastScope.tag) {
                    detail = QStringLiteral("last_scope=%1 file=%2 line=%3")
                                 .arg(QString::fromUtf8(lastScope.tag))
                                 .arg(QString::fromUtf8(watchdogBaseFileName(lastScope.file)))
                                 .arg(lastScope.line);
                } else {
                    detail = QStringLiteral("last_scope=-");
                }
                DASlowLog::emitScopeRecord("mainloop.stall", stallMs * 1000000, stallMs * 1000000, 0,
                                           __FILE__, __LINE__, "run", detail, "watchdog");
                stalled = false;
            }
            lastChange = now;
        } else if (!stalled) {
            const qint64 sinceMs = std::chrono::duration_cast< std::chrono::milliseconds >(now - lastChange).count();
            if (sinceMs >= DASlowLog::stallThresholdMs()) {
                // 进入卡顿状态：只记录开始时刻与当时作用域，不立即输出
                stalled   = true;
                lastScope = DASlowLog::currentMainThreadScope();
            }
        }
    }
}

}  // namespace DA
