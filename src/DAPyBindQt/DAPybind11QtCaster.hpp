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

// ============================================================================
// QByteArray 转换器
// ============================================================================
template<>
struct type_caster< QByteArray >
{
    PYBIND11_TYPE_CASTER(QByteArray, _("bytes"));

    bool load(handle src, bool convert)
    {
        if (!src)
            return false;

        // 处理 None
        if (src.is_none()) {
            value = QByteArray();
            return true;
        }

        // 处理 bytes 对象
        if (PyBytes_Check(src.ptr())) {
            char* data;
            Py_ssize_t size;

            if (PyBytes_AsStringAndSize(src.ptr(), &data, &size) != -1) {
                // 创建 QByteArray，复制数据
                value = QByteArray(data, size);
                return true;
            }
            return false;
        }

        // 处理 bytearray 对象
        if (PyByteArray_Check(src.ptr())) {
            char* data;
            Py_ssize_t size;

            // 获取 bytearray 的数据指针和大小
            data = PyByteArray_AS_STRING(src.ptr());
            size = PyByteArray_GET_SIZE(src.ptr());

            if (data && size >= 0) {
                // 创建 QByteArray，复制数据
                value = QByteArray(data, size);
                return true;
            }
            return false;
        }

        // 如果允许转换，尝试从字符串转换
        if (convert && PyUnicode_Check(src.ptr())) {
            try {
                // 将 Unicode 字符串转换为 UTF-8 编码的 QByteArray
                Py_ssize_t size;
                const char* data = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
                if (data) {
                    value = QByteArray(data, size);
                    return true;
                }
            } catch (...) {
                return false;
            }
        }

        // 如果允许转换，尝试从整数列表转换
        if (convert && (PyList_Check(src.ptr()) || PyTuple_Check(src.ptr()))) {
            try {
                pybind11::sequence seq = reinterpret_borrow< pybind11::sequence >(src);
                QByteArray byte_array;

                for (size_t i = 0; i < seq.size(); ++i) {
                    pybind11::handle item = seq[ i ];

                    // 每个元素必须是 0-255 的整数
                    if (PyLong_Check(item.ptr())) {
                        long val = PyLong_AsLong(item.ptr());
                        if (val >= 0 && val <= 255) {
                            byte_array.append(static_cast< char >(val));
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }

                value = byte_array;
                return true;
            } catch (...) {
                return false;
            }
        }

        return false;
    }

    static handle cast(const QByteArray& src, return_value_policy /* policy */, handle /* parent */)
    {
        if (src.isNull()) {
            Py_RETURN_NONE;
        }

        // 将 QByteArray 转换为 Python bytes 对象
        // 使用 PyBytes_FromStringAndSize 可以正确处理包含空字节的数据
        return PyBytes_FromStringAndSize(src.constData(), src.size());

        // 如果希望返回 bytearray（可变）而不是 bytes（不可变），可以使用以下代码：
        // return PyByteArray_FromStringAndSize(src.constData(), src.size());
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

// QDateTime 转换器
template<>
struct type_caster< QDateTime >
{
    PYBIND11_TYPE_CASTER(QDateTime, _("datetime.datetime"));

    static pybind11::object& get_datetime_type()
    {
        static pybind11::object datetime_type = []() -> pybind11::object {
            try {
                return pybind11::module::import("datetime").attr("datetime");
            } catch (...) {
                return pybind11::none();
            }
        }();
        return datetime_type;
    }
    static pybind11::object& get_pandas_timestamp_type()
    {
        static pybind11::object pd_Timestamp = []() -> pybind11::object {
            try {
                return pybind11::module::import("pandas").attr("Timestamp");
            } catch (...) {
                return pybind11::none();
            }
        }();
        return pd_Timestamp;
    }
    static pybind11::object& get_numpy_timestamp_type()
    {
        static pybind11::object np_datetime64 = []() -> pybind11::object {
            try {
                return pybind11::module::import("numpy").attr("datetime64");
            } catch (...) {
                return pybind11::none();
            }
        }();
        return np_datetime64;
    }
    bool load(pybind11::handle src, bool convert)
    {
        if (!src) {
            return false;
        }
        static pybind11::object& datetime_type = get_datetime_type();
        if (!datetime_type.is_none() && pybind11::isinstance(src, datetime_type)) {
            try {
                // 使用 timestamp() 方法，它自动处理时区转换
                pybind11::object timestamp = src.attr("timestamp");
                double ts                  = timestamp().cast< double >();
                qint64 msecs_since_epoch   = static_cast< qint64 >(ts * 1000);

                // 检查是否有时区信息
                pybind11::object tzinfo = src.attr("tzinfo");
                bool has_tzinfo         = !tzinfo.is_none();

                if (has_tzinfo) {
                    // 有时区信息，使用 timestamp() 方法，它自动处理时区转换
                    pybind11::object timestamp = src.attr("timestamp");
                    double ts                  = timestamp().cast< double >();
                    qint64 msecs_since_epoch   = static_cast< qint64 >(ts * 1000);
                    value                      = QDateTime::fromMSecsSinceEpoch(msecs_since_epoch, Qt::UTC);
                } else {
                    // naive datetime，直接从属性构建 QDateTime（不经过 timestamp 转换）
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
                        // 创建本地时间的 QDateTime
                        value = QDateTime(date, time);
                        return true;
                    }
                }
                return true;
            } catch (...) {
                return false;
            }
        }
        static pybind11::object& np_timestamp = get_numpy_timestamp_type();
        if (!np_timestamp.is_none() && pybind11::isinstance(src, np_timestamp)) {
            // numpy datetime64
            try {
                int64_t ns = src.attr("astype")("datetime64[ns]").attr("view")("int64").cast< int64_t >();
                value      = QDateTime::fromMSecsSinceEpoch(ns / 1000000, Qt::UTC);
                return true;
            } catch (...) {
                return false;
            }
        }

        static pybind11::object& pd_timestamp = get_pandas_timestamp_type();
        if (!pd_timestamp.is_none() && pybind11::isinstance(src, pd_timestamp)) {
            // pandas Timestamp
            try {
                // pandas Timestamp 有自己的 tz 属性
                pybind11::object tz = src.attr("tz");
                bool has_tz         = !tz.is_none();

                if (has_tz) {
                    // 有时区信息，使用 UTC
                    double ts = src.attr("timestamp")().cast< double >();
                    value     = QDateTime::fromMSecsSinceEpoch(static_cast< qint64 >(ts * 1000), Qt::UTC);
                } else {
                    // 无时区信息，当作本地时间处理
                    // 对于 pandas Timestamp，我们使用 to_pydatetime() 转换为 python datetime，然后按照上面的逻辑处理
                    pybind11::object py_dt = src.attr("to_pydatetime")();
                    return load(py_dt, convert);
                }
                return true;
            } catch (...) {
                return false;
            }
        }
        // 所有方法都失败
        return false;
    }

    static pybind11::handle
    cast(const QDateTime& src, pybind11::return_value_policy /* policy */, pybind11::handle /* parent */)
    {
        if (!src.isValid()) {
            Py_RETURN_NONE;
        }
        /* 直接拿“墙上时间”戳，不再 toUTC() */
        qint64 ms = src.toMSecsSinceEpoch();
        double ts = ms / 1000.0;
        try {
            object datetime_type = module_::import("datetime").attr("datetime");
            return datetime_type.attr("fromtimestamp")(ts).release();
        } catch (...) {
            Py_RETURN_NONE;
        }
    }
};

// ============================================================================
// QList<T> 转换器
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
// QSet<T> 转换器
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
        pybind11::set s;

        for (const T& item : src) {
            auto value_conv = reinterpret_steal< object >(value_conv::cast(item, policy, parent));
            if (!value_conv) {
                return handle();
            }

            if (!s.add(value_conv)) {
                return handle();
            }
        }

        return s.release();
    }
};

// ============================================================================
// QHash<K, V> 转换器
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
// QMap<K, V> 转换器
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

// ============================================================================
// QVariant 转换器
// ============================================================================
template<>
struct type_caster< QVariant >
{
    PYBIND11_TYPE_CASTER(QVariant, _("Any"));

    // 辅助函数：检查是否为 None 或为空
    static bool is_none_or_empty(handle src)
    {
        return src.is_none() || (PyObject_Size(src.ptr()) == 0 && PyErr_Occurred() == nullptr);
    }

    // 辅助函数：判断是否是 numpy 对象
    static bool is_numpy_array(handle src)
    {
        try {
            static pybind11::object numpy_module = []() -> pybind11::object {
                try {
                    return pybind11::module::import("numpy");
                } catch (...) {
                    return pybind11::none();
                }
            }();

            if (!numpy_module.is_none()) {
                static pybind11::object ndarray_type = numpy_module.attr("ndarray");
                static pybind11::object generic_type = numpy_module.attr("generic");
                return pybind11::isinstance(src, ndarray_type) || pybind11::isinstance(src, generic_type);
            }
        } catch (...) {
            // 忽略异常，说明 numpy 不可用
        }
        return false;
    }

    // 处理 numpy 数组或标量
    bool handle_numpy_object(handle src)
    {
        try {
            pybind11::object np_obj = reinterpret_borrow< object >(src);
            pybind11::dtype dt      = np_obj.attr("dtype");
            char dtype_char         = dt.char_();

            // 获取 shape 和 ndim
            pybind11::tuple shape = np_obj.attr("shape");
            std::size_t ndim      = pybind11::len(shape);

            // 处理标量（0维数组）
            if (ndim == 0) {
                // 使用 item() 方法获取标量值
                pybind11::object scalar = np_obj.attr("item")();

                switch (dtype_char) {
                case '?':  // bool
                    value = scalar.cast< bool >();
                    return true;
                case 'b':  // int8
                    value = QChar(static_cast< char >(scalar.cast< int >()));
                    return true;
                case 'h':  // int16
                case 'l':  // int32 (通常)
                    value = scalar.cast< int >();
                    return true;
                case 'q':  // int64
                    value = scalar.cast< long long >();
                    return true;
                case 'B':  // uint8
                    value = QChar(static_cast< unsigned char >(scalar.cast< int >()));
                    return true;
                case 'H':  // uint16
                case 'L':  // uint32 (通常)
                    value = scalar.cast< unsigned int >();
                    return true;
                case 'Q':  // uint64
                    value = scalar.cast< unsigned long long >();
                    return true;
                case 'e':  // float16 (半精度)
                case 'f':  // float32
                    value = scalar.cast< float >();
                    return true;
                case 'd':  // float64
                    value = scalar.cast< double >();
                    return true;
                case 'F':  // complex64
                case 'D':  // complex128
                    // 复数转换为 QString
                    value = QString::fromStdString(pybind11::str(scalar));
                    return true;
                case 'U':  // Unicode 字符串
                {
                    // 获取字符串长度
                    pybind11::object item = np_obj.attr("item")();
                    QString str_val;

                    // 尝试不同的转换方式
                    if (PyUnicode_Check(item.ptr())) {
                        Py_ssize_t size;
                        const char* data = PyUnicode_AsUTF8AndSize(item.ptr(), &size);
                        if (data) {
                            str_val = QString::fromUtf8(data, size);
                        }
                    } else {
                        str_val = QString::fromStdString(pybind11::str(item));
                    }

                    value = str_val;
                    return true;
                }
                case 'S':  // bytes 字符串
                {
                    pybind11::object item = np_obj.attr("item")();
                    QByteArray byte_val;

                    if (PyBytes_Check(item.ptr())) {
                        char* data;
                        Py_ssize_t size;
                        if (PyBytes_AsStringAndSize(item.ptr(), &data, &size) != -1) {
                            byte_val = QByteArray(data, size);
                        }
                    } else {
                        byte_val = QByteArray(pybind11::str(item).cast< std::string >().c_str());
                    }

                    value = byte_val;
                    return true;
                }
                case 'M':  // datetime64
                {
                    // 转换为 QDateTime
                    pybind11::object item = np_obj.attr("item")();

                    // 尝试作为 Python datetime 对象处理
                    try {
                        static pybind11::object datetime_type = []() -> pybind11::object {
                            try {
                                return pybind11::module::import("datetime").attr("datetime");
                            } catch (...) {
                                return pybind11::none();
                            }
                        }();

                        if (!datetime_type.is_none() && pybind11::isinstance(item, datetime_type)) {
                            QDateTime dt_val = item.cast< QDateTime >();
                            value            = dt_val;
                            return true;
                        }
                    } catch (...) {
                    }

                    // 如果上面的方法失败，尝试使用时间戳
                    try {
                        // numpy.datetime64 可以转换为整数时间戳
                        pybind11::object timestamp = item.attr("astype")("datetime64[ms]");
                        int64_t ms                 = pybind11::cast< int64_t >(timestamp.attr("view")("int64"));
                        value                      = QDateTime::fromMSecsSinceEpoch(ms);
                        return true;
                    } catch (...) {
                        // 转换为字符串
                        QString str_val = QString::fromStdString(pybind11::str(item));
                        value           = str_val;
                        return true;
                    }
                }
                case 'm':  // timedelta64
                {
                    // 转换为 QString
                    QString str_val = QString::fromStdString(pybind11::str(np_obj.attr("item")()));
                    value           = str_val;
                    return true;
                }
                case 'V':  // void (记录数组)
                {
                    // 转换为 QByteArray
                    QByteArray byte_val = QByteArray(pybind11::str(np_obj.attr("item")()).cast< std::string >().c_str());
                    value = byte_val;
                    return true;
                }
                case 'O':  // Python 对象
                {
                    // 递归转换
                    pybind11::object item = np_obj.attr("item")();
                    type_caster< QVariant > caster;
                    if (caster.load(item, true)) {
                        value = caster.value;
                        return true;
                    }
                    return false;
                }
                default:
                    // 未知类型，尝试转换为字符串
                    value = QString::fromStdString(pybind11::str(np_obj.attr("item")()));
                    return true;
                }
            }
            // 处理多维数组（转换为 QVariantList）
            else {
                // 获取数组大小
                int total_size = 1;
                for (std::size_t i = 0; i < ndim; ++i) {
                    total_size *= shape[ i ].cast< int >();
                }
                // 将 numpy 数组转换为 Python 列表
                pybind11::list py_list = np_obj.attr("tolist")();

                // 递归转换为 QVariantList
                QVariantList qt_list;
                qt_list.reserve(total_size);

                for (auto item : py_list) {
                    type_caster< QVariant > caster;
                    if (caster.load(item, true)) {
                        qt_list.append(caster.value);
                    } else {
                        return false;
                    }
                }

                value = qt_list;
                return true;
            }
        } catch (...) {
            // numpy 转换失败
            return false;
        }

        return false;
    }

    // 从 Python 对象加载到 QVariant
    bool load(handle src, bool convert)
    {
        if (!src)
            return false;

        // 处理 None
        if (src.is_none()) {
            value = QVariant();
            return true;
        }

        // 首先检查是否是 numpy 对象
        if (is_numpy_array(src)) {
            return handle_numpy_object(src);
        }
        // 尝试各种类型的转换
        try {
            // 1. 整数类型
            if (PyLong_Check(src.ptr())) {
                // 尝试作为 int 加载
                try {
                    int int_val = src.cast< int >();
                    value       = QVariant(int_val);
                    return true;
                } catch (...) {
                    // 如果 int 失败，尝试 long long
                    try {
                        qlonglong ll_val = src.cast< qlonglong >();
                        value            = QVariant(ll_val);
                        return true;
                    } catch (...) {
                        // 如果 long long 也失败，尝试 unsigned long long
                        try {
                            qulonglong ull_val = src.cast< qulonglong >();
                            value              = QVariant(ull_val);
                            return true;
                        } catch (...) {
                            // 继续尝试其他类型
                        }
                    }
                }
            }

            // 2. 浮点数
            if (PyFloat_Check(src.ptr())) {
                try {
                    double double_val = src.cast< double >();
                    value             = QVariant(double_val);
                    return true;
                } catch (...) {
                    try {
                        float float_val = src.cast< float >();
                        value           = QVariant(float_val);
                        return true;
                    } catch (...) {
                        // 继续尝试其他类型
                    }
                }
            }

            // 3. 布尔值
            if (PyBool_Check(src.ptr())) {
                bool bool_val = src.cast< bool >();
                value         = QVariant(bool_val);
                return true;
            }

            // 4. 字符串
            if (PyUnicode_Check(src.ptr())) {
                try {
                    QString str_val = src.cast< QString >();
                    value           = QVariant(str_val);
                    return true;
                } catch (...) {
                    // 继续尝试其他类型
                }
            }

            // 5. bytes/bytearray
            if (PyBytes_Check(src.ptr()) || PyByteArray_Check(src.ptr())) {
                try {
                    QByteArray byte_val = src.cast< QByteArray >();
                    value               = QVariant(byte_val);
                    return true;
                } catch (...) {
                    // 继续尝试其他类型
                }
            }

            // 6. 列表/元组
            if (PyList_Check(src.ptr()) || PyTuple_Check(src.ptr())) {
                // 尝试作为 QVariantList 加载
                try {
                    QVariantList list_val = src.cast< QVariantList >();
                    value                 = QVariant(list_val);
                    return true;
                } catch (...) {
                    return false;
                }
            }

            // 7. 字典
            if (PyDict_Check(src.ptr())) {
                // 尝试作为 QVariantMap
                try {
                    QVariantMap map_val = src.cast< QVariantMap >();
                    value               = QVariant(map_val);
                    return true;
                } catch (...) {
                    // 尝试作为 QVariantHash
                    try {
                        QVariantHash hash_val = src.cast< QVariantHash >();
                        value                 = QVariant(hash_val);
                        return true;
                    } catch (...) {
                        // 继续尝试其他类型
                    }
                }
            }

            // 8. 集合
            if (PySet_Check(src.ptr())) {
                try {
                    QVariantList list_val;
                    auto set_obj = reinterpret_borrow< set >(src);
                    for (auto item : set_obj) {
                        type_caster< QVariant > caster;
                        if (caster.load(item, convert)) {
                            list_val.append(caster.value);
                        } else {
                            return false;
                        }
                    }
                    value = QVariant(list_val);
                    return true;
                } catch (...) {
                    // 继续尝试其他类型
                }
            }

            // 9. 日期和时间类型
            // 检查是否是 datetime.datetime
            try {
                static pybind11::object datetime_type = []() {
                    return pybind11::module::import("datetime").attr("datetime");
                }();

                if (pybind11::isinstance(src, datetime_type)) {
                    QDateTime dt_val = src.cast< QDateTime >();
                    value            = QVariant(dt_val);
                    return true;
                }
            } catch (...) {
            }

            // 检查是否是 datetime.date
            try {
                static pybind11::object date_type = []() { return pybind11::module::import("datetime").attr("date"); }();

                if (pybind11::isinstance(src, date_type)) {
                    QDate date_val = src.cast< QDate >();
                    value          = QVariant(date_val);
                    return true;
                }
            } catch (...) {
            }

            // 检查是否是 datetime.time
            try {
                static pybind11::object time_type = []() { return pybind11::module::import("datetime").attr("time"); }();

                if (pybind11::isinstance(src, time_type)) {
                    QTime time_val = src.cast< QTime >();
                    value          = QVariant(time_val);
                    return true;
                }
            } catch (...) {
            }

            // 如果以上都不行，尝试通用转换
            if (convert) {
                try {
                    // 最后尝试：转换成 QString
                    QString str_val = src.cast< QString >();
                    value           = QVariant(str_val);
                    return true;
                } catch (...) {
                }
            }

        } catch (...) {
            // 捕获异常，但不抛出，只是返回转换失败
            return false;
        }

        return false;
    }

    // 将 QVariant 转换为 Python 对象
    static handle cast(const QVariant& src, return_value_policy policy, handle parent)
    {
        if (!src.isValid() || src.isNull()) {
            Py_RETURN_NONE;
        }

        try {
            // 根据 QVariant 的类型进行转换
            int type_id = src.userType();

            // 检查 QMetaType 类型
            switch (type_id) {
            case QMetaType::UnknownType:
                Py_RETURN_NONE;

            // 基本数值类型
            case QMetaType::Int:
                return pybind11::cast(src.toInt()).release();
            case QMetaType::UInt:
                return pybind11::cast(src.toUInt()).release();
            case QMetaType::LongLong:
                return pybind11::cast(src.toLongLong()).release();
            case QMetaType::ULongLong:
                return pybind11::cast(src.toULongLong()).release();
            case QMetaType::Double:
                return pybind11::cast(src.toDouble()).release();
            case QMetaType::Float:
                return pybind11::cast(src.toFloat()).release();
            case QMetaType::Bool:
                return pybind11::cast(src.toBool()).release();

            // 字符串类型
            case QMetaType::QString:
                return pybind11::cast(src.toString()).release();
            case QMetaType::QByteArray:
                return pybind11::cast(src.toByteArray()).release();
            case QMetaType::QChar:
                return pybind11::cast(src.toChar()).release();

            // 日期时间类型
            case QMetaType::QDate:
                return pybind11::cast(src.toDate()).release();
            case QMetaType::QTime:
                return pybind11::cast(src.toTime()).release();
            case QMetaType::QDateTime:
                return pybind11::cast(src.toDateTime()).release();
            // 容器类型
            case QMetaType::QVariantList: {
                QVariantList list = src.toList();
                pybind11::list py_list;
                for (const QVariant& item : list) {
                    py_list.append(cast(item, policy, parent));
                }
                return py_list.release();
            }

            case QMetaType::QVariantMap: {
                QVariantMap map = src.toMap();
                pybind11::dict py_dict;
                for (auto it = map.begin(); it != map.end(); ++it) {
                    py_dict[ pybind11::cast(it.key()) ] = cast(it.value(), policy, parent);
                }
                return py_dict.release();
            }

            case QMetaType::QVariantHash: {
                QVariantHash hash = src.toHash();
                pybind11::dict py_dict;
                for (auto it = hash.begin(); it != hash.end(); ++it) {
                    py_dict[ pybind11::cast(it.key()) ] = cast(it.value(), policy, parent);
                }
                return py_dict.release();
            }

            // 处理 QList<T> 类型
            default: {
                // 尝试检测是否是 QList<T> 类型
                const char* type_name = src.typeName();
                if (type_name) {
                    QString type_str = QString::fromUtf8(type_name);

                    // QList<QString>
                    if (type_str == "QList<QString>") {
                        return pybind11::cast(src.value< QList< QString > >()).release();
                    }
                    // QList<int>
                    else if (type_str == "QList<int>") {
                        return pybind11::cast(src.value< QList< int > >()).release();
                    }
                    // QList<double>
                    else if (type_str == "QList<double>") {
                        return pybind11::cast(src.value< QList< double > >()).release();
                    }
                    // QVector<QString>
                    else if (type_str == "QVector<QString>") {
                        return pybind11::cast(src.value< QVector< QString > >()).release();
                    }
                    // 其他已知类型的 QList
                    else if (type_str.startsWith("QList<") || type_str.startsWith("QVector<")) {
                        // 对于未知类型的列表，转换为 Python 列表
                        QVariantList list = src.toList();
                        pybind11::list py_list;
                        for (const QVariant& item : list) {
                            py_list.append(cast(item, policy, parent));
                        }
                        return py_list.release();
                    }
                    // QMap 或 QHash
                    else if (type_str.startsWith("QMap<") || type_str.startsWith("QHash<")) {
                        // 转换为 Python 字典
                        QVariantMap map = src.toMap();
                        pybind11::dict py_dict;
                        for (auto it = map.begin(); it != map.end(); ++it) {
                            py_dict[ pybind11::cast(it.key()) ] = cast(it.value(), policy, parent);
                        }
                        return py_dict.release();
                    }
                    // QSet
                    else if (type_str.startsWith("QSet<")) {
                        // 转换为 Python 集合
                        QVariantList list = src.toList();
                        pybind11::set py_set;
                        for (const QVariant& item : list) {
                            py_set.add(cast(item, policy, parent));
                        }
                        return py_set.release();
                    }
                }

                // 如果以上都不匹配，尝试使用 toString()
                if (src.canConvert< QString >()) {
                    return pybind11::cast(src.toString()).release();
                }

                // 最后尝试：返回字符串表示
                return pybind11::cast(src.toString()).release();
            }
            }
        } catch (...) {
            // 如果转换失败，返回 None
            Py_RETURN_NONE;
        }
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

inline pybind11::object toPyObject(const QString& qt_str)
{
    return pybind11::cast(qt_str);
}

// QDate 转换函数
inline QDate fromPyDate(pybind11::handle py_date)
{
    return pybind11::cast< QDate >(py_date);
}

inline pybind11::object toPyObject(const QDate& qt_date)
{
    return pybind11::cast(qt_date);
}

// QTime 转换函数
inline QTime fromPyTime(pybind11::handle py_time)
{
    return pybind11::cast< QTime >(py_time);
}

inline pybind11::object toPyObject(const QTime& qt_time)
{
    return pybind11::cast(qt_time);
}

// QDateTime 转换函数
inline QDateTime fromPyDateTime(pybind11::handle py_datetime)
{
    return pybind11::cast< QDateTime >(py_datetime);
}

inline pybind11::object toPyObject(const QDateTime& qt_datetime)
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
inline pybind11::object toPyObject(const QList< T >& qt_list)
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
inline pybind11::object toPyObject(const QVector< T >& qt_vector)
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
inline pybind11::object toPyObject(const QSet< T >& qt_set)
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
inline pybind11::object toPyObject(const QHash< K, V >& qt_hash)
{
    return pybind11::cast(qt_hash);
}

template< typename K, typename V >
inline QMap< K, V > fromPyMap(pybind11::handle py_dict)
{
    return pybind11::cast< QMap< K, V > >(py_dict);
}

template< typename K, typename V >
inline pybind11::object toPyObject(const QMap< K, V >& qt_map)
{
    return pybind11::cast(qt_map);
}

// QVariant 转换函数
inline QVariant fromPyVariant(pybind11::handle py_obj)
{
    return pybind11::cast< QVariant >(py_obj);
}

inline pybind11::object toPyObject(const QVariant& qt_var)
{
    return pybind11::cast(qt_var);
}

inline pybind11::object toPyObject(const QVariant& qt_var, const pybind11::dtype& dt)
{
    return dt.attr("type")(toPyObject(qt_var));
}

// 能转成 QDateTime 吗？
// 支持：datetime.datetime / pandas.Timestamp / numpy.datetime64
inline bool canCastToQDateTime(pybind11::handle src)
{
    if (!src)
        return false;

    try {
        // 1. datetime.datetime
        static pybind11::object datetime_type = pybind11::module::import("datetime").attr("datetime");
        if (pybind11::isinstance(src, datetime_type))
            return true;

        // 2. pandas.Timestamp
        static pybind11::object pd_Timestamp = []() -> pybind11::object {
            try {
                return pybind11::module::import("pandas").attr("Timestamp");
            } catch (...) {
                return pybind11::none();
            }
        }();
        if (!pd_Timestamp.is_none() && pybind11::isinstance(src, pd_Timestamp))
            return true;

        // 3. numpy.datetime64
        static pybind11::object np_datetime64 = []() -> pybind11::object {
            try {
                return pybind11::module::import("numpy").attr("datetime64");
            } catch (...) {
                return pybind11::none();
            }
        }();
        if (!np_datetime64.is_none() && pybind11::isinstance(src, np_datetime64))
            return true;
    } catch (...) {
    }
    return false;
}

inline bool canCastToQDate(pybind11::handle src)
{
    if (!src)
        return false;
    try {
        static pybind11::object date_type = pybind11::module::import("datetime").attr("date");
        return pybind11::isinstance(src, date_type);
    } catch (...) {
    }
    return false;
}

inline bool canCastToQTime(pybind11::handle src)
{
    if (!src)
        return false;
    try {
        static pybind11::object time_type = pybind11::module::import("datetime").attr("time");
        return pybind11::isinstance(src, time_type);
    } catch (...) {
    }
    return false;
}

inline bool canCastToQString(pybind11::handle src)
{
    return src && (PyUnicode_Check(src.ptr()) || PyBytes_Check(src.ptr()));
}

}  // namespace PY
}  // namespace DA
#endif  // DAPYBIND11QTCASTER_HPP
