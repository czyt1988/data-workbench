#include "DAPeakCurvePlotSamples.h"
#include <cmath>

namespace DA
{
namespace
{
/// 跳变阈值 = 8 × 频率分辨率（见 daPeakCurveJumpThresholdHz 的注释）
constexpr double cDAPeakCurveJumpFactor = 8.0;
}

/**
 * @brief 相邻帧「同一个峰」的频率跳变阈值（赫兹）
 *
 * 峰值频率的估计精度由频率分辨率 Δf = fs / nperseg 决定；跳变不超过 8 × Δf 视为连续的峰。
 * 采样率或段长未知时返回 0（表示不断开，退化为整条折线），保证旧数据仍能显示。
 */
double daPeakCurveJumpThresholdHz(const DAPeakCurveResult& curves)
{
    if (!(curves.sampleRateHz > 0.0) || curves.nperseg <= 0) {
        return 0.0;
    }
    return cDAPeakCurveJumpFactor * curves.sampleRateHz / double(curves.nperseg);
}

/**
 * @brief 生成某名次的绘制采样：所有有效帧取点，跳变不超过阈值的相邻帧才连线
 *
 * 缺失帧（该帧不足 N 个峰）与超阈值跳变都会断开折线；断开方式是在折线采样里插入
 * 一个 NaN 点，配合 QwtPlotCurve::BreakAtNan 渲染成独立的连续段。
 *
 * @param curves 峰值曲线结果
 * @param rank 名次（0 基，0 = 最强峰）
 * @param jumpThresholdHz 跳变阈值（赫兹），<= 0 表示不断开
 * @param timeOrigin 时间原点（秒）：时间列首值非 0 时与波形/热图横向对齐
 * @return 点与连续折线采样；rank 越界或结果无效时返回空
 */
DAPeakCurvePlotSamples daPeakCurvePlotSamples(const DAPeakCurveResult& curves,
                                              int rank,
                                              double jumpThresholdHz,
                                              double timeOrigin)
{
    DAPeakCurvePlotSamples samples;
    if (!curves.isValid() || rank < 0 || rank >= curves.topN) {
        return samples;
    }
    const int nFrames = curves.nFrames;
    samples.dotXs.reserve(nFrames);
    samples.dotYs.reserve(nFrames);
    samples.lineXs.reserve(nFrames + nFrames / 8);
    samples.lineYs.reserve(nFrames + nFrames / 8);

    bool hasPrev = false;
    double prevFreq = 0.0;
    for (int i = 0; i < nFrames; ++i) {
        const double freq = curves.at(rank, i);
        if (!std::isfinite(freq)) {
            // 该帧不足 N 个峰：不画点，也不把折线接到下一帧
            hasPrev = false;
            continue;
        }
        const double t = curves.times.at(i) + timeOrigin;
        samples.dotXs.append(t);
        samples.dotYs.append(freq);
        const bool continuous = hasPrev && (jumpThresholdHz <= 0.0 || std::fabs(freq - prevFreq) <= jumpThresholdHz);
        if (!continuous && !samples.lineXs.isEmpty()) {
            // 跳变/缺帧处插入 NaN 断开折线（同 x 上前后各一段，绘图时留空）
            samples.lineXs.append(t);
            samples.lineYs.append(std::nan(""));
        }
        samples.lineXs.append(t);
        samples.lineYs.append(freq);
        hasPrev  = true;
        prevFreq = freq;
    }
    return samples;
}
}  // namespace DA
