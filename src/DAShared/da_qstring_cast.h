#ifndef DA_QSTRING_CAST_H
#define DA_QSTRING_CAST_H
/**
 * @file 这个文件提供了通用的toQString和fromQString方法
 */
#include <cmath>
#include <QString>
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <type_traits>
#include <string>
#ifndef DA_QSTRINGCAST_TOQSTRING_NUMBER
#define DA_QSTRINGCAST_TOQSTRING_NUMBER(type)                                                                          \
    template<>                                                                                                         \
    inline QString toQString< type >(const type& value)                                                                \
    {                                                                                                                  \
        return QString::number(value);                                                                                 \
    }
#endif

#ifndef DA_QSTRINGCAST_FROMQSTRING_NUMBER
#define DA_QSTRINGCAST_FROMQSTRING_NUMBER(type, castFunName)                                                           \
    template<>                                                                                                         \
    inline bool fromQString< type >(const QString& str, type& result)                                                  \
    {                                                                                                                  \
        bool ok;                                                                                                       \
        auto r = str.castFunName(&ok);                                                                                 \
        if (ok) {                                                                                                      \
            result = r;                                                                                                \
        }                                                                                                              \
        return ok;                                                                                                     \
    }
#endif

#ifndef DA_QSTRINGCAST_FROMQSTRING_DIRECT
#define DA_QSTRINGCAST_FROMQSTRING_DIRECT(type, castFunName)                                                           \
    template<>                                                                                                         \
    inline bool fromQString< type >(const QString& str, type& result)                                                  \
    {                                                                                                                  \
        result = str.castFunName();                                                                                    \
        return true;                                                                                                   \
    }
#endif
namespace DA
{
/**
 * @brief 精度计算，可以通过此函数确认浮点数转换时所需要的精度
 * 由于此函数使用在本文件的各种模板函数中，因此必须定义为内联
 * @param value 浮点数值
 * @return
 */
inline int calculatePrecision(double value);

// 将任何类型转换为QString
template< typename T >
/**
 * @brief toQString
 *
 * @code
 * template<>
 * QString toQString< int >(const int& value)
 * {
 *     return QString::number(value);
 * }
 * @endcode
 * @param value
 * @return
 */
inline QString toQString(const T& value)
{
    static_assert(true, "You must provide a specialization of toQString for your type.");
    return QString();  // 这条语句实际上不会被执行，因为静态断言会失败
}
// 用户为特定类型提供的特化示例
DA_QSTRINGCAST_TOQSTRING_NUMBER(int)
DA_QSTRINGCAST_TOQSTRING_NUMBER(unsigned int)
DA_QSTRINGCAST_TOQSTRING_NUMBER(long)
DA_QSTRINGCAST_TOQSTRING_NUMBER(unsigned long)
DA_QSTRINGCAST_TOQSTRING_NUMBER(long long)
DA_QSTRINGCAST_TOQSTRING_NUMBER(unsigned long long)

/**
 * @brief 特化double
 * @param value
 * @return
 */
template<>
inline QString toQString< double >(const double& value)
{
    // 计算浮点数需要的精度
    int p = calculatePrecision(value);
    return QString::number(value, 10, p);
}

/**
 * @brief 特化float
 * @param value
 * @return
 */
template<>
inline QString toQString< float >(const float& value)
{
    // 计算浮点数需要的精度
    int p = calculatePrecision(value);
    return QString::number(value, 10, p);
}

/**
 * @brief 将QString转换为类型T，通过引用返回转换结果，并返回一个bool值表示转换是否成功
 * @param str
 * @param result
 * @return
 */
template< typename T >
inline bool fromQString(const QString& str, T& result)
{
    static_assert(true, "You must provide a specialization of fromQString for your type.");
    return false;  // 这条语句实际上不会被执行，因为静态断言会失败
}

DA_QSTRINGCAST_FROMQSTRING_NUMBER(short, toShort)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(unsigned short, toUShort)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(int, toInt)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(unsigned int, toUInt)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(long, toLong)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(unsigned long, toULong)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(long long, toLongLong)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(unsigned long long, toULongLong)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(float, toFloat)
DA_QSTRINGCAST_FROMQSTRING_NUMBER(double, toDouble)
DA_QSTRINGCAST_FROMQSTRING_DIRECT(std::string, toStdString)
DA_QSTRINGCAST_FROMQSTRING_DIRECT(std::u16string, toStdU16String)
DA_QSTRINGCAST_FROMQSTRING_DIRECT(std::u32string, toStdU32String)
DA_QSTRINGCAST_FROMQSTRING_DIRECT(std::wstring, toStdWString)

inline int calculatePrecision(double value)
{
    if (value == 0.0) {
        return 1;  // 避免除以零的情况，返回默认精度
    }

    // 计算浮点数的绝对值和其对数值
    double absValue   = std::fabs(value);
    double log10Value = std::log10(absValue);

    // 根据浮点数的对数值来确定精度
    int precision = static_cast< int >(-std::floor(log10Value)) + 3;  // 加3是为了保留两位非零有效数字
    if (precision < 0) {
        precision = 0;  // 精度不能是负数
    } else if (precision > 15) {
        precision = 15;  // 设置一个精度的上限，避免过高精度
    }

    return precision;
}
}
#endif  // DA_QSTRING_CAST_H
