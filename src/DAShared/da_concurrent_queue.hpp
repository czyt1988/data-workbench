#ifndef DA_CONCURRENT_QUEUE_H
#define DA_CONCURRENT_QUEUE_H
#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * @brief 这个是专门为生产者消费者实现的安全FIFO
 *
 * 可以实现空等待，满等待
 */
template< typename T >
class da_concurrent_queue
{
public:
    using queue_type       = std::queue< T >;
    using value_type       = typename queue_type::value_type;
    using reference        = typename queue_type::reference;
    using const_reference  = typename queue_type::const_reference;
    using size_type        = typename queue_type::size_type;
    using mutex_type       = std::mutex;
    using lock_guard_type  = std::lock_guard< mutex_type >;
    using unique_lock_type = std::unique_lock< mutex_type >;

public:
    da_concurrent_queue();
    da_concurrent_queue(size_type capacity);
    bool        empty() const;
    std::size_t size() const;
    void        push(const T& v);
    void        set(const T& v);
    T           get();
    T           get(int waitms);

private:
    size_type               mCapacity;  ///< 容积,0代表不做限制
    queue_type              mFifo;      ///< 缓冲队列
    mutable mutex_type      mMutex;     ///< 互斥锁
    std::condition_variable mPushWait;  ///< 写入条件变量
    std::condition_variable mPopWait;   ///< 写入条件变量
};

// 构造函数，不限制容量的队列
template< typename T >
da_concurrent_queue< T >::da_concurrent_queue() : mCapacity(0)
{
}

// 构造函数，可指定最大容量
template< typename T >
da_concurrent_queue< T >::da_concurrent_queue(size_type capacity) : mCapacity(capacity)
{
}

// 队列是否为空
template< typename T >
bool da_concurrent_queue< T >::empty() const
{
    std::lock_guard< std::mutex > lg(mMutex);

    return (mFifo.empty());
}

// 推入fifo
template< typename T >
void da_concurrent_queue< T >::push(const T& v)
{
    std::unique_lock< std::mutex > lg(mMutex);

    if (mCapacity > 0) {
        //只有限定容积时才做推入等待
        while (mFifo.size() >= mCapacity) {
            mPushWait.wait(lg);
        }
    }
    mFifo.push(v);
    mPopWait.notify_one();
}

template< typename T >
void da_concurrent_queue< T >::set(const T& v)
{
    push(v);
}

// 推出
template< typename T >
T da_concurrent_queue< T >::get()
{
    std::unique_lock< std::mutex > lg(mMutex);

    while (mFifo.empty()) {
        mPopWait.wait(lg);
    }
    T v = std::move(mFifo.front());

    mFifo.pop();
    if (mCapacity > 0) {
        //如果有推出，则推入的等待可以唤醒
        mPushWait.notify_one();
    }
    return v;
}

// 有等待时间的获取
template< typename T >
T da_concurrent_queue< T >::get(int waitms)
{
    std::unique_lock< std::mutex > lg(mMutex);
    if (!mPopWait.wait_for(lg, std::chrono::milliseconds(waitms), [this] { return !(this->mFifo.empty()); })) {
        return T();
    }
    T v = std::move(mFifo.front());

    mFifo.pop();
    if (mCapacity > 0) {
        //如果有推出，则推入的等待可以唤醒
        mPushWait.notify_one();
    }
    return v;
}

// 获取队列的尺寸
template< typename T >
std::size_t da_concurrent_queue< T >::size() const
{
    std::lock_guard< std::mutex > lg(mMutex);

    return (mFifo.size());
}

#endif  // DA_CONCURRENT_QUEUE_H
