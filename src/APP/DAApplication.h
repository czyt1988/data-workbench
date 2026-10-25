#ifndef DAAPPLICATION_H
#define DAAPPLICATION_H
// Qt
#include <QApplication>
#include <QPointer>
// stl
#include <atomic>
#include <memory>

class QTimer;

namespace DA
{

class DASlowLogWatchdog;

/**
 * @brief 应用类（通用载体）：当前承担慢日志的事件循环计时与主线程卡顿看门狗
 *
 * 层次 B（事件循环埋点）：重写 notify() 对所有经主线程事件循环派发的事件计时，
 * 按事件类型聚合（固定桶数组），单次超阈值事件立即输出明细（带接收者类名）；
 * 聚合输出由定时器按 DA_SLOW_LOG_AGGREGATE_MS 周期触发。
 *
 * 层次 C（卡顿看门狗）：以成员方式持有 DASlowLogWatchdog，主线程 QTimer 提供
 * 50ms 心跳，看门狗线程监测心跳停滞并输出 mainloop.stall 记录。
 *
 * 本类只做事件计时与看门狗生命周期；聚合计数器与落盘能力都在 DAMessageHandler
 * （DASlowLog/DALogger）。慢日志相关成员全部集中在编译期开关内，关闭时 notify()
 * 仅转发基类，零残留。后续应用级能力（全局异常处理等）可继续加入本类。
 */
class DAApplication : public QApplication
{
    Q_OBJECT
public:
    DAApplication(int& argc, char** argv);
    ~DAApplication() override;

    bool notify(QObject* receiver, QEvent* event) override;

private:
#if defined(DA_ENABLE_SLOW_LOG)
    // 事件聚合桶：内置事件类型 0~1023 一桶，其余归入 other 桶；
    // notify 只在主线程计时（见 notify 实现），普通成员即可，不加锁
    struct EventBucket
    {
        quint64 count { 0 };
        quint64 totalNs { 0 };
        quint64 maxNs { 0 };
    };
    static constexpr int kEventBucketCount = 1024;

    void recordEvent(int typeValue, qint64 ns, const QPointer< QObject >& receiver);
    void flushEventAggregates();

    std::atomic< quint64 > mHeartbeat { 0 };  ///< 主线程心跳计数（QTimer 递增，看门狗线程读取）
    QTimer* mHeartbeatTimer { nullptr };
    QTimer* mAggregateTimer { nullptr };
    std::unique_ptr< DASlowLogWatchdog > mWatchdog;
    EventBucket mEventBuckets[kEventBucketCount];
    EventBucket mEventBucketOther;
#endif
};

}  // namespace DA

#endif  // DAAPPLICATION_H
