#include "DAChartPlotItemFactory.h"
#include <QHash>
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_scaleitem.h"
#include "qwt_plot_legenditem.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_graphicitem.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_textlabel.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_vectorfield.h"
namespace DA
{
/**
 * @brief qwt的默认plotitem注册到工厂
 * @return
 */
static QHash< int, DAChartPlotItemFactory::FpItemCreate > initDAChartPlotItemFactory()
{
    QHash< int, DAChartPlotItemFactory::FpItemCreate > res;
    res[ QwtPlotItem::Rtti_PlotCurve ]         = []() -> QwtPlotItem* { return new QwtPlotCurve(); };
    res[ QwtPlotItem::Rtti_PlotGrid ]          = []() -> QwtPlotItem* { return new QwtPlotGrid(); };
    res[ QwtPlotItem::Rtti_PlotScale ]         = []() -> QwtPlotItem* { return new QwtPlotScaleItem(); };
    res[ QwtPlotItem::Rtti_PlotLegend ]        = []() -> QwtPlotItem* { return new QwtPlotLegendItem(); };
    res[ QwtPlotItem::Rtti_PlotMarker ]        = []() -> QwtPlotItem* { return new QwtPlotMarker(); };
    res[ QwtPlotItem::Rtti_PlotSpectroCurve ]  = []() -> QwtPlotItem* { return new QwtPlotSpectroCurve(); };
    res[ QwtPlotItem::Rtti_PlotIntervalCurve ] = []() -> QwtPlotItem* { return new QwtPlotIntervalCurve(); };
    res[ QwtPlotItem::Rtti_PlotHistogram ]     = []() -> QwtPlotItem* { return new QwtPlotHistogram(); };
    res[ QwtPlotItem::Rtti_PlotSpectrogram ]   = []() -> QwtPlotItem* { return new QwtPlotSpectrogram(); };
    res[ QwtPlotItem::Rtti_PlotGraphic ]       = []() -> QwtPlotItem* { return new QwtPlotGraphicItem(); };
    res[ QwtPlotItem::Rtti_PlotTradingCurve ]  = []() -> QwtPlotItem* { return new QwtPlotTradingCurve(); };
    res[ QwtPlotItem::Rtti_PlotBarChart ]      = []() -> QwtPlotItem* { return new QwtPlotBarChart(); };
    res[ QwtPlotItem::Rtti_PlotMultiBarChart ] = []() -> QwtPlotItem* { return new QwtPlotMultiBarChart(); };
    res[ QwtPlotItem::Rtti_PlotShape ]         = []() -> QwtPlotItem* { return new QwtPlotShapeItem(); };
    res[ QwtPlotItem::Rtti_PlotTextLabel ]     = []() -> QwtPlotItem* { return new QwtPlotTextLabel(); };
    res[ QwtPlotItem::Rtti_PlotZone ]          = []() -> QwtPlotItem* { return new QwtPlotZoneItem(); };
    res[ QwtPlotItem::Rtti_PlotVectorField ]   = []() -> QwtPlotItem* { return new QwtPlotVectorField(); };
    return res;
}

DAChartPlotItemFactory::DAChartPlotItemFactory()
{
}

QwtPlotItem* DAChartPlotItemFactory::createItem(int rtti)
{
    FpItemCreate fp = factoryFunctionMap().value(rtti, nullptr);
    if (!fp) {
        return nullptr;
    }
    return fp();
}

void DAChartPlotItemFactory::registCreateItemFucntion(int rtti, DAChartPlotItemFactory::FpItemCreate fp)
{
    factoryFunctionMap()[ rtti ] = fp;
}

bool DAChartPlotItemFactory::isHaveCreateItemFucntion(int rtti)
{
    return factoryFunctionMap().contains(rtti);
}

/**
 * @brief 工程函数map
 * @return
 */
QHash< int, DAChartPlotItemFactory::FpItemCreate >& DAChartPlotItemFactory::factoryFunctionMap()
{
    static QHash< int, DAChartPlotItemFactory::FpItemCreate > s_ItemCreateFp = initDAChartPlotItemFactory();
    return s_ItemCreateFp;
}
}
