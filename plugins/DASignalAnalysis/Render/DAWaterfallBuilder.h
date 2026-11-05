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
    /**
     * @brief 从 STFT 显示栅格构造瀑布迹线
     * @param stft 时频显示栅格（行=频率、列=时间）
     * @param maxTraces 时间方向最大迹线数
     * @param maxBins 频率方向最大频点数
     * @param traces 输出迹线集合（每条为一条时间切片的频率幅值）
     * @param binValues 输出每条迹线的横坐标（频率）
     * @return 成功返回 true；栅格无效返回 false
     */
    static bool build(const DAStftResult& stft,
                      int maxTraces,
                      int maxBins,
                      QVector< QVector< double > >& traces,
                      QVector< double >& binValues);
};
}  // namespace DA

#endif  // DAWATERFALLBUILDER_H
