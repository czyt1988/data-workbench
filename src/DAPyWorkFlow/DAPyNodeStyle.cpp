#include "DAPyNodeStyle.h"

//!
//! 注意!!!!
//! 在枚举和python属性转换的过程中，会使用enumToString来获取枚举对应的字符串，
//! 因此，枚举对应的字符串，必须和python对应的属性名一致，否则会在脚本调用过程中出错
//!
// ================================== DA::LinkPointLayoutStrategy ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::LinkPointLayoutStrategy,
                                  DA::DAPyNodeStyle::AutoLayoutLinkPoint,
                                  { DA::DAPyNodeStyle::AutoLayoutLinkPoint, "auto" },
                                  { DA::DAPyNodeStyle::ManualLayoutLinkPoint, "manual" });

DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::NodeRenderTemplate,
                                  DA::DAPyNodeStyle::RenderDefaultTemplate,
                                  { DA::DAPyNodeStyle::RenderDefaultTemplate, "nodestyle" },
                                  { DA::DAPyNodeStyle::RenderWidgetTemplate, "widgetstyle" })

// ================================== DA::BodyShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::BodyShape,
                                  DA::DAPyNodeStyle::RoundedRectShape,
                                  { DA::DAPyNodeStyle::RoundedRectShape, "rounded_rect" },
                                  { DA::DAPyNodeStyle::EllipseShape, "ellipse" },
                                  { DA::DAPyNodeStyle::DiamondShape, "diamond" })

// ================================== DA::BodyShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::NamePosition,
                                  DA::DAPyNodeStyle::NameInsideBody,
                                  { DA::DAPyNodeStyle::NameInsideBody, "inside" },
                                  { DA::DAPyNodeStyle::NameBelowBody, "below" })

// ================================== DA::IconPosition ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::IconPosition,
                                  DA::DAPyNodeStyle::IconLeftOfText,
                                  { DA::DAPyNodeStyle::IconLeftOfText, "left_of_text" },
                                  { DA::DAPyNodeStyle::IconAboveText, "above_text" });

// ================================== DA::BodyIconType ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::BodyIconType,
                                  DA::DAPyNodeStyle::NoneBodyIcon,
                                  { DA::DAPyNodeStyle::NoneBodyIcon, "none" },
                                  { DA::DAPyNodeStyle::PixmapBodyIcon, "pixmap" },
                                  { DA::DAPyNodeStyle::SvgBodyIcon, "svg" });
