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
#include "DAPybind11QtCaster.hpp"

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
    ~DAPySeries() override;
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
    // 返回 dtype 的字符串表示（如 "int64"、"float64"、"datetime64[ns]"、"object" 等）
    QString dtypeString() const;
    bool empty() const;
    std::size_t size() const;
    QString name() const;
    pybind11::object iat(std::size_t i) const;
    void iat(std::size_t r, const pybind11::object& v);
    QVariant value(std::size_t i) const;
    // 返回指定位置元素的字符串表示（通过 Python str() 转换，适用于所有类型）
    QString valueAsString(std::size_t i) const;
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
    // 返回最大值的位置索引（iloc 语义），失败返回 -1
    // 两级策略：nanargmax → to_numeric/to_datetime(coerce)+nanargmax
    long idxmaxPosition() const;
    // 返回最小值的位置索引（iloc 语义），失败返回 -1
    // 两级策略：nanargmin → to_numeric/to_datetime(coerce)+nanargmin
    long idxminPosition() const;
    // 生成描述性统计信息（count、mean、std、min、25%、50%、75%、max 等）
    // 返回一个新的 DAPySeries，其 index 为统计项名称
    DAPySeries describe() const;
    // 返回布尔掩码 Series，标记每个元素是否为缺失值（NaN/None）
    DAPySeries isNull() const;
    // 返回 Series 元素之和（对布尔掩码求 True 计数用于缺失值统计）
    qint64 sum() const;

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
 * @tparam T 目标元素类型
 * @tparam VectLikeIte 输出迭代器类型
 * @param begin 输出迭代器
 */
template< typename T, typename VectLikeIte >
void DAPySeries::castTo(VectLikeIte begin) const
{
    pybind11::object series = object();               // 当前 Series
    pybind11::object values = series.attr("values");  // ndarray

    // 获取 dtype 字符串
    pybind11::object dtype_obj = series.attr("dtype");
    std::string dtype_str      = pybind11::str(dtype_obj).cast< std::string >();

    // 检查是否是pandas Series
    if (isSeries(series)) {
        if (dtype_str.find("datetime64") == 0) {

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
            bool has_timezone = false;
            try {
                pybind11::object dt_accessor = series.attr("dt");
                pybind11::object tz          = dt_accessor.attr("tz");
                has_timezone                 = !tz.is_none();
            } catch (...) {
                has_timezone = false;
            }

            pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > buf;
            // naive datetime（无时区）的本地时区偏移（毫秒），用于修正时区双重偏移
            double tz_offset_ms = 0.0;

            if (has_timezone) {
                pybind11::object dt_accessor = series.attr("dt");
                pybind11::object utc_series  = dt_accessor.attr("tz_convert")("UTC");
                buf = utc_series.attr("astype")("int64")
                          .attr("values")
                          .cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();
            } else {
                // naive datetime 代表本地时间，但 astype("int64") 将其当作 UTC 计算 epoch。
                // 后续 QwtDateScaleDraw(Qt::LocalTime) 又会加上本地时区偏移，
                // 导致时间整体偏移一个时区（如中国 +8 小时）。
                // 这里减去本地 UTC 偏移，使毫秒值代表真正的 UTC 时间。
                try {
                    pybind11::object now       = pybind11::module::import("datetime").attr("datetime").attr("now")();
                    pybind11::object aware     = now.attr("astimezone")();
                    pybind11::object offset_td = aware.attr("utcoffset")();
                    if (!offset_td.is_none()) {
                        tz_offset_ms = pybind11::float_(offset_td.attr("total_seconds")()).cast< double >() * 1000.0;
                    }
                } catch (...) {
                    tz_offset_ms = 0.0;
                }
                buf = series.attr("astype")("int64")
                          .attr("values")
                          .cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();
            }

            // 使用捕获的变量进行转换
            std::transform(buf.data(), buf.data() + buf.size(), begin, [ unit_divisor, time_unit, tz_offset_ms ](int64_t raw_val) -> double {
                if (time_unit == "s") {
                    return static_cast< double >(raw_val) * 1000.0 - tz_offset_ms;  // 秒 -> 毫秒
                }
                return static_cast< double >(raw_val / unit_divisor) - tz_offset_ms;
            });
            return;
        }
        // 处理时间增量类型 (timedelta)
        else if (dtype_str.find("timedelta64") == 0) {
            // 转成 int64 (nanoseconds)
            values = series.attr("astype")("int64").attr("values");
            //            pybind11::object ts_local = series.attr("dt")
            //                                            .attr("tz_localize")(pybind11::none())  // 如果 naive，先声明为"本地"
            //                                            .attr("tz_convert")(pybind11::str("local"));  // 有 tz 的也转到本地
            //            values = ts_local.attr("astype")("int64").attr("values");
            // 将纳秒转换为秒
            auto buf = values.cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();
            std::transform(buf.data(), buf.data() + buf.size(), begin, [](int64_t ns) -> double {
                return static_cast< double >(ns) / 1e9;  // 纳秒转秒
            });
            return;
        }
        // 处理分类数据 (categorical)
        else if (dtype_str.find("category") == 0) {
            // 获取分类的代码
            values   = series.attr("cat").attr("codes").attr("values");
            auto buf = values.cast< pybind11::array_t< T, pybind11::array::c_style | pybind11::array::forcecast > >();
            std::copy(buf.data(), buf.data() + buf.size(), begin);
            return;
        }
        // 处理布尔类型
        else if (dtype_str == "bool") {
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
        try {
            static DAPyObjectWrapper pd_to_datetime = DA::PY::importPyType("pandas", "to_datetime");
            static DAPyObjectWrapper pd_NaT         = DA::PY::importPyType("pandas", "NaT");
            pybind11::object sample = series.attr("dropna")().attr("head")(1);
            if (pybind11::len(sample) > 0) {
                // NOTE: 必须使用 pybind11::int_(0) 而非字面量 0。
                // 字面量 0 是空指针常量，会匹配 operator[](const char*) 重载，
                // 传入 nullptr 导致 PyUnicode_FromString(NULL) 崩溃。
                pybind11::object first_val = sample.attr("iat")[ pybind11::int_(0) ];
                if (pybind11::isinstance< pybind11::str >(first_val)) {
                    std::string first_val_str = first_val.cast< std::string >();

                    pybind11::object test_parse = pd_to_datetime.object()(first_val, pybind11::arg("errors") = "coerce");

                    // NaT 是单例实例而非 type，不能用 isinstance 判断，用身份比较
                    if (!test_parse.is_none() && !test_parse.is(pd_NaT.object())) {
                        pybind11::object dt_series = pd_to_datetime.object()(series, pybind11::arg("errors") = "coerce");

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

                        // pd.to_datetime 对字符串产生 naive datetime（无时区），
                        // 同 datetime64 分支一样需要减去本地 UTC 偏移，
                        // 否则 QwtDateScaleDraw(Qt::LocalTime) 会造成时区双重偏移。
                        double tz_offset_ms = 0.0;
                        try {
                            pybind11::object now       = pybind11::module::import("datetime").attr("datetime").attr("now")();
                            pybind11::object aware     = now.attr("astimezone")();
                            pybind11::object offset_td = aware.attr("utcoffset")();
                            if (!offset_td.is_none()) {
                                tz_offset_ms = pybind11::float_(offset_td.attr("total_seconds")()).cast< double >() * 1000.0;
                            }
                        } catch (...) {
                            tz_offset_ms = 0.0;
                        }

                        pybind11::object int_series = dt_series.attr("astype")("int64");
                        values                      = int_series.attr("values");
                        auto buf =
                            values.cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();

                        std::transform(
                            buf.data(), buf.data() + buf.size(), begin, [ unit_divisor, time_unit, tz_offset_ms ](int64_t raw_val) -> double {
                                if (time_unit == "s") {
                                    return static_cast< double >(raw_val) * 1000.0 - tz_offset_ms;
                                }
                                return static_cast< double >(raw_val / unit_divisor) - tz_offset_ms;
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

    // 对于其他类型
    auto buf = values.cast< pybind11::array_t< T, pybind11::array::c_style | pybind11::array::forcecast > >();
    std::copy(buf.data(), buf.data() + buf.size(), begin);
}
}  // namespace DA

DAPYBINDQT_API QDebug operator<<(QDebug dbg, const DA::DAPySeries& ser);
Q_DECLARE_METATYPE(DA::DAPySeries)
#endif  // DASERIES_H
