#ifndef DATIMESCALEDRAW_H
#define DATIMESCALEDRAW_H
#include <QString>
#include "qwt_scale_draw.h"

namespace DA
{
/**
 * @brief 时间轴标签格式（与视图状态 timeFormat 一一对应）
 */
enum class DATimeAxisFormat
{
    Seconds = 0,         ///< 秒（默认，与 Qwt 默认标签一致）
    Milliseconds = 1,    ///< 毫秒（值 × 1000，取整）
    MinutesSeconds = 2   ///< mm:ss.s（以信号时间原点为 0 点）
};

/**
 * @brief 时间轴标签格式化（纯函数，便于单测）
 *
 * 轴标题已带单位（Time (s) / Time (ms) / Time (mm:ss)），标签本身只出数值。
 * @param value 轴值（秒，含信号时间原点）
 * @param format 0=秒 / 1=毫秒 / 2=mm:ss.s；其它值按秒处理
 * @param origin 信号时间原点（秒）：mm:ss.s 以它为 0 点
 * @return 标签文本
 */
QString daFormatTimeAxisLabel(double value, int format, double origin);

/**
 * @brief 时间轴刻度标签绘制：按格式重写 Qwt 默认标签（波形 x 与云图 x 共用）
 *
 * 切换格式时不要复用旧实例：plot 接管 scaleDraw 所有权，换格式要 new 一个新实例
 * 交给 setAxisScaleDraw（旧实例随 plot 释放）。
 */
class DATimeScaleDraw : public QwtScaleDraw
{
public:
    DATimeScaleDraw(int format, double origin);
    ~DATimeScaleDraw() override = default;

    // 按格式格式化刻度标签
    QwtText label(double value) const override;

    int timeFormat() const;
    double timeOrigin() const;

private:
    int mTimeFormat { 0 };
    double mTimeOrigin { 0.0 };
};
}  // namespace DA

#endif  // DATIMESCALEDRAW_H
