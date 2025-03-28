﻿#ifndef DAFIGUREAPI_H
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
enum class ChartTypes
{
	Curve,    ///< 曲线
	Scatter,  ///< 散点
	Bar,      ///< 柱状
	Unknow = 1000
};
}

#endif  // DAFIGUREAPI_H
