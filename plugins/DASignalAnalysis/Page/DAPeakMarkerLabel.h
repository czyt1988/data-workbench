#ifndef DAPEAKMARKERLABEL_H
#define DAPEAKMARKERLABEL_H
#include <QString>

namespace DA
{
/**
 * @brief 峰值标记文本模板的占位符（丝带「标记显示」对话框与视图共用）
 *
 * 模板里这三个占位符会被替换成峰值排序号、频率、幅值，例如默认模板
 * `{index}-{fre} Hz` 渲染为 `1-32.5 Hz`。未知占位符原样保留，用户可以用
 * 备注/单位等自由文本包围占位符。
 */
namespace DAPeakMarkerLabel
{
/// 排序号占位符（1 起）
inline constexpr const char* cIndexPlaceholder = "{index}";
/// 频率占位符（赫兹）
inline constexpr const char* cFrequencyPlaceholder = "{fre}";
/// 幅值占位符（按当前幅值显示口径换算后的值；dB 模式自动带 " dB"）
inline constexpr const char* cAmplitudePlaceholder = "{amp}";
}  // namespace DAPeakMarkerLabel

// 默认峰值标记文本模板（序号-频率，例如 1-32.5 Hz）
QString daDefaultPeakMarkerText();
// 紧凑数字格式：最多保留 maxDecimals 位小数并去掉无意义的尾零（32.50 → 32.5，100.00 → 100）；
// 极大/极小值改用科学计数法，避免标记文本过长
QString daFormatCompactNumber(double value, int maxDecimals = 2);
// 渲染峰值标记文本：替换 {index}/{fre}/{amp}；dbMode 为 true 时幅值追加 " dB"；
// 空模板返回空串（视图据此不显示标记文本）
QString daFormatPeakMarkerText(const QString& textTemplate,
                               int index,
                               double frequencyHz,
                               double amplitude,
                               bool dbMode);
}  // namespace DA

#endif  // DAPEAKMARKERLABEL_H
