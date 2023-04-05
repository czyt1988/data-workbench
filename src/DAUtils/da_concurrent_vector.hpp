#ifndef DA_CONCURRENT_VECTOR_HPP
#define DA_CONCURRENT_VECTOR_HPP
#include <vector>
#include <mutex>

/**
 * @brief 线程安全的list封装
 *
 * @note 仅仅是对原来的std::list进行并行封装，不考虑极致效率，对插入时安全的，
 *
 * @note 这并不是完全线程安全的容器，例如在进行迭代时，是无法保证安全性的，这时候需要外部锁来保证线程安全.
 * 另外，对于删除是不确定的，因为你有可能在不同线程拿了有覆盖范围的迭代器，在某个线程已经删除了，但在别的线程
 * 又对已经剔除的迭代器进行删除，这样就导致不可预料的情况，因此，迭代操作是要额外加锁，或者保证只在某个线程进行删除，
 * 或者不同线程删除的元素是不同的
 *
 * @author czy
 */
template< typename T >
class da_concurrent_vector
{
public:
    using vector_type      = std::vector< T >;
    using value_type       = vector_type::value_type;
    using reference        = vector_type::reference;
    using const_reference  = vector_type::const_reference;
    using size_type        = vector_type::size_type;
    using iterator         = vector_type::iterator;
    using const_iterator   = vector_type::const_iterator;
    using mutex_type       = std::mutex;
    using lock_guard_type  = std::lock_guard< mutex_type >;
    using unique_lock_type = std::unique_lock< mutex_type >;

public:
    da_concurrent_vector();
    da_concurrent_vector(size_type count);
    da_concurrent_vector(std::initializer_list< T > init);
    ~da_concurrent_vector();
    reference       front();
    const_reference front() const;
    reference       back();
    const_reference back() const;
    bool            empty() const;
    size_type       size() const;
    void            clear();
    iterator        insert(iterator pos, const T& value);
    iterator        insert(const_iterator pos, const T& value);
    void            insert(iterator pos, size_type count, const T& value);
    iterator        insert(const_iterator pos, size_type count, const T& value);
    void            push_back(const T& value);
    void            push_front(const T& value);
    void            pop_front();
    void            pop_back();
    iterator        erase(iterator pos);
    iterator        erase(const_iterator pos);
    iterator        erase(iterator first, iterator last);
    iterator        erase(const_iterator first, const_iterator last);
    iterator        begin();
    const_iterator  begin() const;
    iterator        end();
    const_iterator  end() const;

private:
    vector_type< T >   m_inner_list;
    mutable mutex_type m_mutex;
};

template< typename T >
da_concurrent_vector< T >::da_concurrent_vector()
{
}

template< typename T >
da_concurrent_vector< T >::da_concurrent_vector(da_concurrent_vector::size_type count) : m_inner_list(count)
{
}

template< typename T >
da_concurrent_vector< T >::da_concurrent_vector(std::initializer_list< T > init) : m_inner_list(init)
{
}

template< typename T >
da_concurrent_vector< T >::~da_concurrent_vector()
{
}

template< typename T >
da_concurrent_vector< T >::reference da_concurrent_vector< T >::front()
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.front());
}

template< typename T >
da_concurrent_vector< T >::const_reference da_concurrent_vector< T >::front() const
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.front());
}

template< typename T >
da_concurrent_vector< T >::reference da_concurrent_vector< T >::back()
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.back());
}

template< typename T >
da_concurrent_vector< T >::const_reference da_concurrent_vector< T >::back() const
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.back());
}

template< typename T >
bool da_concurrent_vector< T >::empty() const
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.empty());
}

template< typename T >
da_concurrent_vector< T >::size_type da_concurrent_vector< T >::size() const
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.size());
}

template< typename T >
void da_concurrent_vector< T >::clear()
{
    unique_lock_type lg(m_mutex);

    m_inner_list.clear();
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::insert(da_concurrent_vector< T >::iterator pos, const T& value)
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.insert(pos, value));
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::insert(da_concurrent_vector< T >::const_iterator pos, const T& value)
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.insert(pos, value));
}

template< typename T >
void da_concurrent_vector< T >::insert(da_concurrent_vector< T >::iterator pos, da_concurrent_vector< T >::size_type count, const T& value)
{
    unique_lock_type lg(m_mutex);

    m_inner_list.insert(pos, count, value);
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::insert(da_concurrent_vector< T >::const_iterator pos,
                                                                      da_concurrent_vector< T >::size_type      count,
                                                                      const T&                                  value)
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.insert(pos, count, value));
}

template< typename T >
void da_concurrent_vector< T >::push_back(const T& value)
{
    unique_lock_type lg(m_mutex);

    m_inner_list.push_back(value);
}

template< typename T >
void da_concurrent_vector< T >::push_front(const T& value)
{
    unique_lock_type lg(m_mutex);

    m_inner_list.push_front(value);
}

template< typename T >
void da_concurrent_vector< T >::pop_front()
{
    unique_lock_type lg(m_mutex);

    m_inner_list.pop_front();
}

template< typename T >
void da_concurrent_vector< T >::pop_back()
{
    unique_lock_type lg(m_mutex);

    m_inner_list.pop_back();
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::erase(da_concurrent_vector< T >::iterator pos)
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.erase(pos));
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::erase(da_concurrent_vector< T >::const_iterator pos)
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.erase(pos));
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::erase(da_concurrent_vector< T >::iterator first,
                                                                     da_concurrent_vector< T >::iterator last)
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.erase(first, last));
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::erase(da_concurrent_vector< T >::const_iterator first,
                                                                     da_concurrent_vector< T >::const_iterator last)
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.erase(first, last));
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::begin()
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.begin());
}

template< typename T >
da_concurrent_vector< T >::const_iterator da_concurrent_vector< T >::begin() const
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.begin());
}

template< typename T >
da_concurrent_vector< T >::iterator da_concurrent_vector< T >::end()
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.end());
}

template< typename T >
da_concurrent_vector< T >::const_iterator da_concurrent_vector< T >::end() const
{
    unique_lock_type lg(m_mutex);

    return (m_inner_list.end());
}

#endif  // gree_concurrent_vector_HPP
