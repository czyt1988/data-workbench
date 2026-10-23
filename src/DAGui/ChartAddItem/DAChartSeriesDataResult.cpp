#include "DAChartSeriesDataResult.h"
// DA
#include "DAAutoincrementSeries.hpp"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
// std
#include <algorithm>
#include <exception>
#include <iterator>
#include <utility>
#include <vector>

namespace DA
{
namespace
{
// 取一列数据，data 为空或不是 dataframe 时返回空序列
DAPySeries seriesOf(const DAData& data, const QString& column)
{
    if (data.isNull()) {
        return DAPySeries();
    }
    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        return DAPySeries();
    }
    return df[ column ];
}

// 记录转换异常
void setConversionError(DAChartSeriesDataError& error, const std::exception& e)
{
    error.code   = DAChartSeriesDataError::ConversionFailed;
    error.detail = QString::fromUtf8(e.what());
}

// 把 pandas.Series 转换为 double 数组
bool castToDouble(const DAPySeries& series, std::vector< double >& out, DAChartSeriesDataError& error)
{
    out.clear();
    try {
        out.reserve(series.size());
        series.castTo< double >(std::back_inserter(out));
    } catch (const std::exception& e) {
        setConversionError(error, e);
        return false;
    }
    return true;
}
}  // namespace

/**
 * @brief 按配置提取单条 xy 点列
 *
 * 长度取 x 与 y 的较小值（与原有单序列行为一致）。
 * 本函数不访问任何界面元素，可用于脚本、智能体与单元测试。
 */
DAChartSeriesResult extractXYSeries(const DAChartXYSeriesSpec& spec)
{
    DAChartSeriesResult result;
    DAChartSeriesDataError& error = result.error;
    if (spec.xAutoincrement && spec.yAutoincrement) {
        error.code = DAChartSeriesDataError::BothAutoincrement;
        return result;
    }
    if (spec.xAutoincrement) {
        const DAAutoincrementSeries< double > xinc(spec.xBase, spec.xStep);
        if (spec.ySeries.isEmpty()) {
            error.code = DAChartSeriesDataError::NoYSeries;
            return result;
        }
        const QPair< DAData, QString >& yPick = spec.ySeries.first();
        const DAPySeries y = seriesOf(yPick.first, yPick.second);
        if (y.isNone()) {
            error.code       = DAChartSeriesDataError::SeriesNotConvertible;
            error.seriesName = yPick.second;
            return result;
        }
        std::vector< double > vy;
        if (!castToDouble(y, vy, error)) {
            return result;
        }
        result.points.resize(static_cast< int >(vy.size()));
        for (int i = 0; i < static_cast< int >(vy.size()); ++i) {
            result.points[ i ].setX(xinc[ static_cast< std::size_t >(i) ]);
            result.points[ i ].setY(vy[ static_cast< std::size_t >(i) ]);
        }
        return result;
    }
    if (spec.yAutoincrement) {
        const DAPySeries x = seriesOf(spec.xData, spec.xColumn);
        if (x.isNone()) {
            error.code = DAChartSeriesDataError::NoXSeries;
            return result;
        }
        std::vector< double > vx;
        if (!castToDouble(x, vx, error)) {
            return result;
        }
        const DAAutoincrementSeries< double > yinc(spec.yBase, spec.yStep);
        result.points.resize(static_cast< int >(vx.size()));
        for (int i = 0; i < static_cast< int >(vx.size()); ++i) {
            result.points[ i ].setX(vx[ static_cast< std::size_t >(i) ]);
            result.points[ i ].setY(yinc[ static_cast< std::size_t >(i) ]);
        }
        return result;
    }
    const DAPySeries x = seriesOf(spec.xData, spec.xColumn);
    const DAPySeries y = spec.ySeries.isEmpty() ? DAPySeries() : seriesOf(spec.ySeries.first().first, spec.ySeries.first().second);
    if (x.isNone()) {
        error.code = DAChartSeriesDataError::NoXSeries;
        return result;
    }
    if (y.isNone()) {
        error.code = DAChartSeriesDataError::NoYSeries;
        return result;
    }
    std::vector< double > vx;
    std::vector< double > vy;
    if (!castToDouble(x, vx, error) || !castToDouble(y, vy, error)) {
        return result;
    }
    const std::size_t size = std::min(vx.size(), vy.size());
    result.points.resize(static_cast< int >(size));
    for (std::size_t i = 0; i < size; ++i) {
        result.points[ static_cast< int >(i) ].setX(vx[ i ]);
        result.points[ static_cast< int >(i) ].setY(vy[ i ]);
    }
    return result;
}

/**
 * @brief 按配置提取多条 xy 点列（1 个 x + 多个 y）
 *
 * x 为实际序列时要求每条 y 与 x 等长；x 自增时按每条 y 的长度生成 x；
 * y 自增时只生成一条曲线，命名固定为 "y"。
 * 本函数不访问任何界面元素，可用于脚本、智能体与单元测试。
 */
DAChartMultiSeriesResult extractMultiXYSeries(const DAChartXYSeriesSpec& spec)
{
    DAChartMultiSeriesResult result;
    DAChartSeriesDataError& error = result.error;
    if (spec.xAutoincrement && spec.yAutoincrement) {
        error.code = DAChartSeriesDataError::BothAutoincrement;
        return result;
    }
    if (!spec.yAutoincrement && spec.ySeries.isEmpty()) {
        error.code = DAChartSeriesDataError::NoYSeries;
        return result;
    }
    // 先准备 x
    DAAutoincrementSeries< double > xinc;
    std::vector< double > vx;
    std::size_t xSize = 0;
    if (spec.xAutoincrement) {
        xinc = DAAutoincrementSeries< double >(spec.xBase, spec.xStep);
    } else {
        const DAPySeries x = seriesOf(spec.xData, spec.xColumn);
        if (x.isNone()) {
            error.code = DAChartSeriesDataError::NoXSeries;
            return result;
        }
        if (!castToDouble(x, vx, error)) {
            return result;
        }
        xSize = vx.size();
    }
    // y 自增：只有一条曲线，长度同 x
    if (spec.yAutoincrement) {
        const DAAutoincrementSeries< double > yinc(spec.yBase, spec.yStep);
        QVector< QPointF > points(static_cast< int >(xSize));
        for (std::size_t i = 0; i < xSize; ++i) {
            points[ static_cast< int >(i) ].setX(vx[ i ]);
            points[ static_cast< int >(i) ].setY(yinc[ i ]);
        }
        result.series.append(points);
        result.names.append(QStringLiteral("y"));
        return result;
    }
    // 逐条 y 生成
    for (const QPair< DAData, QString >& yPick : std::as_const(spec.ySeries)) {
        const DAPySeries y = seriesOf(yPick.first, yPick.second);
        if (y.isNone()) {
            error.code       = DAChartSeriesDataError::SeriesNotConvertible;
            error.seriesName = yPick.second;
            return result;
        }
        std::vector< double > vy;
        if (!castToDouble(y, vy, error)) {
            return result;
        }
        if (spec.xAutoincrement) {
            const int size = static_cast< int >(vy.size());
            QVector< QPointF > points(size);
            for (int i = 0; i < size; ++i) {
                points[ i ].setX(xinc[ static_cast< std::size_t >(i) ]);
                points[ i ].setY(vy[ static_cast< std::size_t >(i) ]);
            }
            result.series.append(points);
            result.names.append(yPick.second);
            continue;
        }
        if (vy.size() != xSize) {
            error.code          = DAChartSeriesDataError::LengthMismatch;
            error.seriesName    = yPick.second;
            error.seriesSize    = static_cast< qulonglong >(vy.size());
            error.xSeriesName   = spec.xColumn;
            error.xSize         = static_cast< qulonglong >(xSize);
            return result;
        }
        QVector< QPointF > points(static_cast< int >(xSize));
        for (std::size_t i = 0; i < xSize; ++i) {
            points[ static_cast< int >(i) ].setX(vx[ i ]);
            points[ static_cast< int >(i) ].setY(vy[ i ]);
        }
        result.series.append(points);
        result.names.append(yPick.second);
    }
    return result;
}

}  // end DA
