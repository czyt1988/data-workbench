#ifndef DAINDEXEDVECTOR_H
#define DAINDEXEDVECTOR_H
#include <QVector>
namespace DA
{
/**
 * @brief 这是个带当前索引的vector，提供next，previous等带索引记忆的操作
 *
 * 通过++方法可以进行快捷的位移和迭代
 * 如：
 * @code
 * DAIndexedVector<int> v({1,2,3});
 * int a = v++;//a=2,当前游标在位置0，v++会让游标移动一位，然后返回当前位置，也就是返回2
 * int b = ++v;//b=2,当前位置为1，++v，会返回当前位置，再移动，因此b还是2，但v的游标已经移动到下一个位置
 * int c = v.current();//c = 3
 * int d = v++;//d=1,之前是在游标索引为2的地方，再移动已经到末尾，会回到初始位置，返回1
 * @endcode
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
    // 前缀操作,++v,返回下一个
    T operator++();
    // 后缀操作,v++，返回当前，并移动到下一个
    T operator++(int);
    // 获取前一个元素(索引前移)
    T previous();
    // 把索引移动到下一个,如果超过范围，会回尾部
    void moveToPrevious();
    //前缀操作--v,回退并返回值
    T operator--();
    // 后缀操作,v--，返回当前，并回退
    T operator--(int);
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
T DAIndexedVector< T >::operator--()
{
    moveToPrevious();
    return current();
}

template< typename T >
T DAIndexedVector< T >::operator--(int)
{
    auto oldv = current();
    moveToPrevious();
    return oldv;
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
