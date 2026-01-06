#ifndef DAPYBIND11QTCASTER_HPP
#define DAPYBIND11QTCASTER_HPP
#include "DAPybind11InQt.h"
#include <QString>
#include <QList>
#include <QDateTime>
#include <QList>
#include <QVector>
#include <QSet>
#include <QHash>
#include <QMap>

#include <QVariant>

namespace pybind11
{
namespace detail
{

// QString 转换器 - 优化版
template<>
struct type_caster< QString >
{
    PYBIND11_TYPE_CASTER(QString, _("str"));

    bool load(handle src, bool convert)
    {
        if (!src)
            return false;

        // 处理Unicode字符串
        if (PyUnicode_Check(src.ptr())) {
            Py_ssize_t size;
            const char* data = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
            if (data) {
                value = QString::fromUtf8(data, size);
                return true;
            }
        }
        // 处理bytes对象
        else if (convert && PyBytes_Check(src.ptr())) {
            char* data;
            Py_ssize_t size;
            if (PyBytes_AsStringAndSize(src.ptr(), &data, &size) != -1) {
                value = QString::fromUtf8(data, size);
                return true;
            }
        }
        return false;
    }

    static handle cast(const QString& src, return_value_policy /* policy */, handle /* parent */)
    {
        QByteArray utf8 = src.toUtf8();
        return PyUnicode_FromStringAndSize(utf8.constData(), utf8.size());
    }
};

// QDate 转换器 - 修复静态变量问题
template<>
struct type_caster< QDate >
{
    PYBIND11_TYPE_CASTER(QDate, _("datetime.date"));

    // 延迟加载datetime模块
    static pybind11::object& get_date_type()
    {
        static pybind11::object date_type = []() { return pybind11::module::import("datetime").attr("date"); }();
        return date_type;
    }

    bool load(handle src, bool convert)
    {
        if (!src)
            return false;

        if (pybind11::isinstance(src, get_date_type())) {
            int year  = src.attr("year").cast< int >();
            int month = src.attr("month").cast< int >();
            int day   = src.attr("day").cast< int >();

            if (QDate::isValid(year, month, day)) {
                value = QDate(year, month, day);
                return true;
            }
        }
        return false;
    }

    static handle cast(const QDate& src, return_value_policy /* policy */, handle /* parent */)
    {
        if (!src.isValid()) {
            Py_RETURN_NONE;
        }
        return get_date_type()(src.year(), src.month(), src.day()).release();
    }
};

// QTime 转换器
template<>
struct type_caster< QTime >
{
    PYBIND11_TYPE_CASTER(QTime, _("datetime.time"));

    static pybind11::object& get_time_type()
    {
        static pybind11::object time_type = []() { return pybind11::module::import("datetime").attr("time"); }();
        return time_type;
    }

    bool load(handle src, bool convert)
    {
        if (!src)
            return false;

        if (pybind11::isinstance(src, get_time_type())) {
            int hour        = src.attr("hour").cast< int >();
            int minute      = src.attr("minute").cast< int >();
            int second      = src.attr("second").cast< int >();
            int microsecond = src.attr("microsecond").cast< int >();

            int msec = microsecond / 1000;
            if (QTime::isValid(hour, minute, second, msec)) {
                value = QTime(hour, minute, second, msec);
                return true;
            }
        }
        return false;
    }

    static handle cast(const QTime& src, return_value_policy /* policy */, handle /* parent */)
    {
        if (!src.isValid()) {
            Py_RETURN_NONE;
        }
        return get_time_type()(src.hour(), src.minute(), src.second(), src.msec() * 1000).release();
    }
};

// QDateTime 转换器 - 修复版，正确处理时区
template<>
struct type_caster< QDateTime >
{
    PYBIND11_TYPE_CASTER(QDateTime, _("datetime.datetime"));

    static pybind11::object& get_datetime_type()
    {
        static pybind11::object datetime_type = []() { return pybind11::module::import("datetime").attr("datetime"); }();
        return datetime_type;
    }

    static pybind11::object& get_timezone_utc()
    {
        static pybind11::object utc_tz = []() {
            pybind11::module datetime_module = pybind11::module::import("datetime");
            pybind11::object timezone        = datetime_module.attr("timezone");
            pybind11::object utc             = datetime_module.attr("utc");
            return timezone.attr("utc");
        }();
        return utc_tz;
    }

    bool load(handle src, bool convert)
    {
        if (!src)
            return false;

        if (!pybind11::isinstance(src, get_datetime_type())) {
            return false;
        }

        try {
            // 检查是否有tzinfo属性
            pybind11::object tzinfo = src.attr("tzinfo");
            bool has_tzinfo         = !tzinfo.is_none();

            qint64 msecs_since_epoch;

            if (has_tzinfo) {
                // 有时区信息，转换为UTC时间戳
                pybind11::object timestamp = src.attr("timestamp");
                double ts                  = timestamp().cast< double >();
                msecs_since_epoch          = static_cast< qint64 >(ts * 1000);
                value                      = QDateTime::fromMSecsSinceEpoch(msecs_since_epoch, Qt::UTC);
            } else {
                // naive datetime，假定为本地时间
                // 使用Python的日历计算避免timestamp()的问题
                int year        = src.attr("year").cast< int >();
                int month       = src.attr("month").cast< int >();
                int day         = src.attr("day").cast< int >();
                int hour        = src.attr("hour").cast< int >();
                int minute      = src.attr("minute").cast< int >();
                int second      = src.attr("second").cast< int >();
                int microsecond = src.attr("microsecond").cast< int >();
                int msec        = microsecond / 1000;

                QDate date(year, month, day);
                QTime time(hour, minute, second, msec);

                if (date.isValid() && time.isValid()) {
                    value = QDateTime(date, time, Qt::LocalTime);
                    return true;
                }
                return false;
            }
            return true;
        } catch (...) {
            // 捕获所有异常，转换失败
            return false;
        }
    }

    static handle cast(const QDateTime& src, return_value_policy /* policy */, handle /* parent */)
    {
        if (!src.isValid()) {
            Py_RETURN_NONE;
        }

        // 转换为UTC时间进行计算
        QDateTime utc_dt = src.toUTC();
        qint64 msecs     = utc_dt.toMSecsSinceEpoch();
        double seconds   = msecs / 1000.0;

        // 使用fromtimestamp创建带时区的datetime对象
        pybind11::object py_dt = get_datetime_type().attr("fromtimestamp")(seconds, get_timezone_utc());

        return py_dt.release();
    }
};

// ============================================================================
// QList<T> 转换器 (现代版)
// ============================================================================
template< typename T >
struct type_caster< QList< T > >
{
    using Native = QList< T >;
    PYBIND11_TYPE_CASTER(Native, _("List"));

    using value_conv = make_caster< T >;

    bool load(handle src, bool convert)
    {
        if (!isinstance< list >(src)) {
            return false;
        }

        auto l = reinterpret_borrow< list >(src);
        value.clear();
        value.reserve(static_cast< int >(l.size()));

        for (auto it = l.begin(); it != l.end(); ++it) {
            value_conv conv;
            if (!conv.load(*it, convert)) {
                return false;
            }
            value.append(cast_op< T >(conv));
        }
        return true;
    }

    static handle cast(const Native& src, return_value_policy policy, handle parent)
    {
        list l(src.size());
        size_t index = 0;

        for (const T& item : src) {
            auto value_conv = reinterpret_steal< object >(value_conv::cast(item, policy, parent));
            if (!value_conv) {
                return handle();
            }
            PyList_SET_ITEM(l.ptr(), index++, value_conv.release().ptr());
        }
        return l.release();
    }
};

// ============================================================================
// QVector<T> 转换器 (现代版)
// ============================================================================
template< typename T >
struct type_caster< QVector< T > >
{
    using Native = QVector< T >;
    PYBIND11_TYPE_CASTER(Native, _("List"));

    using value_conv = make_caster< T >;

    bool load(handle src, bool convert)
    {
        if (!isinstance< list >(src)) {
            return false;
        }

        auto l = reinterpret_borrow< list >(src);
        value.clear();
        value.reserve(static_cast< int >(l.size()));

        for (auto it = l.begin(); it != l.end(); ++it) {
            value_conv conv;
            if (!conv.load(*it, convert)) {
                return false;
            }
            value.append(cast_op< T >(conv));
        }
        return true;
    }

    static handle cast(const Native& src, return_value_policy policy, handle parent)
    {
        list l(src.size());
        size_t index = 0;

        for (const T& item : src) {
            auto value_conv = reinterpret_steal< object >(value_conv::cast(item, policy, parent));
            if (!value_conv) {
                return handle();
            }
            PyList_SET_ITEM(l.ptr(), index++, value_conv.release().ptr());
        }
        return l.release();
    }
};

// ============================================================================
// QSet<T> 转换器 (现代版，兼容C++11)
// ============================================================================
template< typename T >
struct type_caster< QSet< T > >
{
    using Native = QSet< T >;
    PYBIND11_TYPE_CASTER(Native, _("Set"));

    using value_conv = make_caster< T >;

    bool load(handle src, bool convert)
    {
        // 尝试作为集合加载
        if (isinstance< set >(src)) {
            auto s = reinterpret_borrow< set >(src);
            value.clear();

            for (auto it = s.begin(); it != s.end(); ++it) {
                value_conv conv;
                if (!conv.load(*it, convert)) {
                    return false;
                }
                value.insert(cast_op< T >(conv));
            }
            return true;
        }

        // 如果不是集合，尝试作为列表加载（允许从列表转换）
        if (isinstance< list >(src)) {
            auto l = reinterpret_borrow< list >(src);
            value.clear();

            for (auto it = l.begin(); it != l.end(); ++it) {
                value_conv conv;
                if (!conv.load(*it, convert)) {
                    return false;
                }
                value.insert(cast_op< T >(conv));
            }
            return true;
        }

        return false;
    }

    static handle cast(const Native& src, return_value_policy policy, handle parent)
    {
        set s;

        for (const T& item : src) {
            auto value_conv = reinterpret_steal< object >(value_conv::cast(item, policy, parent));
            if (!value_conv) {
                return handle();
            }

            if (s.add(value_conv).failed()) {
                return handle();
            }
        }

        return s.release();
    }
};

// ============================================================================
// QHash<K, V> 转换器 (现代版，兼容C++11)
// ============================================================================
template< typename K, typename V >
struct type_caster< QHash< K, V > >
{
    using Native = QHash< K, V >;
    PYBIND11_TYPE_CASTER(Native, _("Dict"));

    using key_conv   = make_caster< K >;
    using value_conv = make_caster< V >;

    bool load(handle src, bool convert)
    {
        if (!isinstance< dict >(src)) {
            return false;
        }

        auto d = reinterpret_borrow< dict >(src);
        value.clear();

        for (auto it = d.begin(); it != d.end(); ++it) {
            key_conv ck;
            value_conv cv;

            if (!ck.load(it->first, convert) || !cv.load(it->second, convert)) {
                return false;
            }

            value.insert(cast_op< K >(ck), cast_op< V >(cv));
        }
        return true;
    }

    static handle cast(const Native& src, return_value_policy policy, handle parent)
    {
        dict d;

        for (auto it = src.begin(); it != src.end(); ++it) {
            auto k = reinterpret_steal< object >(key_conv::cast(it.key(), policy, parent));
            auto v = reinterpret_steal< object >(value_conv::cast(it.value(), policy, parent));

            if (!k || !v) {
                return handle();
            }

            d[ k ] = v;
        }

        return d.release();
    }
};

// ============================================================================
// QMap<K, V> 转换器 (现代版，兼容C++11)
// ============================================================================
template< typename K, typename V >
struct type_caster< QMap< K, V > >
{
    using Native = QMap< K, V >;
    PYBIND11_TYPE_CASTER(Native, _("Dict"));

    using key_conv   = make_caster< K >;
    using value_conv = make_caster< V >;

    bool load(handle src, bool convert)
    {
        if (!isinstance< dict >(src)) {
            return false;
        }

        auto d = reinterpret_borrow< dict >(src);
        value.clear();

        for (auto it = d.begin(); it != d.end(); ++it) {
            key_conv ck;
            value_conv cv;

            if (!ck.load(it->first, convert) || !cv.load(it->second, convert)) {
                return false;
            }

            value.insert(cast_op< K >(ck), cast_op< V >(cv));
        }
        return true;
    }

    static handle cast(const Native& src, return_value_policy policy, handle parent)
    {
        dict d;

        for (auto it = src.begin(); it != src.end(); ++it) {
            auto k = reinterpret_steal< object >(key_conv::cast(it.key(), policy, parent));
            auto v = reinterpret_steal< object >(value_conv::cast(it.value(), policy, parent));

            if (!k || !v) {
                return handle();
            }

            d[ k ] = v;
        }

        return d.release();
    }
};
}  // namespace detail
}  // namespace pybind11

// 辅助函数，方便手动转换
namespace DA
{
namespace PY
{
// QString 转换函数
inline QString fromPyString(pybind11::handle py_str)
{
    return pybind11::cast< QString >(py_str);
}

inline pybind11::object toPyString(const QString& qt_str)
{
    return pybind11::cast(qt_str);
}

// QDate 转换函数
inline QDate fromPyDate(pybind11::handle py_date)
{
    return pybind11::cast< QDate >(py_date);
}

inline pybind11::object toPyDate(const QDate& qt_date)
{
    return pybind11::cast(qt_date);
}

// QTime 转换函数
inline QTime fromPyTime(pybind11::handle py_time)
{
    return pybind11::cast< QTime >(py_time);
}

inline pybind11::object toPyTime(const QTime& qt_time)
{
    return pybind11::cast(qt_time);
}

// QDateTime 转换函数
inline QDateTime fromPyDateTime(pybind11::handle py_datetime)
{
    return pybind11::cast< QDateTime >(py_datetime);
}

inline pybind11::object toPyDateTime(const QDateTime& qt_datetime)
{
    return pybind11::cast(qt_datetime);
}

// 列表转换
template< typename T >
inline QList< T > fromPyList(pybind11::handle py_list)
{
    return pybind11::cast< QList< T > >(py_list);
}

template< typename T >
inline pybind11::object toPyList(const QList< T >& qt_list)
{
    return pybind11::cast(qt_list);
}

// 向量转换
template< typename T >
inline QVector< T > fromPyVector(pybind11::handle py_list)
{
    return pybind11::cast< QVector< T > >(py_list);
}

template< typename T >
inline pybind11::object toPyVector(const QVector< T >& qt_vector)
{
    return pybind11::cast(qt_vector);
}

// 集合转换
template< typename T >
inline QSet< T > fromPySet(pybind11::handle py_set)
{
    return pybind11::cast< QSet< T > >(py_set);
}

template< typename T >
inline pybind11::object toPySet(const QSet< T >& qt_set)
{
    return pybind11::cast(qt_set);
}

// 从列表创建集合
template< typename T >
inline QSet< T > fromPyListToSet(pybind11::handle py_list)
{
    return pybind11::cast< QSet< T > >(py_list);
}

// 字典转换
template< typename K, typename V >
inline QHash< K, V > fromPyDict(pybind11::handle py_dict)
{
    return pybind11::cast< QHash< K, V > >(py_dict);
}

template< typename K, typename V >
inline pybind11::object toPyDict(const QHash< K, V >& qt_hash)
{
    return pybind11::cast(qt_hash);
}

template< typename K, typename V >
inline QMap< K, V > fromPyMap(pybind11::handle py_dict)
{
    return pybind11::cast< QMap< K, V > >(py_dict);
}

template< typename K, typename V >
inline pybind11::object toPyMap(const QMap< K, V >& qt_map)
{
    return pybind11::cast(qt_map);
}

}  // namespace PY
}  // namespace DA
#endif  // DAPYBIND11QTCASTER_HPP
