#ifndef DASTFTRASTERDATA_H
#define DASTFTRASTERDATA_H
#include <QVector>
#include "qwt_matrix_raster_data.h"

namespace DA
{
/**
 * @brief 时频热图栅格数据（方案 §7.3）
 *
 * 继承 QwtMatrixRasterData：矩阵按「行 = 频率、列 = 时间」的行主序布局，
 * 并允许显式指定 Z 轴（色标）上下限，用于压 dB 动态范围。
 *
 * 不用 DAChartWidget::addSpectrogram(QwtGridRasterData*)：该数据类型的 Z 区间
 * 固定为数据实际范围，无法手动控制色标上下限。
 */
class DAStftRasterData : public QwtMatrixRasterData
{
public:
    DAStftRasterData();
    ~DAStftRasterData() override;

    // 填充栅格：flatRowMajor 为行主序矩阵（values[row*cols+col]，行 0 = 最低频率，列 0 = 最早时间）
    void setGrid(const QVector< double >& flatRowMajor,
                 int rows,
                 int cols,
                 double timeMin,
                 double timeMax,
                 double freqMin,
                 double freqMax);

    // 设置色标（Z 轴）上下限；任何一端为 NaN 时恢复由数据自动确定
    void setValueRange(double vmin, double vmax);

    int rowCount() const;
    int columnCount() const;

private:
    double mAutoMin { 0.0 };
    double mAutoMax { 0.0 };
};
}  // namespace DA

#endif  // DASTFTRASTERDATA_H
