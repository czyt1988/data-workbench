#include "DAPyNodeStyle.h"
// ================================== DA::LinkPointLayoutStrategy ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::LinkPointLayoutStrategy,
                                  DA::DAPyNodeStyle::Auto,
                                  { DA::DAPyNodeStyle::Auto, "auto" },
                                  { DA::DAPyNodeStyle::Manual, "manual" });

DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::NodeRenderTemplate,
                                  DA::DAPyNodeStyle::NodeStyleTemplate,
                                  { DA::DAPyNodeStyle::NodeStyleTemplate, "nodestyle" },
                                  { DA::DAPyNodeStyle::WidgetTemplate, "widgetstyle" })

// ================================== DA::BodyShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::BodyShape,
                                  DA::DAPyNodeStyle::RoundedRect,
                                  { DA::DAPyNodeStyle::RoundedRect, "rounded_rect" },
                                  { DA::DAPyNodeStyle::Ellipse, "ellipse" })

// ================================== DA::BodyShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::NamePosition,
                                  DA::DAPyNodeStyle::Inside,
                                  { DA::DAPyNodeStyle::Inside, "inside" },
                                  { DA::DAPyNodeStyle::Below, "below" })

// ================================== DA::IconPosition ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::IconPosition,
                                  DA::DAPyNodeStyle::LeftOfText,
                                  { DA::DAPyNodeStyle::LeftOfText, "left_of_text" },
                                  { DA::DAPyNodeStyle::AboveText, "above_text" });
// ================================== DA::BodyIconType ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeStyle::BodyIconType,
                                  DA::DAPyNodeStyle::None,
                                  { DA::DAPyNodeStyle::None, "none" },
                                  { DA::DAPyNodeStyle::Pixmap, "pixmap" },
                                  { DA::DAPyNodeStyle::Svg, "svg" });
