#ifndef DAPYSERIES_H
#define DAPYSERIES_H
#include "DAPyBindQtGlobal.h"
#include "DAPyObjectWrapper.h"
#include <QDebug>
#include <QList>
#include <QVariant>
#include <QDateTime>
#include "DAPybind11InQt.h"
#include "DAPyIndex.h"
#include "numpy/DAPyDType.h"

namespace pybind11
{
class dtype;
}
namespace DA
{
/**
 * @brief 对Pandas.Series的Qt封装
 */
class DAPYBINDQT_API DAPySeries : public DAPyObjectWrapper
{
public:
    DAPySeries() = default;
    DAPySeries(const DAPySeries& s);
    DAPySeries(DAPySeries&& s);
    DAPySeries(const pybind11::object& obj);
    DAPySeries(pybind11::object&& obj);
    ~DAPySeries();
    DAPySeries& operator=(const pybind11::object& obj);
    DAPySeries& operator=(const DAPySeries& s);
    DAPySeries& operator=(const DAPyObjectWrapper& obj);
    DAPySeries& operator=(pybind11::object&& obj);
    DAPySeries& operator=(DAPySeries&& s);
    DAPySeries& operator=(DAPyObjectWrapper&& obj);
    pybind11::object operator[](std::size_t i) const;
    pybind11::object operator[](const QString& colName) const;

public:
    pybind11::dtype dtype() const;
    DAPyDType dtypeObject() const;
    bool empty() const;
    std::size_t size() const;
    QString name() const;
    pybind11::object iat(std::size_t i) const;
    void iat(std::size_t r, const pybind11::object& v);
    QVariant value(std::size_t i) const;
    bool setValue(std::size_t i, const QVariant& v);
    bool isNumeric() const;
    bool isDateTime() const;
    bool isString() const;
    bool isCategorical() const;

    DAPyIndex index() const;
    QStringList indexAsStringList() const;
    QVector< double > indexAsDoubleVector() const;
    QVector< QDateTime > indexAsDateTimeVector() const;
    DAPySeries astype(const pybind11::dtype& dt) const;
    DAPySeries toDateTime() const;

public:
    static bool isSeries(const pybind11::object& obj);
    template< typename T, typename VectLikeIte >
    void castTo(VectLikeIte begin) const;

protected:
    void checkObjectValid();

public:
    QString toString(std::size_t maxele = 12) const;
};

DAPYBINDQT_API std::vector< double > toVectorDouble(const DA::DAPySeries& ser);
DAPYBINDQT_API QVector< double > toQVectorDouble(const DA::DAPySeries& ser);

/**
 * @brief 把series转换为一个容器数组
 *
 * 支持的数据类型转换：
 * - 数值类型：直接转换
 * - 日期时间类型（datetime64）：转换为毫秒时间戳
 * - 日期字符串（object类型中可解析为日期的字符串）：自动转换为毫秒时间戳
 * - 时间增量类型：转换为秒
 * - 分类数据：转换为分类代码
 * - 布尔类型：转换为0/1
 *
 * @code
 * // 示例1：转换为double向量
 * DAPySeries x;
 * std::vector<double> vx;
 * vx.reserve(x.size());
 * x.castTo<double>(std::back_inserter(vx));
 *
 * // 示例2：转换为QVector
 * QVector<double> qv;
 * qv.reserve(x.size());
 * x.castTo<double>(std::back_inserter(qv));
 * @endcode
 *
 * @param begin 输出迭代器
 *
 * @note 对于日期时间类型，会转换为毫秒时间戳（从1970-01-01开始）
 * @note 对于日期字符串（如"2025/12/06 19:02:48"），会自动解析并转换为毫秒时间戳
 * @note 对于无法解析的字符串类型，会抛出异常
 */
template< typename T, typename VectLikeIte >
void DAPySeries::castTo(VectLikeIte begin) const
{
    pybind11::object series = object();               // 当前 Series
    pybind11::object values = series.attr("values");  // ndarray

    // 获取 dtype 字符串
    pybind11::object dtype_obj = series.attr("dtype");
    std::string dtype_str      = pybind11::str(dtype_obj).cast< std::string >();

    // 调试信息：打印 dtype 和 series 名称
    qDebug() << "[DAPySeries::castTo] dtype_str =" << QString::fromStdString(dtype_str) << ", series name =" << name()
             << ", size =" << size();

    // 检查是否是pandas Series
    if (pybind11::isinstance(series, pybind11::module::import("pandas").attr("Series"))) {
        if (dtype_str.find("datetime64") == 0) {
            qDebug() << "[DAPySeries::castTo] Detected datetime64 type, converting...";

            int64_t unit_divisor  = 1'000'000;  // 默认假设纳秒，转换为毫秒
            std::string time_unit = "ns";       // 默认时间单位

            // 解析 datetime64 的精度单位，如 datetime64[us], datetime64[ns], datetime64[ms], datetime64[s]
            size_t bracket_pos = dtype_str.find('[');
            if (bracket_pos != std::string::npos) {
                size_t end_bracket = dtype_str.find(']', bracket_pos);
                if (end_bracket != std::string::npos) {
                    time_unit = dtype_str.substr(bracket_pos + 1, end_bracket - bracket_pos - 1);
                }
            }

            // 根据时间单位确定转换因子（目标：毫秒）
            if (time_unit == "ns") {
                unit_divisor = 1'000'000;  // 纳秒 -> 毫秒
            } else if (time_unit == "us") {
                unit_divisor = 1'000;  // 微秒 -> 毫秒
            } else if (time_unit == "ms") {
                unit_divisor = 1;  // 已经是毫秒
            } else if (time_unit == "s") {
                unit_divisor = 1;  // 秒 -> 毫秒需要乘以1000，但这里特殊处理
            }

            qDebug() << "[DAPySeries::castTo] Detected time unit:" << QString::fromStdString(time_unit)
                     << ", divisor to ms:" << unit_divisor;

            bool has_timezone = false;
            try {
                pybind11::object dt_accessor = series.attr("dt");
                pybind11::object tz          = dt_accessor.attr("tz");
                has_timezone                 = !tz.is_none();
            } catch (...) {
                has_timezone = false;
            }

            qDebug() << "[DAPySeries::castTo] has_timezone =" << has_timezone;

            pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > buf;

            if (has_timezone) {
                pybind11::object dt_accessor = series.attr("dt");
                pybind11::object utc_series  = dt_accessor.attr("tz_convert")("UTC");
                buf                          = utc_series.attr("astype")("int64")
                          .attr("values")
                          .cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();
            } else {
                buf = series.attr("astype")("int64")
                          .attr("values")
                          .cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();
            }

            // 调试信息：打印前几个时间戳
            if (buf.size() > 0) {
                int64_t raw_val = buf.data()[ 0 ];
                double ms_val;
                if (time_unit == "s") {
                    ms_val = static_cast< double >(raw_val) * 1000.0;  // 秒 -> 毫秒
                } else {
                    ms_val = static_cast< double >(raw_val / unit_divisor);
                }
                qDebug() << "[DAPySeries::castTo] First raw timestamp (" << QString::fromStdString(time_unit)
                         << "):" << raw_val;
                qDebug() << "[DAPySeries::castTo] First converted timestamp (ms):" << ms_val;
                QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast< qint64 >(ms_val));
                qDebug() << "[DAPySeries::castTo] First datetime:" << dt.toString("yyyy/MM/dd hh:mm:ss");
            }

            // 使用捕获的变量进行转换
            std::transform(buf.data(), buf.data() + buf.size(), begin, [ unit_divisor, time_unit ](int64_t raw_val) -> double {
                if (time_unit == "s") {
                    return static_cast< double >(raw_val) * 1000.0;  // 秒 -> 毫秒
                }
                return static_cast< double >(raw_val / unit_divisor);
            });
            return;
        }
        // 处理时间增量类型 (timedelta)
        else if (dtype_str.find("timedelta64") == 0) {
            qDebug() << "[DAPySeries::castTo] Detected timedelta64 type, converting...";
            // 转成 int64 (nanoseconds)
            values = series.attr("astype")("int64").attr("values");
            //            pybind11::object ts_local = series.attr("dt")
            //                                            .attr("tz_localize")(pybind11::none())  // 如果 naive，先声明为"本地"
            //                                            .attr("tz_convert")(pybind11::str("local"));  // 有 tz 的也转到本地
            //            values = ts_local.attr("astype")("int64").attr("values");
            // 将纳秒转换为秒
            auto buf =
                values.cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();
            std::transform(buf.data(), buf.data() + buf.size(), begin, [](int64_t ns) -> double {
                return static_cast< double >(ns) / 1e9;  // 纳秒转秒
            });
            return;
        }
        // 处理分类数据 (categorical)
        else if (dtype_str.find("category") == 0) {
            qDebug() << "[DAPySeries::castTo] Detected category type, converting...";
            // 获取分类的代码
            values   = series.attr("cat").attr("codes").attr("values");
            auto buf = values.cast< pybind11::array_t< T, pybind11::array::c_style | pybind11::array::forcecast > >();
            std::copy(buf.data(), buf.data() + buf.size(), begin);
            return;
        }
        // 处理布尔类型
        else if (dtype_str == "bool") {
            qDebug() << "[DAPySeries::castTo] Detected bool type, converting...";
            values = series.attr("astype")("int8").attr("values");
            auto buf = values.cast< pybind11::array_t< int8_t, pybind11::array::c_style | pybind11::array::forcecast > >();
            std::transform(buf.data(), buf.data() + buf.size(), begin, [](int8_t b) -> T { return static_cast< T >(b); });
            return;
        }
    }

    // 处理 object/str/string 类型（可能是日期字符串）
    // 检查是否为字符串类型 dtype，尝试检测是否可以转换为日期时间
    // 支持: object, str, string, string[pyarrow] 等类型
    auto is_string_dtype = [](const std::string& dt) -> bool {
        if (dt == "object")
            return true;
        if (dt == "str")
            return true;
        if (dt.find("string") == 0)
            return true;  // string, string[pyarrow], etc.
        return false;
    };

    if (is_string_dtype(dtype_str)) {
        qDebug() << "[DAPySeries::castTo] Detected string/object type, trying to parse as datetime...";
        try {
            pybind11::module pd     = pybind11::module::import("pandas");
            pybind11::object sample = series.attr("dropna")().attr("head")(1);
            qDebug() << "[DAPySeries::castTo] sample length =" << pybind11::len(sample);
            if (pybind11::len(sample) > 0) {
                pybind11::object first_val = sample.attr("iat")[ 0 ];
                qDebug() << "[DAPySeries::castTo] first_val type is string:"
                         << pybind11::isinstance< pybind11::str >(first_val);
                if (pybind11::isinstance< pybind11::str >(first_val)) {
                    std::string first_val_str = first_val.cast< std::string >();
                    qDebug() << "[DAPySeries::castTo] first_val =" << QString::fromStdString(first_val_str);

                    pybind11::object pd_to_datetime = pd.attr("to_datetime");
                    pybind11::object test_parse     = pd_to_datetime(first_val, pybind11::arg("errors") = "coerce");
                    qDebug() << "[DAPySeries::castTo] test_parse is none:" << test_parse.is_none();
                    qDebug() << "[DAPySeries::castTo] test_parse is NaT:"
                             << pybind11::isinstance(test_parse, pd.attr("NaT"));

                    if (!test_parse.is_none() && !pybind11::isinstance(test_parse, pd.attr("NaT"))) {
                        qDebug() << "[DAPySeries::castTo] Successfully parsed as datetime, converting entire series...";
                        pybind11::object dt_series = pd_to_datetime(series, pybind11::arg("errors") = "coerce");

                        // 获取转换后的 dtype 以确定时间单位
                        pybind11::object dt_dtype_obj = dt_series.attr("dtype");
                        std::string dt_dtype_str      = pybind11::str(dt_dtype_obj).cast< std::string >();

                        int64_t unit_divisor  = 1'000'000;  // 默认纳秒 -> 毫秒
                        std::string time_unit = "ns";

                        size_t bracket_pos = dt_dtype_str.find('[');
                        if (bracket_pos != std::string::npos) {
                            size_t end_bracket = dt_dtype_str.find(']', bracket_pos);
                            if (end_bracket != std::string::npos) {
                                time_unit = dt_dtype_str.substr(bracket_pos + 1, end_bracket - bracket_pos - 1);
                            }
                        }

                        if (time_unit == "ns") {
                            unit_divisor = 1'000'000;
                        } else if (time_unit == "us") {
                            unit_divisor = 1'000;
                        } else if (time_unit == "ms") {
                            unit_divisor = 1;
                        } else if (time_unit == "s") {
                            unit_divisor = 1;
                        }

                        qDebug() << "[DAPySeries::castTo] Parsed datetime dtype:" << QString::fromStdString(dt_dtype_str)
                                 << ", time_unit:" << QString::fromStdString(time_unit);

                        pybind11::object int_series = dt_series.attr("astype")("int64");
                        values                      = int_series.attr("values");
                        auto buf =
                            values.cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();

                        if (buf.size() > 0) {
                            int64_t raw_val = buf.data()[ 0 ];
                            double ms_val;
                            if (time_unit == "s") {
                                ms_val = static_cast< double >(raw_val) * 1000.0;
                            } else {
                                ms_val = static_cast< double >(raw_val / unit_divisor);
                            }
                            qDebug() << "[DAPySeries::castTo] First raw timestamp ("
                                     << QString::fromStdString(time_unit) << "):" << raw_val;
                            qDebug() << "[DAPySeries::castTo] First converted timestamp (ms):" << ms_val;
                            QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast< qint64 >(ms_val));
                            qDebug() << "[DAPySeries::castTo] First datetime:" << dt.toString("yyyy/MM/dd hh:mm:ss");
                        }

                        std::transform(buf.data(), buf.data() + buf.size(), begin, [ unit_divisor, time_unit ](int64_t raw_val) -> double {
                            if (time_unit == "s") {
                                return static_cast< double >(raw_val) * 1000.0;
                            }
                            return static_cast< double >(raw_val / unit_divisor);
                        });
                        return;
                    } else {
                        qDebug() << "[DAPySeries::castTo] Failed to parse as datetime, treating as regular string";
                    }
                }
            }
        } catch (const std::exception& e) {
            qDebug() << "[DAPySeries::castTo] Exception caught:" << e.what();
        } catch (...) {
            qDebug() << "[DAPySeries::castTo] Unknown exception caught";
        }
    }

    qDebug() << "[DAPySeries::castTo] Using default numeric conversion for dtype:" << QString::fromStdString(dtype_str);
    // 对于其他类型
    auto buf = values.cast< pybind11::array_t< T, pybind11::array::c_style | pybind11::array::forcecast > >();
    std::copy(buf.data(), buf.data() + buf.size(), begin);
}
}  // namespace DA

DAPYBINDQT_API QDebug operator<<(QDebug dbg, const DA::DAPySeries& ser);
Q_DECLARE_METATYPE(DA::DAPySeries)
#endif  // DASERIES_H
