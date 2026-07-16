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
    Unknow = 1000
};
}

#endif  // DAFIGUREAPI_H
