#include "DAPyDType.h"
#include "DAPyModuleNumpy.h"
#include "DAPybind11QtCaster.hpp"
#include <QCoreApplication>
#include <QMetaType>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyDType
//===================================================
/**
 * @brief 拷贝构造
 * @param s 另一个DAPyDType
 */
DAPyDType::DAPyDType(const DAPyDType& s) : DAPyObjectWrapper(s)
{
    checkObjectValid();
}

/**
 * @brief 移动构造
 * @param s 右值DAPyDType
 */
DAPyDType::DAPyDType(DAPyDType&& s) : DAPyObjectWrapper(std::move(s))
{
    checkObjectValid();
}

/**
 * @brief 构造，从pybind11::object构造
 * @param obj pybind11对象
 */
DAPyDType::DAPyDType(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
    checkObjectValid();
}

/**
 * @brief 构造，从pybind11::object右值构造
 * @param obj pybind11对象右值
 */
DAPyDType::DAPyDType(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
    checkObjectValid();
}

/**
 * @brief 构造，从pybind11::dtype构造
 * @param obj pybind11::dtype对象
 */
DAPyDType::DAPyDType(const pybind11::dtype& obj) : DAPyObjectWrapper()
{
    object() = obj;
}

/**
 * @brief 构造，从pybind11::dtype右值构造
 * @param obj pybind11::dtype对象右值
 */
DAPyDType::DAPyDType(pybind11::dtype&& obj) : DAPyObjectWrapper(std::move(obj))
{
}

/**
 * @brief 构造，通过类型名构造
 * @param dtypename dtype类型名
 */
DAPyDType::DAPyDType(const QString& dtypename) : DAPyObjectWrapper()
{
    try {
        object() = pybind11::dtype(dtypename.toStdString());
    } catch (const std::exception& e) {
        object() = pybind11::none();
        dealException(e);
    }
}

/**
 * @brief 析构
 */
DAPyDType::~DAPyDType()
{
}

/**
 * @brief 判断对象是否为numpy.dtype实例
 * @param obj 要判断的对象
 * @return 如果是dtype返回true
 */
bool DAPyDType::isDtypeObj(const pybind11::object& obj)
{
    return DAPyModuleNumpy::getInstance().isInstanceDtype(obj);
}

/**
 * @brief 赋值操作符
 * @param obj pybind11::dtype对象
 * @return 返回自身的引用
 */
DAPyDType& DAPyDType::operator=(const pybind11::dtype& obj)
{
    object() = obj;
    return *this;
}

/**
 * @brief 赋值操作符
 * @param obj pybind11对象
 * @return 返回自身的引用
 */
DAPyDType& DAPyDType::operator=(const pybind11::object& obj)
{
    object() = obj;
    checkObjectValid();
    return *this;
}

/**
 * @brief 拷贝赋值操作符
 * @param obj 另一个DAPyDType
 * @return 返回自身的引用
 */
DAPyDType& DAPyDType::operator=(const DAPyDType& obj)
{
    object() = obj.object();
    return *this;
}

/**
 * @brief 赋值操作符，从DAPyObjectWrapper赋值
 * @param obj DAPyObjectWrapper对象
 * @return 返回自身的引用
 */
DAPyDType& DAPyDType::operator=(const DAPyObjectWrapper& obj)
{
    object() = obj.object();
    checkObjectValid();
    return *this;
}

/**
 * @brief 判断两个dtype是否是一个类型
 * @param other
 * @return
 */
bool DAPyDType::operator==(const DAPyDType& other) const
{
    if (isNone() || other.isNone()) {
        return isNone() && other.isNone();
    }
    if (isExtensionDtype() || other.isExtensionDtype()) {
        return name() == other.name();
    }
    return char_() == other.char_();
}

/**
 * @brief 判断两个dtype是否不是同一类型
 * @param other 另一个DAPyDType
 * @return 如果不是同一类型返回true
 */
bool DAPyDType::operator!=(const DAPyDType& other) const
{
    return !(*this == other);
}

/**
 * @brief 获取dtype对应的Python类型对象
 * @param v 用于获取类型的值
 * @return 返回pybind11::object
 */
pybind11::object DAPyDType::type(const QVariant& v) const
{
    try {
        return attr("type")(pybind11::cast(v));
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return pybind11::none();
}

/**
 * @brief 获取dtype的名称
 * @return 返回dtype名称字符串
 */
QString DAPyDType::name() const
{
    try {
        pybind11::str s = attr("name");
        return s.cast< QString >();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return QString();
}

/**
 * @brief 获取dtype的种类字符
 * @return 返回kind字符，如'i'表示整数
 */
char DAPyDType::kind() const
{
    try {
        if (isExtensionDtype()) {
            QString n = name();
            if (isNullableInt())
                return 'i';
            if (isNullableUInt())
                return 'u';
            if (isNullableBool())
                return '?';
            if (isNullableString())
                return 'U';
            if (isCategorical())
                return 'O';
            if (isDatetimeTZ())
                return 'M';
            if (isPeriod())
                return 'M';
            if (isInterval())
                return 'O';
            if (isArrow())
                return 'O';
            return 'O';
        }
        return attr("kind").cast< char >();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return 0;
}

/**
 * @brief 获取dtype的字符码
 * @return 返回字符码
 */
char DAPyDType::char_() const
{
    try {
        if (isExtensionDtype()) {
            return kind();
        }
        return attr("char").cast< char >();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return 0;
}

/**
 * @brief 获取dtype的数字编号
 * @return 返回数字编号
 */
int DAPyDType::num() const
{
    try {
        if (isExtensionDtype()) {
            return -1;
        }
        return attr("num").cast< int >();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return -1;
}

/**
 * @brief 映射为QMetaType类型id
 *
 * 映射规则与QVariant caster（DAPybind11QtCaster.hpp的handle_numpy_object）的
 * 实际转换结果对齐：int8/uint8转QChar、timedelta/complex转QString等，
 * 使schema声明的列类型与批量取数产出的QVariant类型一致。
 * object等运行时才能确定元素类型的dtype返回UnknownType
 * @return QMetaType类型id，无法确定返回QMetaType::UnknownType
 */
int DAPyDType::toMetaType() const
{
    if (isNone()) {
        return QMetaType::UnknownType;
    }
    switch (char_()) {
    case '?':  // bool（含BooleanDtype）
        return QMetaType::Bool;
    case 'b':  // int8
    case 'B':  // uint8
        return QMetaType::QChar;
    case 'h':  // int16
    case 'l':  // int32
    case 'i':  // int（部分平台）
        return QMetaType::Int;
    case 'q':  // int64（含nullable Int64Dtype）
        return QMetaType::LongLong;
    case 'H':  // uint16
    case 'L':  // uint32
    case 'I':  // uint（部分平台）
        return QMetaType::UInt;
    case 'Q':  // uint64
        return QMetaType::ULongLong;
    case 'e':  // float16
    case 'f':  // float32
        return QMetaType::Float;
    case 'd':  // float64
        return QMetaType::Double;
    case 'M':  // datetime64（含DatetimeTZDtype）
        return QMetaType::QDateTime;
    case 'U':  // unicode字符串（含StringDtype）
        return QMetaType::QString;
    case 'S':  // bytes
        return QMetaType::QByteArray;
    case 'm':  // timedelta64，caster转为QString
    case 'F':  // complex64，caster转为QString
    case 'D':  // complex128，caster转为QString
        return QMetaType::QString;
    default:
        // 'O'(object)、'V'(void)等运行时确定，无法静态声明
        break;
    }
    return QMetaType::UnknownType;
}

/**
 * @brief 判断是否为整数类型
 * @return 如果是整数类型返回true
 */
bool DAPyDType::isInt() const
{
    if (isNone()) {
        return false;
    }
    if (isExtensionDtype()) {
        return isNullableInt();
    }
    return kind() == 'i';
}

/**
 * @brief 判断是否为无符号整数类型
 * @return 如果是无符号整数类型返回true
 */
bool DAPyDType::isUInt() const
{
    if (isNone()) {
        return false;
    }
    if (isExtensionDtype()) {
        return isNullableUInt();
    }
    return kind() == 'u';
}

/**
 * @brief 判断是否为浮点类型
 * @return 如果是浮点类型返回true
 */
bool DAPyDType::isFloat() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'f';
}

/**
 * @brief 判断是否为时间差类型
 * @return 如果是时间差类型返回true
 */
bool DAPyDType::isTimedelta() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'm';
}

/**
 * @brief 判断是否为日期时间类型
 * @return 如果是日期时间类型返回true
 */
bool DAPyDType::isDatetime() const
{
    if (isNone()) {
        return false;
    }
    if (isExtensionDtype()) {
        return isDatetimeTZ() || isPeriod();
    }
    return kind() == 'M';
}

/**
 * @brief 判断是否为复数类型
 * @return 如果是复数类型返回true
 */
bool DAPyDType::isComplex() const
{
    if (isNone()) {
        return false;
    }
    return kind() == 'c';
}

/**
 * @brief 判断是否为布尔类型
 * @return 如果是布尔类型返回true
 */
bool DAPyDType::isBool() const
{
    if (isNone()) {
        return false;
    }
    if (isExtensionDtype()) {
        return isNullableBool();
    }
    return kind() == '?';
}

/**
 * @brief 判断是否为字符串类型
 * @return 如果是字符串类型返回true
 */
bool DAPyDType::isStr() const
{
    if (isNone()) {
        return false;
    }
    if (isExtensionDtype()) {
        return isNullableString();
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
    return isInt() || isUInt() || isFloat() || isNullableInt() || isNullableUInt();
}

/**
 * @brief 是否为 pandas 扩展类型
 * @return
 */
bool DAPyDType::isExtensionDtype() const
{
    if (isNone()) {
        return false;
    }
    try {
        pybind11::module np = pybind11::module::import("numpy");
        if (pybind11::isinstance(object(), np.attr("dtype"))) {
            return false;
        }
        return true;
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为可空整数类型 (Int64, Int32, Int16, Int8)
 * @return
 */
bool DAPyDType::isNullableInt() const
{
    if (isNone()) {
        return false;
    }
    try {
        QString n = name();
        return n == "Int64" || n == "Int32" || n == "Int16" || n == "Int8";
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为可空无符号整数类型 (UInt64, UInt32, UInt16, UInt8)
 * @return
 */
bool DAPyDType::isNullableUInt() const
{
    if (isNone()) {
        return false;
    }
    try {
        QString n = name();
        return n == "UInt64" || n == "UInt32" || n == "UInt16" || n == "UInt8";
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为可空布尔类型
 * @return
 */
bool DAPyDType::isNullableBool() const
{
    if (isNone()) {
        return false;
    }
    try {
        return name() == "boolean";
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为可空字符串类型
 * @return
 */
bool DAPyDType::isNullableString() const
{
    if (isNone()) {
        return false;
    }
    try {
        QString n = name();
        return n == "string" || n.startsWith("string[");
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为分类类型
 * @return
 */
bool DAPyDType::isCategorical() const
{
    if (isNone()) {
        return false;
    }
    try {
        QString n = name();
        return n == "category";
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为带时区的日期时间类型
 * @return
 */
bool DAPyDType::isDatetimeTZ() const
{
    if (isNone()) {
        return false;
    }
    try {
        QString n = name();
        return n.startsWith("datetime64[") && n.contains("tz=");
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为周期类型
 * @return
 */
bool DAPyDType::isPeriod() const
{
    if (isNone()) {
        return false;
    }
    try {
        QString n = name();
        return n.startsWith("period[");
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为区间类型
 * @return
 */
bool DAPyDType::isInterval() const
{
    if (isNone()) {
        return false;
    }
    try {
        QString n = name();
        return n.startsWith("interval[");
    } catch (...) {
    }
    return false;
}

/**
 * @brief 是否为 Arrow 类型
 * @return
 */
bool DAPyDType::isArrow() const
{
    if (isNone()) {
        return false;
    }
    try {
        pybind11::module pd          = pybind11::module::import("pandas");
        pybind11::object arrow_dtype = pd.attr("ArrowDtype");
        return pybind11::isinstance(object(), arrow_dtype);
    } catch (...) {
    }
    return false;
}

/**
 * @brief 转换为 numpy dtype
 *
 * 对于扩展类型，返回等效的 numpy dtype
 * @return
 */
pybind11::dtype DAPyDType::toNumpyDtype() const
{
    if (isNone()) {
        return pybind11::dtype();
    }
    try {
        pybind11::module np = pybind11::module::import("numpy");
        if (pybind11::isinstance(object(), np.attr("dtype"))) {
            return object().cast< pybind11::dtype >();
        }
        return np.attr("dtype")("O");
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return pybind11::dtype();
}

/**
 * @brief 从任意 dtype 对象创建 DAPyDType
 *
 * 支持创建 numpy.dtype 和 pandas 扩展类型
 * @param obj
 * @return
 */
DAPyDType DAPyDType::fromObject(const pybind11::object& obj)
{
    DAPyDType dt;
    dt.object() = obj;
    dt.checkObjectValid();
    return dt;
}

/**
 * @brief 获取所有的 numpy dtype 类型名称
 * @return
 */
QStringList DAPyDType::dtypeNames()
{
    static QStringList s_dtypeNames(
        { "bool",
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
          "object" }
    );
    return s_dtypeNames;
}

/**
 * @brief 获取所有的 pandas 扩展类型名称
 * @return
 */
QStringList DAPyDType::extensionDtypeNames()
{
    static QStringList s_extDtypeNames(
        { "Int64", "Int32", "Int16", "Int8", "UInt64", "UInt32", "UInt16", "UInt8", "boolean", "string", "category" }
    );
    return s_extDtypeNames;
}

/**
 * @brief 获取显示名称
 *
 * 对于扩展类型，返回扩展类型名称；对于 numpy dtype，返回标准名称
 * @return
 */
QString DAPyDType::displayName() const
{
    if (isNone()) {
        return QCoreApplication::translate("DAPyDType", "None");  // cn:无
    }
    QString n = name();
    if (isExtensionDtype()) {
        if (isNullableString()) {
            return QCoreApplication::translate("DAPyDType", "string (nullable)");  // cn:string（可空）
        }
        if (isNullableBool()) {
            return QCoreApplication::translate("DAPyDType", "boolean (nullable)");  // cn:boolean（可空）
        }
        if (isNullableInt()) {
            return n + " " + QCoreApplication::translate("DAPyDType", "(nullable)");  // cn:（可空）
        }
        if (isNullableUInt()) {
            return n + " " + QCoreApplication::translate("DAPyDType", "(nullable)");  // cn:（可空）
        }
        if (isCategorical()) {
            return QCoreApplication::translate("DAPyDType", "category");  // cn:category
        }
        if (isDatetimeTZ()) {
            return QCoreApplication::translate("DAPyDType", "datetime (with timezone)");  // cn:datetime（带时区）
        }
        if (isPeriod()) {
            return QCoreApplication::translate("DAPyDType", "period");  // cn:period
        }
        if (isInterval()) {
            return QCoreApplication::translate("DAPyDType", "interval");  // cn:interval
        }
        if (isArrow()) {
            return QCoreApplication::translate("DAPyDType", "Arrow") + "[" + n + "]";  // cn:Arrow
        }
        return n;
    }
    return n;
}

/**
 * @brief 检测对象是否为有效的 dtype 或扩展类型
 */
void DAPyDType::checkObjectValid()
{
    if (object().is_none()) {
        return;
    }
    pybind11::module np = pybind11::module::import("numpy");
    if (pybind11::isinstance(object(), np.attr("dtype"))) {
        return;
    }
    try {
        if (pybind11::hasattr(object(), "name")) {
            return;
        }
    } catch (...) {
    }
    object() = pybind11::none();
}

/**
 * @brief 检测扩展类型
 */
void DAPyDType::detectExtensionType()
{
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
    if (d.isExtensionDtype()) {
        dbg.noquote() << d.displayName() << " (pandas extension type)";
    } else {
        dbg.noquote() << d.name() << "(kind=" << d.kind() << ",char=" << d.char_() << ",num=" << d.num() << ")";
    }
    return (dbg);
}
