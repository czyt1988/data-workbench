#ifndef DAGUIENUMSTRINGUTILS_H
#define DAGUIENUMSTRINGUTILS_H
#include "DAEnumStringUtils.hpp"
#include "DAGuiAPI.h"
#include "DAColorTheme.h"
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

// DAUtils.DA::DAColorTheme::ColorThemeStyle
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, DA::DAColorTheme::ColorThemeStyle)

// ------------------------------------------
// qwt
// ------------------------------------------
#include "qwt_plot.h"
#include "qwt_text.h"
#include "qwt_axis.h"
#include "qwt_scale_div.h"
#include "qwt_scale_draw.h"
#include "qwt_date.h"
// QWT.QwtDate::Week0Type
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtPlotItem::RttiValues)
// QWT.QwtPlot::LegendPosition
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtPlot::LegendPosition)
// QWT.QwtText::TextFormat
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtText::TextFormat)
// QWT.QwtAxis::Position
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtAxis::Position)
// QWT.QwtScaleDiv::TickType
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtScaleDiv::TickType)
// QWT.QwtScaleDraw::Alignment
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtScaleDraw::Alignment)
// QWT.QwtDate::Week0Type
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtDate::Week0Type)
// QWT.QwtDate::Week0Type
DA_ENUM_STRING_DECLARE_EXPORT(DAGUI_API, QwtDate::IntervalType)
#endif  // DAGUIENUMSTRINGUTILS_H
