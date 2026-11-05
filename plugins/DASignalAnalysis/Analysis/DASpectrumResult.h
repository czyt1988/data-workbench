#ifndef DASPECTRUMRESULT_H
#define DASPECTRUMRESULT_H
#include <QVector>
#include <QString>

namespace DA
{
/**
 * @brief 频谱结果（幅度谱 + 韦尔奇功率谱）
 *
 * 幅值谱按计算口径保存（峰值幅度或均方根，未取 dB）；显示层按当前幅值模式换算。
 */
struct DASpectrumResult
{
    QVector< double > freqs;       ///< 幅值谱频率轴（赫兹）
    QVector< double > amplitudes;  ///< 幅值谱（peak 或 rms 口径）
    QVector< double > psdFreqs;    ///< 功率谱频率轴（赫兹）
    QVector< double > psdValues;   ///< 功率谱密度（幅值²/赫兹）
    int segLen { 0 };              ///< 实际段长（采样点数）
    double resolutionHz { 0.0 };   ///< 实际频率分辨率（段长决定）
    QString window;                ///< 使用的窗函数
    bool rms { false };            ///< 幅值谱是否为均方根口径

    bool isValid() const
    {
        return !freqs.isEmpty() && freqs.size() == amplitudes.size();
    }
};
}  // namespace DA

#endif  // DASPECTRUMRESULT_H
