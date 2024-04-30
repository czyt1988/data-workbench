#ifndef DAINDEXEDVECTOR_H
#define DAINDEXEDVECTOR_H
#include <QVector>
namespace DA
{
/**
 * @brief 这是个带当前索引的vector，提供next，previous等带索引记忆的操作
 *
 *
 */
template< typename T >
class DAIndexedVector : public QVector< T >
{
public:
    DAIndexedVector() = default;
    DAIndexedVector(int size);
    DAIndexedVector(int size, const T& t);
    DAIndexedVector(const DAIndexedVector< T >& v);
    DAIndexedVector(const std::initializer_list< T >& v);
    // 获取下一个元素(索引后移)
    T next();
    // 把索引移动到下一个,如果超过范围，会回到头
    void moveToNext();
    // 前缀操作
    T operator++();
    // 后缀操作
    T operator++(int);
    // 获取前一个元素(索引前移)
    T previous();
    // 把索引移动到下一个,如果超过范围，会回尾部
    void moveToPrevious();
    DAIndexedVector& operator--();
    // 获取当前的元素，在调用前使用isValidIndex确认索引的正确性
    T current() const;
    // 判断当前索引是否是第一个
    bool isFirstIndex() const;
    // 判断当前索引是否是最后一个
    bool isLastIndex() const;
    // 判断当前索引是否是合理范围内
    bool isValidIndex() const;
    /**
     * @brief 获取当前的索引
     * @return
     */
    int getCurrentIndex() const;
    // 设置当前的索引
    void setCurrentIndex(int v);
    // 获取当前索引下的元素
    T get() const;
    // 设置当前索引下的元素
    void set(const T& v);

private:
    int mIndex { 0 };
};

template< typename T >
DAIndexedVector< T >::DAIndexedVector(int size) : QVector< T >(size)
{
}

template< typename T >
DAIndexedVector< T >::DAIndexedVector(int size, const T& t) : QVector< T >(size, t)
{
}

template< typename T >
DAIndexedVector< T >::DAIndexedVector(const DAIndexedVector< T >& v) : QVector< T >(v)
{
    mIndex = v.mIndex;
}

template< typename T >
DAIndexedVector< T >::DAIndexedVector(const std::initializer_list< T >& v) : QVector< T >(v)
{
}

template< typename T >
T DAIndexedVector< T >::next()
{
    moveToNext();
    return current();
}

template< typename T >
void DAIndexedVector< T >::moveToNext()
{
    if (mIndex >= 0 && mIndex < QVector< T >::size() - 1) {
        ++mIndex;
    } else {
        mIndex = 0;
    }
}

template< typename T >
T DAIndexedVector< T >::operator++()
{
    moveToNext();
    return current();
}
template< typename T >
T DAIndexedVector< T >::operator++(int)
{
    auto oldv = current();
    moveToNext();
    return oldv;
}

template< typename T >
T DAIndexedVector< T >::previous()
{
    moveToPrevious();
    return current();
}

template< typename T >
void DAIndexedVector< T >::moveToPrevious()
{
    if (mIndex > 0 && mIndex < QVector< T >::size()) {
        --mIndex;
    } else {
        mIndex = QVector< T >::size() - 1;
    }
}

template< typename T >
DAIndexedVector< T >& DAIndexedVector< T >::operator--()
{
    moveToPrevious();
    return *this;
}

template< typename T >
T DAIndexedVector< T >::current() const
{
    return QVector< T >::at(mIndex);
}

template< typename T >
bool DAIndexedVector< T >::isFirstIndex() const
{
    return (mIndex == 0);
}

template< typename T >
bool DAIndexedVector< T >::isLastIndex() const
{
    return (mIndex == (QVector< T >::size() - 1));
}

template< typename T >
bool DAIndexedVector< T >::isValidIndex() const
{
    return (mIndex >= 0) && (mIndex < QVector< T >::size());
}

template< typename T >
int DAIndexedVector< T >::getCurrentIndex() const
{
    return mIndex;
}

template< typename T >
void DAIndexedVector< T >::setCurrentIndex(int v)
{
    mIndex = v;
}

template< typename T >
T DAIndexedVector< T >::get() const
{
    return QVector< T >::at(mIndex);
}

template< typename T >
void DAIndexedVector< T >::set(const T& v)
{
    QVector< T >::operator[](mIndex) = v;
}

}
#endif  // DAINDEXEDVECTOR_H
