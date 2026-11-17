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
    // dB 参考由幅值口径派生（D7）：写盘前统一规范化，避免旧参数里 dbFS + one 的矛盾组合被继续传播
    DASignalAnalysisParams normalized = *this;
    normalized.normalizeDbReference();
    QJsonObject obj;
    obj[ QStringLiteral("removeDc") ]       = normalized.removeDc;
    obj[ QStringLiteral("detrend") ]        = normalized.detrend;
    obj[ QStringLiteral("window") ]         = normalized.window;
    obj[ QStringLiteral("amplitudeMode") ]  = normalized.amplitudeMode;
    obj[ QStringLiteral("dbReference") ]    = normalized.dbReference;
    obj[ QStringLiteral("resolutionHz") ]   = normalized.resolutionHz;
    obj[ QStringLiteral("zeroPadding") ]    = normalized.zeroPadding;
    obj[ QStringLiteral("average") ]        = normalized.average;
    obj[ QStringLiteral("overlap") ]        = normalized.overlap;
    obj[ QStringLiteral("stftWindowMs") ]   = normalized.stftWindowMs;
    obj[ QStringLiteral("stftOverlap") ]    = normalized.stftOverlap;
    obj[ QStringLiteral("stftZeroPadding") ]= normalized.stftZeroPadding;
    obj[ QStringLiteral("peakMode") ]       = normalized.peakMode;
    obj[ QStringLiteral("peakTopN") ]       = normalized.peakTopN;
    obj[ QStringLiteral("peakRelDb") ]      = normalized.peakRelDb;
    obj[ QStringLiteral("peakProminence") ] = normalized.peakProminence;
    obj[ QStringLiteral("peakMinSpacingHz") ] = normalized.peakMinSpacingHz;
    obj[ QStringLiteral("peakMinHeight") ]  = normalized.peakMinHeight;
    obj[ QStringLiteral("calibration") ]    = normalized.calibration;
    obj[ QStringLiteral("unit") ]           = normalized.unit;
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
    // 旧工程可能存在「dBFS + 相对 1.0」的矛盾组合：加载时按幅值口径重新解释（D7 兼容说明）
    p.normalizeDbReference();
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
 * @brief 转 Python 编排参数字典（键名 = spectrum.py / stft.py / peaks.py 读取的键名）
 *
 * 与 toVariantMap() 的区别：这里刻意保持原生类型（int 仍是 int、double 是 double），
 * 不经 JSON 往返（QJsonObject 只有 double，会把 peakTopN 之类的整数变成浮点）。
 * 唯一真相在此函数：DASignalAnalysisWorker 只做 QVariant → Python 对象的搬运，
 * 键名/类型契约由 DASignalAnalysisTest 的 paramsPythonDict 用例锁定。
 */
QVariantMap DASignalAnalysisParams::toPythonDict() const
{
    // dB 参考由幅值口径派生（D7）：送 Python 前统一规范化
    DASignalAnalysisParams normalized = *this;
    normalized.normalizeDbReference();
    QVariantMap p;
    p[ QStringLiteral("removeDc") ]         = normalized.removeDc;
    p[ QStringLiteral("detrend") ]          = normalized.detrend;
    p[ QStringLiteral("window") ]           = normalized.window;
    p[ QStringLiteral("amplitudeMode") ]    = normalized.amplitudeMode;
    p[ QStringLiteral("dbReference") ]      = normalized.dbReference;
    p[ QStringLiteral("resolutionHz") ]     = normalized.resolutionHz;
    p[ QStringLiteral("zeroPadding") ]      = normalized.zeroPadding;
    p[ QStringLiteral("average") ]          = normalized.average;
    p[ QStringLiteral("overlap") ]          = normalized.overlap;
    p[ QStringLiteral("stftWindowMs") ]     = normalized.stftWindowMs;
    p[ QStringLiteral("stftOverlap") ]      = normalized.stftOverlap;
    p[ QStringLiteral("stftZeroPadding") ]  = normalized.stftZeroPadding;
    p[ QStringLiteral("peakMode") ]         = normalized.peakMode;
    p[ QStringLiteral("peakTopN") ]         = normalized.peakTopN;
    p[ QStringLiteral("peakRelDb") ]        = normalized.peakRelDb;
    p[ QStringLiteral("peakProminence") ]   = normalized.peakProminence;
    p[ QStringLiteral("peakMinSpacingHz") ] = normalized.peakMinSpacingHz;
    p[ QStringLiteral("peakMinHeight") ]    = normalized.peakMinHeight;
    p[ QStringLiteral("calibration") ]      = normalized.calibration;
    p[ QStringLiteral("unit") ]             = normalized.unit;
    return p;
}

/**
 * @brief 默认参数（「恢复默认」使用）
 *
 * resolutionHz=10 赫兹与 stftWindowMs=100 毫秒互相自洽（Δf = 1/窗长，评审 N-C4）；
 * peakMinSpacingHz 保持 -1 表示「自动 = 5 倍实际分辨率」（评审 N-C5/N-U6）。
 */
DASignalAnalysisParams DASignalAnalysisParams::defaults()
{
    return DASignalAnalysisParams();
}

/**
 * @brief 由幅值口径派生 dB 参考（D7）
 *
 * 界面已取消独立「dB 参考」入口：dB(re 1.0) 口径对应 dbReference=one，
 * 其余口径（含 dBFS 与非 dB 口径）一律 fullScale。纯函数便于单测。
 */
QString DASignalAnalysisParams::dbReferenceForAmplitudeMode(const QString& amplitudeMode)
{
    return (amplitudeMode == QLatin1String("dbRef1")) ? QStringLiteral("one") : QStringLiteral("fullScale");
}

/**
 * @brief 就地规范化 dbReference（写盘 / 送 Python / 对话框返回前调用）
 */
void DASignalAnalysisParams::normalizeDbReference()
{
    dbReference = dbReferenceForAmplitudeMode(amplitudeMode);
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

/**
 * @brief 自动最小间距 = 5 倍频率分辨率（D17）
 *
 * peaks.py 对 <=0 的最小间距按「5 × 实际分辨率」兜底；界面回显与表单默认值
 * 必须用同一口径，避免「显示 0 / 实际算 50」的两套真相（评审 N-U6）。
 */
double signalAutoPeakMinSpacingHz(double resolutionHz)
{
    if (!(resolutionHz > 0.0)) {
        return 0.0;
    }
    return 5.0 * resolutionHz;
}

/**
 * @brief 最小间距有效值（用户手改值优先，否则按 5 倍实际分辨率派生）
 *
 * 参数里的 peakMinSpacingHz 是「-1 = 自动」哨兵（跨对话框保留未手改语义，评审 N-C5），
 * 表单不能直接写 -1（min=0 会被夹成 0），所以回显前统一经本函数换算成派生值。
 */
double signalEffectivePeakMinSpacingHz(const DASignalAnalysisParams& params, double derivedResolutionHz)
{
    if (params.peakMinSpacingHz > 0.0) {
        return params.peakMinSpacingHz;
    }
    const double base = (derivedResolutionHz > 0.0) ? derivedResolutionHz : params.resolutionHz;
    return signalAutoPeakMinSpacingHz(base);
}
}  // namespace DA
