#ifndef DAQTENUMTYPESTRINGUTILS_H
#define DAQTENUMTYPESTRINGUTILS_H
#include <QString>
#include <QHash>
#include <Qt>
#include <QFont>
#include <QPainter>
#include <QPixmap>
#include "DAUtilsAPI.h"
#include "DAEnumStringUtils.hpp"

/**
 * @file 枚举字符串转换类
 *
 *@code
 * #include "DAQtEnumTypeStringUtils.h"
 * #include <QDebug>
 * //DA_ENUM_STRING_INSENSITIVE必须在DA命名空间下使用
 *
 * DA_ENUM_STRING_INSENSITIVE(Qt::AlignmentFlag,
 *                  Qt::AlignLeft,
 *                  { Qt::AlignLeft, "left" },
 *                  { Qt::AlignHCenter, "hcenter" },
 *                  { Qt::AlignRight, "right" },
 *                  { Qt::AlignTop, "top" },
 *                  { Qt::AlignBottom, "bottom" },
 *                  { Qt::AlignVCenter, "vcenter" });
 * // 为 Qt::Alignment 定义映射（而非 Qt::AlignmentFlag）
 * DA_ENUM_STRING_INSENSITIVE(Qt::Alignment,
 *                  Qt::AlignLeft,
 *                  { Qt::AlignLeft, "left" },
 *                  { Qt::AlignHCenter, "hcenter" },
 *                  { Qt::AlignRight, "right" },
 *                  { Qt::AlignTop, "top" },
 *                  { Qt::AlignBottom, "bottom" },
 *                  { Qt::AlignVCenter, "vcenter" });
 *
 * void test() {
 *  // 测试 Qt::AlignmentFlag
 *  Qt::AlignmentFlag flag = DA::stringToEnum<Qt::AlignmentFlag>("hcenter");
 *  qDebug() << enumToString(flag); // 输出 "hcenter"
 *
 *  // 测试 Qt::Alignment (QFlags)
 *  Qt::Alignment align = DA::stringToEnum<Qt::Alignment>("left");
 *  qDebug() << enumToString(align); // 输出 "left"
 * }
 * @endcode
 */
// ------------------------------------------
//           Qt::Alignment
// ------------------------------------------
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::AlignmentFlag)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::Alignment)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::PenStyle)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::BrushStyle)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::AspectRatioMode)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::TransformationMode)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::TimeSpec)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, QFont::Weight)
#endif  // DAQTENUMTYPESTRINGUTILS_H
