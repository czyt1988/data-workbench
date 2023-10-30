#ifndef DAVECTOR_H
#define DAVECTOR_H
// DA
#include "DAAlgorithm.h"
// Qt
#include <QVector>
#include <QDebug>
namespace DA
{
//==============================================================
// DAVector
//==============================================================
/**
 * @brief 系列数据,系列允许设置名字
 */
template< typename T, typename StringType = QString >
class DAVector : public QVector< T >
{
public:
    using String = StringType;

public:
    DAVector() : QVector< T >()
    {
    }

    DAVector(int size) : QVector< T >(size)
    {
    }

    DAVector(int size, const T& t) : QVector< T >(size, t)
    {
    }

    DAVector(std::initializer_list< T > args) : QVector< T >(args)
    {
    }

    DAVector(const StringType& n) : QVector< T >(), mName(n)
    {
    }

    const StringType& name() const;
    StringType& name();
    void setName(const StringType& n);
    StringType getName() const;

private:
    StringType mName;  ///< 系列名，参考pandas.Series.name
};

template< typename T, typename StringType >
const StringType& DAVector< T, StringType >::name() const
{
    return (mName);
}

template< typename T, typename StringType >
StringType& DAVector< T, StringType >::name()
{
    return (mName);
}

template< typename T, typename StringType >
void DAVector< T, StringType >::setName(const StringType& n)
{
    mName = n;
}

template< typename T, typename StringType >
StringType DAVector< T, StringType >::getName() const
{
    return (mName);
}
}
#endif  // DAVECTOR_H
