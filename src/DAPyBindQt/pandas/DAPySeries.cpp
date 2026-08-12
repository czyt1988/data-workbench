#include "DAPySeries.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyModulePandas.h"
#include "numpy/DAPyDType.h"
#include <QDateTime>
#include <iterator>
#include "DALogCategory.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAPySeries
//===================================================

/**
 * @brief 拷贝构造
 * @param s 另一个DAPySeries
 */
DAPySeries::DAPySeries(const DAPySeries& s) : DAPyObjectWrapper(s)
{
    if (!s.isNone()) {
        checkObjectValid();
    }
}

/**
 * @brief 移动构造
 * @param s 右值DAPySeries
 */
DAPySeries::DAPySeries(DAPySeries&& s) : DAPyObjectWrapper(std::move(s))
{
}

/**
 * @brief 构造，从pybind11::object构造
 * @param obj pybind11对象
 */
DAPySeries::DAPySeries(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
    if (!obj.is_none()) {
        checkObjectValid();
    }
}

/**
 * @brief 构造，从pybind11::object右值构造
 * @param obj pybind11对象右值
 */
DAPySeries::DAPySeries(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
    if (!obj.is_none()) {
        checkObjectValid();
    }
}

/**
 * @brief 析构
 */
DAPySeries::~DAPySeries()
{
}

/**
 * @brief 赋值操作符
 * @param obj pybind11对象
 * @return 返回自身的引用
 */
DAPySeries& DAPySeries::operator=(const pybind11::object& obj)
{
    object() = obj;
    checkObjectValid();
    return *this;
}

/**
 * @brief 移动赋值操作符
 * @param obj pybind11对象右值
 * @return 返回自身的引用
 */
DAPySeries& DAPySeries::operator=(pybind11::object&& obj)
{
    object() = std::move(obj);
    checkObjectValid();
    return *this;
}

/**
 * @brief 拷贝赋值操作符
 * @param s 另一个DAPySeries
 * @return 返回自身的引用
 */
DAPySeries& DAPySeries::operator=(const DAPySeries& s)
{
    if (this != &s) {
        DAPyObjectWrapper::operator=(s);  // 调用基类赋值
        checkObjectValid();
    }
    return *this;
}

/**
 * @brief 移动赋值操作符
 * @param s 右值DAPySeries
 * @return 返回自身的引用
 */
DAPySeries& DAPySeries::operator=(DAPySeries&& s)
{
    if (this != &s) {
        DAPyObjectWrapper::operator=(std::move(s));  // 调用基类移动赋值
        checkObjectValid();
    }
    return *this;
}

/**
 * @brief 赋值操作符，从DAPyObjectWrapper赋值
 * @param obj DAPyObjectWrapper对象
 * @return 返回自身的引用
 */
DAPySeries& DAPySeries::operator=(const DAPyObjectWrapper& obj)
{
    DAPyObjectWrapper::operator=(obj);
    checkObjectValid();
    return *this;
}

/**
 * @brief 移动赋值操作符，从DAPyObjectWrapper移动赋值
 * @param obj DAPyObjectWrapper对象右值
 * @return 返回自身的引用
 */
DAPySeries& DAPySeries::operator=(DAPyObjectWrapper&& obj)
{
    DAPyObjectWrapper::operator=(std::move(obj));
    checkObjectValid();
    return *this;
}

/**
 * @brief 通过位置索引获取元素
 * @param i 位置索引
 * @return 对应位置的pybind11::object
 */
pybind11::object DAPySeries::operator[](std::size_t i) const
{
    return iat(i);
}

/**
 * @brief 针对索引是字符串的场景
 * @param colName
 * @return
 */
pybind11::object DAPySeries::operator[](const QString& colName) const
{
    return object()[ pybind11::cast(colName) ];
}
/**
 * @brief Return the dtype object of the underlying data.
 *
 * 对于 pandas 扩展类型（如 StringDtype、Int64Dtype、ArrowDtype 等），
 * 由于它们不是 numpy.dtype 的实例，无法直接转换为 pybind11::dtype。
 * 此函数会检测扩展类型，并返回等效的 numpy dtype：
 * - StringDtype -> numpy.dtype('O') (object)
 * - 其他扩展类型 -> numpy.dtype('O')
 * @return
 */
pybind11::dtype DAPySeries::dtype() const
{
    try {
        pybind11::object dtype_obj = object().attr("dtype");
        pybind11::module np        = pybind11::module::import("numpy");
        if (pybind11::isinstance(dtype_obj, np.attr("dtype"))) {
            return dtype_obj.cast< pybind11::dtype >();
        }
        return np.attr("dtype")("O");
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return pybind11::none();
}

/**
 * @brief 获取 dtype 对象，支持 numpy dtype 和 pandas 扩展类型
 *
 * 此方法返回 DAPyDType 对象，可以正确处理 numpy dtype 和 pandas 扩展类型。
 * 对于扩展类型，可以通过 DAPyDType 的方法获取扩展类型信息。
 * @return
 */
DAPyDType DAPySeries::dtypeObject() const
{
    try {
        return DAPyDType::fromObject(object().attr("dtype"));
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return DAPyDType();
}

/**
 * @brief 返回 dtype 的字符串表示
 *
 * 直接对 series.dtype 调用 Python str()，返回如 "int64"、"float64"、
 * "datetime64[ns]"、"object"、"category" 等原始字符串。
 * 与 DAPyDType::name() 不同，此方法保留完整 dtype 表示（含时间单位等）。
 * @return dtype 字符串，失败返回空字符串
 */
QString DAPySeries::dtypeString() const
{
    try {
        return pybind11::str(object().attr("dtype")).cast< QString >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return QString();
}

/**
 * @brief 返回指定位置元素的字符串表示
 *
 * 通过 Python str() 转换 iat(i) 的值，适用于所有类型（数值、日期、字符串、
 * Timestamp、NaN 等）。当 value() 因类型不兼容返回空 QVariant 时，此方法
 * 仍能正确输出。
 * @param i 位置索引
 * @return 元素的字符串表示
 */
QString DAPySeries::valueAsString(std::size_t i) const
{
    try {
        return pybind11::str(iat(i)).cast< QString >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return QString();
}

/**
 * @brief Indicator whether Series/DataFrame is empty.
 * @return If Series/DataFrame is empty, return True, if not return False.
 */
bool DAPySeries::empty() const
{
    try {
        pybind11::bool_ obj = object().attr("empty");
        return obj.cast< bool >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return true;
}

/**
 * @brief Series.size
 *
 * Return the number of elements in the underlying data.
 * @return
 */
std::size_t DAPySeries::size() const
{
    try {
        pybind11::int_ obj = object().attr("size");
        return obj.cast< std::size_t >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return 0;
}

/**
 * @brief Series.name
 *
 * Return the name of the Series.
 *
 * The name of a Series becomes its index or column name if it is used to form a DataFrame. It is also used whenever
 * displaying the Series using the interpreter.
 * @return
 */
QString DAPySeries::name() const
{
    try {
        pybind11::str obj = object().attr("name");
        return obj.cast< QString >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return QString();
}

/**
 * @brief DASeries::iat Access a single value for a row/column pair by integer position.
 * @param i
 * @return
 */
pybind11::object DAPySeries::iat(std::size_t i) const
{
    pybind11::object obj_iat = object().attr("iat");
    return obj_iat[ pybind11::int_(i) ];
}

/**
 * @brief 设置指定位置的元素值
 * @param r 位置索引
 * @param v 要设置的值
 */
void DAPySeries::iat(size_t r, const pybind11::object& v)
{
    pybind11::object obj_iat     = object().attr("iat");
    obj_iat[ pybind11::int_(r) ] = v;
}

/**
 * @brief 获取指定位置的元素值
 * @param i 位置索引
 * @return 返回QVariant形式的值
 */
QVariant DAPySeries::value(size_t i) const
{
    try {
        return pybind11::cast< QVariant >(iat(i));
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return QVariant();
}

/**
 * @brief 设置指定位置的元素值
 * @param i 位置索引
 * @param v 要设置的QVariant值
 * @return 设置成功返回true
 */
bool DAPySeries::setValue(size_t i, const QVariant& v)
{
    try {
        iat(i, pybind11::cast(v));
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
        return false;
    }
    return true;
}

/**
 * @brief 判断是否为数值类型
 * @return 如果是数值类型返回true
 */
bool DAPySeries::isNumeric() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    return dtype_str.find("int") == 0 || dtype_str.find("float") == 0 || dtype_str.find("complex") == 0;
}

/**
 * @brief 判断是否为日期时间类型
 * @return 如果是日期时间类型返回true
 */
bool DAPySeries::isDateTime() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    return dtype_str.find("datetime") == 0;
}

/**
 * @brief 判断是否为字符串类型
 * @return 如果是字符串类型返回true
 */
bool DAPySeries::isString() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    // 支持 object, str, string, string[pyarrow] 等类型
    if (dtype_str == "object")
        return true;
    if (dtype_str == "str")
        return true;
    if (dtype_str.find("string") == 0)
        return true;
    return false;
}

/**
 * @brief 判断是否为分类类型
 * @return 如果是分类类型返回true
 */
bool DAPySeries::isCategorical() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    return dtype_str.find("category") != std::string::npos;
}

/**
 * @brief 获取Series的索引
 * @return 返回DAPyIndex对象
 */
DAPyIndex DAPySeries::index() const
{
    return DAPyIndex(object().attr("index"));
}

/**
 * @brief 获取索引的字符串列表
 * @return 返回索引的QStringList
 */
QStringList DAPySeries::indexAsStringList() const
{
    QStringList result;
    if (isNone()) {
        return result;
    }

    DAPyIndex index_obj    = index();
    std::size_t index_size = index_obj.size();
    result.reserve(static_cast< int >(index_size));

    for (std::size_t i = 0; i < index_size; ++i) {
        result.append(index_obj.value(i).toString());
    }

    return result;
}

/**
 * @brief 获取索引的浮点数向量
 * @return 返回索引的QVector<double>
 */
QVector< double > DAPySeries::indexAsDoubleVector() const
{
    QVector< double > result;
    if (isNone()) {
        return result;
    }

    DAPyIndex index_obj    = index();
    std::size_t index_size = index_obj.size();
    result.reserve(static_cast< int >(index_size));

    for (std::size_t i = 0; i < index_size; ++i) {
        bool isok;
        double value = index_obj.value(i).toDouble(&isok);
        if (isok) {
            result.append(value);
        } else {
            result.append(std::numeric_limits< double >::quiet_NaN());
        }
    }

    return result;
}

/**
 * @brief 获取索引的日期时间向量
 * @return 返回索引的QVector<QDateTime>
 */
QVector< QDateTime > DAPySeries::indexAsDateTimeVector() const
{
    QVector< QDateTime > result;
    if (isNone()) {
        return result;
    }

    // 如果索引是日期时间类型，直接转换
    if (isDateTime()) {
        DAPyIndex index_obj    = index();
        std::size_t index_size = index_obj.size();
        result.reserve(static_cast< int >(index_size));

        for (std::size_t i = 0; i < index_size; ++i) {
            QVariant var = index_obj.value(i);
            if (var.canConvert< QDateTime >()) {
                result.append(var.toDateTime());
            } else {
                result.append(QDateTime());
            }
        }
    }

    return result;
}

/**
 * @brief 类型转换
 * @param dt 目标dtype
 * @return 转换后的DAPySeries
 */
DAPySeries DAPySeries::astype(const pybind11::dtype& dt) const
{
    return DAPySeries(object().attr("astype")(dt));
}

/**
 * @brief 转换为日期时间类型
 * @return 转换后的DAPySeries
 */
DAPySeries DAPySeries::toDateTime() const
{
    if (isDateTime()) {
        return *this;
    }
    // 尝试转换为日期时间
    return DAPySeries(pybind11::module::import("pandas").attr("to_datetime")(object()));
}

/**
 * @brief 返回最大值的位置索引
 *
 * 两级策略：
 * 1. numpy.nanargmax(series.values) —— 直接返回位置索引，跳过 NaN，
 *    天然处理重复 index label 和各种 index 类型
 * 2. object 列回退：先 to_numeric(coerce)，如果全 NaN 再 to_datetime(coerce)，
 *    解决 object 列中 float/NaN 与 Timestamp 混合的 TypeError
 * 对空列、全 NaN 列、全非数值且非时间列，最终返回 -1。
 * @return 位置索引（>=0），失败返回 -1
 */
long DAPySeries::idxmaxPosition() const
{
    // 1) nanargmax：直接返回位置，跳过 NaN
    try {
        pybind11::object values = object().attr("values");
        pybind11::object result = pybind11::module::import("numpy").attr("nanargmax")(values);
        return result.cast< long >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    // 2) object 列回退：先尝试 to_numeric，全 NaN 则尝试 to_datetime
    try {
        auto pd = pybind11::module::import("pandas");
        auto np = pybind11::module::import("numpy");

        // 2a) to_numeric(coerce)
        pybind11::object numeric = pd.attr("to_numeric")(object(), pybind11::arg("errors") = "coerce");
        long nanCount            = numeric.attr("isna")().attr("sum")().cast< long >();
        long totalLen            = numeric.attr("size").cast< long >();
        if (nanCount < totalLen) {
            // 有非 NaN 值，取极值
            pybind11::object values = numeric.attr("values");
            pybind11::object result = np.attr("nanargmax")(values);
            return result.cast< long >();
        }

        // 2b) to_datetime(coerce)
        pybind11::object dt     = pd.attr("to_datetime")(object(), pybind11::arg("errors") = "coerce");
        pybind11::object values = dt.attr("values");
        pybind11::object result = np.attr("nanargmax")(values);
        return result.cast< long >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
        return -1;
    }
}

/**
 * @brief 返回最小值的位置索引
 *
 * 两级策略：
 * 1. numpy.nanargmin(series.values) —— 直接返回位置索引，跳过 NaN，
 *    天然处理重复 index label 和各种 index 类型
 * 2. object 列回退：先 to_numeric(coerce)，如果全 NaN 再 to_datetime(coerce)，
 *    解决 object 列中 float/NaN 与 Timestamp 混合的 TypeError
 * 对空列、全 NaN 列、全非数值且非时间列，最终返回 -1。
 * @return 位置索引（>=0），失败返回 -1
 */
long DAPySeries::idxminPosition() const
{
    // 1) nanargmin：直接返回位置，跳过 NaN
    try {
        pybind11::object values = object().attr("values");
        pybind11::object result = pybind11::module::import("numpy").attr("nanargmin")(values);
        return result.cast< long >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    // 2) object 列回退：先尝试 to_numeric，全 NaN 则尝试 to_datetime
    try {
        auto pd = pybind11::module::import("pandas");
        auto np = pybind11::module::import("numpy");

        // 2a) to_numeric(coerce)
        pybind11::object numeric = pd.attr("to_numeric")(object(), pybind11::arg("errors") = "coerce");
        long nanCount            = numeric.attr("isna")().attr("sum")().cast< long >();
        long totalLen            = numeric.attr("size").cast< long >();
        if (nanCount < totalLen) {
            // 有非 NaN 值，取极值
            pybind11::object values = numeric.attr("values");
            pybind11::object result = np.attr("nanargmin")(values);
            return result.cast< long >();
        }

        // 2b) to_datetime(coerce)
        pybind11::object dt     = pd.attr("to_datetime")(object(), pybind11::arg("errors") = "coerce");
        pybind11::object values = dt.attr("values");
        pybind11::object result = np.attr("nanargmin")(values);
        return result.cast< long >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
        return -1;
    }
}

/**
 * @brief 生成描述性统计信息
 *
 * 调用 pandas Series.describe()，返回一个新的 Series，其 index 为统计项名称
 * （数值列：count/mean/std/min/25%/50%/75%/max，对象列：count/unique/top/freq）。
 * @return 统计信息 Series，失败返回空 Series
 */
DAPySeries DAPySeries::describe() const
{
    try {
        return DAPySeries(object().attr("describe")());
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return DAPySeries();
}

/**
 * @brief pandas.Series.isnull()
 *
 * 返回布尔掩码 Series，标记每个元素是否为缺失值（NaN/None/NaT）。
 * 对布尔掩码调用 sum() 可统计缺失值数量。
 * @return 布尔掩码 Series，失败返回空 Series
 */
DAPySeries DAPySeries::isNull() const
{
    try {
        return DAPySeries(object().attr("isnull")());
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return DAPySeries();
}

/**
 * @brief pandas.Series.sum()
 *
 * 返回 Series 元素之和。对布尔掩码 Series（如 isNull() 的返回值）调用，
 * 可统计 True 的数量（即缺失值计数）。
 * @return 元素之和，失败返回 0
 */
qint64 DAPySeries::sum() const
{
    try {
        pybind11::object result = object().attr("sum")();
        return result.cast< qint64 >();
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return 0;
}

/**
 * @brief 判断对象是否为pandas.Series实例
 * @param obj 要判断的对象
 * @return 如果是Series返回true
 */
bool DAPySeries::isSeries(const pybind11::object& obj)
{
    return DAPyModulePandas::getInstance().isInstanceSeries(obj);
}

/**
 * @brief 检查对象是否为有效的Series，无效则置为None
 */
void DAPySeries::checkObjectValid()
{
    if (!isSeries(object())) {
        object() = pybind11::none();
        daCritical << QObject::tr(
            "DAPySeries: the Python object type is not pandas.Series");  // cn:DAPySeries：Python 对象类型不是 pandas.Series
    }
}

/**
 * @brief 打印为字符串
 * @param maxele 最大显示元素数
 * @return 字符串表示
 */
QString DAPySeries::toString(std::size_t maxele) const
{
    QString str;
    if (isNone()) {
        return QString();
    }
    str += name();
    str += " | ";
    str += pybind11::cast< QString >(dtype());
    str += " [";
    std::size_t s = size();
    if (s > maxele) {
        const std::size_t hc = maxele / 2;
        for (std::size_t i = 0; i < hc; ++i) {
            str += value(i).toString();
            str += ",";
        }
        str += "......";
        for (std::size_t i = s - hc; i < s; ++i) {
            str += value(i).toString();
            if (i != s - 1) {
                str += ",";
            }
        }
    } else {
        for (std::size_t i = 0; i < s; ++i) {
            str += value(i).toString();
            if (i != s - 1) {
                str += ",";
            }
        }
    }
    str += "]";
    return str;
}

/**
 * @brief series 转换为vector< double >
 * @param ser
 * @return
 */
std::vector< double > toVectorDouble(const DAPySeries& ser)
{
    try {
        DAPyDType dt(ser.dtype());
        if (dt.isNone()) {
            return std::vector< double >();
        }
        std::vector< double > res;
        res.reserve(ser.size());
        ser.castTo< double >(std::back_insert_iterator< std::vector< double > >(res));
        return res;
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return std::vector< double >();
    }
}

/**
 * @brief series 转换为QVector< double >
 * @param ser
 * @return
 */
QVector< double > toQVectorDouble(const DAPySeries& ser)
{
    try {
        DAPyDType dt(ser.dtype());
        if (dt.isNone()) {
            return QVector< double >();
        }
        QVector< double > res;
        res.reserve(static_cast< int >(ser.size()));
        ser.castTo< double >(std::back_insert_iterator< QVector< double > >(res));
        return res;
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return QVector< double >();
    }
}

}  // end of DA

/**
 * @brief operator <<
 * @param dbg
 * @param ser
 * @return
 */
QDebug operator<<(QDebug dbg, const DA::DAPySeries& ser)
{
    QDebugStateSaver saver(dbg);
    dbg.noquote() << ser.toString();
    return (dbg);
}
