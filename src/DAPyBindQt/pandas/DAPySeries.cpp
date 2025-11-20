#include "DAPySeries.h"
#include "DAPybind11QtTypeCast.h"
#include "DAPyModulePandas.h"
#include <QDateTime>
#include <iterator>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAPySeries
//===================================================
DAPySeries::DAPySeries(const DAPySeries& s) : DAPyObjectWrapper(s)
{
    if (!s.isNone()) {
        checkObjectValid();
    }
}

DAPySeries::DAPySeries(DAPySeries&& s) : DAPyObjectWrapper(std::move(s))
{
}

DAPySeries::DAPySeries(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
    if (!obj.is_none()) {
        checkObjectValid();
    }
}

DAPySeries::DAPySeries(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
    if (!obj.is_none()) {
        checkObjectValid();
    }
}

DAPySeries::~DAPySeries()
{
}

DAPySeries& DAPySeries::operator=(const pybind11::object& obj)
{
    _object = obj;
    checkObjectValid();
    return *this;
}

DAPySeries& DAPySeries::operator=(pybind11::object&& obj)
{
    _object = std::move(obj);
    checkObjectValid();
    return *this;
}

DAPySeries& DAPySeries::operator=(const DAPySeries& s)
{
    if (this != &s) {
        DAPyObjectWrapper::operator=(s);  // 调用基类赋值
        checkObjectValid();
    }
    return *this;
}

DAPySeries& DAPySeries::operator=(DAPySeries&& s)
{
    if (this != &s) {
        DAPyObjectWrapper::operator=(std::move(s));  // 调用基类移动赋值
        checkObjectValid();
    }
    return *this;
}

DAPySeries& DAPySeries::operator=(const DAPyObjectWrapper& obj)
{
    DAPyObjectWrapper::operator=(obj);
    checkObjectValid();
    return *this;
}

DAPySeries& DAPySeries::operator=(DAPyObjectWrapper&& obj)
{
    DAPyObjectWrapper::operator=(std::move(obj));
    checkObjectValid();
    return *this;
}

QVariant DAPySeries::operator[](std::size_t i) const
{
    return iat(i);
}

/**
 * @brief Return the dtype object of the underlying data.
 * @return
 */
pybind11::dtype DAPySeries::dtype() const
{
    try {
        return object().attr("dtype");
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return pybind11::none();
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
        return DA::PY::toString(obj);
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
QVariant DAPySeries::iat(std::size_t i) const
{
    try {
        pybind11::object obj_iat = object().attr("iat");
        pybind11::object obj_v   = obj_iat[ pybind11::int_(i) ];
        return DA::PY::toVariant(obj_v);
    } catch (const std::exception& e) {
        qCritical().noquote() << e.what();
    }
    return QVariant();
}

bool DAPySeries::isNumeric() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    return dtype_str.find("int") == 0 || dtype_str.find("float") == 0 || dtype_str.find("complex") == 0;
}

bool DAPySeries::isDateTime() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    return dtype_str.find("datetime") == 0;
}

bool DAPySeries::isString() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    return dtype_str == "object";
}

bool DAPySeries::isCategorical() const
{
    if (isNone()) {
        return false;
    }
    std::string dtype_str = pybind11::str(dtype()).cast< std::string >();
    return dtype_str.find("category") != std::string::npos;
}

pybind11::object DAPySeries::index() const
{
    return object().attr("index");
}

QStringList DAPySeries::indexAsStringList() const
{
    QStringList result;
    if (isNone()) {
        return result;
    }

    pybind11::object index_obj = index();
    std::size_t index_size     = pybind11::len(index_obj);
    result.reserve(static_cast< int >(index_size));

    for (std::size_t i = 0; i < index_size; ++i) {
        pybind11::object item = index_obj[ pybind11::int_(i) ];
        result.append(PY::toString(item));
    }

    return result;
}

QVector< double > DAPySeries::indexAsDoubleVector() const
{
    QVector< double > result;
    if (isNone()) {
        return result;
    }

    pybind11::object index_obj = index();
    std::size_t index_size     = pybind11::len(index_obj);
    result.reserve(static_cast< int >(index_size));

    for (std::size_t i = 0; i < index_size; ++i) {
        pybind11::object item = index_obj[ pybind11::int_(i) ];
        try {
            double value = item.cast< double >();
            result.append(value);
        } catch (...) {
            result.append(std::numeric_limits< double >::quiet_NaN());
        }
    }

    return result;
}

QVector< QDateTime > DAPySeries::indexAsDateTimeVector() const
{
    QVector< QDateTime > result;
    if (isNone()) {
        return result;
    }

    // 如果索引是日期时间类型，直接转换
    if (isDateTime()) {
        pybind11::object index_obj = index();
        std::size_t index_size     = pybind11::len(index_obj);
        result.reserve(static_cast< int >(index_size));

        for (std::size_t i = 0; i < index_size; ++i) {
            pybind11::object item = index_obj[ pybind11::int_(i) ];
            QVariant var          = DA::PY::toVariant(item);
            if (var.canConvert< QDateTime >()) {
                result.append(var.toDateTime());
            } else {
                result.append(QDateTime());
            }
        }
    }

    return result;
}

DAPySeries DAPySeries::astype(const pybind11::dtype& dt) const
{
    return DAPySeries(object().attr("astype")(dt));
}

DAPySeries DAPySeries::toDateTime() const
{
    if (isDateTime()) {
        return *this;
    }
    // 尝试转换为日期时间
    return DAPySeries(pybind11::module::import("pandas").attr("to_datetime")(object()));
}

bool DAPySeries::isSeries(const pybind11::object& obj)
{
    return DAPyModulePandas::getInstance().isInstanceSeries(obj);
}

void DAPySeries::checkObjectValid()
{
    if (!isSeries(object())) {
        object() = pybind11::none();
        qCritical() << QObject::tr("DAPySeries  get python object type is not pandas.Series");
    }
}

QString DAPySeries::toString(std::size_t maxele) const
{
    QString str;
    if (isNone()) {
        return QString();
    }
    str += name();
    str += " | ";
    str += DA::PY::toString(dtype());
    str += " [";
    std::size_t s = size();
    if (s > maxele) {
        const std::size_t hc = maxele / 2;
        for (std::size_t i = 0; i < hc; ++i) {
            str += iat(i).toString();
            str += ",";
        }
        str += "......";
        for (std::size_t i = s - hc; i < s; ++i) {
            str += iat(i).toString();
            if (i != s - 1) {
                str += ",";
            }
        }
    } else {
        for (std::size_t i = 0; i < s; ++i) {
            str += iat(i).toString();
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
        if (!dt.isNumeral()) {
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
        if (!dt.isNumeral()) {
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
