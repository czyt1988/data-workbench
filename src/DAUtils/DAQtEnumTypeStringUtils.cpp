#include "DAQtEnumTypeStringUtils.h"

// ================================== Qt::AlignmentFlag ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::AlignmentFlag,
                                  Qt::AlignLeft,
                                  { Qt::AlignLeft, "left" },
                                  { Qt::AlignHCenter, "hcenter" },
                                  { Qt::AlignRight, "right" },
                                  { Qt::AlignTop, "top" },
                                  { Qt::AlignBottom, "bottom" },
                                  { Qt::AlignVCenter, "vcenter" });
// 为 Qt::Alignment 定义映射（而非 Qt::AlignmentFlag）
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::Alignment,
                                  Qt::AlignLeft,
                                  { Qt::AlignLeft, "left" },
                                  { Qt::AlignHCenter, "hcenter" },
                                  { Qt::AlignRight, "right" },
                                  { Qt::AlignTop, "top" },
                                  { Qt::AlignBottom, "bottom" },
                                  { Qt::AlignVCenter, "vcenter" });
// ------------------------------------------
//           Qt::PenStyle
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::PenStyle,
                                  Qt::SolidLine,
                                  { Qt::NoPen, "none" },
                                  { Qt::SolidLine, "solid" },
                                  { Qt::DashLine, "dash" },
                                  { Qt::DotLine, "dot" },
                                  { Qt::DashDotLine, "dashdot" },
                                  { Qt::DashDotDotLine, "dashdotdot" });

// ------------------------------------------
//           Qt::BrushStyle
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::BrushStyle,
                                  Qt::SolidPattern,
                                  { Qt::NoBrush, "none" },
                                  { Qt::SolidPattern, "solid" },
                                  { Qt::HorPattern, "horizontal" },
                                  { Qt::VerPattern, "vertical" },
                                  { Qt::CrossPattern, "cross" },
                                  { Qt::DiagCrossPattern, "diagcross" });

// ------------------------------------------
//           Qt::AspectRatioMode
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::AspectRatioMode,
                                  Qt::IgnoreAspectRatio,
                                  { Qt::IgnoreAspectRatio, "ignore" },
                                  { Qt::KeepAspectRatio, "keep" },
                                  { Qt::KeepAspectRatioByExpanding, "expand" });

// ------------------------------------------
//           Qt::TransformationMode
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::TransformationMode,
                                  Qt::FastTransformation,
                                  { Qt::FastTransformation, "fast" },
                                  { Qt::SmoothTransformation, "smooth" });

// ------------------------------------------
//           QFont::Weight
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(QFont::Weight,
                                  QFont::Normal,
                                  { QFont::Thin, "thin" },
                                  { QFont::Light, "light" },
                                  { QFont::Normal, "normal" },
                                  { QFont::Medium, "medium" },
                                  { QFont::DemiBold, "demibold" },
                                  { QFont::Bold, "bold" },
                                  { QFont::Black, "black" });
