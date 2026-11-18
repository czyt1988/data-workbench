#include "DATimeScaleDraw.h"
#include <QLocale>
#include <QString>
#include <cmath>
#include "qwt_text.h"

namespace DA
{
/**
 * @brief 时间轴标签格式化
 *
 * - 秒：直接用 QLocale 的数值格式（与 Qwt 默认标签同源，切换格式后仍保持一致）；
 * - 毫秒：值 × 1000 取整；
 * - mm:ss.s：相对时间原点换算成「分钟:秒.十分位」，秒补足两位、十分位一位。
 * @param value 轴值（秒，含时间原点）
 * @param format 0=秒 / 1=毫秒 / 2=mm:ss.s
 * @param origin 信号时间原点（秒）
 * @return 标签文本
 */
QString daFormatTimeAxisLabel(double value, int format, double origin)
{
    switch (format) {
    case static_cast< int >(DATimeAxisFormat::Milliseconds): {
        return QString::number(value * 1000.0, 'f', 0);
    }
    case static_cast< int >(DATimeAxisFormat::MinutesSeconds): {
        double rel = value - origin;
        if (!(rel > 0.0)) {
            rel = 0.0;  // 原点之前（或无效值）统一显示 0:00.0
        }
        const qint64 totalTenths  = qint64(std::llround(rel * 10.0));
        const int tenths          = int(totalTenths % 10);
        const qint64 totalSeconds = totalTenths / 10;
        const int seconds         = int(totalSeconds % 60);
        const qint64 minutes      = totalSeconds / 60;
        return QStringLiteral("%1:%2.%3")
            .arg(minutes)
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(tenths);
    }
    case static_cast< int >(DATimeAxisFormat::Seconds):
    default:
        return QLocale().toString(value);
    }
}

DATimeScaleDraw::DATimeScaleDraw(int format, double origin) : mTimeFormat(format), mTimeOrigin(origin)
{
}

/**
 * @brief 按配置的格式输出刻度标签
 * @param value 轴值（秒）
 * @return 标签文本
 */
QwtText DATimeScaleDraw::label(double value) const
{
    return QwtText(daFormatTimeAxisLabel(value, mTimeFormat, mTimeOrigin));
}

/**
 * @brief 当前时间格式（0=秒 / 1=毫秒 / 2=mm:ss.s）
 */
int DATimeScaleDraw::timeFormat() const
{
    return mTimeFormat;
}

/**
 * @brief 当前时间原点（秒）
 */
double DATimeScaleDraw::timeOrigin() const
{
    return mTimeOrigin;
}
}  // namespace DA