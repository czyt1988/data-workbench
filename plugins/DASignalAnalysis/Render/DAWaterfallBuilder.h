#ifndef DAWATERFALLBUILDER_H
#define DAWATERFALLBUILDER_H
#include <QVector>
#include "Analysis/DAStftResult.h"

namespace DA
{
/**
 * @brief 三维瀑布数据准备（方案 §7.4）
 *
 * 不喂全分辨率矩阵：时间方向抽帧（N≈200）、频率方向抽频点（M≈512，取每段最大值），
 * 逐帧生成一条迹线交给 Qwt3DWaterfall::pushTrace。
 */
class DAWaterfallBuilder
{
public:
    // 从 STFT 显示栅格构造瀑布迹线：maxTraces 时间方向最大迹线数、maxBins 频率方向最大频点数；
    // traces/binValues 为输出（binValues 是每条迹线的横坐标/频率）。栅格无效返回 false。
    static bool build(const DAStftResult& stft,
                      int maxTraces,
                      int maxBins,
                      QVector< QVector< double > >& traces,
                      QVector< double >& binValues);
};
}  // namespace DA

#endif  // DAWATERFALLBUILDER_H
