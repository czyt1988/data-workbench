#include "DAGraphicsViewEnumStringUtils.h"
// std
#include <type_traits>
namespace DA
{

/**
 * @brief 计算DAAspectDirection枚举的哈希值
 * @param key 枚举值
 * @param seed 哈希种子
 * @return 哈希值
 */
uint qHash(const DA::DAAspectDirection& key, uint seed) noexcept
{
    using underlying_type = std::underlying_type_t< DA::DAAspectDirection >;
    return ::qHash(static_cast< underlying_type >(key), seed);
}
}

// ================================== DA::DAGraphicsLinkItem::EndPointType ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAGraphicsLinkItem::EndPointType,
                                  DA::DAGraphicsLinkItem::EndPointNone,
                                  { DA::DAGraphicsLinkItem::EndPointNone, "none" },
                                  { DA::DAGraphicsLinkItem::EndPointTriangType, "triang" });

// ================================== DA::DAGraphicsLinkItem::LinkLineStyle ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAGraphicsLinkItem::LinkLineStyle,
                                  DA::DAGraphicsLinkItem::LinkLineKnuckle,
                                  { DA::DAGraphicsLinkItem::LinkLineBezier, "bezier" },
                                  { DA::DAGraphicsLinkItem::LinkLineStraight, "straight" },
                                  { DA::DAGraphicsLinkItem::LinkLineKnuckle, "knuckle" });

// ================================== DA::DAShapeKeyPoint::KeyPoint ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAShapeKeyPoint::KeyPoint,
                                  DA::DAShapeKeyPoint::None,
                                  { DA::DAShapeKeyPoint::TopLeft, "topLeft" },
                                  { DA::DAShapeKeyPoint::TopCenter, "topCenter" },
                                  { DA::DAShapeKeyPoint::TopRight, "topRight" },
                                  { DA::DAShapeKeyPoint::CenterLeft, "centerLeft" },
                                  { DA::DAShapeKeyPoint::Center, "center" },
                                  { DA::DAShapeKeyPoint::CenterRight, "centerRight" },
                                  { DA::DAShapeKeyPoint::BottomLeft, "bottomLeft" },
                                  { DA::DAShapeKeyPoint::BottomCenter, "bottomCenter" },
                                  { DA::DAShapeKeyPoint::BottomRight, "bottomRight" },
                                  { DA::DAShapeKeyPoint::None, "none" });

// ================================== DA::AspectDirection ==================================

DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAAspectDirection,
                                  DA::DAAspectDirection::East,
                                  { DA::DAAspectDirection::East, "east" },
                                  { DA::DAAspectDirection::South, "south" },
                                  { DA::DAAspectDirection::West, "west" },
                                  { DA::DAAspectDirection::North, "north" });
