#include "DAApplication.h"
// DAMessageHandler
#include "DALog.h"
// Qt
#include <QEvent>
#include <QThread>
#include <QTimer>
// stl
#include <chrono>
#include <cstdio>
#include <thread>

#if defined(DA_ENABLE_SLOW_LOG)
#include "DASlowLogWatchdog.h"
#endif

namespace DA
{

#if defined(DA_ENABLE_SLOW_LOG)
namespace
{

// 常见事件类型 → 名称（Qt 无内置转字符串接口）；其余返回 type_<数字>。
// 只在慢事件明细与聚合输出时调用，不在每个事件上调用。
// 返回指针指向线程局部缓冲，调用方须立即使用（下一轮调用会覆写）。
const char* eventTypeName(int typeValue)
{
    switch (static_cast< QEvent::Type >(typeValue)) {
    case QEvent::Timer: return "timer";
    case QEvent::Paint: return "paint";
    case QEvent::UpdateRequest: return "updateRequest";
    case QEvent::Resize: return "resize";
    case QEvent::LayoutRequest: return "layoutRequest";
    case QEvent::MouseMove: return "mouseMove";
    case QEvent::MouseButtonPress: return "mouseButtonPress";
    case QEvent::MouseButtonRelease: return "mouseButtonRelease";
    case QEvent::MouseButtonDblClick: return "mouseButtonDblClick";
    case QEvent::Wheel: return "wheel";
    case QEvent::KeyPress: return "keyPress";
    case QEvent::KeyRelease: return "keyRelease";
    case QEvent::DragEnter: return "dragEnter";
    case QEvent::DragMove: return "dragMove";
    case QEvent::DragLeave: return "dragLeave";
    case QEvent::Drop: return "drop";
    case QEvent::MetaCall: return "metaCall";
    case QEvent::DeferredDelete: return "deferredDelete";
    case QEvent::ChildAdded: return "childAdded";
    case QEvent::ChildRemoved: return "childRemoved";
    case QEvent::Show: return "show";
    case QEvent::Hide: return "hide";
    case QEvent::Close: return "close";
    case QEvent::FocusIn: return "focusIn";
    case QEvent::FocusOut: return "focusOut";
    case QEvent::ToolTip: return "toolTip";
    case QEvent::ContextMenu: return "contextMenu";
    case QEvent::WindowActivate: return "windowActivate";
    case QEvent::WindowDeactivate: return "windowDeactivate";
    default: break;
    }
    static thread_local char sBuf[32];
    std::snprintf(sBuf, sizeof(sBuf), "type_%d", typeValue);
    return sBuf;
}

}  // namespace
#endif

/**
 * @brief 构造：登记主线程、启动心跳/聚合定时器与卡顿看门狗（仅慢日志编译且运行期开启时）
 */
DAApplication::DAApplication(int& argc, char** argv) : QApplication(argc, argv)
{
#if defined(DA_ENABLE_SLOW_LOG)
    // 供慢日志作用域判定主线程（发布栈帧给看门狗读取）
    DASlowLog::setMainThreadId(std::this_thread::get_id());
    if (DASlowLog::isEnabled()) {
        // 主线程心跳：每 50ms 加一，看门狗线程每 100ms 检查
        mHeartbeatTimer = new QTimer(this);
        mHeartbeatTimer->setInterval(50);
        connect(mHeartbeatTimer, &QTimer::timeout, this, [this]() {
            mHeartbeat.fetch_add(1, std::memory_order_relaxed);
        });
        mHeartbeatTimer->start();
        // 聚合输出：高频计数器与事件桶按 DA_SLOW_LOG_AGGREGATE_MS 周期写 slow.log
        mAggregateTimer = new QTimer(this);
        mAggregateTimer->setInterval(static_cast< int >(DASlowLog::aggregateIntervalMs()));
        connect(mAggregateTimer, &QTimer::timeout, this, [this]() {
            DASlowLog::flushCounters();
            flushEventAggregates();
        });
        mAggregateTimer->start();
        // 卡顿看门狗（析构时 stop + join）
        mWatchdog.reset(new DASlowLogWatchdog(&mHeartbeat));
        mWatchdog->start();
    }
#endif
}

DAApplication::~DAApplication()
{
#if defined(DA_ENABLE_SLOW_LOG)
    if (mWatchdog) {
        mWatchdog->stop();
        mWatchdog.reset();
    }
#endif
}

/**
 * @brief 事件派发计时：全部事件计时 + 按类型聚合 + 超阈值单事件明细
 *
 * 只统计主线程派发的事件（聚合桶是无锁普通成员，且慢日志的看门狗语义只关心
 * 主线程）；运行期关闭或处于慢日志输出路径中时仅转发基类，一次原子读开销。
 * 编译期关闭时整个函数体只有基类转发。
 */
bool DAApplication::notify(QObject* receiver, QEvent* event)
{
#if defined(DA_ENABLE_SLOW_LOG)
    if (!DASlowLog::isEnabled() || !DASlowLog::isMainThread() || DASlowLog::inEmission()) {
        return QApplication::notify(receiver, event);
    }
    const int typeValue = static_cast< int >(event->type());
    // receiver 可能在事件派发过程中被析构（关闭面板、删除控件等），必须在派发前
    // 建立 QPointer；派发返回后原指针可能已悬空，不能再直接解引用
    QPointer< QObject > receiverGuard(receiver);
    const auto start  = std::chrono::steady_clock::now();
    const bool result = QApplication::notify(receiver, event);
    const qint64 ns   = std::chrono::duration_cast< std::chrono::nanoseconds >(
                          std::chrono::steady_clock::now() - start)
                          .count();
    recordEvent(typeValue, ns, receiverGuard);
    return result;
#else
    return QApplication::notify(receiver, event);
#endif
}

#if defined(DA_ENABLE_SLOW_LOG)

/**
 * @brief 记录一次事件派发耗时：入桶聚合；单次超阈值立即输出明细
 */
void DAApplication::recordEvent(int typeValue, qint64 ns, const QPointer< QObject >& receiver)
{
    EventBucket& b = (typeValue >= 0 && typeValue < kEventBucketCount) ? mEventBuckets[ typeValue ] : mEventBucketOther;
    ++b.count;
    b.totalNs += static_cast< quint64 >(ns);
    if (static_cast< quint64 >(ns) > b.maxNs) {
        b.maxNs = static_cast< quint64 >(ns);
    }

    // 单事件明细：先用「默认阈值与全部标签覆盖中的最小值」快速筛选，
    // 避免每个事件都构造标签字符串查覆盖表；通过筛选后才按 qt.event.<类型名> 精确判定
    if (ns >= DASlowLog::minThresholdMs() * 1000000) {
        const QByteArray tag = QByteArrayLiteral("qt.event.") + eventTypeName(typeValue);
        if (ns >= DASlowLog::thresholdMs(tag.constData()) * 1000000) {
            const QString detail = QStringLiteral("receiver=%1")
                                       .arg(receiver ? QString::fromLatin1(receiver->metaObject()->className())
                                                     : QStringLiteral("-"));
            DASlowLog::emitScopeRecord(tag.constData(), ns, ns, 0, __FILE__, __LINE__, "notify", detail, "main");
        }
    }
}

/**
 * @brief 输出并清零全部非空事件聚合桶（由聚合定时器周期触发）
 */
void DAApplication::flushEventAggregates()
{
    for (int i = 0; i < kEventBucketCount; ++i) {
        EventBucket& b = mEventBuckets[ i ];
        if (b.count == 0) {
            continue;
        }
        DASlowLog::emitEventAggregate(eventTypeName(i), b.count, static_cast< qint64 >(b.totalNs),
                                      static_cast< qint64 >(b.maxNs));
        b        = EventBucket();
    }
    if (mEventBucketOther.count > 0) {
        DASlowLog::emitEventAggregate("other", mEventBucketOther.count,
                                      static_cast< qint64 >(mEventBucketOther.totalNs),
                                      static_cast< qint64 >(mEventBucketOther.maxNs));
        mEventBucketOther = EventBucket();
    }
}

#endif

}  // namespace DA
