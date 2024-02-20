#include "DAPyDType.h"
#include "DAPyModuleNumpy.h"
#include "DAPybind11QtTypeCast.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyDType
//===================================================
DAPyDType::DAPyDType(const DAPyDType& s) : DAPyObjectWrapper(s)
{
    checkObjectValid();
}

DAPyDType::DAPyDType(DAPyDType&& s) : DAPyObjectWrapper(s)
{
    checkObjectValid();
}

DAPyDType::DAPyDType(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
    checkObjectValid();
}

DAPyDType::DAPyDType(pybind11::object&& obj) : DAPyObjectWrapper(obj)
{
    checkObjectValid();
}

DAPyDType::DAPyDType(const pybind11::dtype& obj)
{
    object() = obj;
}

DAPyDType::DAPyDType(pybind11::dtype&& obj)
{
    object() = std::move(obj);
}

DAPyDType::DAPyDType(const QString& dtypename)
{
    try {
        object() = pybind11::dtype(dtypename.toStdString());
    } catch (const std::exception& e) {
        object() = pybind11::none();
        dealException(e);
    }
}

DAPyDType::~DAPyDType()
{
}

bool DAPyDType::isDtypeObj(const pybind11::object& obj)
{
    return DAPyModuleNumpy::getInstance().isInstanceDtype(obj);
}

DAPyDType& DAPyDType::operator=(const pybind11::dtype& obj)
{
    object() = obj;
    return *this;
}

DAPyDType& DAPyDType::operator=(const pybind11::object& obj)
{
    if (isDtypeObj(obj)) {
        object() = obj;
    }
    return *this;
}

DAPyDType& DAPyDType::operator=(const DAPyDType& obj)
{
    object() = obj.object();
    return *this;
}

DAPyDType& DAPyDType::operator=(const DAPyObjectWrapper& obj)
{
    if (isDtypeObj(obj.object())) {
        object() = obj.object();
    }
    return *this;
}

/**
 * @brief 判断两个dtype是否是一个类型
 * @param other
 * @return
 */
bool DAPyDType::operator==(const DAPyDType& other) const
{
    return char_() == other.char_();
}

bool DAPyDType::operator!=(const DAPyDType& other) const
{
    return char_() != other.char_();
}

pybind11::object DAPyDType::type(const QVariant& v) const
{
    try {
        object().attr("type")(DA::PY::toPyObject(v));
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return pybind11::none();
}

QString DAPyDType::name() const
{
    try {
        pybind11::str s = object().attr("name");
        return DA::PY::toString(s);
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return QString();
}

char DAPyDType::kind() const
{
    try {
        return object().attr("kind").cast< char >();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return 0;
}

char DAPyDType::char_() const
{
    try {
        return object().attr("char").cast< char >();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return 0;
}

int DAPyDType::num() const
{
    try {
        return object().attr("num").cast< int >();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return -1;
}

bool DAPyDType::isInt() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'i';
}

bool DAPyDType::isUInt() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'u';
}

bool DAPyDType::isFloat() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'f';
}

bool DAPyDType::isTimedelta() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'm';
}

bool DAPyDType::isDatetime() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'M';
}

bool DAPyDType::isComplex() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'c';
}

bool DAPyDType::isBool() const
{
    if (isNone()) {
        return false;
    }
    return kind() == '?';
}

bool DAPyDType::isStr() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'U';
}

/**
 * @brief 是否为数字
 * @note 注意，bool和复数不算数字
 * @return
 */
bool DAPyDType::isNumeral() const
{
    return isInt() || isUInt() || isFloat();
}

/**
 * @brief 获取所有的dtype类型名称
 * @return
 */
QStringList DAPyDType::dtypeNames()
{
    static QStringList s_dtypeNames({ "bool",
                                      "int8",
                                      "int16",
                                      "int32",
                                      "int64",
                                      "uint8",
                                      "uint16",
                                      "uint32",
                                      "uint64",
                                      "float16",
                                      "float32",
                                      "float64",
                                      "complex64",
                                      "complex128",
                                      "<U0",
                                      "datetime64",
                                      "timedelta64",
                                      "object" });
    return s_dtypeNames;
}

void DAPyDType::checkObjectValid()
{
    if (!isDtypeObj(object())) {
        object() = pybind11::none();
    }
}

/**
 * @brief DAPyDType的qdebug输出
 * @param dbg
 * @param d
 * @return
 */
QDebug operator<<(QDebug dbg, const DA::DAPyDType& d)
{
    QDebugStateSaver saver(dbg);
    dbg.noquote() << d.name() << "(kind=" << d.kind() << ",char=" << d.char_() << ",num=" << d.num() << ")";
    return (dbg);
}
