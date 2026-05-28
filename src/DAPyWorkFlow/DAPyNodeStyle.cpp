#include "DAPyNodeStyle.h"

DA_ENUM_STRING_INSENSITIVE_DEFINE(DAPyNodeStyle::NodeRenderTemplate,
                                  DAPyNodeStyle::NodeStyleTemplate,
                                  { DAPyNodeStyle::NodeStyleTemplate, "nodestyle" },
                                  { DAPyNodeStyle::WidgetTemplate, "widgetstyle" })
// ================================== DA::BodyShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DAPyNodeStyle::BodyShape,
                                  DAPyNodeStyle::RoundedRect,
                                  { DAPyNodeStyle::RoundedRect, "rounded_rect" },
                                  { DAPyNodeStyle::Ellipse, "ellipse" });
