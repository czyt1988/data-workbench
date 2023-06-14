#ifndef DAAUTOINCREMENTSERIES_H
#define DAAUTOINCREMENTSERIES_H
#include <algorithm>
namespace DA
{

template< typename T >
class DAAutoincrementSeries
{
public:
    typedef T Type;

public:
    DAAutoincrementSeries();
    DAAutoincrementSeries(T baseValue_, T stepValue_);
    //基准
    T getBaseValue() const;
    //步长值
    T getStepValue() const;
    //设置基准值
    void setBaseValue(T baseValue_);
    //设置基准值
    void setStepValue(T stepValue_);
    //生成序列
    template< typename IteBegin, typename IteEnd >
    void generate(IteBegin begin_, IteEnd end_);
    //获取值
    T at(std::size_t index) const;
    //支持[]取值
    T operator[](std::size_t index) const;

private:
    T mBase;  ///< 基础值
    T mStep;  ///< 步长
};

/**
 * @brief 默认构造是从0开始，1步长的一个自增序列
 */
template< typename T >
DAAutoincrementSeries< T >::DAAutoincrementSeries()
{
    mBase = 0;
    mStep = 1;
}

/**
 * @brief 构造是从baseValue开始，stepValue步长的一个自增序列
 * @param baseValue 初始值
 * @param stepValue 步长
 */
template< typename T >
DAAutoincrementSeries< T >::DAAutoincrementSeries(T baseValue_, T stepValue_)
{
    mBase = baseValue_;
    mStep = stepValue_;
}

/**
 * @brief 基准
 */
template< typename T >
T DAAutoincrementSeries< T >::getBaseValue() const
{
    return mBase;
}

/**
 * @brief 步长
 */
template< typename T >
T DAAutoincrementSeries< T >::getStepValue() const
{
    return mStep;
}

/**
 * @brief 设置基准
 */
template< typename T >
void DAAutoincrementSeries< T >::setBaseValue(T baseValue_)
{
    mBase = baseValue_;
}

/**
 * @brief 设置步长
 */
template< typename T >
void DAAutoincrementSeries< T >::setStepValue(T stepValue_)
{
    mStep = stepValue_;
}

/**
 * @brief 获取第index个自增值
 * @param index
 * @return
 */
template< typename T >
T DAAutoincrementSeries< T >::at(std::size_t index) const
{
    return (mBase + index * mStep);
}

/**
 * @brief 支持[]取值
 * @param index
 * @return
 */
template< typename T >
T DAAutoincrementSeries< T >::operator[](size_t index) const
{
    return (mBase + index * mStep);
}

/**
 * @brief 生成序列
 *
 * @example 例如：
 *
 * @code
 *
 * @endcode
 * @param begin_ 开始迭代器
 * @param end_ 结束迭代器
 * @note 值要支持和int的乘积
 */
template< typename T >
template< typename IteBegin, typename IteEnd >
void DAAutoincrementSeries< T >::generate(IteBegin begin_, IteEnd end_)
{
    std::size_t n = 0;
    while (begin_ < end_) {
        *begin_ = at(n);
        ++n;
        ++begin_;
    }
}

}  // end DA
#endif  // DAAUTOINCREMENTSERIES_H
