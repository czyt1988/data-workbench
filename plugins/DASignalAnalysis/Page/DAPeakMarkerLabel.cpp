#include "DAPeakMarkerLabel.h"
#include <cmath>

namespace DA
{
namespace
{
/// 使用科学计数法的绝对值范围（与峰值表 'g' 口径接近，避免 1e-9 这类值显示成 0）
constexpr double cScientificLowerBound = 1.0e-4;
constexpr double cScientificUpperBound = 1.0e7;
}  // namespace

/**
 * @brief 默认峰值标记文本模板
 */
QString daDefaultPeakMarkerText()
{
    return QStringLiteral("{index}-{fre} Hz");
}

/**
 * @brief 紧凑数字格式：定点小数去尾零，极端值用科学计数法
 *
 * 峰值标记文本空间有限，不能像峰值表那样固定两位小数：32.50 显示成 32.5、
 * 100.00 显示成 100 更紧凑。|v| 过小/过大时改用 'g' 格式（保留 6 位有效数字），
 * 否则定点格式会退化成 0 或一长串数字。
 */
QString daFormatCompactNumber(double value, int maxDecimals)
{
    if (!std::isfinite(value)) {
        return QStringLiteral("0");
    }
    const double absValue = std::fabs(value);
    if (absValue > 0.0 && (absValue < cScientificLowerBound || absValue >= cScientificUpperBound)) {
        return QString::number(value, 'g', 6);
    }
    const int decimals = qBound(0, maxDecimals, 9);
    QString text       = QString::number(value, 'f', decimals);
    // 去掉小数点后的尾零；整数结果连小数点一起去掉（-0 归一成 0）
    if (text.contains(QLatin1Char('.'))) {
        while (text.endsWith(QLatin1Char('0'))) {
            text.chop(1);
        }
        if (text.endsWith(QLatin1Char('.'))) {
            text.chop(1);
        }
    }
    if (text == QLatin1String("-0")) {
        text = QStringLiteral("0");
    }
    return text;
}

/**
 * @brief 渲染峰值标记文本
 *
 * 占位符替换顺序固定（序号 → 频率 → 幅值），替换进来的内容里不会包含占位符，
 * 因此不存在二次替换问题；未知占位符原样保留。幅值已是显示口径的值（dB 时
 * 直接补单位），与峰值表/频谱图的换算口径一致。
 */
QString daFormatPeakMarkerText(const QString& textTemplate,
                               int index,
                               double frequencyHz,
                               double amplitude,
                               bool dbMode)
{
    if (textTemplate.isEmpty()) {
        return QString();
    }
    QString text = textTemplate;
    text.replace(QLatin1String(DAPeakMarkerLabel::cIndexPlaceholder), QString::number(index));
    text.replace(QLatin1String(DAPeakMarkerLabel::cFrequencyPlaceholder),
                 daFormatCompactNumber(frequencyHz, 2));
    QString amplitudeText = daFormatCompactNumber(amplitude, dbMode ? 2 : 6);
    if (dbMode) {
        amplitudeText += QStringLiteral(" dB");
    }
    text.replace(QLatin1String(DAPeakMarkerLabel::cAmplitudePlaceholder), amplitudeText);
    return text;
}
}  // namespace DA
