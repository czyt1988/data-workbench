#ifndef DASTRINGUTIL_H
#define DASTRINGUTIL_H
#include "DAUtilsAPI.h"
#include <QSet>
#include <QString>
/**
 *@file 此文件包含了字符串相关的工具方法
 */
namespace DA
{
//生成一个唯一的字符串，如果出现了重复字符串，将会在这个字符串后面加上_1,这个函数常用于生成一个唯一的名字
DAUTILS_API QString makeUniqueString(const QSet< QString >& stringSet, const QString& str, const QString& split = "_");
}  // end of DA
#endif  // DASTRINGUTIL_H
