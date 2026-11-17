#ifndef DASIGNALTIMETEXT_H
#define DASIGNALTIMETEXT_H
#include <QString>
#include <QtGlobal>

namespace DA
{
/**
 * @brief 把毫秒时长格式化为「分:秒.十分之一秒」（纯函数，便于单测）
 *
 * 播放器控制条的时间文本口径：`mm:ss.s`（如 `00:12.3`）。四舍五入到十分之一秒；
 * 负数视为无效，返回占位文本 `--:--.-`（无播放源时使用）。
 * @param ms 毫秒（<0 表示无效）
 * @return 形如 `01:30.5` 的时间文本；无效时返回占位文本
 */
QString daSignalFormatTimeText(qint64 ms);

/**
 * @brief 播放器控制条的「当前 / 总长」时间文本（纯函数）
 *
 * `hasSource=false`（未装入可播放媒体源）时整体返回 `--:--.- / --:--.-`；
 * 位置会被夹在 `[0, durationMs]` 内，避免后端偶发越界值显示成负数。
 * @param positionMs 当前播放位置（毫秒）
 * @param durationMs 总时长（毫秒）
 * @param hasSource 是否已装入可播放媒体源
 * @return `当前位置 / 总时长` 文本
 */
QString daSignalFormatTimeRange(qint64 positionMs, qint64 durationMs, bool hasSource);
}  // namespace DA

#endif  // DASIGNALTIMETEXT_H