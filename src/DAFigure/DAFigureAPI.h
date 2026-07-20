#ifndef DAFIGUREAPI_H
#define DAFIGUREAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAFIGURE_BUILD)
#define DAFIGURE_API Q_DECL_EXPORT
#else
#define DAFIGURE_API Q_DECL_IMPORT
#endif

namespace DA
{
/**
 * @brief 图表类型
 */
enum class DAChartTypes
{
    Curve,        ///< 曲线
    Scatter,      ///< 散点
    Bar,          ///< 柱状
    ErrorBar,     ///< 误差棒
    Box,          ///< 箱线图
    Spectrogram,  ///< 谱图
    MultiBar,     ///< 多重柱状图
    Histogram,    ///< 直方图
    Contour,      ///< 等高线图
    VectorField,  ///< 向量场图

    // === 统计绘图类型 ===
    StatsHistplot,      ///< 直方图+KDE叠加
    StatsKdeplot1d,     ///< 一维核密度图
    StatsKdeplot2d,     ///< 二维核密度图
    StatsBoxplot,       ///< 箱线图（统计版）
    StatsHeatmap,       ///< 热力图
    StatsScatterplot,   ///< 散点图（统计版）
    StatsBarplot,       ///< 柱状图（统计版）
    StatsRegplot,       ///< 回归图
    StatsECDFplot,      ///< 经验累积分布图
    Unknow = 1000
};
}

#endif  // DAFIGUREAPI_H
