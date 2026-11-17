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
 *
 * 同时导出纯函数（daWaterfallTraceTimes / daNearestTraceIndex / daStftToDb），
 * 便于单测「迹线时间映射」「当前迹线槽位」「dB 迹线换算」。
 */
class DAWaterfallBuilder
{
public:
    // 从 STFT 显示栅格构造瀑布迹线：maxTraces 时间方向最大迹线数、maxBins 频率方向最大频点数；
    // traces/binValues 为输出（binValues 是每条迹线的横坐标/频率）。栅格无效返回 false；
    // traceTimes 可选：输出每条迹线对应的时间（秒，含时间原点），与 traces 一一对应。
    static bool build(const DAStftResult& stft,
                      int maxTraces,
                      int maxBins,
                      QVector< QVector< double > >& traces,
                      QVector< double >& binValues,
                      QVector< double >* traceTimes = nullptr);
};

/**
 * @brief 每条迹线对应的时间（秒，含时间原点）
 *
 * 与 DAWaterfallBuilder::build 的抽帧规则完全一致（取区间代表列），
 * 供「当前帧迹线高亮」把当前时间映射到槽位。
 * @param stft 时频栅格
 * @param maxTraces 时间方向最大迹线数
 * @param timeOrigin 信号时间原点（秒）
 * @return 迹线时间列表；栅格无效返回空列表
 */
QVector< double > daWaterfallTraceTimes(const DAStftResult& stft, int maxTraces, double timeOrigin);

/**
 * @brief 当前时间最近的迹线槽位
 * @param times 迹线时间列表（升序）
 * @param t 当前时间（秒，含时间原点）
 * @return 槽位下标；times 为空返回 -1；t 越界钳制到首/尾
 */
int daNearestTraceIndex(const QVector< double >& times, double t);

/**
 * @brief 幅值 → dB（峰值归一，clamp 到 [floor, 0]）
 *
 * dB = 20·log10(v / peak)；peak <= 0 时按 1.0 兜底；非有限值或 v <= 0 取 floor。
 * @param values 线性幅值
 * @param peak 归一化峰值（整份栅格的峰值，与云图同口径）
 * @param floor dB 下限（应 < 0，否则按 -1 兜底）
 * @return dB 值列表（长度与输入一致）
 */
QVector< double > daStftToDb(const QVector< double >& values, double peak, double floor);
}  // namespace DA

#endif  // DAWATERFALLBUILDER_H
