#ifndef DADUALCURSORREADOUT_H
#define DADUALCURSORREADOUT_H
#include <QVector>
#include "Analysis/DAStftResult.h"

namespace DA
{
/**
 * @brief 双光标读数（纯数据，便于单测）
 *
 * 时间轴（波形/云图）：A/B 时间、Δt（有符号）与 1/|Δt|（等效频率）；
 * 频谱轴：A/B 频率、A/B 幅值（对已绘制曲线线性插值）、Δf 与 ΔA。
 */
struct DADualCursorReadout
{
    bool valid { false };        ///< 读数是否有效（两个游标都已放置）
    bool timeAxis { true };      ///< true=时间轴（波形/云图）/ false=频率轴（频谱）
    double a { 0.0 };            ///< A 位置（秒，含时间原点 / 赫兹）
    double b { 0.0 };            ///< B 位置（秒，含时间原点 / 赫兹）
    double delta { 0.0 };        ///< B - A（有符号）
    double equivalentHz { 0.0 }; ///< 时间轴：1/|Δt|（Δ=0 时为 0）
    bool hasValues { false };    ///< 频谱轴：是否有 A/B 幅值
    double aValue { 0.0 };       ///< A 处幅值（显示口径）
    double bValue { 0.0 };       ///< B 处幅值（显示口径）
    double deltaValue { 0.0 };   ///< B - A 的幅值差
    bool hasPeakFrequency { false };  ///< 云图：A 所在列的主峰频率是否可用
    double peakFrequency { 0.0 };     ///< A 所在列的主峰频率（赫兹）
};

/// 时间轴读数：x 值已含时间原点；Δt 有符号，等效频率用 |Δt|
DADualCursorReadout daDualCursorReadoutTime(double a, double b);

/// 频谱轴读数：A/B 频率按已绘制曲线线性插值取幅值（越界钳制到端点）
DADualCursorReadout daDualCursorReadoutSpectrum(double aFreq,
                                                double bFreq,
                                                const QVector< double >& curveFreqs,
                                                const QVector< double >& curveValues);

/// 线性插值（xs 单调升序；x 越界钳到首/尾；曲线为空或长度不匹配返回 false）
double daInterpolateCurve(const QVector< double >& xs, const QVector< double >& ys, double x, bool* ok = nullptr);

/// STFT 某一列的主峰频率（列号越界钳制；栅格无效返回 false）
double daStftColumnPeakFrequency(const DAStftResult& stft, int column, bool* ok = nullptr);
}  // namespace DA

#endif  // DADUALCURSORREADOUT_H