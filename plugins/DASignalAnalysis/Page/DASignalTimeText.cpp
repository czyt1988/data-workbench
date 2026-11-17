#include "DASignalTimeText.h"

namespace DA
{
namespace
{
/// 无播放源/无效时间的占位文本
const QString& timeTextPlaceholder()
{
    static const QString s_placeholder = QStringLiteral("--:--.-");
    return s_placeholder;
}
}  // namespace

/**
 * @brief 把毫秒时长格式化为「分:秒.十分之一秒」
 *
 * 实现口径：先按 100 毫秒（十分之一秒）四舍五入量化，再拆出分 / 秒 / 十分位，
 * 避免浮点误差在不同平台上出现 `59.9` → `60.0` 的进位差异（纯整数运算）。
 */
QString daSignalFormatTimeText(qint64 ms)
{
    if (ms < 0) {
        return timeTextPlaceholder();
    }
    // 量化到十分之一秒：四舍五入（+50 再整除）
    const qint64 deciseconds = (ms + 50) / 100;
    const qint64 minutes     = deciseconds / 600;
    const qint64 seconds     = (deciseconds / 10) % 60;
    const qint64 tenths      = deciseconds % 10;
    return QStringLiteral("%1:%2.%3")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(tenths);
}

/**
 * @brief 播放器控制条的「当前 / 总长」时间文本
 */
QString daSignalFormatTimeRange(qint64 positionMs, qint64 durationMs, bool hasSource)
{
    if (!hasSource) {
        return timeTextPlaceholder() + QStringLiteral(" / ") + timeTextPlaceholder();
    }
    const qint64 duration = qMax< qint64 >(0, durationMs);
    const qint64 position = qBound< qint64 >(0, positionMs, duration);
    return daSignalFormatTimeText(position) + QStringLiteral(" / ") + daSignalFormatTimeText(duration);
}
}  // namespace DA