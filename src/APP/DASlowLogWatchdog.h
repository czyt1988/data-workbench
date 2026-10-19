#ifndef DASLOWLOGWATCHDOG_H
#define DASLOWLOGWATCHDOG_H
// Qt
#include <QtGlobal>
// stl
#include <atomic>
#include <thread>

namespace DA
{

/**
 * @brief 主线程卡顿看门狗：独立线程监测心跳，把卡顿区间记录进慢日志
 *
 * 心跳由 DAApplication 的 QTimer（50ms）在主线程递增；本类以 std::thread
 * 每 100ms 检查一次：心跳计数在 DA_SLOW_LOG_STALL_MS（默认 500ms）内没有
 * 变化即进入卡顿状态（记录开始时刻与主线程当时的栈顶作用域，不立即输出，
 * 避免一条卡顿刷出多条）；心跳恢复变化时输出一条 mainloop.stall 记录，
 * detail 带卡住时主线程最后进入的作用域（DASlowLog::currentMainThreadScope，
 * 无活动作用域时为 last_scope=-）。
 *
 * 已知误报（预期噪声，见实施方案 §9.3）：
 * - 调试器断点暂停主线程会被记录为卡顿；
 * - 系统睡眠/休眠恢复后会记录一段很长的卡顿。
 *
 * 生命周期由 DAApplication 持有：构造后 start()，析构前 stop()（幂等，join 监听线程）。
 */
class DASlowLogWatchdog
{
public:
    explicit DASlowLogWatchdog(std::atomic< quint64 >* heartbeat);
    ~DASlowLogWatchdog();

    void start();
    void stop();  ///< 幂等：置停止标志并 join 监听线程

    DASlowLogWatchdog(const DASlowLogWatchdog&)            = delete;
    DASlowLogWatchdog& operator=(const DASlowLogWatchdog&) = delete;

private:
    void run();

    std::atomic< quint64 >* mHeartbeat;  ///< 主线程心跳计数（只读）
    std::atomic< bool > mRunning { false };
    std::thread mThread;
};

}  // namespace DA

#endif  // DASLOWLOGWATCHDOG_H
