#include "DAGuiEnumStringUtils.h"

#include "DAAbstractArchiveTask.h"
namespace DA
{
void daGuiInitializeTypes()
{
    qRegisterMetaType< DA::DAAbstractArchiveTask::Mode >("DA::DAAbstractArchiveTask::Mode");
}
}
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
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtPlotItem::RttiValues,
                                  QwtPlotItem::Rtti_PlotUserItem,
                                  { QwtPlotItem::Rtti_PlotItem, "PlotItem" },
                                  { QwtPlotItem::Rtti_PlotGrid, "PlotGrid" },
                                  { QwtPlotItem::Rtti_PlotScale, "PlotScale" },
                                  { QwtPlotItem::Rtti_PlotLegend, "PlotLegend" },
                                  { QwtPlotItem::Rtti_PlotMarker, "PlotMarker" },
                                  { QwtPlotItem::Rtti_PlotCurve, "PlotCurve" },
                                  { QwtPlotItem::Rtti_PlotSpectroCurve, "PlotSpectroCurve" },
                                  { QwtPlotItem::Rtti_PlotIntervalCurve, "PlotIntervalCurve" },
                                  { QwtPlotItem::Rtti_PlotHistogram, "PlotHistogram" },
                                  { QwtPlotItem::Rtti_PlotSpectrogram, "PlotSpectrogram" },
                                  { QwtPlotItem::Rtti_PlotGraphic, "PlotGraphic" },
                                  { QwtPlotItem::Rtti_PlotTradingCurve, "PlotTradingCurve" },
                                  { QwtPlotItem::Rtti_PlotBarChart, "PlotBarChart" },
                                  { QwtPlotItem::Rtti_PlotMultiBarChart, "PlotMultiBarChart" },
                                  { QwtPlotItem::Rtti_PlotShape, "PlotShape" },
                                  { QwtPlotItem::Rtti_PlotTextLabel, "PlotTextLabel" },
                                  { QwtPlotItem::Rtti_PlotZone, "PlotZone" },
                                  { QwtPlotItem::Rtti_PlotVectorField, "PlotVectorField" },
                                  { QwtPlotItem::Rtti_PlotUserItem, "PlotUserItem" });
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
// ================================== QWT.QwtAxis::Position ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtScaleDiv::TickType,
                                  QwtScaleDiv::MinorTick,
                                  { QwtScaleDiv::NoTick, "no" },
                                  { QwtScaleDiv::MinorTick, "minor" },
                                  { QwtScaleDiv::MediumTick, "medium" },
                                  { QwtScaleDiv::MajorTick, "major" });
// ================================== QWT.QwtAxis::Position ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtScaleDraw::Alignment,
                                  QwtScaleDraw::BottomScale,
                                  { QwtScaleDraw::BottomScale, "bottom" },
                                  { QwtScaleDraw::TopScale, "top" },
                                  { QwtScaleDraw::LeftScale, "left" },
                                  { QwtScaleDraw::RightScale, "right" });
// ================================== QWT.QwtDate::Week0Type ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtDate::Week0Type,
                                  QwtDate::FirstThursday,
                                  { QwtDate::FirstThursday, "firstThursday" },
                                  { QwtDate::FirstDay, "firstDay" });
// ================================== QWT.QwtDate::IntervalType ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(QwtDate::IntervalType,
                                  QwtDate::Millisecond,
                                  { QwtDate::Millisecond, "msec" },
                                  { QwtDate::Second, "sec" },
                                  { QwtDate::Minute, "min" },
                                  { QwtDate::Hour, "hour" },
                                  { QwtDate::Day, "day" },
                                  { QwtDate::Week, "week" },
                                  { QwtDate::Month, "mon" },
                                  { QwtDate::Year, "year" });
