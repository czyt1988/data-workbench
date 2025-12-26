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
    // 操作符
    DAPySeries& operator=(const pybind11::object& obj);
    DAPySeries& operator=(const DAPySeries& s);
    DAPySeries& operator=(const DAPyObjectWrapper& obj);
    DAPySeries& operator=(pybind11::object&& obj);
    DAPySeries& operator=(DAPySeries&& s);
    DAPySeries& operator=(DAPyObjectWrapper&& obj);
    QVariant operator[](std::size_t i) const;

public:
    // 获取dtype
    pybind11::dtype dtype() const;
    // Series.empty
    bool empty() const;
    // Series.size
    std::size_t size() const;
    // Series.name
    QString name() const;
    // Series.iat
    QVariant iat(std::size_t i) const;
    bool iat(std::size_t r, const QVariant& v);
    // 类型判断
    bool isNumeric() const;
    bool isDateTime() const;
    bool isString() const;
    bool isCategorical() const;

    // 索引相关
    DAPyIndex index() const;
    QStringList indexAsStringList() const;
    QVector< double > indexAsDoubleVector() const;
    QVector< QDateTime > indexAsDateTimeVector() const;
    // 数据转换
    DAPySeries astype(const pybind11::dtype& dt) const;
    DAPySeries toDateTime() const;

public:
    // 判断是否为series
    static bool isSeries(const pybind11::object& obj);
    // 把series转换为一个容器数组
    template< typename T, typename VectLikeIte >
    void castTo(VectLikeIte begin) const;

protected:
    // 检测是否为dataframe，如果不是将会设置为none
    void checkObjectValid();

public:
    // 转换为文本
    QString toString(std::size_t maxele = 12) const;
};

DAPYBINDQT_API std::vector< double > toVectorDouble(const DA::DAPySeries& ser);
DAPYBINDQT_API QVector< double > toQVectorDouble(const DA::DAPySeries& ser);

/**
 * @brief 把series转换为一个容器数组
 *
 * 支持的数据类型转换：
 * - 数值类型：直接转换
 * - 日期时间类型：转换为毫秒时间戳
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
 * @note 对于字符串类型，抛出异常因此需要先检查是否为字符串
 */
template< typename T, typename VectLikeIte >
void DAPySeries::castTo(VectLikeIte begin) const
{
    pybind11::object series = object();               // 当前 Series
    pybind11::object values = series.attr("values");  // ndarray

    // 检查是否是pandas Series
    if (pybind11::isinstance(series, pybind11::module::import("pandas").attr("Series"))) {
        pybind11::object dtype = series.attr("dtype");
        std::string dtype_str  = pybind11::str(dtype).cast< std::string >();

        // 处理日期时间类型 (C++17兼容的方式)
        if (dtype_str.find("datetime64") == 0) {
            // 检查是否有时区信息
            bool has_timezone = false;
            try {
                pybind11::object dt_accessor = series.attr("dt");
                pybind11::object tz          = dt_accessor.attr("tz");
                has_timezone                 = !tz.is_none();
            } catch (...) {
                has_timezone = false;
            }

            if (has_timezone) {
                // 有时区信息：先转换为UTC，再处理
                pybind11::object dt_accessor = series.attr("dt");
                pybind11::object utc_series  = dt_accessor.attr("tz_convert")("UTC");
                values                       = utc_series.attr("astype")("int64").attr("values");
            } else {
                // 没有时区信息：直接转换
                values = series.attr("astype")("int64").attr("values");
            }

            // 转换时间戳到本地local
            auto buf = values.cast< pybind11::array_t< int64_t, pybind11::array::c_style | pybind11::array::forcecast > >();

            std::transform(buf.data(), buf.data() + buf.size(), begin, [](int64_t ns) -> double {
                qint64 utcMs = ns / 1'000'000;
                return utcMs;
            });
            return;
        }
        // 处理时间增量类型 (timedelta)
        else if (dtype_str.find("timedelta64") == 0) {
            // 转成 int64 (nanoseconds)
            values = series.attr("astype")("int64").attr("values");
            //            pybind11::object ts_local = series.attr("dt")
            //                                            .attr("tz_localize")(pybind11::none())  // 如果 naive，先声明为“本地”
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

    // 对于其他类型
    auto buf = values.cast< pybind11::array_t< T, pybind11::array::c_style | pybind11::array::forcecast > >();
    std::copy(buf.data(), buf.data() + buf.size(), begin);
}
}  // namespace DA

DAPYBINDQT_API QDebug operator<<(QDebug dbg, const DA::DAPySeries& ser);
Q_DECLARE_METATYPE(DA::DAPySeries)
#endif  // DASERIES_H
