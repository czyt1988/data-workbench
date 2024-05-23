#ifndef DAQTCONTAINERUTIL_H
#define DAQTCONTAINERUTIL_H
#include <QList>
#include <QSet>
#include <QVector>
#include <QHash>
#include <QMap>


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
