#ifndef DAPEAKCURVEPLOTSAMPLES_H
#define DAPEAKCURVEPLOTSAMPLES_H
#include <QVector>
#include "Analysis/DAPeakResult.h"

namespace DA
{
/**
 * @brief 单条名次曲线的绘制采样（点 + 连续折线）
 *
 * 峰值曲线是「每帧独立取前 N」：同名次在相邻帧不一定是同一个峰，直接按帧连线会出现
 * 横跨频段的伪连线（噪声/名次互换时最明显）。这里把两种情况拆开：
 * - dotXs/dotYs：所有有效帧（某帧不足 N 个峰时该名次缺失，不画点）；
 * - lineXs/lineYs：只在相邻帧频率跳变不超过阈值时连线，跳变处插入 NaN 断开。
 *
 * 这样折线只表示「连续的峰」，跳变点仍以点保留，不丢数据。
 */
struct DAPeakCurvePlotSamples
{
    QVector< double > dotXs;   ///< 点：时间（秒）
    QVector< double > dotYs;   ///< 点：频率（赫兹）
    QVector< double > lineXs;  ///< 折线：时间（秒），断开处为 NaN
    QVector< double > lineYs;  ///< 折线：频率（赫兹），断开处为 NaN

    bool hasDots() const
    {
        return !dotXs.isEmpty();
    }
    bool hasLines() const
    {
        return !lineXs.isEmpty();
    }
};

/**
 * @brief 相邻帧「同一个峰」的频率跳变阈值（赫兹）
 *
 * 峰值频率的估计精度由频率分辨率 Δf = fs / nperseg 决定；跳变不超过 8 × Δf 视为连续的峰。
 * 采样率或段长未知时返回 0（表示不断开，退化为整条折线）。
 */
double daPeakCurveJumpThresholdHz(const DAPeakCurveResult& curves);

/**
 * @brief 生成某名次（0 基）的绘制采样
 * @param curves 峰值曲线结果
 * @param rank 名次（0 基，0 = 最强峰）
 * @param jumpThresholdHz 跳变阈值（赫兹），<= 0 表示不断开
 * @param timeOrigin 时间原点（秒）：时间列首值非 0 时与波形/热图横向对齐
 * @return 点与连续折线采样；rank 越界或结果无效时返回空
 */
DAPeakCurvePlotSamples daPeakCurvePlotSamples(const DAPeakCurveResult& curves,
                                              int rank,
                                              double jumpThresholdHz,
                                              double timeOrigin = 0.0);
}  // namespace DA

#endif  // DAPEAKCURVEPLOTSAMPLES_H
