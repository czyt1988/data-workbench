#ifndef DASIGNALAMPLITUDE_H
#define DASIGNALAMPLITUDE_H
#include <QString>
#include <QtGlobal>
#include <cmath>

namespace DA
{
/**
 * @brief 信号幅值显示口径换算（唯一真相）
 *
 * 计算层始终产出「峰值幅度」（未乘标定），显示/导出层按当前幅值模式换算：
 * - peak：峰值幅度（乘标定）；
 * - rms ：均方根 = 峰值/√2（仅 k>=1 的频点；直流量不除 √2）；
 * - dbFS：20·log10(|x| / 满量程参考)，参考 = 信号峰值（标定后）；
 * - dbRef1：20·log10(|x| / 1.0)。
 *
 * 频谱图、峰值表、峰值曲线与导出统一调用这里，保证同一数值三处一致（评审 C2/C4/I9）。
 */
namespace DASignalAmplitude
{
/// 分贝换算的下限保护
inline constexpr double cEps = 1e-30;

/// 是否为分贝类显示模式
inline bool isDbMode(const QString& amplitudeMode)
{
    return amplitudeMode == QLatin1String("dbFS") || amplitudeMode == QLatin1String("dbRef1");
}

/// 是否为均方根显示模式
inline bool isRmsMode(const QString& amplitudeMode)
{
    return amplitudeMode == QLatin1String("rms");
}

/**
 * @brief 峰值幅度 → 显示值
 * @param peakAmplitude 峰值幅度（计算层输出，未乘标定）
 * @param isDc 是否直流分量/零频（均方根换算时直流不除 √2）
 * @param amplitudeMode peak / rms / dbFS / dbRef1
 * @param dbReference fullScale / one（dbFS 时使用；one 等价 dbRef1）
 * @param calibration 标定系数
 * @param signalPeakCalibrated 信号峰值（已乘标定），作为 dBFS 参考
 */
inline double toDisplay(double peakAmplitude,
                        bool isDc,
                        const QString& amplitudeMode,
                        const QString& dbReference,
                        double calibration,
                        double signalPeakCalibrated)
{
    const double calibrated = peakAmplitude * calibration;
    if (isRmsMode(amplitudeMode)) {
        return isDc ? calibrated : calibrated / 1.4142135623730951;
    }
    if (isDbMode(amplitudeMode)) {
        const bool refIsOne = (dbReference == QLatin1String("one")) || (amplitudeMode == QLatin1String("dbRef1"));
        const double ref    = refIsOne ? 1.0 : qMax(signalPeakCalibrated, cEps);
        return 20.0 * std::log10(qMax(std::fabs(calibrated), cEps) / ref);
    }
    return calibrated;
}

/// 显示口径的坐标轴/表头文字（英文源，调用方 tr）
inline QString axisLabelKey(const QString& amplitudeMode)
{
    if (isDbMode(amplitudeMode)) {
        return amplitudeMode == QLatin1String("dbRef1") ? QStringLiteral("dB (re 1.0)") : QStringLiteral("dB (FS)");
    }
    if (isRmsMode(amplitudeMode)) {
        return QStringLiteral("RMS");
    }
    return QStringLiteral("Amplitude");
}
}  // namespace DASignalAmplitude
}  // namespace DA

#endif  // DASIGNALAMPLITUDE_H
