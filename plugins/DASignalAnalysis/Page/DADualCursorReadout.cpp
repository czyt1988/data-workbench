#include "DADualCursorReadout.h"
#include <cmath>
#include <limits>

namespace DA
{
/**
 * @brief 时间轴读数
 *
 * Δt 保留符号（B 在 A 左边时为负），等效频率用 |Δt|：Δt 为 0 时给 0。
 * @param a A 游标位置（秒，含时间原点）
 * @param b B 游标位置（秒，含时间原点）
 * @return 读数
 */
DADualCursorReadout daDualCursorReadoutTime(double a, double b)
{
    DADualCursorReadout out;
    out.valid     = true;
    out.timeAxis  = true;
    out.a         = a;
    out.b         = b;
    out.delta     = b - a;
    const double absDelta = std::fabs(out.delta);
    out.equivalentHz      = (absDelta > 0.0) ? (1.0 / absDelta) : 0.0;
    return out;
}

/**
 * @brief 线性插值（xs 单调升序）
 *
 * 越界钳制到首/尾端点（读数不出现外推值）；空曲线或长度不一致返回 false。
 * @param xs 横坐标（升序）
 * @param ys 纵坐标
 * @param x 查询点
 * @param ok 可选输出：是否成功
 * @return 插值结果
 */
double daInterpolateCurve(const QVector< double >& xs, const QVector< double >& ys, double x, bool* ok)
{
    if (ok) {
        *ok = false;
    }
    if (xs.isEmpty() || xs.size() != ys.size()) {
        return 0.0;
    }
    if (x <= xs.first()) {
        if (ok) {
            *ok = true;
        }
        return ys.first();
    }
    if (x >= xs.last()) {
        if (ok) {
            *ok = true;
        }
        return ys.last();
    }
    // 二分找到 x 所在区间（xs 升序）
    int lo = 0;
    int hi = xs.size() - 1;
    while (hi - lo > 1) {
        const int mid = (lo + hi) / 2;
        if (xs.at(mid) <= x) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    const double x0 = xs.at(lo);
    const double x1 = xs.at(hi);
    const double span = x1 - x0;
    if (!(span > 0.0)) {
        if (ok) {
            *ok = true;
        }
        return ys.at(lo);
    }
    const double t = (x - x0) / span;
    if (ok) {
        *ok = true;
    }
    return ys.at(lo) + t * (ys.at(hi) - ys.at(lo));
}

/**
 * @brief 频谱轴读数：A/B 频率 + 对已绘制曲线插值取幅值
 * @param aFreq A 频率（赫兹）
 * @param bFreq B 频率（赫兹）
 * @param curveFreqs 当前已绘制曲线的频率（升序）
 * @param curveValues 当前已绘制曲线的幅值（显示口径）
 * @return 读数；没有曲线时 valid=true 但 hasValues=false
 */
DADualCursorReadout daDualCursorReadoutSpectrum(double aFreq,
                                                double bFreq,
                                                const QVector< double >& curveFreqs,
                                                const QVector< double >& curveValues)
{
    DADualCursorReadout out;
    out.valid    = true;
    out.timeAxis = false;
    out.a        = aFreq;
    out.b        = bFreq;
    out.delta    = bFreq - aFreq;
    bool aOk     = false;
    bool bOk     = false;
    const double aAmp = daInterpolateCurve(curveFreqs, curveValues, aFreq, &aOk);
    const double bAmp = daInterpolateCurve(curveFreqs, curveValues, bFreq, &bOk);
    if (aOk && bOk) {
        out.hasValues  = true;
        out.aValue     = aAmp;
        out.bValue     = bAmp;
        out.deltaValue = bAmp - aAmp;
    }
    return out;
}

/**
 * @brief STFT 某一列的主峰频率：该列所有频率行里幅值最大的那一行
 * @param stft 时频栅格
 * @param column 列号（越界钳制）
 * @param ok 可选输出：是否成功
 * @return 主峰频率（赫兹）
 */
double daStftColumnPeakFrequency(const DAStftResult& stft, int column, bool* ok)
{
    if (ok) {
        *ok = false;
    }
    if (!stft.isValid()) {
        return 0.0;
    }
    const int col = qBound(0, column, stft.columns - 1);
    int bestRow   = -1;
    double best   = -std::numeric_limits< double >::infinity();
    for (int r = 0; r < stft.rows; ++r) {
        const double v = stft.values.value(r * stft.columns + col);
        if (std::isfinite(v) && v > best) {
            best    = v;
            bestRow = r;
        }
    }
    if (bestRow < 0) {
        return 0.0;
    }
    if (ok) {
        *ok = true;
    }
    return stft.freqs.value(bestRow, 0.0);
}
}  // namespace DA