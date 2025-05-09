#include "DAGraphicsViewEnumStringUtils.h"

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
// 定义 qHash 函数
uint qHash(DA::AspectDirection key, uint seed)
{
    return ::qHash(static_cast< int >(key), seed);
}
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::AspectDirection,
                                  DA::AspectDirection::East,
                                  { DA::AspectDirection::East, "east" },
                                  { DA::AspectDirection::South, "south" },
                                  { DA::AspectDirection::West, "west" },
                                  { DA::AspectDirection::North, "north" });
