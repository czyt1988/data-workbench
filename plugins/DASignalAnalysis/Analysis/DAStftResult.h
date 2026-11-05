#ifndef DASTFTRESULT_H
#define DASTFTRESULT_H
#include <QVector>

namespace DA
{
/**
 * @brief 时频显示栅格结果（STFT 按显示精度池化后的数据）
 *
 * 行 = 频率（行 0 最低）、列 = 时间（列 0 最早）；values 为行主序线性幅值，
 * NaN 表示该单元无数据（信号短于窗长等）。
 */
struct DAStftResult
{
    QVector< double > times;   ///< 列中心时间（秒）
    QVector< double > freqs;   ///< 行中心频率（赫兹）
    QVector< double > values;  ///< 行主序幅值（rows * columns）
    int rows { 0 };
    int columns { 0 };
    int nperseg { 0 };
    int hop { 0 };
    double windowSeconds { 0.0 };  ///< 窗长（秒），波形矩形宽

    bool isValid() const
    {
        return rows > 0 && columns > 0 && values.size() == rows * columns && !times.isEmpty() && !freqs.isEmpty();
    }
};
}  // namespace DA

#endif  // DASTFTRESULT_H
