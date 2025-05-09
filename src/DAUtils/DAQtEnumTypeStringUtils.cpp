#include "DAQtEnumTypeStringUtils.h"

// ================================== Qt::AlignmentFlag ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::AlignmentFlag,
                                  Qt::AlignLeft,
                                  { Qt::AlignLeft, "AlignLeft" },
                                  { Qt::AlignRight, "AlignRight" },
                                  { Qt::AlignHCenter, "AlignHCenter" },
                                  { Qt::AlignJustify, "AlignJustify" },
                                  { Qt::AlignAbsolute, "AlignAbsolute" },
                                  { Qt::AlignHorizontal_Mask, "AlignHorizontal_Mask" },
                                  { Qt::AlignTop, "AlignTop" },
                                  { Qt::AlignBottom, "AlignBottom" },
                                  { Qt::AlignVCenter, "AlignVCenter" },
                                  { Qt::AlignBaseline, "AlignBaseline" },
                                  { Qt::AlignVertical_Mask, "AlignVertical_Mask" },
                                  { Qt::AlignCenter, "AlignCenter" });
// 为 Qt::Alignment 定义映射（而非 Qt::AlignmentFlag）
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::Alignment,
                                  Qt::AlignLeft,
                                  { Qt::AlignLeft, "AlignLeft" },
                                  { Qt::AlignRight, "AlignRight" },
                                  { Qt::AlignHCenter, "AlignHCenter" },
                                  { Qt::AlignJustify, "AlignJustify" },
                                  { Qt::AlignAbsolute, "AlignAbsolute" },
                                  { Qt::AlignHorizontal_Mask, "AlignHorizontal_Mask" },
                                  { Qt::AlignTop, "AlignTop" },
                                  { Qt::AlignBottom, "AlignBottom" },
                                  { Qt::AlignVCenter, "AlignVCenter" },
                                  { Qt::AlignBaseline, "AlignBaseline" },
                                  { Qt::AlignVertical_Mask, "AlignVertical_Mask" },
                                  { Qt::AlignCenter, "AlignCenter" });
// ------------------------------------------
//           Qt::PenStyle
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::PenStyle,
                                  Qt::SolidLine,
                                  { Qt::NoPen, "NoPen" },
                                  { Qt::SolidLine, "SolidLine" },
                                  { Qt::DashLine, "DashLine" },
                                  { Qt::DotLine, "DotLine" },
                                  { Qt::DashDotLine, "DashDotLine" },
                                  { Qt::DashDotDotLine, "DashDotDotLine" },
                                  { Qt::CustomDashLine, "CustomDashLine" });

// ------------------------------------------
//           Qt::BrushStyle
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::BrushStyle,
                                  Qt::SolidPattern,
                                  { Qt::NoBrush, "NoBrush" },
                                  { Qt::SolidPattern, "SolidPattern" },
                                  { Qt::Dense1Pattern, "Dense1Pattern" },
                                  { Qt::Dense2Pattern, "Dense2Pattern" },
                                  { Qt::Dense3Pattern, "Dense3Pattern" },
                                  { Qt::Dense4Pattern, "Dense4Pattern" },
                                  { Qt::Dense5Pattern, "Dense5Pattern" },
                                  { Qt::Dense6Pattern, "Dense6Pattern" },
                                  { Qt::Dense7Pattern, "Dense7Pattern" },
                                  { Qt::HorPattern, "HorPattern" },
                                  { Qt::VerPattern, "VerPattern" },
                                  { Qt::CrossPattern, "CrossPattern" },
                                  { Qt::BDiagPattern, "BDiagPattern" },
                                  { Qt::FDiagPattern, "FDiagPattern" },
                                  { Qt::DiagCrossPattern, "DiagCrossPattern" },
                                  { Qt::LinearGradientPattern, "LinearGradientPattern" },
                                  { Qt::RadialGradientPattern, "RadialGradientPattern" },
                                  { Qt::ConicalGradientPattern, "ConicalGradientPattern" },
                                  { Qt::TexturePattern, "TexturePattern" });

// ------------------------------------------
//           Qt::AspectRatioMode
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::AspectRatioMode,
                                  Qt::IgnoreAspectRatio,
                                  { Qt::IgnoreAspectRatio, "IgnoreAspectRatio" },
                                  { Qt::KeepAspectRatio, "KeepAspectRatio" },
                                  { Qt::KeepAspectRatioByExpanding, "KeepAspectRatioByExpanding" });

// ------------------------------------------
//           Qt::TransformationMode
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::TransformationMode,
                                  Qt::FastTransformation,
                                  { Qt::FastTransformation, "FastTransformation" },
                                  { Qt::SmoothTransformation, "SmoothTransformation" });

// ------------------------------------------
//           Qt::TimeSpec
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::TimeSpec,
                                  Qt::LocalTime,
                                  { Qt::LocalTime, "LocalTime" },
                                  { Qt::UTC, "UTC" },
                                  { Qt::OffsetFromUTC, "OffsetFromUTC" },
                                  { Qt::TimeZone, "TimeZone" });
// ------------------------------------------
//           QFont::Weight
// ------------------------------------------
DA_ENUM_STRING_INSENSITIVE_DEFINE(QFont::Weight,
                                  QFont::Normal,
                                  { QFont::Thin, "Thin" },
                                  { QFont::Light, "Light" },
                                  { QFont::Normal, "Normal" },
                                  { QFont::Medium, "Medium" },
                                  { QFont::DemiBold, "DemiBold" },
                                  { QFont::Bold, "Bold" },
                                  { QFont::ExtraLight, "ExtraLight" },
                                  { QFont::ExtraBold, "ExtraBold" },
                                  { QFont::Black, "Black" });
