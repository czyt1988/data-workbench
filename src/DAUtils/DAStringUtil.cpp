#include "DAStringUtil.h"
#pragma warning(disable : 4819)

#ifndef DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP
#define DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(type, qstringCastFun)                                                 \
    type fromQString(const QString& str, type defaultValue)                                                            \
    {                                                                                                                  \
        bool ok(false);                                                                                                \
        auto v = str.qstringCastFun(&ok);                                                                              \
        return (ok ? v : defaultValue);                                                                                \
    }
#endif
#ifndef DA_STRING_CAST_QSTRINGINNER_CPP
#define DA_STRING_CAST_QSTRINGINNER_CPP(type, qstringCastFun)                                                          \
    type fromQString(const QString& str, const type& defaultValue)                                                     \
    {                                                                                                                  \
        Q_UNUSED(defaultValue);                                                                                        \
        return str.qstringCastFun();                                                                                   \
    }
#endif
namespace DA
{
/**
 * @brief 生成一个唯一的字符串
 *
 * 如果str在stringSet出现过，将会在这个字符串后面加上_1，并继续判断是否在stringSet出现过出现过，直到没有为止
 *
 * 这个函数常用于生成一个唯一的名字
 * @param stringSet 字符串数组
 * @param str 待检测的字符串
 * @param split 分隔符，分隔符后面将加入数字
 * @return 返回一个不会出现在stringSet的字符串
 */
QString makeUniqueString(const QSet< QString >& stringSet, const QString& str, const QString& split)
{
    if (!stringSet.contains(str)) {
        return str;
    }
    QString n = str;
    int index = -1;
    do {
        index = n.lastIndexOf(split);
        if (index <= 0) {  // 等于0也要包含
            n = (n + split + "1");
        } else if (index != 0) {
            if (index == n.size() - 1) {
                // 以split结尾
                n = n + "1";
            } else {
                // 说明有{split}xx,但要确认这个xx是数字
                QString suf = n.mid(index + 1);
                bool isnum  = false;
                int num     = suf.toInt(&isnum);
                if (isnum) {
                    // 说明是_num结尾
                    n = (n.mid(0, index) + split + QString::number(++num));
                } else {
                    n = (n + split + "1");
                }
            }
        }
    } while (stringSet.contains(n));
    return n;
}

/**
 * @brief 字符串转换为值
 * @param str 字符串
 * @param defaultValue 默认参数，如果转换失败，将返回默认参数
 * @return 转换的结果
 */

/**
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(short, toShort)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(unsigned short, toUShort)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(int, toInt)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(unsigned int, toUInt)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(long, toLong)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(unsigned long, toULong)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(long long, toLongLong)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(unsigned long long, toULongLong)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(float, toFloat)
DA_STRING_CAST_QSTRINGINNER_WITHBOOL_CPP(double, toDouble)
DA_STRING_CAST_QSTRINGINNER_CPP(std::string, toStdString)
DA_STRING_CAST_QSTRINGINNER_CPP(std::u16string, toStdU16String)
DA_STRING_CAST_QSTRINGINNER_CPP(std::u32string, toStdU32String)
DA_STRING_CAST_QSTRINGINNER_CPP(std::wstring, toStdWString)
**/

}
