#include "DAWaterfallBuilder.h"
#include <cmath>

namespace DA
{
/**
 * @brief 抽帧抽频点构造瀑布迹线
 *
 * 时间方向：把 columns 列均匀抽到 maxTraces 条迹线（取区间代表列）；
 * 频率方向：把 rows 行均匀抽到 maxBins 个频点（每段取最大值，避免窄峰被抹平）。
 */
bool DAWaterfallBuilder::build(const DAStftResult& stft,
                               int maxTraces,
                               int maxBins,
                               QVector< QVector< double > >& traces,
                               QVector< double >& binValues)
{
    traces.clear();
    binValues.clear();
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
    for (int t = 0; t < maxTraces; ++t) {
        const int colStart = int(std::lround(double(t) * stft.columns / maxTraces));
        const int colEnd   = int(std::lround(double(t + 1) * stft.columns / maxTraces));
        const int col      = qBound(0, (colStart + qMax(colStart + 1, colEnd) - 1) / 2, stft.columns - 1);
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
    }
    return !traces.isEmpty();
}
}  // namespace DA
