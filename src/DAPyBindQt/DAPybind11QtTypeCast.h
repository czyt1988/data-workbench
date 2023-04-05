#ifndef DAPYBIND11QTTYPECAST_H
#define DAPYBIND11QTTYPECAST_H
#include "DAPyBindQtGlobal.h"
#include "DAPybind11InQt.h"
#include "numpy/DAPyDType.h"
#include <QVariant>
#include <QSet>

/**
 * @file 此文件用于qt类型和pybind11类型的转换
 */

namespace DA
{
namespace PY
{
// 把常用的类型注册到元对象，此函数最好在初始化的时候调用
DAPYBINDQT_API void registerMetaType();
// QVariant 转换为pybind11::object
DAPYBINDQT_API pybind11::object toPyObject(const QVariant& var);
DAPYBINDQT_API pybind11::object toPyObject(const QVariant& var, const pybind11::dtype& dt);
// pybind11::object 转换为 QVariant
DAPYBINDQT_API QVariant toVariant(const pybind11::object& obj);
//把字符串按照dtype转换为qvariant
DAPYBINDQT_API QVariant toVariant(const QString& str, const DAPyDType& dt);
// pybind11::str 转换为QString
DAPYBINDQT_API QString toString(const pybind11::str& obj);
// 把QString转换为pybind11::str
DAPYBINDQT_API pybind11::str toString(const QString& str);
DAPYBINDQT_API QString toString(const pybind11::dtype& dtype);
// QVariantHash转换为pybind11::dict
DAPYBINDQT_API pybind11::dict toDict(const QVariantHash& qvhash);
DAPYBINDQT_API pybind11::dict toDict(const QVariantMap& qvmap);
// QVariantList转换为pybind11::list
DAPYBINDQT_API pybind11::list toList(const QVariantList& list);
DAPYBINDQT_API pybind11::list toList(const QStringList& list);
// pybind11::object 转QList<QString>
DAPYBINDQT_API QList< QString > toStringList(const pybind11::object& obj, QString* err = nullptr);

template< typename T >
pybind11::list toList(const QList< T >& qtlist);
template< typename T >
pybind11::list toList(const QSet< T >& qtlist);

template< typename T >
pybind11::list toList(const QList< T >& arr)
{
    pybind11::list pylist;
    for (const T& v : arr) {
        pybind11::object o = pybind11::cast(v);
        pylist.append(o);
    }
    return pylist;
}

template< typename T >
pybind11::list toList(const QSet< T >& arr)
{
    pybind11::list pylist;
    for (const T& v : arr) {
        pybind11::object o = pybind11::cast(v);
        pylist.append(o);
    }
    return pylist;
}
}  // namespace PY
}  // namespace DA

#endif  // DAPYBIND11QTTYPECAST_H
