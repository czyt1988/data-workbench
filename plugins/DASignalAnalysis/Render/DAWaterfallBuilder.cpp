#include "DAWaterfallBuilder.h"
#include <cmath>
#include <limits>

namespace DA
{
namespace
{
/**
 * @brief 第 t 条迹线取用的 STFT 列
 *
 * 把 columns 列均匀抽到 maxTraces 条迹线，取该区间的代表列（区间中点）。
 * build 与 daWaterfallTraceTimes 共用，保证「迹线 ↔ 时间」完全对齐。
 */
int waterfallTraceColumn(int columns, int maxTraces, int t)
{
    const int colStart = int(std::lround(double(t) * columns / maxTraces));
    const int colEnd   = int(std::lround(double(t + 1) * columns / maxTraces));
    return qBound(0, (colStart + qMax(colStart + 1, colEnd) - 1) / 2, columns - 1);
}

/// 单个幅值 → dB（peak 归一、clamp 到 [floor, 0]；非有限/非正取 floor）
double stftValueToDb(double v, double peak, double floor)
{
    if (!std::isfinite(v) || v <= 0.0) {
        return floor;
    }
    const double db = 20.0 * std::log10(v / peak);
    if (!std::isfinite(db)) {
        return floor;
    }
    return qBound(floor, db, 0.0);
}

/// dB 下限的有效值：[floor, 0] 必须是有效区间（floor >= 0 时按 -1 兜底）
double effectiveDbFloor(double floor)
{
    return (floor < 0.0) ? floor : -1.0;
}
}  // namespace

/**
 * @brief 抽帧抽频点构造瀑布迹线
 *
 * 时间方向：把 columns 列均匀抽到 maxTraces 条迹线（取区间代表列）；
 * 频率方向：把 rows 行均匀抽到 maxBins 个频点（每段取最大值，避免窄峰被抹平）。
 * @param stft 时频栅格
 * @param maxTraces 时间方向最大迹线数
 * @param maxBins 频率方向最大频点数
 * @param traces 输出：逐条迹线（长度 maxBins）
 * @param binValues 输出：每条迹线的横坐标（频率）
 * @param traceTimes 可选输出：每条迹线的代表时间（秒，含时间原点）
 * @return 栅格有效且产出至少一条迹线时返回 true
 */
bool DAWaterfallBuilder::build(const DAStftResult& stft,
                               int maxTraces,
                               int maxBins,
                               QVector< QVector< double > >& traces,
                               QVector< double >& binValues,
                               QVector< double >* traceTimes)
{
    traces.clear();
    binValues.clear();
    if (traceTimes) {
        traceTimes->clear();
    }
    if (!stft.isValid()) {
        return false;
    }
    maxTraces = qBound(1, maxTraces, stft.columns);
    maxBins   = qBound(1, maxBins, stft.rows);

    // 频率方向抽点（取每段最大值）
    QVector< int > binEdges(maxBins + 1);
    for (int i = 0; i <= maxBins; ++i) {
        binEdges[ i ] = int(std::lround(double(i) * stft.rows / maxBins));
    }
    binValues.resize(maxBins);
    for (int i = 0; i < maxBins; ++i) {
        const int s = qBound(0, binEdges[ i ], stft.rows - 1);
        const int e = qBound(s + 1, binEdges[ i + 1 ], stft.rows);
        binValues[ i ] = stft.freqs.value((s + e - 1) / 2);
    }

    // 时间方向抽帧
    traces.reserve(maxTraces);
    if (traceTimes) {
        traceTimes->reserve(maxTraces);
    }
    for (int t = 0; t < maxTraces; ++t) {
        const int col = waterfallTraceColumn(stft.columns, maxTraces, t);
        QVector< double > trace(maxBins);
        for (int b = 0; b < maxBins; ++b) {
            double best = 0.0;
            const int s = qBound(0, binEdges[ b ], stft.rows - 1);
            const int e = qBound(s + 1, binEdges[ b + 1 ], stft.rows);
            for (int r = s; r < e; ++r) {
                const double v = stft.values.value(r * stft.columns + col);
                if (std::isfinite(v)) {
                    best = qMax(best, v);
                }
            }
            trace[ b ] = best;
        }
        traces.append(trace);
        if (traceTimes) {
            traceTimes->append(stft.times.value(col));
        }
    }
    return !traces.isEmpty();
}

/**
 * @brief 每条迹线对应的时间（秒，含时间原点）
 */
QVector< double > daWaterfallTraceTimes(const DAStftResult& stft, int maxTraces, double timeOrigin)
{
    QVector< double > times;
    if (!stft.isValid()) {
        return times;
    }
    const int count = qBound(1, maxTraces, stft.columns);
    times.reserve(count);
    for (int t = 0; t < count; ++t) {
        const int col = waterfallTraceColumn(stft.columns, count, t);
        times.append(stft.times.value(col) + timeOrigin);
    }
    return times;
}

/**
 * @brief 当前时间最近的迹线槽位
 */
int daNearestTraceIndex(const QVector< double >& times, double t)
{
    if (times.isEmpty()) {
        return -1;
    }
    int best    = 0;
    double bestD = std::numeric_limits< double >::infinity();
    for (int i = 0; i < times.size(); ++i) {
        const double d = std::fabs(times.at(i) - t);
        if (d < bestD) {
            bestD = d;
            best  = i;
        }
    }
    return best;
}

/**
 * @brief 幅值 → dB（峰值归一，clamp 到 [floor, 0]）
 */
QVector< double > daStftToDb(const QVector< double >& values, double peak, double floor)
{
    const double effectiveFloor = effectiveDbFloor(floor);
    const double reference      = (std::isfinite(peak) && peak > 0.0) ? peak : 1.0;
    QVector< double > out;
    out.reserve(values.size());
    for (double v : values) {
        out.append(stftValueToDb(v, reference, effectiveFloor));
    }
    return out;
}
}  // namespace DA
