#include "DAPeakMarkerSamples.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace DA
{
namespace
{
/// 吸附邻域半径（样本数）：峰值表频率与曲线离散频点的偏差通常只有一两个 bin，
/// 邻域取 ±2 可覆盖零填充/峰值插值带来的偏差，又不会跳到相邻的另一个峰上
constexpr int cSnapRadius = 2;

/// 频率比较容差：判断峰值频率是否正好落在某个曲线样本上（Hz）
constexpr double cFrequencyEps = 1e-9;

/**
 * @brief 样本 i 是否为局部极大点（忽略非有限邻居）
 *
 * 只是「正好命中曲线样本」这条捷径的判定条件：命中的样本本身是局部极大时保持不动，
 * 不被邻域里等值/噪声样本带偏。
 */
bool isLocalMaxAt(const QVector< double >& values, int i)
{
    if (i < 0 || i >= values.size()) {
        return false;
    }
    const double cur = values.at(i);
    if (!std::isfinite(cur)) {
        return false;
    }
    if (i > 0) {
        const double left = values.at(i - 1);
        if (std::isfinite(left) && cur < left) {
            return false;
        }
    }
    if (i + 1 < values.size()) {
        const double right = values.at(i + 1);
        if (std::isfinite(right) && cur < right) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 在 [begin, end] 邻域里找数值最大的样本下标（并列时取离 nearest 最近的）
 * @return 有效样本下标；窗口内没有有限值返回 -1
 */
int findLocalMaxIndex(const QVector< double >& values, int begin, int end, int nearest)
{
    double maxValue = 0.0;
    bool hasValue   = false;
    for (int i = begin; i <= end; ++i) {
        const double v = values.at(i);
        if (!std::isfinite(v)) {
            continue;
        }
        if (!hasValue || v > maxValue) {
            hasValue = true;
            maxValue = v;
        }
    }
    if (!hasValue) {
        return -1;
    }
    // 并列极大：取离峰值频率最近的那个样本，避免标记在噪声平台上来回偏移
    int best      = -1;
    int bestDelta = std::numeric_limits< int >::max();
    for (int i = begin; i <= end; ++i) {
        if (values.at(i) != maxValue) {
            continue;
        }
        const int delta = qAbs(i - nearest);
        if (best < 0 || delta < bestDelta) {
            best      = i;
            bestDelta = delta;
        }
    }
    return best;
}

/// 曲线频率是否严格递增（lower_bound 与「超出范围即跳过」的前提）
bool isAscending(const QVector< double >& freqs)
{
    for (int i = 1; i < freqs.size(); ++i) {
        if (!std::isfinite(freqs.at(i)) || !(freqs.at(i) > freqs.at(i - 1))) {
            return false;
        }
    }
    return true;
}
}  // namespace

/**
 * @brief 把峰值标记吸附到当前已绘制频谱曲线的峰顶
 *
 * 峰值表给出的频率来自计算层（可能带插值/零填充），与频谱图按离散频点绘制的曲线
 * 存在一两个 bin 的偏差；直接按「峰值频率 + 幅值换算」画点会落在曲线旁边（用户报告的
 * 峰值标记偏离曲线问题）。这里对每个峰值频率：
 * 1. 在曲线频率轴上用 std::lower_bound 找到最近样本 i（对数轴已由调用方裁掉 0 Hz 起点，
 *    曲线频率本身单调递增即可）；
 * 2. 以 i 为中心取半径为 cSnapRadius 的邻域，取邻域内曲线数值最大的样本作为峰顶；
 *    i 正好落在样本点、且该样本本身是局部极大时保持不动；
 * 3. 峰值频率落在曲线频率范围外、曲线为空/长度不一致/频率非单调、曲线值为 NaN 时
 *    跳过该峰（结果条数可能少于输入峰数）。
 *
 * 复杂度 O(N·k + M)（N=峰数、M=曲线点数、k=邻域半径），可安全用于上万点的频谱曲线。
 * 输出保持输入峰的先后顺序（被跳过的峰不占位）。
 *
 * @param curveFreqs 已绘制曲线的频率（赫兹，严格递增）
 * @param curveValues 已绘制曲线的显示值（与频率一一对应）
 * @param peakFreqs 峰值表频率（赫兹）
 * @return 每个可放置峰值的标记坐标（x=曲线频率、y=曲线显示值）
 */
QVector< QPointF > snapPeakMarkersToCurve(const QVector< double >& curveFreqs,
                                          const QVector< double >& curveValues,
                                          const QVector< double >& peakFreqs)
{
    QVector< QPointF > markers;
    const int curveCount = curveFreqs.size();
    if (curveCount <= 0 || curveValues.size() != curveCount || peakFreqs.isEmpty()) {
        return markers;
    }
    if (!isAscending(curveFreqs)) {
        return markers;
    }
    const double freqMin = curveFreqs.first();
    const double freqMax = curveFreqs.last();
    markers.reserve(peakFreqs.size());
    for (double peakFreq : peakFreqs) {
        if (!std::isfinite(peakFreq)) {
            continue;
        }
        // 曲线范围外（含对数轴起点之前的 0 Hz 峰）：纯函数口径是跳过，显示层按原值兜底
        if (peakFreq < freqMin || peakFreq > freqMax) {
            continue;
        }
        // 最近样本：lower_bound 找第一个 >= peakFreq 的位置，再与左邻比较距离
        const auto it = std::lower_bound(curveFreqs.cbegin(), curveFreqs.cend(), peakFreq);
        int nearest   = int(it - curveFreqs.cbegin());
        if (nearest >= curveCount) {
            nearest = curveCount - 1;
        } else if (nearest > 0) {
            const double distHigh = curveFreqs.at(nearest) - peakFreq;
            const double distLow  = peakFreq - curveFreqs.at(nearest - 1);
            if (distLow <= distHigh) {
                --nearest;
            }
        }
        // 正好命中样本且样本自身是局部极大：保持不变（标记锚点稳定，不被邻居噪声带偏）
        if (std::fabs(curveFreqs.at(nearest) - peakFreq) <= cFrequencyEps && isLocalMaxAt(curveValues, nearest)) {
            markers.append(QPointF(curveFreqs.at(nearest), curveValues.at(nearest)));
            continue;
        }
        const int begin = qMax(0, nearest - cSnapRadius);
        const int end   = qMin(curveCount - 1, nearest + cSnapRadius);
        const int best  = findLocalMaxIndex(curveValues, begin, end, nearest);
        if (best < 0) {
            continue;  // 邻域全是 NaN/非有限值：跳过该峰
        }
        markers.append(QPointF(curveFreqs.at(best), curveValues.at(best)));
    }
    return markers;
}
}  // namespace DA
