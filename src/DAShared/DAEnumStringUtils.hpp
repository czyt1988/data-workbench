#ifndef DAENUMSTRINGUTILS_H
#define DAENUMSTRINGUTILS_H
#include <QString>
#include <QHash>
/**
 * @file 枚举字符串转换类
 *
 *@code
 * #include "DAQtEnumTypeStringUtils.h"
 * #include <QDebug>
 * //DA_ENUM_STRING_INSENSITIVE必须在DA命名空间下使用
 * DA_ENUM_STRING_DECLARE(Qt::AlignmentFlag)
 * DA_ENUM_STRING_DECLARE(Qt::Alignment)
 *
 * DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::AlignmentFlag,
 *                  Qt::AlignLeft,
 *                  { Qt::AlignLeft, "left" },
 *                  { Qt::AlignHCenter, "hcenter" },
 *                  { Qt::AlignRight, "right" },
 *                  { Qt::AlignTop, "top" },
 *                  { Qt::AlignBottom, "bottom" },
 *                  { Qt::AlignVCenter, "vcenter" });
 * // 为 Qt::Alignment 定义映射（而非 Qt::AlignmentFlag）
 * DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::Alignment,
 *                  Qt::AlignLeft,
 *                  { Qt::AlignLeft, "left" },
 *                  { Qt::AlignHCenter, "hcenter" },
 *                  { Qt::AlignRight, "right" },
 *                  { Qt::AlignTop, "top" },
 *                  { Qt::AlignBottom, "bottom" },
 *                  { Qt::AlignVCenter, "vcenter" });
 *
 * void test() {

 *  Qt::AlignmentFlag flag = DA::stringToEnum<Qt::AlignmentFlag>("hcenter");//如果带第二个参数，能告诉编译器具体枚举类型，可以不用告知编译器具体类型
 *  qDebug() << enumToString(flag); // 输出 "hcenter"
 *
 *  Qt::Alignment align = DA::stringToEnum<Qt::Alignment>("left");
 *  qDebug() << enumToString(align); // 输出 "left"
 * }
 * @endcode
 *
 * 如果你写一个库，你的库定义了很多枚举，你想把这些枚举进行转换，可以如下实现
 *
 * DAQtEnumTypeStringUtils.h:
 * @code
 * #include "DAUtilsAPI.h" //导出符号DAUTILS_API
 * DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::PenStyle)
 * @endcode
 *
 * DAQtEnumTypeStringUtils.cpp:
 * @code
 * #include "DAQtEnumTypeStringUtils.h"
 * DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::PenStyle,
                                  Qt::SolidLine,
                                  { Qt::NoPen, "none" },
                                  { Qt::SolidLine, "solid" },
                                  { Qt::DashLine, "dash" },
                                  { Qt::DotLine, "dot" },
                                  { Qt::DashDotLine, "dashdot" },
                                  { Qt::DashDotDotLine, "dashdotdot" });
 * @endcode
 *
 * 通过上面两步，你就能导出DA::stringToEnum<Qt::PenStyle>和DA::enumToString<Qt::PenStyle>两个转换函数
 */

namespace DA
{

// =================================================================================
//                          DAEnumTraits 模板声明
// =================================================================================
/**
 * @brief 枚举类型特性模板，用于定义枚举与字符串的映射关系
 * @tparam T 枚举类型
 * @details 用户需通过 DA_ENUM_STRING_DECLARE以及DA_ENUM_STRING_SENSITIVE_DEFINE/DA_ENUM_STRING_INSENSITIVE_DEFINE宏特化此模板
 *
 * @par 示例：
 * @code
 * enum class Color { Red, Green };
 * DA_ENUM_STRING_DECLARE(Color)
 * DA_ENUM_SENSITIVE(Color, Color::Red, {Color::Red, "Red"}, {Color::Green, "Green"});
 * @endcode
 */
template< typename T >
struct DAEnumTraits;

// =================================================================================
//                         通用转换函数（模板实现）
// =================================================================================
/**
 * @brief 将枚举值转换为对应的字符串
 * @tparam EnumType 枚举类型（自动推导）
 * @param value 枚举值
 * @return 对应的字符串，若未找到映射则返回默认值字符串
 *
 * @par 示例：
 * @code
 * MyEnum e = MyEnum::Value;
 * QString str = enumToString(e); // 返回 "Value"
 * @endcode
 */
template< typename EnumType >
QString enumToString(EnumType value)
{
	const auto& map = DAEnumTraits< EnumType >::enumToStringMap;
	auto it         = map.find(value);
	return (it != map.end()) ? *it : DAEnumTraits< EnumType >::defaultValueStr;
}

/**
 * @brief 将字符串转换为对应的枚举值
 * @tparam EnumType 枚举类型（需显式指定）
 * @param s 输入字符串
 * @param defaultValue 转换失败时返回的默认值（可选）
 * @return 对应的枚举值，若未找到映射则返回 defaultValue
 *
 * @par 示例：
 * @code
 * MyEnum e = stringToEnum<MyEnum>("Value");
 * MyEnum e2 = stringToEnum<MyEnum>("invalid", MyEnum::Default);
 * @endcode
 */
template< typename EnumType >
EnumType stringToEnum(const QString& s, EnumType defaultValue = DAEnumTraits< EnumType >::defaultValue)
{
	const auto& map = DAEnumTraits< EnumType >::stringToEnumMap;
	QString key     = DAEnumTraits< EnumType >::caseSensitive ? s : s.toLower();
	auto it         = map.find(key);
	return (it != map.end()) ? *it : defaultValue;
}

// 辅助类型定义
template< typename EnumType >
using DAEnumEntry = std::pair< EnumType, const char* >;

}  // End DA
// =================================================================================
//                         宏定义
// =================================================================================

// ======================= 声明宏 =======================
/**
 * @def 不带导出符号的枚举定义，这个适用于不需要对枚举转换函数导出的情景
 */
#ifndef DA_ENUM_STRING_DECLARE
#define DA_ENUM_STRING_DECLARE(EnumType)                                                                               \
	template<>                                                                                                         \
	struct DA::DAEnumTraits< EnumType >                                                                                \
	{                                                                                                                  \
	public:                                                                                                            \
		static const QHash< EnumType, QString > enumToStringMap;                                                       \
		static const QHash< QString, EnumType > stringToEnumMap;                                                       \
		static const bool caseSensitive;                                                                               \
		static const EnumType defaultValue;                                                                            \
		static const QString defaultValueStr;                                                                          \
	};
#endif

/**
 * @def 带导出符号的枚举定义，这个适用于需要对枚举转换函数导出的情景,例如库
 */
#ifndef DA_ENUM_STRING_DECLARE_EXPORT
#define DA_ENUM_STRING_DECLARE_EXPORT(EXPORT_API, EnumType)                                                            \
	template<>                                                                                                         \
	struct EXPORT_API DA::DAEnumTraits< EnumType >                                                                     \
	{                                                                                                                  \
	public:                                                                                                            \
		static const QHash< EnumType, QString > enumToStringMap;                                                       \
		static const QHash< QString, EnumType > stringToEnumMap;                                                       \
		static const bool caseSensitive;                                                                               \
		static const EnumType defaultValue;                                                                            \
		static const QString defaultValueStr;                                                                          \
	};
#endif
/**
 * @def DA_ENUM_SENSITIVE(EnumType, DefaultValue, ...)
 * @brief 定义大小写敏感的枚举字符串映射
 * @param EnumType 枚举类型（需包含命名空间，如 MyNamespace::Color）
 * @param DefaultValue 默认枚举值（转换失败时返回此值）
 * @param ... 枚举项列表，格式为 {枚举值, "字符串"}，逗号分隔
 *
 * @par 示例：
 * @code
 * enum class Direction { North, South };
 * DA_ENUM_SENSITIVE(Direction, Direction::North,
 *     {Direction::North, "North"},
 *     {Direction::South, "South"}
 * );
 * // stringToEnum<Direction>("north") 返回 Direction::North（严格匹配大小写）
 * @endcode
 *
 * 如果你在库里面使用，你需要进行导出：
 * @code
 * // LibA/EnumA.hpp
 * #pragma once
 * #include "DAEnumStringUtils.hpp"
 *
 * namespace LibA {
 *  enum class Status { Ok, Error };
 * }
 * // 定义枚举映射
 * DA_ENUM_INSENSITIVE(LibA::Status, LibA::Status::Ok,
 *  { LibA::Status::Ok,    "ok" },
 *  { LibA::Status::Error, "error" }
 *
 * // 显式实例化模板特化并导出
 * template class LIBA_EXPORT DAEnumTraits<LibA::Status>;
 * template LIBA_EXPORT QString enumToString<LibA::Status>(LibA::Status);
 * template LIBA_EXPORT LibA::Status stringToEnum<LibA::Status>(const QString&, LibA::Status);
 * @endcode
 */
#ifndef DA_ENUM_STRING_SENSITIVE_DEFINE
#define DA_ENUM_STRING_SENSITIVE_DEFINE(EnumType, DefaultValue, ...)                                                   \
	const QHash< EnumType, QString > DA::DAEnumTraits< EnumType >::enumToStringMap = { __VA_ARGS__ };                  \
	const QHash< QString, EnumType > DA::DAEnumTraits< EnumType >::stringToEnumMap = []() {                            \
		QHash< QString, EnumType > tmp;                                                                                \
		const std::initializer_list< DAEnumEntry< EnumType > >& entries = { __VA_ARGS__ };                             \
		for (const auto& pair : entries) {                                                                             \
			tmp.insert(pair.second, pair.first);                                                                       \
		}                                                                                                              \
		return tmp;                                                                                                    \
	}();                                                                                                               \
    const bool DA::DAEnumTraits< EnumType >::caseSensitive      = false;                                               \
    const EnumType DA::DAEnumTraits< EnumType >::defaultValue   = DefaultValue;                                        \
    const QString DA::DAEnumTraits< EnumType >::defaultValueStr = DA::DAEnumTraits< EnumType >::enumToStringMap.value( \
        DefaultValue)
#endif

// ---------------------------------------------------------------------------------
/**
 * @def DA_ENUM_INSENSITIVE(EnumType, DefaultValue, ...)
 * @brief 定义大小写不敏感的枚举字符串映射（统一转为小写匹配）
 * @param EnumType 枚举类型（需包含命名空间）
 * @param DefaultValue 默认枚举值
 * @param ... 枚举项列表，格式为 {枚举值, "字符串"}
 *
 * @par 示例：
 * @code
 * enum class Status { Ok, Error };
 * DA_ENUM_INSENSITIVE(Status, Status::Ok,
 *     {Status::Ok, "OK"},
 *     {Status::Error, "ERROR"}
 * );
 * // stringToEnum<Status>("ok") 和 stringToEnum<Status>("OK") 均返回 Status::Ok
 * @endcode
 *
 * 如果你在库里面使用，你需要进行导出：
 * @code
 * // LibA/EnumA.hpp
 * #pragma once
 * #include "DAEnumStringUtils.hpp"
 *
 * namespace LibA {
 *  enum class Status { Ok, Error };
 *
 * // 定义枚举映射
 * DA_ENUM_INSENSITIVE(Status, Status::Ok,
 *  { Status::Ok,    "ok" },
 *  { Status::Error, "error" }
 * );
 * }
 *
 * // 显式实例化模板特化并导出
 * template class LIBA_EXPORT DAEnumTraits<LibA::Status>;
 * template LIBA_EXPORT QString enumToString<LibA::Status>(LibA::Status);
 * template LIBA_EXPORT LibA::Status stringToEnum<LibA::Status>(const QString&, LibA::Status);
 * @endcode
 */
#ifndef DA_ENUM_STRING_INSENSITIVE_DEFINE
#define DA_ENUM_STRING_INSENSITIVE_DEFINE(EnumType, DefaultValue, ...)                                                 \
	const QHash< EnumType, QString > DA::DAEnumTraits< EnumType >::enumToStringMap = { __VA_ARGS__ };                  \
	const QHash< QString, EnumType > DA::DAEnumTraits< EnumType >::stringToEnumMap = []() {                            \
		QHash< QString, EnumType > tmp;                                                                                \
		const std::initializer_list< DAEnumEntry< EnumType > >& entries = { __VA_ARGS__ };                             \
		for (const auto& pair : entries) {                                                                             \
			tmp.insert(QString(pair.second).toLower(), pair.first);                                                    \
		}                                                                                                              \
		return tmp;                                                                                                    \
	}();                                                                                                               \
    const bool DA::DAEnumTraits< EnumType >::caseSensitive      = false;                                               \
    const EnumType DA::DAEnumTraits< EnumType >::defaultValue   = DefaultValue;                                        \
    const QString DA::DAEnumTraits< EnumType >::defaultValueStr = DA::DAEnumTraits< EnumType >::enumToStringMap.value( \
        DefaultValue)
#endif

#endif  // DAENUMSTRINGUTILS_H
