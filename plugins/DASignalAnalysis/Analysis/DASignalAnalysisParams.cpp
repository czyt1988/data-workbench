#include "DASignalAnalysisParams.h"

namespace DA
{
namespace
{
QJsonObject variantMapToJson(const QVariantMap& v)
{
    return QJsonObject::fromVariantMap(v);
}

/// 兼容老表单的字符串数值（如 peakTopN 曾是 "10"/"20"/"50"）
int jsonToInt(const QJsonValue& v, int fallback)
{
    if (v.isString()) {
        bool ok = false;
        const int n = v.toString().toInt(&ok);
        return ok ? n : fallback;
    }
    if (v.isDouble()) {
        return v.toInt(fallback);
    }
    return fallback;
}
}  // namespace

/**
 * @brief 序列化为 JSON（工程保存）
 */
QJsonObject DASignalAnalysisParams::toJson() const
{
    QJsonObject obj;
    obj[ QStringLiteral("removeDc") ]       = removeDc;
    obj[ QStringLiteral("detrend") ]        = detrend;
    obj[ QStringLiteral("window") ]         = window;
    obj[ QStringLiteral("amplitudeMode") ]  = amplitudeMode;
    obj[ QStringLiteral("dbReference") ]    = dbReference;
    obj[ QStringLiteral("resolutionHz") ]   = resolutionHz;
    obj[ QStringLiteral("zeroPadding") ]    = zeroPadding;
    obj[ QStringLiteral("average") ]        = average;
    obj[ QStringLiteral("overlap") ]        = overlap;
    obj[ QStringLiteral("stftWindowMs") ]   = stftWindowMs;
    obj[ QStringLiteral("stftOverlap") ]    = stftOverlap;
    obj[ QStringLiteral("stftZeroPadding") ]= stftZeroPadding;
    obj[ QStringLiteral("peakMode") ]       = peakMode;
    obj[ QStringLiteral("peakTopN") ]       = peakTopN;
    obj[ QStringLiteral("peakRelDb") ]      = peakRelDb;
    obj[ QStringLiteral("peakProminence") ] = peakProminence;
    obj[ QStringLiteral("peakMinSpacingHz") ] = peakMinSpacingHz;
    obj[ QStringLiteral("peakMinHeight") ]  = peakMinHeight;
    obj[ QStringLiteral("calibration") ]    = calibration;
    obj[ QStringLiteral("unit") ]           = unit;
    return obj;
}

/**
 * @brief 从 JSON 反序列化（缺失键取默认值）
 */
DASignalAnalysisParams DASignalAnalysisParams::fromJson(const QJsonObject& obj)
{
    DASignalAnalysisParams p;
    p.removeDc        = obj.value(QStringLiteral("removeDc")).toBool(p.removeDc);
    p.detrend         = obj.value(QStringLiteral("detrend")).toString(p.detrend);
    p.window          = obj.value(QStringLiteral("window")).toString(p.window);
    p.amplitudeMode   = obj.value(QStringLiteral("amplitudeMode")).toString(p.amplitudeMode);
    p.dbReference     = obj.value(QStringLiteral("dbReference")).toString(p.dbReference);
    p.resolutionHz    = obj.value(QStringLiteral("resolutionHz")).toDouble(p.resolutionHz);
    p.zeroPadding     = obj.value(QStringLiteral("zeroPadding")).toInt(p.zeroPadding);
    p.average         = obj.value(QStringLiteral("average")).toString(p.average);
    p.overlap         = obj.value(QStringLiteral("overlap")).toDouble(p.overlap);
    p.stftWindowMs    = obj.value(QStringLiteral("stftWindowMs")).toDouble(p.stftWindowMs);
    p.stftOverlap     = obj.value(QStringLiteral("stftOverlap")).toDouble(p.stftOverlap);
    p.stftZeroPadding = obj.value(QStringLiteral("stftZeroPadding")).toInt(p.stftZeroPadding);
    p.peakMode        = obj.value(QStringLiteral("peakMode")).toString(p.peakMode);
    p.peakTopN        = jsonToInt(obj.value(QStringLiteral("peakTopN")), p.peakTopN);
    p.peakRelDb       = obj.value(QStringLiteral("peakRelDb")).toDouble(p.peakRelDb);
    p.peakProminence  = obj.value(QStringLiteral("peakProminence")).toDouble(p.peakProminence);
    p.peakMinSpacingHz = obj.value(QStringLiteral("peakMinSpacingHz")).toDouble(p.peakMinSpacingHz);
    p.peakMinHeight   = obj.value(QStringLiteral("peakMinHeight")).toDouble(p.peakMinHeight);
    p.calibration     = obj.value(QStringLiteral("calibration")).toDouble(p.calibration);
    p.unit            = obj.value(QStringLiteral("unit")).toString(p.unit);
    return p;
}

/**
 * @brief 转表单值映射（字段名与参数表单一致）
 */
QVariantMap DASignalAnalysisParams::toVariantMap() const
{
    return toJson().toVariantMap();
}

/**
 * @brief 从表单值映射构造（表单值是 QVariant，数值统一 toDouble）
 */
DASignalAnalysisParams DASignalAnalysisParams::fromVariantMap(const QVariantMap& v)
{
    return fromJson(variantMapToJson(v));
}

/**
 * @brief 默认参数（「恢复默认」使用）
 */
DASignalAnalysisParams DASignalAnalysisParams::defaults()
{
    return DASignalAnalysisParams();
}

/**
 * @brief 频率分辨率 → STFT 窗长（毫秒）
 *
 * 方案 §6.3：STFT 的段长由目标频率分辨率反推（nperseg = fs/Δf），
 * 对应窗长(秒) = 1/Δf，因此窗长(ms) = 1000/Δf。
 */
double signalStftWindowMsFromResolution(double resolutionHz)
{
    if (!(resolutionHz > 0.0)) {
        return 0.0;
    }
    return 1000.0 / resolutionHz;
}

/**
 * @brief STFT 窗长（毫秒）→ 频率分辨率（赫兹）
 */
double signalResolutionFromStftWindowMs(double windowMs)
{
    if (!(windowMs > 0.0)) {
        return 0.0;
    }
    return 1000.0 / windowMs;
}
}  // namespace DA
