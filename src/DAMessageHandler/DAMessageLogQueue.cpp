#include "DAMessageLogQueue.h"
// Qt
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QCoreApplication>
// stl
#include <atomic>
#include <deque>

namespace DA
{
class DAMessageLogQueue::PrivateData
{
    DA_DECLARE_PUBLIC(DAMessageLogQueue)
public:
    PrivateData(DAMessageLogQueue* p);
    ~PrivateData();

public:
    mutable QMutex mMutex;                             ///< 保护 mMessages 和 mCapacity
    std::deque< DAMessageLogItem > mMessages;          ///< 消息缓冲（O(1) 头尾操作）
    int mCapacity { 1000 };                            ///< 容量
    std::atomic_int mCachedSize { 0 };                 ///< 缓存的队列大小（供 onTimeout 等无锁读取）

    std::unique_ptr< QTimer > mTimer;                 ///< 惰性发射定时器
    int mEmitIntervalMs { 1000 };                     ///< 发射间隔
    bool mIsLazyEmit { true };                        ///< 是否惰性发射
    std::atomic_bool mNeedEmitAppended { false };     ///< 标记需要发射 messageQueueAppended
    std::atomic_bool mNeedEmitSizeChanged { false };  ///< 标记需要发射 messageQueueSizeChanged
    std::atomic_bool mDelayCreateTimer { false };  ///< QApp 未启动时延迟创建 timer，push 检测到后通过 QueuedConnection 在主线程创建
};

DAMessageLogQueue::PrivateData::PrivateData(DAMessageLogQueue* p) : q_ptr(p)
{
}

DAMessageLogQueue::PrivateData::~PrivateData()
{
}

//===================================================
// DAMessageLogQueue
//===================================================

DAMessageLogQueue::DAMessageLogQueue() : QObject(nullptr), DA_PIMPL_CONSTRUCT
{
    ensureTimer();
}

DAMessageLogQueue::~DAMessageLogQueue()
{
}

// 文件内辅助说明：singletonPtr() 是 DAMessageLogQueue 的静态私有成员，
// 作为 atexit 析构链的锚点。析构顺序：DALogger（后注册）先析构 → s_queue（先注册）后析构，
// 保证 DALogger 调用 spdlog::shutdown() 时 queue 仍然存活。
std::shared_ptr< DAMessageLogQueue >& DAMessageLogQueue::singletonPtr()
{
    static std::shared_ptr< DAMessageLogQueue > s_queue(new DAMessageLogQueue());
    return s_queue;
}

/**
 * @brief 获取全局单例
 * @return
 */
DAMessageLogQueue& DAMessageLogQueue::instance()
{
    return *singletonPtr();
}

/**
 * @brief 获取单例的 weak_ptr（供 DAMessageLogSink 安全访问）
 *
 * 程序退出时单例析构后，weak_ptr 会 lock 失败，sink 据此跳过推送，
 * 避免 spdlog 后台线程在 queue 析构后访问已释放内存。
 * @return
 */
std::weak_ptr< DAMessageLogQueue > DAMessageLogQueue::weakInstance()
{
    return std::weak_ptr< DAMessageLogQueue >(singletonPtr());
}

/**
 * @brief 推入一条消息（由 DAMessageLogSink 在后台线程调用）
 *
 * 队列满后循环覆写最旧消息（O(1)）。信号标记为惰性发射模式由 timer 统一发射，
 * 非惰性模式立即发射。
 * @param item
 */
void DAMessageLogQueue::push(const DAMessageLogItem& item)
{
    push(DAMessageLogItem(item));  // 拷贝后转发到右值重载，避免信号逻辑重复
}

/**
 * @brief 推入一条消息（右值版本，避免拷贝）
 * @param item
 */
void DAMessageLogQueue::push(DAMessageLogItem&& item)
{
    DA_D(d);
    bool sizeChanged = false;
    int newSize = 0;
    {
        QMutexLocker lc(&d->mMutex);
        if (static_cast< int >(d->mMessages.size()) >= d->mCapacity) {
            d->mMessages.pop_front();  // std::deque: O(1)
        } else {
            sizeChanged = true;
        }
        d->mMessages.push_back(std::move(item));
        newSize = static_cast< int >(d->mMessages.size());
        d->mCachedSize.store(newSize, std::memory_order_release);
    }
    // 标记信号（惰性模式由 timer 发射，非惰性模式立即发射）
    if (sizeChanged) {
        d->mNeedEmitSizeChanged.store(true, std::memory_order_release);
    }
    d->mNeedEmitAppended.store(true, std::memory_order_release);
    // 若 timer 延迟创建标记为 true 且 QApp 已启动，通过主线程事件循环创建 timer。
    // push 在 spdlog 后台线程调用，不能直接创建 QTimer（QTimer 必须在主线程创建）。
    if (d->mDelayCreateTimer.load(std::memory_order_acquire)) {
        if (!QCoreApplication::startingUp() && !QCoreApplication::closingDown()) {
            QMetaObject::invokeMethod(this, [ this ]() { ensureTimer(); }, Qt::QueuedConnection);
        }
    }
    if (!d->mIsLazyEmit) {
        if (QCoreApplication::startingUp() || QCoreApplication::closingDown()) {
            return;
        }
        // 非惰性模式：立即发射（跨线程时 Qt 自动使用 QueuedConnection）
        if (d->mNeedEmitAppended.exchange(false, std::memory_order_acq_rel)) {
            emit messageQueueAppended();
        }
        if (d->mNeedEmitSizeChanged.exchange(false, std::memory_order_acq_rel)) {
            emit messageQueueSizeChanged(newSize);  // 使用持锁时缓存的值
        }
    }
}

/**
 * @brief 获取指定索引的消息
 * @param index
 * @return 索引越界时返回无效的 DAMessageLogItem
 */
DAMessageLogItem DAMessageLogQueue::at(int index) const
{
    DA_DC(dc);
    QMutexLocker lc(&dc->mMutex);
    if (index < 0 || index >= static_cast< int >(dc->mMessages.size())) {
        return DAMessageLogItem();  // 返回无效项
    }
    return dc->mMessages[ static_cast< std::size_t >(index) ];
}

/**
 * @brief 队列当前消息数量
 * @return
 */
int DAMessageLogQueue::size() const
{
    DA_DC(dc);
    QMutexLocker lc(&dc->mMutex);
    return static_cast< int >(dc->mMessages.size());
}

/**
 * @brief 清空队列
 */
void DAMessageLogQueue::clear()
{
    DA_D(d);
    int oldSize = 0;
    {
        QMutexLocker lc(&d->mMutex);
        oldSize = static_cast< int >(d->mMessages.size());
        d->mMessages.clear();
        d->mCachedSize.store(0, std::memory_order_release);
    }
    if (oldSize > 0) {
        d->mNeedEmitSizeChanged.store(true, std::memory_order_release);
        if (!d->mIsLazyEmit) {
            if (!QCoreApplication::startingUp() && !QCoreApplication::closingDown()) {
                if (d->mNeedEmitSizeChanged.exchange(false, std::memory_order_acq_rel)) {
                    emit messageQueueSizeChanged(0);
                }
            }
        }
    }
}

/**
 * @brief 设置队列容量（环形缓冲大小）
 *
 * 若新容量小于当前消息数，截断多余消息。忽略非正容量。
 * @param c
 */
void DAMessageLogQueue::setCapacity(int c)
{
    if (c <= 0) {
        return;  // 忽略非法值
    }
    bool truncated = false;
    DA_D(d);
    {
        QMutexLocker lc(&d->mMutex);
        d->mCapacity = c;
        // 若当前消息数超过新容量，截断多余消息
        while (static_cast< int >(d->mMessages.size()) > c) {
            d->mMessages.pop_front();
            truncated = true;
        }
        d->mCachedSize.store(static_cast< int >(d->mMessages.size()), std::memory_order_release);
    }
    if (truncated) {
        d->mNeedEmitSizeChanged.store(true, std::memory_order_release);
    }
}

/**
 * @brief 获取队列容量
 * @return
 */
int DAMessageLogQueue::capacity() const
{
    DA_DC(dc);
    QMutexLocker lc(&dc->mMutex);
    return dc->mCapacity;
}

/**
 * @brief 设置惰性信号发射
 * @param on
 * @param intervalMs
 */
void DAMessageLogQueue::setLazyEmit(bool on, int intervalMs)
{
    DA_D(d);
    d->mIsLazyEmit     = on;
    d->mEmitIntervalMs = intervalMs;
    if (on) {
        ensureTimer();
        if (d->mTimer) {
            d->mTimer->setInterval(intervalMs);
        }
    } else {
        d->mTimer.reset();
    }
}

/**
 * @brief 确保定时器已创建
 *
 * 如果 QApplication 尚未启动，标记延迟创建，等下次调用再创建。
 */
void DAMessageLogQueue::ensureTimer()
{
    DA_D(d);
    if (!d->mIsLazyEmit) {
        return;
    }
    if (QCoreApplication::startingUp() || QCoreApplication::closingDown()) {
        d->mDelayCreateTimer.store(true, std::memory_order_release);
        return;
    }
    if (!d->mTimer) {
        d->mTimer.reset(new QTimer());
        d->mTimer->setInterval(d->mEmitIntervalMs);
        connect(d->mTimer.get(), &QTimer::timeout, this, &DAMessageLogQueue::onTimeout);
        d->mTimer->start();
    }
    d->mDelayCreateTimer.store(false, std::memory_order_release);
}

/**
 * @brief 定时器超时，检查并发射信号
 */
void DAMessageLogQueue::onTimeout()
{
    DA_D(d);
    if (d->mDelayCreateTimer.load(std::memory_order_acquire)) {
        ensureTimer();
    }
    bool needAppended    = d->mNeedEmitAppended.exchange(false, std::memory_order_acq_rel);
    bool needSizeChanged = d->mNeedEmitSizeChanged.exchange(false, std::memory_order_acq_rel);
    if (needAppended) {
        emit messageQueueAppended();
    }
    if (needSizeChanged) {
        emit messageQueueSizeChanged(d->mCachedSize.load(std::memory_order_acquire));
    }
}

}  // namespace DA
