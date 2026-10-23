#include "DAChartAddItemWidgetFactory.h"
// Qt
#include <QCoreApplication>
// std
#include <utility>
// qwt
#include "qwt_plot_curve.h"
// DAGui
#include "DAChartAddCurveWidget.h"
#include "DAChartAddBarWidget.h"
#include "DAChartAddIntervalCurveWidget.h"
#include "DAChartAddTradingCurveWidget.h"
#include "DAChartAddErrorBarWidget.h"
#include "DAChartAddBoxChartWidget.h"
#include "DAChartAddMultiBarWidget.h"
#include "DAChartAddHistogramWidget.h"
#include "DAChartAddContourWidget.h"
#include "DAChartAddVectorFieldWidget.h"
#include "DAChartAddSpectrogramWidget.h"
#include "DAChartAdd3DSurfaceWidget.h"
#include "DAChartAdd3DBarWidget.h"
#include "DAChartAdd3DLineWidget.h"
#include "DAChartAddStatsHistplotWidget.h"
#include "DAChartAddStatsKdeplot1dWidget.h"
#include "DAChartAddStatsKdeplot2dWidget.h"
#include "DAChartAddStatsBoxplotWidget.h"
#include "DAChartAddStatsHeatmapWidget.h"
#include "DAChartAddStatsScatterplotWidget.h"
#include "DAChartAddStatsBarplotWidget.h"
#include "DAChartAddStatsRegplotWidget.h"
#include "DAChartAddStatsEcdfplotWidget.h"

namespace DA
{
namespace
{
// 图标资源目录
const char* const kIconDir = ":/DAGui/ChartType/icon/chart-type/";
// 二维/三维引导窗口的翻译上下文
const char* const kChartGuideContext = "DADialogChartGuide";
// 统计引导窗口的翻译上下文
const char* const kStatsGuideContext = "DADialogStatsChartGuide";

// 通用的页面创建函数
template< typename W >
std::function< DAAbstractChartAddWidgetBase*() > widgetCreator()
{
    return []() -> DAAbstractChartAddWidgetBase* { return new W; };
}

// 散点图复用曲线页面，创建后把线条改为点状
void applyScatterStyle(QwtPlotItem* item)
{
    if (item && item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
        static_cast< QwtPlotCurve* >(item)->setStyle(QwtPlotCurve::Dots);
    }
}
}  // namespace

/**
 * @brief 翻译后的显示文本
 */
QString DAChartAddItemWidgetDescriptor::text() const
{
    if (nullptr == textSource) {
        return QString();
    }
    return QCoreApplication::translate(textContext, textSource);
}

/**
 * @brief 列表图标
 */
QIcon DAChartAddItemWidgetDescriptor::icon() const
{
    if (nullptr == iconFile) {
        return QIcon();
    }
    return QIcon(QString::fromLatin1(kIconDir) + QString::fromLatin1(iconFile));
}

//===================================================
// DAChartAddItemWidgetFactory
//===================================================

/**
 * @brief 获取注册表单例
 */
DAChartAddItemWidgetFactory& DAChartAddItemWidgetFactory::instance()
{
    static DAChartAddItemWidgetFactory s_instance;
    return s_instance;
}

DAChartAddItemWidgetFactory::DAChartAddItemWidgetFactory()
{
    registerBuiltinTypes();
}

/**
 * @brief 注册内置图表类型
 *
 * 注册顺序即引导窗口左侧列表的显示顺序。
 * 文案使用 QT_TRANSLATE_NOOP 标记源串（供 lupdate 提取），
 * 在显示时按当前语言翻译，因此不受注册时机影响。
 */
void DAChartAddItemWidgetFactory::registerBuiltinTypes()
{
    const char* const ctx = kChartGuideContext;
    // === 二维 ===
    registerType({ DAChartTypes::Curve,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "curve"),  // cn:曲线
                   "chart-curve.svg",
                   widgetCreator< DAChartAddCurveWidget >(),
                   {} });
    registerType({ DAChartTypes::Scatter,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "scatter"),  // cn:散点
                   "chart-scatter.svg",
                   widgetCreator< DAChartAddCurveWidget >(),
                   applyScatterStyle,
                   DAChartTypes::Curve });
    registerType({ DAChartTypes::Bar,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "bar"),  // cn:柱状
                   "chart-bar.svg",
                   widgetCreator< DAChartAddBarWidget >(),
                   {} });
    registerType({ DAChartTypes::ErrorBar,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "error bar"),  // cn:误差棒
                   "chart-intervalcurve.svg",
                   widgetCreator< DAChartAddErrorBarWidget >(),
                   {} });
    registerType({ DAChartTypes::Box,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "box"),  // cn:箱体
                   "chart-OHLC.svg",
                   widgetCreator< DAChartAddBoxChartWidget >(),
                   {} });
    registerType({ DAChartTypes::Spectrogram,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "cloud map"),  // cn:云图
                   "chart-spectrogram.svg",
                   widgetCreator< DAChartAddSpectrogramWidget >(),
                   {} });
    registerType({ DAChartTypes::MultiBar,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "multi bar"),  // cn:多重柱状
                   "chart-multibar.svg",
                   widgetCreator< DAChartAddMultiBarWidget >(),
                   {} });
    registerType({ DAChartTypes::Histogram,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "histogram"),  // cn:直方图
                   "chart-histogram.svg",
                   widgetCreator< DAChartAddHistogramWidget >(),
                   {} });
    registerType({ DAChartTypes::Contour,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "contour"),  // cn:等高线
                   "chart-spectrocurve.svg",
                   widgetCreator< DAChartAddContourWidget >(),
                   {} });
    registerType({ DAChartTypes::VectorField,
                   DAChartAddWidgetFamily::Curve2D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "vector field"),  // cn:向量场
                   "chart-vectorfield.svg",
                   widgetCreator< DAChartAddVectorFieldWidget >(),
                   {} });
    // === 三维 ===
    registerType({ DAChartTypes::Surface3D,
                   DAChartAddWidgetFamily::Surface3D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "surface 3D"),  // cn:3D曲面
                   "chart-surface3d.svg",
                   widgetCreator< DAChartAdd3DSurfaceWidget >(),
                   {} });
    registerType({ DAChartTypes::Bar3D,
                   DAChartAddWidgetFamily::Surface3D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "bar 3D"),  // cn:3D柱状
                   "chart-bar3d.svg",
                   widgetCreator< DAChartAdd3DBarWidget >(),
                   {} });
    registerType({ DAChartTypes::Line3D,
                   DAChartAddWidgetFamily::Surface3D,
                   ctx,
                   QT_TRANSLATE_NOOP("DADialogChartGuide", "line 3D"),  // cn:3D线图
                   "chart-line3d.svg",
                   widgetCreator< DAChartAdd3DLineWidget >(),
                   {} });
    // === 统计 ===
    registerType({ DAChartTypes::StatsHistplot,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "Histplot"),  // cn:直方图
                   "stats-histplot.svg",
                   widgetCreator< DAChartAddStatsHistplotWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsKdeplot1d,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "KDE 1D"),  // cn:一维核密度
                   "stats-kdeplot.svg",
                   widgetCreator< DAChartAddStatsKdeplot1dWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsKdeplot2d,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "KDE 2D"),  // cn:二维核密度
                   "stats-kdeplot-2d.svg",
                   widgetCreator< DAChartAddStatsKdeplot2dWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsBoxplot,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "Boxplot"),  // cn:箱线图
                   "stats-boxplot.svg",
                   widgetCreator< DAChartAddStatsBoxplotWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsHeatmap,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "Heatmap"),  // cn:热力图
                   "stats-heatmap.svg",
                   widgetCreator< DAChartAddStatsHeatmapWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsScatterplot,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "Scatter"),  // cn:散点图
                   "stats-scatterplot.svg",
                   widgetCreator< DAChartAddStatsScatterplotWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsBarplot,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "Barplot"),  // cn:柱状图
                   "stats-barplot.svg",
                   widgetCreator< DAChartAddStatsBarplotWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsRegplot,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "Regplot"),  // cn:回归图
                   "stats-regplot.svg",
                   widgetCreator< DAChartAddStatsRegplotWidget >(),
                   {} });
    registerType({ DAChartTypes::StatsECDFplot,
                   DAChartAddWidgetFamily::Stats,
                   kStatsGuideContext,
                   QT_TRANSLATE_NOOP("DADialogStatsChartGuide", "ECDF"),  // cn:经验累积分布
                   "stats-ecdfplot.svg",
                   widgetCreator< DAChartAddStatsEcdfplotWidget >(),
                   {} });
}

/**
 * @brief 注册一种图表类型
 * @param desc 描述；同类型重复注册时覆盖旧描述
 */
void DAChartAddItemWidgetFactory::registerType(const DAChartAddItemWidgetDescriptor& desc)
{
    for (int i = 0; i < mDescriptors.size(); ++i) {
        if (mDescriptors[ i ].type == desc.type) {
            mDescriptors[ i ] = desc;
            return;
        }
    }
    mDescriptors.append(desc);
}

/**
 * @brief 是否已注册该类型
 */
bool DAChartAddItemWidgetFactory::isRegistered(DA::DAChartTypes type) const
{
    return descriptor(type) != nullptr;
}

/**
 * @brief 查找图表类型描述
 * @return 未注册返回 nullptr
 */
const DAChartAddItemWidgetDescriptor* DAChartAddItemWidgetFactory::descriptor(DA::DAChartTypes type) const
{
    for (const DAChartAddItemWidgetDescriptor& desc : std::as_const(mDescriptors)) {
        if (desc.type == type) {
            return &desc;
        }
    }
    return nullptr;
}

/**
 * @brief 按注册顺序返回某一家族的全部描述
 */
QList< DAChartAddItemWidgetDescriptor > DAChartAddItemWidgetFactory::descriptors(DAChartAddWidgetFamily family) const
{
    QList< DAChartAddItemWidgetDescriptor > res;
    for (const DAChartAddItemWidgetDescriptor& desc : std::as_const(mDescriptors)) {
        if (desc.family == family) {
            res.append(desc);
        }
    }
    return res;
}

/**
 * @brief 按注册顺序返回全部描述
 */
QList< DAChartAddItemWidgetDescriptor > DAChartAddItemWidgetFactory::allDescriptors() const
{
    return mDescriptors;
}

/**
 * @brief 判断是否为三维图表类型
 */
bool DAChartAddItemWidgetFactory::is3DType(DA::DAChartTypes type)
{
    const DAChartAddItemWidgetDescriptor* desc = instance().descriptor(type);
    return desc && desc->family == DAChartAddWidgetFamily::Surface3D;
}

}  // end DA
