#include "DAPyWorkFlowEnumStringUtils.h"
#include "DAPyNodeStyleDefine.h"
#include "DAPyNodeStyle.h"

// ================================== DA::DAPyNodeState ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeState,
                                  DA::Idle,
                                  { DA::Idle, "idle" },
                                  { DA::Waiting, "waiting" },
                                  { DA::Running, "running" },
                                  { DA::Success, "success" },
                                  { DA::Error, "error" },
                                  { DA::Skipped, "skipped" });

// ================================== DA::BodyShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::BodyShape,
                                  DA::BodyShape::RoundedRect,
                                  { DA::BodyShape::RoundedRect, "rounded_rect" },
                                  { DA::BodyShape::Ellipse, "ellipse" });

// ================================== DA::PortShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::PortShape,
                                  DA::PortShape::Rect,
                                  { DA::PortShape::Rect, "rect" },
                                  { DA::PortShape::Circle, "circle" },
                                  { DA::PortShape::Diamond, "diamond" });

// ================================== DA::NamePosition ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::NamePosition,
                                  DA::NamePosition::Inside,
                                  { DA::NamePosition::Inside, "inside" },
                                  { DA::NamePosition::Below, "below" });

// ================================== DA::IconPosition ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::IconPosition,
                                  DA::IconPosition::LeftOfText,
                                  { DA::IconPosition::LeftOfText, "left_of_text" },
                                  { DA::IconPosition::AboveText, "above_text" });

// ================================== DA::PortSide (AspectDirection) ==================================
// PortSide 是 AspectDirection 的类型别名，其 DAEnumTraits 定义已在
// DAGraphicsViewEnumStringUtils.cpp 中完成（DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::AspectDirection, ...)）
// 不重复定义，避免 ODR 违反和链接时多重定义错误
// enumToString(PortSide::West) 和 stringToEnum<PortSide>() 自动复用 AspectDirection 的转换

// ================================== DA::BodyIconType ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::BodyIconType,
                                  DA::BodyIconType::None,
                                  { DA::BodyIconType::None, "none" },
                                  { DA::BodyIconType::Pixmap, "pixmap" },
                                  { DA::BodyIconType::Svg, "svg" });

// ================================== DA::RenderTemplate ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::RenderTemplate,
                                  DA::RenderTemplate::NodeStyleTemplate,
                                  { DA::RenderTemplate::NodeStyleTemplate, "node_style" },
                                  { DA::RenderTemplate::WidgetTemplate, "widget" });

// ================================== DA::LinkPointLayoutStrategy ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::LinkPointLayoutStrategy,
                                  DA::LinkPointLayoutStrategy::Auto,
                                  { DA::LinkPointLayoutStrategy::Auto, "auto" },
                                  { DA::LinkPointLayoutStrategy::Manual, "manual" });