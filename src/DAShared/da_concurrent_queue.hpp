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
    size_type               m_capacity;  ///< 容积,0代表不做限制
    queue_type              m_fifo;      ///< 缓冲队列
    mutable mutex_type      m_mutex;     ///< 互斥锁
    std::condition_variable m_pushWait;  ///< 写入条件变量
    std::condition_variable m_popWait;   ///< 写入条件变量
};

/**
 * @brief 构造函数，不限制容量的队列
 */
template< typename T >
da_concurrent_queue< T >::da_concurrent_queue() : m_capacity(0)
{
}

/**
 * @brief 构造函数，可指定最大容量
 * @param capacity 容量，0代表不做限制
 */
template< typename T >
da_concurrent_queue< T >::da_concurrent_queue(size_type capacity) : m_capacity(capacity)
{
}

/**
 * @brief 队列是否为空
 * @return
 */
template< typename T >
bool da_concurrent_queue< T >::empty() const
{
    std::lock_guard< std::mutex > lg(m_mutex);

    return (m_fifo.empty());
}

/**
 * @brief 推入fifo
 *
 * 如果队列已经满，则等待，直到有容积
 * @param v
 */
template< typename T >
void da_concurrent_queue< T >::push(const T& v)
{
    std::unique_lock< std::mutex > lg(m_mutex);

    if (m_capacity > 0) {
        //只有限定容积时才做推入等待
        while (m_fifo.size() >= m_capacity) {
            m_pushWait.wait(lg);
        }
    }
    m_fifo.push(v);
    m_popWait.notify_one();
}

template< typename T >
void da_concurrent_queue< T >::set(const T& v)
{
    push(v);
}

/**
 * @brief 推出
 * @return
 */
template< typename T >
T da_concurrent_queue< T >::get()
{
    std::unique_lock< std::mutex > lg(m_mutex);

    while (m_fifo.empty()) {
        m_popWait.wait(lg);
    }
    T v = m_fifo.front();

    m_fifo.pop();
    if (m_capacity > 0) {
        //如果有推出，则推入的等待可以唤醒
        m_pushWait.notify_one();
    }
    //通过移动语义，避免拷贝(不用显示声明std::move(v),编译器会优化)
    return v;
}

/**
 * @brief 有等待时间的获取
 * @param waitms 等待的毫秒，如果超过时间还无法获取也返回一个默认构造的类型
 * @return
 */
template< typename T >
T da_concurrent_queue< T >::get(int waitms)
{
    std::unique_lock< std::mutex > lg(m_mutex);
    if (!m_popWait.wait_for(lg, std::chrono::milliseconds(waitms), [this] { return !(this->m_fifo.empty()); })) {
        return T();
    }
    T v = m_fifo.front();

    m_fifo.pop();
    if (m_capacity > 0) {
        //如果有推出，则推入的等待可以唤醒
        m_pushWait.notify_one();
    }
    //通过移动语义，避免拷贝(不用显示声明std::move(v),编译器会优化)
    return v;
}

/**
 * @brief 获取队列的尺寸
 * @return
 */
template< typename T >
std::size_t da_concurrent_queue< T >::size() const
{
    std::lock_guard< std::mutex > lg(m_mutex);

    return (m_fifo.size());
}

#endif  // SAFEQUEUE_H
