#ifndef DAQTCONTAINERUTIL_H
#define DAQTCONTAINERUTIL_H
#include <QList>
#include <QSet>
#include <QVector>
#include <QHash>
#include <QMap>
#include <memory>

/**
 * @brief 针对智能指针的qHash函数，可以让std::shared_ptr作为QHash和QSet的key
 * @param ptr
 * @param seed
 * @return
 */
template< typename T >
uint qHash(const std::shared_ptr< T >& ptr, uint seed = 0)
{
    return qHash(ptr.get(), seed);
}

namespace DA
{

template< typename T >
QList< T > qset_to_qlist(const QSet< T >& v)
{
#if QT_VERSION_MAJOR >= 6
    return QList< T >(v.begin(), v.end());
#else
#if QT_VERSION_MINOR >= 14
    return QList< T >(v.begin(), v.end());
#else
    return v.toList();
#endif
#endif
}

template< typename T >
QSet< T > qlist_to_qset(const QList< T >& v)
{
#if QT_VERSION_MAJOR >= 6
    return QSet< T >(v.begin(), v.end());
#else
#if QT_VERSION_MINOR >= 14
    return QSet< T >(v.begin(), v.end());
#else
    return v.toSet();
#endif
#endif
}

template< typename T >
QList< T > unique_qlist(const QList< T >& v)
{
    return qset_to_qlist(qlist_to_qset(v));
}

}

#endif  // DAQTCONTAINERUTIL_H
