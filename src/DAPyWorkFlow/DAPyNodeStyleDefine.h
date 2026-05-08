#ifndef DAPYNODESTYLEDEFINE_H
#define DAPYNODESTYLEDEFINE_H
// std
#include <type_traits>
#include "DAPyWorkFlowAPI.h"
#include "DAEnumStringUtils.hpp"
#include "DAGraphicsViewGlobal.h"
#include "DAGraphicsViewEnumStringUtils.h"
#include "DAQtEnumTypeStringUtils.h"
#include <QString>

/**
 * @file DAPyNodeStyleDefine.h
 * @brief Python工作流节点渲染样式枚举定义
 *
 * 本文件定义了节点渲染增强方案所需的所有枚举类型及字符串转换函数，
 * 包括节点体形状、端口形状、名称位置、图标位置、端口方位及渲染模板类型。
 *
 * 所有枚举值与现有代码保持向后兼容（如 stringToRenderTemplate("rect") 返回 NodeStyleTemplate）。
 */

namespace DA
{

// =================================================================================
//                          BodyShape — 节点体形状
// =================================================================================
/**
 * @brief 节点体形状枚举
 */
enum class BodyShape
{
    RoundedRect = 0,  ///< 圆角矩形
    Ellipse     = 1   ///< 椭圆形
};

// =================================================================================
//                          PortShape — 端口形状
// =================================================================================
/**
 * @brief 端口形状枚举
 */
enum class PortShape
{
    Rect    = 0,  ///< 矩形端口
    Circle  = 1,  ///< 圆形端口
    Diamond = 2   ///< 菱形端口
};

// =================================================================================
//                          NamePosition — 节点名称位置
// =================================================================================
/**
 * @brief 节点名称位置枚举
 */
enum class NamePosition
{
    Inside = 0,  ///< 名称在节点内部
    Below  = 1   ///< 名称在节点下方
};

// =================================================================================
//                          IconPosition — 图标位置
// =================================================================================
/**
 * @brief 图标位置枚举
 */
enum class IconPosition
{
    LeftOfText = 0,  ///< 图标在文本左侧
    AboveText  = 1   ///< 图标在文本上方
};

// =================================================================================
//                          PortSide — 端口方位（别名）
// =================================================================================
/**
 * @brief 端口方位类型别名
 *
 * 复用 DAGraphicsViewGlobal.h 中定义的 AspectDirection 枚举值，
 * 避免重复定义。此处仅提供类型别名以增强语义。
 *
 * @see AspectDirection
 */
using PortSide = AspectDirection;

// PortSide 是 AspectDirection 的类型别名，其 DAEnumTraits 已在
// DAGraphicsViewEnumStringUtils.h 中声明（DA_ENUM_STRING_DECLARE_EXPORT(DAGRAPHICSVIEW_API, DA::AspectDirection)）
// 不重复声明，避免 C2766 重复模板特化错误
// enumToString(PortSide::West) 和 stringToEnum<PortSide>() 自动复用 AspectDirection 的转换

// =================================================================================
//                          BodyIconType — 节点体图标类型
// =================================================================================
/**
 * @brief 节点体图标类型枚举
 */
enum class BodyIconType
{
    None   = 0,  ///< 无图标
    Pixmap = 1,  ///< 位图图标（QIcon/QPixmap）
    Svg    = 2   ///< SVG矢量图标
};

// =================================================================================
//                          RenderTemplate — 渲染模板类型（简化版）
// =================================================================================
/**
 * @brief 渲染模板类型枚举（简化版）
 *
 * 仅保留两种模板类型：
 * - NodeStyleTemplate: 使用 DAPyNodeStyleDefine 中的样式配置进行绘制
 * - WidgetTemplate: 嵌入 Qt Widget
 *
 * 向后兼容：stringToRenderTemplate("rect") 和 stringToRenderTemplate("svg")
 * 均返回 NodeStyleTemplate。
 */
enum class RenderTemplate
{
    NodeStyleTemplate = 0,  ///< 节点样式模板（使用 BodyShape/PortShape 等配置绘制）
    WidgetTemplate    = 1   ///< 嵌入Widget模板
};

// uint qHash(const DA::RenderTemplate& key, uint seed) noexcept
//{
//     using underlying_type = std::underlying_type_t< DA::RenderTemplate >;
//     return ::qHash(static_cast< underlying_type >(key), seed);
// }

}  // namespace DA
// 实现位于DAPyWorkFlowEnumStringUtils.cpp
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::NamePosition)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::PortShape)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::IconPosition)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::BodyIconType)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::BodyShape)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::RenderTemplate)

#endif  // DAPYNODESTYLEDEFINE_H
