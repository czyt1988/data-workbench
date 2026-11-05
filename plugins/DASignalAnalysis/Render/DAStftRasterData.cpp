#include "DAStftRasterData.h"
#include "qwt_interval.h"
#include <cmath>
#include <limits>

namespace DA
{
DAStftRasterData::DAStftRasterData()
{
    // 放大时平滑（方案 §7.3）
    setResampleMode(QwtMatrixRasterData::BilinearInterpolation);
}

DAStftRasterData::~DAStftRasterData()
{
}

/**
 * @brief 填充栅格数据并设置坐标轴区间
 *
 * QwtMatrixRasterData 的矩阵布局：values[row * numColumns + col]，
 * 行对应 Y（频率），列对应 X（时间）（qwt_matrix_raster_data.h 约定）。
 * Z 轴（色标）区间由数据有限值的最小/最大值自动确定（qwt 不会自动计算 Z 区间）。
 */
void DAStftRasterData::setGrid(const QVector< double >& flatRowMajor,
                               int rows,
                               int cols,
                               double timeMin,
                               double timeMax,
                               double freqMin,
                               double freqMax)
{
    mAutoMin = std::numeric_limits< double >::quiet_NaN();
    mAutoMax = std::numeric_limits< double >::quiet_NaN();
    if (rows <= 0 || cols <= 0) {
        return;
    }
    setValueMatrix(flatRowMajor, cols);
    setInterval(Qt::XAxis, QwtInterval(timeMin, timeMax));
    setInterval(Qt::YAxis, QwtInterval(freqMin, freqMax));
    // 自动 Z 范围：忽略 NaN（无数据单元）
    double mn = std::numeric_limits< double >::infinity();
    double mx = -std::numeric_limits< double >::infinity();
    for (double v : flatRowMajor) {
        if (std::isfinite(v)) {
            mn = qMin(mn, v);
            mx = qMax(mx, v);
        }
    }
    if (mn <= mx) {
        mAutoMin = mn;
        mAutoMax = mx;
        setInterval(Qt::ZAxis, QwtInterval(mn, mx));
    }
}

/**
 * @brief 设置色标（Z 轴）上下限
 *
 * 任何一端为 NaN 或 vmin >= vmax 时回退到数据自动范围。
 */
void DAStftRasterData::setValueRange(double vmin, double vmax)
{
    if (std::isnan(vmin) || std::isnan(vmax) || vmin >= vmax) {
        if (!std::isnan(mAutoMin) && !std::isnan(mAutoMax)) {
            setInterval(Qt::ZAxis, QwtInterval(mAutoMin, mAutoMax));
        }
        return;
    }
    setInterval(Qt::ZAxis, QwtInterval(vmin, vmax));
}

int DAStftRasterData::rowCount() const
{
    return numRows();
}

int DAStftRasterData::columnCount() const
{
    return numColumns();
}
}  // namespace DA
