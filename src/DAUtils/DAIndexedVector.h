#ifndef DAINDEXEDVECTOR_H
#define DAINDEXEDVECTOR_H
#include <QVector>
namespace DA
{
/**
 * @brief 这是个带索引的vector，提供next，previous等带索引的操作
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
    //获取下一个元素(索引后移)
    T next();
    //获取前一个元素(索引前移)
    T previous();
    //获取当前的索引
    int getCurrentIndex() const;
    //设置当前的索引
    void setCurrentIndex(int v);
    //获取当前索引下的元素
    T get() const;
    //设置当前索引下的元素
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
    if (mIndex < size() - 1) {
        ++mIndex;
    } else {
        mIndex = 0;
    }
    return at(mIndex);
}

template< typename T >
T DAIndexedVector< T >::previous()
{
    if (mIndex > 0) {
        --mIndex;
    } else {
        mIndex = size() - 1;
    }
    return at(mIndex);
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
    return at(mIndex);
}

template< typename T >
void DAIndexedVector< T >::set(const T& v)
{
    operator[](mIndex) = v;
}

}
#endif  // DAINDEXEDVECTOR_H
