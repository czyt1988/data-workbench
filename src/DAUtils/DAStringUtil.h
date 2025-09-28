#ifndef DASTRINGUTIL_H
#define DASTRINGUTIL_H
#include "DAUtilsAPI.h"
#include <QSet>
#include <QString>
#include <string>
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

/**
 * @brief QString转换为系统编码的wstring
 *
 * 此函数主要针对windows操作系统，把QString转换为系统编码的std::wstring
 */
DAUTILS_API std::wstring qstringToSystemWString(const QString& qstr);
DAUTILS_API std::wstring stringToSystemWString(const std::string& str);
DAUTILS_API int getSystemCodePage();
#ifdef Q_OS_WIN
DAUTILS_API std::string convertToUtf8(const std::string& str, unsigned int codePage);
#endif
// 字符串转换为值
// type fromQString(const QString& str,type defaultval = 0)
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

/**
 * @brief 将容器中的元素转换为字符串，并使用指定的分隔符连接。
 *
 *
 * 这个模板函数接受一个迭代器范围（begin到end），遍历这个范围内的所有元素，
 *
 * 将它们转换为字符串，并使用指定的分隔符连接起来。转换过程使用了QString的arg方法。
 *
 * @tparam
 * Ite 迭代器类型，应该是支持*操作符和++操作符的迭代器。
 * @param begin 迭代器范围的开始。
 * @param
 * end 迭代器范围的结束。
 * @param split 用于连接元素的分隔符，默认为";"。
 * @return 返回一个QString，包含所有元素，元素之间用分隔符连接。
 * 
 * @code
 * std::vector<int> vec = {1, 2, 3, 4, 5};
 * QString str = vectorToString(vec.begin(),
 * vec.end(), ",");
 * // str 现在是 "1,2,3,4,5"
 * @endcode
 */
template< typename Ite >
QString vectorToString(Ite begin, Ite end, const QString& split = ";")
{
	QString res;
	for (Ite i = begin; i != end; ++i) {
		if (!res.isEmpty()) {
			res.append(split);
		}
		res.append(QString("%1").arg(*i));
	}
	return res;
}

/**
 * @brief 将字符串按照指定的分隔符分割，并将分割后的字符串转换为指定类型的元素存储在vector中。
 *

 * * 这个模板函数接受一个QString，一个函数指针（或可调用对象）用于将字符串转换为指定类型T，

 * * 以及一个分隔符。函数将字符串按照分隔符分割成多个子字符串，然后对每个子字符串调用转换函数，

 * * 将结果存储在std::vector<T>中。
 *
 * @tparam T 目标元素的类型。
 * @tparam FpStrCastTo 一个可调用对象，接受一个QString参数,和一个T的默认参数，并返回一个T类型的值。例如double
 * fromQString(const QString& s,double defaultValue = 0)
 * @param str 要分割和转换的源字符串。
 * @param fp
 * 用于将QString转换为T类型的函数指针或可调用对象。
 * @param split 用于分割字符串的分隔符，默认为";"。
 * @return 返回一个std::vector<T>，包含所有转换后的元素。
 *
 * @code
 * int stringToInt(const QString& str) { return str.toInt(); }
 *
 * QString str = "1;2;3;4;5";
 *
 * std::vector<int> vec = stringToVector(str, stringToInt, ";");
 * // vec 现在是 {1, 2, 3, 4, 5}
 * @endcode
 */
template< typename T, typename FpStrCastTo >
std::vector< T > stringToVector(const QString& str, FpStrCastTo fp, const QString& split = ";")
{
	std::vector< T > res;
	const QStringList splitlist = str.split(split);
	for (const auto& s : splitlist) {
		res.emplace_back(fp(s));
	}
	return res;
}

}  // end of DA
#endif  // DASTRINGUTIL_H
