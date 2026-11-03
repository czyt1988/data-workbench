#include "Chart/DAStatsPlotRequests.h"
#include "Chart/DAStatsPlotCoordinator.h"
#include <QObject>
#include <QVariantList>

namespace DA
{

// ============================================================
// DAStatsHistplotRequest — histplot
// ============================================================

DAChartTypes DAStatsHistplotRequest::type() const
{
    return DAChartTypes::StatsHistplot;
}

bool DAStatsHistplotRequest::isValid() const
{
    return isValidBase() && !column.isEmpty();
}

QString DAStatsHistplotRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (column.isEmpty()) {
        return QObject::tr("Please select a data column before plotting");  // cn: 请先选择数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsHistplotRequest::toPythonParams() const
{
    QVariantMap p;
    p[ "bins" ]      = bins;
    p[ "binwidth" ]  = binwidth;
    if (binrange.has_value()) {
        QVariantList range;
        range << binrange->first << binrange->second;
        p[ "binrange" ] = range;
    }
    p[ "stat" ]       = stat;
    p[ "cumulative" ] = cumulative;
    return p;
}

void DAStatsHistplotRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                         DAChartPlotRenderer& renderer,
                                         const DAPyDataFrame& df) const
{
    coordinator.plotHistplot(*this, renderer, df);
}

// ============================================================
// DAStatsKdeplot1dRequest — kdeplot_1d
// ============================================================

DAChartTypes DAStatsKdeplot1dRequest::type() const
{
    return DAChartTypes::StatsKdeplot1d;
}

bool DAStatsKdeplot1dRequest::isValid() const
{
    return isValidBase() && !column.isEmpty();
}

QString DAStatsKdeplot1dRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (column.isEmpty()) {
        return QObject::tr("Please select a data column before plotting");  // cn: 请先选择数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsKdeplot1dRequest::toPythonParams() const
{
    QVariantMap p;
    p[ "bw_method" ] = bwMethod;
    if (bwValue.has_value()) {
        p[ "bw_value" ] = *bwValue;
    }
    p[ "grid_size" ]  = gridSize;
    p[ "cumulative" ] = cumulative;
    return p;
}

void DAStatsKdeplot1dRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                         DAChartPlotRenderer& renderer,
                                         const DAPyDataFrame& df) const
{
    coordinator.plotKdeplot1d(*this, renderer, df);
}

// ============================================================
// DAStatsKdeplot2dRequest — kdeplot_2d
// ============================================================

DAChartTypes DAStatsKdeplot2dRequest::type() const
{
    return DAChartTypes::StatsKdeplot2d;
}

bool DAStatsKdeplot2dRequest::isValid() const
{
    return isValidBase() && !xColumn.isEmpty() && !yColumn.isEmpty();
}

QString DAStatsKdeplot2dRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (xColumn.isEmpty()) {
        return QObject::tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
    }
    if (yColumn.isEmpty()) {
        return QObject::tr("Please select a Y-axis data column before plotting");  // cn: 请先选择 Y 轴数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsKdeplot2dRequest::toPythonParams() const
{
    QVariantMap p;
    p[ "bw_method" ] = bwMethod;
    if (bwValue.has_value()) {
        p[ "bw_value" ] = *bwValue;
    }
    p[ "grid_size" ] = gridSize;
    p[ "levels" ]    = levels;
    p[ "thresh" ]    = thresh;
    return p;
}

void DAStatsKdeplot2dRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                         DAChartPlotRenderer& renderer,
                                         const DAPyDataFrame& df) const
{
    coordinator.plotKdeplot2d(*this, renderer, df);
}

// ============================================================
// DAStatsBoxplotRequest — boxplot
// ============================================================

DAChartTypes DAStatsBoxplotRequest::type() const
{
    return DAChartTypes::StatsBoxplot;
}

bool DAStatsBoxplotRequest::isValid() const
{
    return isValidBase() && !columns.isEmpty();
}

QString DAStatsBoxplotRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (columns.isEmpty()) {
        return QObject::tr("Please select one or more data columns before plotting");  // cn: 请先选择数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsBoxplotRequest::toPythonParams() const
{
    QVariantMap p;
    QVariantList cols;
    for (const QString& c : columns) {
        if (!c.isEmpty()) {
            cols << c;
        }
    }
    p[ "columns" ] = cols;
    if (!hue.isEmpty()) {
        p[ "hue" ] = hue;
    }
    p[ "whis" ] = whis;
    return p;
}

void DAStatsBoxplotRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                        DAChartPlotRenderer& renderer,
                                        const DAPyDataFrame& df) const
{
    coordinator.plotBoxplot(*this, renderer, df);
}

// ============================================================
// DAStatsHeatmapRequest — heatmap
// ============================================================

DAChartTypes DAStatsHeatmapRequest::type() const
{
    return DAChartTypes::StatsHeatmap;
}

bool DAStatsHeatmapRequest::isValid() const
{
    return isValidBase() && !indexColumn.isEmpty() && !columnsColumn.isEmpty();
}

QString DAStatsHeatmapRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (indexColumn.isEmpty()) {
        return QObject::tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
    }
    if (columnsColumn.isEmpty()) {
        return QObject::tr("Please select a Y-axis data column before plotting");  // cn: 请先选择 Y 轴数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsHeatmapRequest::toPythonParams() const
{
    QVariantMap p;
    p[ "aggfunc" ] = aggfunc;
    if (standardScale.has_value()) {
        p[ "standard_scale" ] = *standardScale;
    }
    if (vmin.has_value()) {
        p[ "vmin" ] = *vmin;
    }
    if (vmax.has_value()) {
        p[ "vmax" ] = *vmax;
    }
    if (center.has_value()) {
        p[ "center" ] = *center;
    }
    return p;
}

void DAStatsHeatmapRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                       DAChartPlotRenderer& renderer,
                                       const DAPyDataFrame& df) const
{
    coordinator.plotHeatmap(*this, renderer, df);
}

// ============================================================
// DAStatsScatterplotRequest — scatterplot（纯 C++ 提取绘制）
// ============================================================

DAChartTypes DAStatsScatterplotRequest::type() const
{
    return DAChartTypes::StatsScatterplot;
}

bool DAStatsScatterplotRequest::isValid() const
{
    return isValidBase() && !xColumn.isEmpty() && !yColumn.isEmpty();
}

QString DAStatsScatterplotRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (xColumn.isEmpty()) {
        return QObject::tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
    }
    if (yColumn.isEmpty()) {
        return QObject::tr("Please select a Y-axis data column before plotting");  // cn: 请先选择 Y 轴数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsScatterplotRequest::toPythonParams() const
{
    // 此类型不调用任何 Python 统计函数（纯 C++ 提取 x/y 绘制），
    // 实现为空以保持 9 个请求类接口统一。
    return QVariantMap();
}

void DAStatsScatterplotRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                           DAChartPlotRenderer& renderer,
                                           const DAPyDataFrame& df) const
{
    coordinator.plotScatterplot(*this, renderer, df);
}

// ============================================================
// DAStatsBarplotRequest — barplot / count plot
// ============================================================

DAChartTypes DAStatsBarplotRequest::type() const
{
    return DAChartTypes::StatsBarplot;
}

bool DAStatsBarplotRequest::isValid() const
{
    return isValidBase() && !xColumn.isEmpty();
}

QString DAStatsBarplotRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (xColumn.isEmpty()) {
        return QObject::tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsBarplotRequest::toPythonParams() const
{
    QVariantMap p;
    p[ "estimator" ] = estimator;
    if (!hue.isEmpty()) {
        p[ "hue" ] = hue;
    }
    p[ "n_boot" ] = nBoot;
    return p;
}

void DAStatsBarplotRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                       DAChartPlotRenderer& renderer,
                                       const DAPyDataFrame& df) const
{
    coordinator.plotBarplot(*this, renderer, df);
}

// ============================================================
// DAStatsRegplotRequest — regplot
// ============================================================

DAChartTypes DAStatsRegplotRequest::type() const
{
    return DAChartTypes::StatsRegplot;
}

bool DAStatsRegplotRequest::isValid() const
{
    return isValidBase() && !xColumn.isEmpty() && !yColumn.isEmpty();
}

QString DAStatsRegplotRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (xColumn.isEmpty()) {
        return QObject::tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
    }
    if (yColumn.isEmpty()) {
        return QObject::tr("Please select a Y-axis data column before plotting");  // cn: 请先选择 Y 轴数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsRegplotRequest::toPythonParams() const
{
    QVariantMap p;
    p[ "order" ]  = order;
    p[ "n_boot" ] = nBoot;
    if (ci > 0) {
        p[ "ci" ] = ci;
    }
    return p;
}

void DAStatsRegplotRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                       DAChartPlotRenderer& renderer,
                                       const DAPyDataFrame& df) const
{
    coordinator.plotRegplot(*this, renderer, df);
}

// ============================================================
// DAStatsEcdfplotRequest — ecdfplot
// ============================================================

DAChartTypes DAStatsEcdfplotRequest::type() const
{
    return DAChartTypes::StatsECDFplot;
}

bool DAStatsEcdfplotRequest::isValid() const
{
    return isValidBase() && !column.isEmpty();
}

QString DAStatsEcdfplotRequest::errorString() const
{
    const QString base = baseErrorString();
    if (!base.isEmpty()) {
        return base;
    }
    if (column.isEmpty()) {
        return QObject::tr("Please select a data column before plotting");  // cn: 请先选择数据列再绘图
    }
    return QString();
}

QVariantMap DAStatsEcdfplotRequest::toPythonParams() const
{
    QVariantMap p;
    p[ "stat" ]         = stat;
    p[ "complementary" ] = complementary;
    if (!weights.isEmpty()) {
        p[ "weights" ] = weights;
    }
    return p;
}

void DAStatsEcdfplotRequest::dispatchTo(DAStatsPlotCoordinator& coordinator,
                                        DAChartPlotRenderer& renderer,
                                        const DAPyDataFrame& df) const
{
    coordinator.plotEcdfplot(*this, renderer, df);
}

}  // namespace DA
