#include "DAGuiEnumStringUtils.h"

//===============================================================
// DAUtils
//===============================================================
// ================================== DAUtils.DA::DAColorTheme::ColorThemeStyle ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAColorTheme::ColorThemeStyle,
                                  DA::DAColorTheme::Style_UserDefine,
                                  { DA::DAColorTheme::Style_Archambault, "Archambault" },
                                  { DA::DAColorTheme::Style_Cassatt1, "Cassatt1" },
                                  { DA::DAColorTheme::Style_Cassatt2, "Cassatt2" },
                                  { DA::DAColorTheme::Style_Demuth, "Demuth" },
                                  { DA::DAColorTheme::Style_Derain, "Derain" },
                                  { DA::DAColorTheme::Style_Egypt, "Egypt" },
                                  { DA::DAColorTheme::Style_Greek, "Greek" },
                                  { DA::DAColorTheme::Style_Hiroshige, "Hiroshige" },
                                  { DA::DAColorTheme::Style_Hokusai2, "Hokusai2" },
                                  { DA::DAColorTheme::Style_Hokusai3, "Hokusai3" },
                                  { DA::DAColorTheme::Style_Ingres, "Ingres" },
                                  { DA::DAColorTheme::Style_Isfahan1, "Isfahan1" },
                                  { DA::DAColorTheme::Style_Isfahan2, "Isfahan2" },
                                  { DA::DAColorTheme::Style_Java, "Java" },
                                  { DA::DAColorTheme::Style_Johnson, "Johnson" },
                                  { DA::DAColorTheme::Style_Kandinsky, "Kandinsky" },
                                  { DA::DAColorTheme::Style_Morgenstern, "Morgenstern" },
                                  { DA::DAColorTheme::Style_OKeeffe1, "OKeeffe1" },
                                  { DA::DAColorTheme::Style_OKeeffe2, "OKeeffe2" },
                                  { DA::DAColorTheme::Style_Pillement, "Pillement" },
                                  { DA::DAColorTheme::Style_Tam, "Tam" },
                                  { DA::DAColorTheme::Style_Troy, "Troy" },
                                  { DA::DAColorTheme::Style_VanGogh3, "VanGogh3" },
                                  { DA::DAColorTheme::Style_Veronese, "Veronese" },
                                  { DA::DAColorTheme::Style_UserDefine, "UserDefine" });

//===============================================================
// QWT
//===============================================================
// ================================== QWT.QwtPlot::LegendPosition ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtPlot::LegendPosition,
                                  QwtPlot::RightLegend,
                                  { QwtPlot::LeftLegend, "left" },
                                  { QwtPlot::RightLegend, "right" },
                                  { QwtPlot::BottomLegend, "bottom" },
                                  { QwtPlot::TopLegend, "top" });

// ================================== QWT.QwtText::TextFormat ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtText::TextFormat,
                                  QwtText::AutoText,
                                  { QwtText::AutoText, "auto" },
                                  { QwtText::PlainText, "plain" },
                                  { QwtText::RichText, "rich" },
                                  { QwtText::MathMLText, "mathml" },
                                  { QwtText::TeXText, "tex" },
                                  { QwtText::OtherFormat, "other" });
// ================================== QWT.QwtAxis::Position ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtAxis::Position,
                                  QwtAxis::YLeft,
                                  { QwtAxis::YLeft, "yleft" },
                                  { QwtAxis::YRight, "yright" },
                                  { QwtAxis::XBottom, "xbottom" },
                                  { QwtAxis::XTop, "xtop" });
