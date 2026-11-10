#ifndef DASIGNALMEMORYESTIMATE_H
#define DASIGNALMEMORYESTIMATE_H
#include <QtGlobal>
#include <algorithm>

namespace DA
{
/**
 * @brief 信号分析各任务的峰值内存估算（方案 D5）
 *
 * D1 取数架构落地后，频谱/功率谱/热图通过 C++ 注入的 readRange provider 按块取数，
 * 峰值内存**不随信号总长度增长**，只与「段长（频率分辨率/窗长）」和显示栅格尺寸有关；
 * 峰值表与峰值曲线仍整段取数（float32），内存与采样点数线性相关。
 *
 * 估算口径（与 Python 侧实际分配对应）：
 * - 取数：文件/数据帧均按块，常驻 float32 整段仅出现在峰值表/峰值曲线路径；
 * - FFT：分块 float64 成帧矩阵（频谱/PSD 每块 256 帧，峰值曲线每块 512 帧）；
 * - 频谱与 PSD 在同一任务里顺序执行，不同时驻留两块矩阵；
 * - 频谱/峰值表/峰值曲线/热图通常同批提交，用 concurrentCopies 放大整段取数副本。
 *
 * 该头文件保持纯逻辑（只依赖 QtGlobal/STL），供 DASignalAnalysisTest 直接断言公式。
 */
enum class DASignalAnalysisStage
{
    Spectrum,    ///< 整段频谱 + 功率谱（流式）
    PeakTable,   ///< 峰值表（整段取数 + 分块成帧）
    PeakCurves,  ///< 每帧前 N 峰值曲线（整段取数 + 分块成帧）
    Envelope,    ///< 波形包络（按可见区段取数）
    Heatmap      ///< 时频热图（流式）
};

/// 默认分析内存预算：1.5 GB（方案 §11 / 评审 N-C1c）
constexpr qint64 kDASignalAnalysisBudgetBytes = qint64(1.5 * 1024.0 * 1024.0 * 1024.0);

/// 频谱/PSD 每块帧数（与 DAPyBase/spectral.py 的 _SEG_BLOCK_FRAMES/_PSD_BLOCK_FRAMES 一致）
constexpr int kDASignalSpectrumBlockFrames = 256;
/// 峰值曲线每块帧数（与 DASignalAnalysis/peaks.py 的 _BLOCK_FRAMES 一致）
constexpr int kDASignalPeakCurveBlockFrames = 512;
/// 每块以外的小对象/结果缓冲固定预留（约 8 MB）
constexpr qint64 kDASignalEstimateOverheadBytes = qint64(8) * 1024 * 1024;

/**
 * @brief 估算输入（全部按当前实际 dtype 口径：取数 float32、FFT 段内 float64）
 */
struct DASignalMemoryInputs
{
    qint64 sampleCount { 0 };            ///< 每通道采样点数
    double sampleRate { 48000.0 };       ///< 采样率（Hz）
    double resolutionHz { 10.0 };        ///< 频谱目标频率分辨率（Hz）
    double stftWindowMs { 100.0 };       ///< STFT 窗长（ms）
    double stftOverlapPercent { 50.0 };  ///< STFT 重叠率（百分比，50 表示 50%）
    int peakTopN { 10 };                 ///< 峰值曲线每帧前 N
    int gridColumns { 0 };               ///< 热图目标列数（0 表示未知，按 0 计）
    int gridRows { 0 };                  ///< 热图目标行数
    qint64 rangeSamples { 0 };           ///< 按区段取数的任务（包络）实际区段点数；<=0 表示整段
    int concurrentCopies { 2 };          ///< 整段取数副本的并发系数（频谱/峰值/曲线同批提交）
};

/// 频率分辨率 → 频谱段长（与 spectral.seg_len_from_resolution 同口径：round(fs/Δf)，下限 8）
inline qint64 dasignalSegmentLength(double sampleRate, double resolutionHz, qint64 sampleCount)
{
    const double fs  = (sampleRate > 0.0) ? sampleRate : 1.0;
    const double res = (resolutionHz > 0.0) ? resolutionHz : 1e-9;
    qint64 nperseg    = qint64(std::max(8.0, std::round(fs / res)));
    if (sampleCount > 0) {
        nperseg = std::min(nperseg, sampleCount);
    }
    return std::max< qint64 >(2, nperseg);
}

/// STFT 窗长 → STFT 段长（与 stft._stft_layout 同口径：round(fs*ms/1000)，clamp 到采样点数）
inline qint64 dasignalStftSegmentLength(double sampleRate, double windowMs, qint64 sampleCount)
{
    const double fs = (sampleRate > 0.0) ? sampleRate : 1.0;
    const double ms = (windowMs > 0.0) ? windowMs : 1.0;
    qint64 nperseg  = qint64(std::max(2.0, std::round(fs * ms / 1000.0)));
    if (sampleCount > 0) {
        nperseg = std::min(nperseg, sampleCount);
    }
    return std::max< qint64 >(2, nperseg);
}

/**
 * @brief 估算某分析任务的峰值内存（字节）
 *
 * 频谱/热图路径对 sampleCount 不敏感（流式）；峰值表/曲线路径含整段 float32 副本
 * 与并发系数；包络只按区段估算。
 */
inline qint64 dasignalEstimateAnalysisBytes(DASignalAnalysisStage stage, const DASignalMemoryInputs& in)
{
    const qint64 copies = std::max< qint64 >(1, in.concurrentCopies);
    const qint64 samples = std::max< qint64 >(0, in.sampleCount);
    switch (stage) {
    case DASignalAnalysisStage::Spectrum: {
        const qint64 nperseg = dasignalSegmentLength(in.sampleRate, in.resolutionHz, samples);
        const qint64 fftBlock = qint64(kDASignalSpectrumBlockFrames) * nperseg * qint64(sizeof(double));
        const qint64 freqs    = (nperseg / 2 + 1) * qint64(sizeof(double)) * 3;  // freqs + amp + psd
        return fftBlock + freqs + kDASignalEstimateOverheadBytes;
    }
    case DASignalAnalysisStage::PeakTable: {
        const qint64 nperseg = dasignalSegmentLength(in.sampleRate, in.resolutionHz, samples);
        const qint64 whole    = samples * qint64(sizeof(float)) * copies;
        const qint64 fftBlock = qint64(kDASignalSpectrumBlockFrames) * nperseg * qint64(sizeof(double));
        return whole + fftBlock + kDASignalEstimateOverheadBytes;
    }
    case DASignalAnalysisStage::PeakCurves: {
        const qint64 nperseg = dasignalStftSegmentLength(in.sampleRate, in.stftWindowMs, samples);
        const qint64 overlap = qint64(std::max(0.0, std::min(99.0, in.stftOverlapPercent)));
        qint64 hop           = qint64(std::max(1.0, std::round(double(nperseg) * (1.0 - double(overlap) / 100.0))));
        const qint64 frames  = (samples > nperseg) ? ((samples - nperseg) / hop + 1) : 0;
        const qint64 whole    = samples * qint64(sizeof(float)) * copies;
        const qint64 fftBlock = qint64(kDASignalPeakCurveBlockFrames) * nperseg * qint64(sizeof(double));
        const qint64 curves   = qint64(std::max(1, in.peakTopN)) * frames * qint64(sizeof(double)) * 2;
        return whole + fftBlock + curves + kDASignalEstimateOverheadBytes;
    }
    case DASignalAnalysisStage::Envelope: {
        const qint64 range = (in.rangeSamples > 0) ? in.rangeSamples : samples;
        const qint64 data  = range * qint64(sizeof(float)) + qint64(std::max(0, in.gridColumns)) * qint64(sizeof(double)) * 2;
        return data + kDASignalEstimateOverheadBytes;
    }
    case DASignalAnalysisStage::Heatmap: {
        const qint64 nperseg  = dasignalStftSegmentLength(in.sampleRate, in.stftWindowMs, samples);
        const qint64 fftBlock  = qint64(kDASignalSpectrumBlockFrames) * nperseg * qint64(sizeof(double));
        const qint64 grid      = qint64(std::max(0, in.gridColumns)) * qint64(std::max(0, in.gridRows)) * qint64(sizeof(float));
        return fftBlock + grid + kDASignalEstimateOverheadBytes;
    }
    }
    return 0;
}
}  // namespace DA

#endif  // DASIGNALMEMORYESTIMATE_H
