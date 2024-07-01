#ifndef DASTRINGUTIL_H
#define DASTRINGUTIL_H
#include "DAUtilsAPI.h"
#include <QSet>
#include <QString>
#ifndef DA_STRING_CAST_D_H
#define DA_STRING_CAST_D_H(type, value) DAUTILS_API type fromQString(const QString& str, type defaultValue = value);
#endif
#ifndef DA_STRING_CAST_H
#define DA_STRING_CAST_H(type) DAUTILS_API type fromQString(const QString& str, const type& defaultValue = type());
#endif
/**
 *@file 此文件包含了字符串相关的工具方法
 */
namespace DA
{
// 生成一个唯一的字符串，如果出现了重复字符串，将会在这个字符串后面加上_1,这个函数常用于生成一个唯一的名字
DAUTILS_API QString makeUniqueString(const QSet< QString >& stringSet, const QString& str, const QString& split = "_");
// 字符串转换为值
// type fromQString
/**
DA_STRING_CAST_D_H(short, 0)
DA_STRING_CAST_D_H(unsigned short, 0)
DA_STRING_CAST_D_H(int, 0)
DA_STRING_CAST_D_H(unsigned int, 0)
DA_STRING_CAST_D_H(long, 0)
DA_STRING_CAST_D_H(unsigned long, 0)
DA_STRING_CAST_D_H(long long, 0)
DA_STRING_CAST_D_H(unsigned long long, 0)
DA_STRING_CAST_D_H(float, 0.0f)
DA_STRING_CAST_D_H(double, 0.0)
DA_STRING_CAST_H(std::string)
DA_STRING_CAST_H(std::u16string)
DA_STRING_CAST_H(std::u32string)
DA_STRING_CAST_H(std::wstring)
**/
}  // end of DA
#endif  // DASTRINGUTIL_H
