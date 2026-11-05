#ifndef DASIGNALANALYSISPARAMS_H
#define DASIGNALANALYSISPARAMS_H
#include <QString>
#include <QVariantMap>
#include <QJsonObject>

namespace DA
{
/**
 * @brief 信号分析参数（方案 §8.6 分组口径，D18/D21）
 *
 * 一个信号标签页记住一份参数；参数不进全局撤销栈，改参数后立即重算一次。
 * JSON 键名与插件 Python 编排层（spectrum.py / stft.py / peaks.py）保持一致。
 */
struct DASignalAnalysisParams
{
    // ---------------- 信号预处理 ----------------
    bool removeDc { false };            ///< 去直流
    QString detrend { "none" };         ///< 去趋势：none / constant / linear

    // ---------------- 频谱 ----------------
    QString window { "hann" };          ///< 窗函数：rectangular/hann/hamming/blackman/flattop
    QString amplitudeMode { "peak" };   ///< 幅值口径：peak（峰值幅度）/ rms（均方根）
    QString dbReference { "fullScale" };///< dB 参考：fullScale（相对满量程）/ one（相对 1.0）
    double resolutionHz { 10.0 };       ///< 频率分辨率（赫兹，段长由它反推）
    int zeroPadding { 0 };              ///< 零填充点数（高级，0 表示等于段长）

    // ---------------- 功率谱 ----------------
    QString average { "mean" };         ///< 平均方式：mean / median
    double overlap { 50.0 };            ///< 重叠率（百分比）

    // ---------------- 短时傅里叶变换 ----------------
    double stftWindowMs { 50.0 };       ///< 窗长（毫秒）
    double stftOverlap { 50.0 };        ///< 跳步重叠率（百分比）
    int stftZeroPadding { 0 };          ///< 零填充点数（高级，0 表示等于窗长）
    // 窗函数与频谱共用 window 字段（D18：同一信号共用窗函数口径，避免两个真相）

    // ---------------- 峰值查找 ----------------
    QString peakMode { "simple" };      ///< 模式：simple / advanced
    int peakTopN { 10 };                ///< 前 N 个（10/20/50）
    double peakRelDb { 40.0 };          ///< 相对门限（高于最强峰多少 dB）
    double peakProminence { 0.0 };      ///< 突出度（0 表示不限制，仅高级模式生效）
    double peakMinSpacingHz { -1.0 };   ///< 最小间距（赫兹，<0 表示按 5 倍频率分辨率自动）
    double peakMinHeight { 0.0 };       ///< 绝对高度门限（0 表示不限制，仅高级模式生效）

    // ---------------- 标定 ----------------
    double calibration { 1.0 };         ///< 标定系数
    QString unit;                       ///< 幅值单位文本

    // 序列化（工程保存与参数对话框共用）
    QJsonObject toJson() const;
    static DASignalAnalysisParams fromJson(const QJsonObject& obj);
    // 表单值映射（字段名与 Form/DASignalFormSpecBuilder 一致）
    QVariantMap toVariantMap() const;
    static DASignalAnalysisParams fromVariantMap(const QVariantMap& v);
    // 恢复默认
    static DASignalAnalysisParams defaults();
};
}  // namespace DA

#endif  // DASIGNALANALYSISPARAMS_H
