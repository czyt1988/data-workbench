#ifndef DACHARTSERIESDATARESULT_H
#define DACHARTSERIESDATARESULT_H
#include "DAGuiAPI.h"
#include <QPointF>
#include <QVector>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include "DAData.h"

namespace DA
{

/**
 * @brief 序列取数失败的原因
 *
 * 取数过程不依赖任何界面元素，失败原因通过本结构回传，
 * 由调用方决定是弹出界面提示、写日志还是返回给脚本/智能体。
 */
struct DAGUI_API DAChartSeriesDataError
{
    enum Code
    {
        NoError,               ///< 没有错误
        NoXSeries,             ///< 未选择 x 序列
        NoYSeries,             ///< 未选择 y 序列
        BothAutoincrement,     ///< x 与 y 同时启用自增
        InvalidXInitialValue,  ///< x 自增初值不是浮点数
        InvalidXStepValue,     ///< x 自增步长不是浮点数
        InvalidYInitialValue,  ///< y 自增初值不是浮点数
        InvalidYStepValue,     ///< y 自增步长不是浮点数
        SeriesNotConvertible,  ///< 序列为空值或无法转换为数值
        LengthMismatch,        ///< y 与 x 的长度不一致
        ConversionFailed       ///< 转换为数值过程中抛出异常
    };

    Code code { NoError };              ///< 失败原因
    QString seriesName;                 ///< 出错或相关的序列名
    qulonglong seriesSize { 0 };        ///< 相关序列长度
    QString xSeriesName;                ///< 长度校验时的 x 序列名
    qulonglong xSize { 0 };             ///< 长度校验时的 x 序列长度
    QString detail;                     ///< 附加信息（如异常文本）

    bool isOk() const { return code == NoError; }
};

/**
 * @brief 一次 xy 取数的输入描述
 *
 * 由界面层从控件状态收集，交给无界面的取数函数使用。
 */
struct DAGUI_API DAChartXYSeriesSpec
{
    DAData xData;                              ///< x 所属数据
    QString xColumn;                           ///< x 列名，为空表示使用自增序列
    QList< QPair< DAData, QString > > ySeries;  ///< y 序列列表；y 自增时为空
    bool xAutoincrement { false };              ///< x 是否使用自增序列
    bool yAutoincrement { false };              ///< y 是否使用自增序列
    double xBase { 0.0 };                       ///< x 自增初值
    double xStep { 1.0 };                       ///< x 自增步长
    double yBase { 0.0 };                       ///< y 自增初值
    double yStep { 1.0 };                       ///< y 自增步长
};

/**
 * @brief 单条 xy 序列的取数结果
 */
struct DAGUI_API DAChartSeriesResult
{
    QVector< QPointF > points;  ///< 取到的点列；失败时为空
    DAChartSeriesDataError error;
    bool isOk() const { return error.isOk(); }
};

/**
 * @brief 多条 xy 序列的取数结果（1 个 x + 多个 y）
 */
struct DAGUI_API DAChartMultiSeriesResult
{
    QList< QVector< QPointF > > series;  ///< 每组点列，与 names 一一对应
    QStringList names;                   ///< 每组点列的推荐命名
    DAChartSeriesDataError error;
    bool isOk() const { return error.isOk(); }
};

// 按配置提取单条 xy 点列（长度取 x、y 的较小值，与旧行为一致）
DAGUI_API DAChartSeriesResult extractXYSeries(const DAChartXYSeriesSpec& spec);
// 按配置提取多条 xy 点列（要求每条 y 与 x 等长）
DAGUI_API DAChartMultiSeriesResult extractMultiXYSeries(const DAChartXYSeriesSpec& spec);

}  // end DA

#endif  // DACHARTSERIESDATARESULT_H
