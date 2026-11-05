#ifndef DAPEAKRESULT_H
#define DAPEAKRESULT_H
#include <QVector>
#include <cmath>

namespace DA
{
/**
 * @brief 整段频谱的前 N 个峰（峰值表数据，方案 §8.3）
 *
 * 列：序号、频率、幅值、相对主峰（dB）、占比（%）。
 * 幅值按计算口径保存（峰值幅度），显示层按幅值模式换算。
 */
struct DAPeakResult
{
    QVector< double > frequencies;  ///< 频率（赫兹）
    QVector< double > magnitudes;   ///< 幅值（峰值幅度口径）
    QVector< double > relDb;        ///< 相对主峰（分贝，主峰为 0）
    QVector< double > share;        ///< 功率占比（%，该峰幅值² / 全部峰幅值² 之和）
    double resolutionHz { 0.0 };

    bool isValid() const
    {
        return !frequencies.isEmpty() && frequencies.size() == magnitudes.size();
    }
};

/**
 * @brief 每帧前 N 峰值曲线（方案 D10）
 *
 * 第 k 条曲线 = 每帧第 k 名的频率随时间变化；某帧不足 N 个峰时该名次为 NaN（曲线断开）。
 * freqByRank 为行主序：rank * nFrames + frame。
 */
struct DAPeakCurveResult
{
    QVector< double > times;      ///< 帧中心时间（秒），长度 = nFrames
    QVector< double > freqByRank; ///< N × nFrames 频率（NaN 表示缺失）
    int topN { 0 };
    int nFrames { 0 };
    int nperseg { 0 };

    bool isValid() const
    {
        return topN > 0 && nFrames > 0 && freqByRank.size() == topN * nFrames && times.size() == nFrames;
    }

    double at(int rank, int frame) const
    {
        if (rank < 0 || rank >= topN || frame < 0 || frame >= nFrames) {
            return std::nan("");
        }
        return freqByRank[ rank * nFrames + frame ];
    }
};
}  // namespace DA

#endif  // DAPEAKRESULT_H
