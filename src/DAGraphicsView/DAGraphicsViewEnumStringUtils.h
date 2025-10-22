#ifndef DAGRAPHICSVIEWENUMSTRINGUTILS_H
#define DAGRAPHICSVIEWENUMSTRINGUTILS_H
#include "DAGraphicsViewGlobal.h"
#include "DAEnumStringUtils.hpp"
#include "DAGraphicsLinkItem.h"
#include "DAShapeKeyPoint.h"
#include "DAShapeKeyPoint.h"
/**
 * @file 枚举字符串转换类
 *
 * 本文件提供了一组工具，用于在枚举类型和字符串之间进行相互转换。通过定义的@sa enumToString 和@sa stringToEnum
 * 函数，可以轻松地将枚举值转换为字符串，或将字符串转换回枚举值。
 *
 * 示例用法：
 * 假设已定义如下枚举：
 * @code
 * class XX {
 * public:
 *     enum Type {
 *         // 枚举值...
 *     };
 * };
 *
 * // 导出枚举转换功能
 * DA_ENUM_STRING_DECLARE_EXPORT(XX_API, XX::Type)
 * @endcode
 *
 * 在其他地方可以这样使用转换功能：
 * @code
 * // 枚举转字符串
 * QString str = enumToString(XX::TypeValue);
 *
 * // 字符串转枚举
 * XX::Type val1 = stringToEnum(str, XX::TypeValue);  // 指定默认值
 * XX::Type val2 = stringToEnum<XX::Type>(str);       // 使用模板参数指定枚举类型
 * @endcode
 *
 * 通过这种方式，可以方便地在枚举和字符串之间进行转换，适用于配置文件解析、网络数据传输、用户输入处理等多种场景。
 */

// ------------------------------------------
// DA::DAGraphicsLinkItem::EndPointType
// ------------------------------------------
DA_ENUM_STRING_DECLARE_EXPORT(DAGRAPHICSVIEW_API, DA::DAGraphicsLinkItem::EndPointType)

// ------------------------------------------
// DA::DAShapeKeyPoint::KeyPoint
// ------------------------------------------
DA_ENUM_STRING_DECLARE_EXPORT(DAGRAPHICSVIEW_API, DA::DAShapeKeyPoint::KeyPoint)

// ------------------------------------------
// DA::DAGraphicsLinkItem::LinkLineStyle
// ------------------------------------------
DA_ENUM_STRING_DECLARE_EXPORT(DAGRAPHICSVIEW_API, DA::DAGraphicsLinkItem::LinkLineStyle)

// ------------------------------------------
// DA::AspectDirection
// ------------------------------------------
DA_ENUM_STRING_DECLARE_EXPORT(DAGRAPHICSVIEW_API, DA::AspectDirection)

#endif  // DAGRAPHICSVIEWENUMSTRINGUTILS_H
