#include "DAPyLinkPointStyle.h"

// ================================== DA::PortShape ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyLinkPointStyle::PortShape,
                                  DA::DAPyLinkPointStyle::Rect,
                                  { DA::DAPyLinkPointStyle::Rect, "rect" },
                                  { DA::DAPyLinkPointStyle::Circle, "circle" },
                                  { DA::DAPyLinkPointStyle::Diamond, "diamond" });
