#ifndef DAMESSAGELOGQUEUE_H
#define DAMESSAGELOGQUEUE_H
#include "DAMessageHandlerGlobal.h"
#include "DAMessageLogItem.h"
#include <QObject>
#include <memory>

namespace DA
{
/**
 * @brief 全局日志消息队列（单例）
 *
 * 此类是 spdlog custom sink (DAMessageLogSink) 与 UI (DAMessageLogsModel) 之间的桥梁。
 *
 * - push() 由 DAMessageLogSink 在 spdlog 后台线程调用，线程安全。
 * - at()/size()/clear()/setCapacity() 由 UI 主线程调用。
 * - 使用 QTimer 惰性发射信号，避免高频日志导致 UI 卡顿。
 *
 * @note 由于使用了惰性信号，messageQueueAppended 和 messageQueueSizeChanged 是互斥的：
 * 队列未满时触发 messageQueueSizeChanged，队列满后循环覆写时触发 messageQueueAppended。
 *
 * @note 单例生命周期由内部 shared_ptr 管理。DAMessageLogSink 通过 weakInstance() 持有
 * weak_ptr，在程序退出时若 queue 已析构，sink 的 log() 会安全跳过推送，避免 use-after-free。
 *
 * @see DAMessageLogSink
 * @see DAMessageLogsModel
 */
class DAMESSAGEHANDLER_API DAMessageLogQueue : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAMessageLogQueue)
public:
    /**
     * @brief 获取全局单例
     * @return
     */
    static DAMessageLogQueue& instance();

    /**
     * @brief 获取单例的 weak_ptr（供 DAMessageLogSink 安全访问）
     *
     * 程序退出时单例析构后，weak_ptr 会 lock 失败，sink 据此跳过推送，
     * 避免 spdlog 后台线程在 queue 析构后访问已释放内存。
     * @return
     */
    static std::weak_ptr< DAMessageLogQueue > weakInstance();

    ~DAMessageLogQueue();

    /**
     * @brief 推入一条消息（由 DAMessageLogSink 在后台线程调用）
     * @param item
     */
    void push(const DAMessageLogItem& item);

    /**
     * @brief 获取指定索引的消息
     * @param index
     * @return
     */
    DAMessageLogItem at(int index) const;

    /**
     * @brief 队列当前消息数量
     * @return
     */
    int size() const;

    /**
     * @brief 清空队列
     */
    void clear();

    /**
     * @brief 设置队列容量（环形缓冲大小）
     * @param c
     */
    void setCapacity(int c);

    /**
     * @brief 获取队列容量
     * @return
     */
    int capacity() const;

    /**
     * @brief 设置惰性信号发射
     *
     * 开启时（默认），信号按固定间隔发射；关闭时，每次 push 立即发射。
     * @param on 是否开启惰性发射
     * @param intervalMs 发射间隔（毫秒），默认 1000ms
     */
    void setLazyEmit(bool on, int intervalMs = 1000);

private:
    DAMessageLogQueue();
    Q_DISABLE_COPY(DAMessageLogQueue)
    void onTimeout();
    void ensureTimer();

    // 持有单例的 shared_ptr 控制块，作为 atexit 析构锚点
    static std::shared_ptr< DAMessageLogQueue >& singletonPtr();

Q_SIGNALS:
    /**
     * @brief 有消息插入（队列满后循环覆写时触发）
     */
    void messageQueueAppended();
    /**
     * @brief 消息队列尺寸变化
     * @param newSize 新尺寸
     */
    void messageQueueSizeChanged(int newSize);
};

}  // namespace DA
#endif  // DAMESSAGELOGQUEUE_H
