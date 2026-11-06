#ifndef DASIGNALIMPORTER_H
#define DASIGNALIMPORTER_H
#include <QString>
#include <QStringList>
#include "DASignal.h"
#include "DAData.h"

namespace DA
{
/**
 * @brief 导入流程编排（方案 §5）
 *
 * - 音频：WAV/AIFF/FLAC/OGG 直读；MP3/M4A/AAC 经系统 FFmpeg 转临时 WAV；
 * - 视频：FFmpeg 抽音频轨到临时目录后按文件来源处理；
 * - 数据帧：引用式指定「时间列 + 幅值列 + 采样率」。
 *
 * FFmpeg 探测顺序：插件配置里的路径 → 系统 PATH 中的 ffmpeg（方案 §5.4）。
 */
class DASignalImporter
{
public:
    // ---------------- FFmpeg ----------------
    // 当前可用的 FFmpeg 路径（配置路径优先，其次 PATH），未找到返回空串
    static QString ffmpegPath();
    // 设置并持久化 FFmpeg 路径（空串表示清除配置，回到 PATH 探测）
    static void setFFmpegPath(const QString& path);
    // FFmpeg 是否可用
    static bool isFFmpegAvailable();
    // 配置文件的路径（DADir::getConfigPath()/signal-analysis-config.json）
    static QString configFilePath();

    // ---------------- 音频/视频 ----------------
    // 导入音频文件（自动选择直读或 FFmpeg 转码路径）
    static DASignalPtr importAudioFile(const QString& path, QString* error);
    // 导入视频文件（抽取音频轨到临时目录）
    static DASignalPtr importVideoFile(const QString& path, QString* error);
    // 文件是否需要 FFmpeg（压缩音频或视频）
    static bool fileNeedsFFmpeg(const QString& path);
    // 文件是否为视频
    static bool isVideoFile(const QString& path);
    // 探测音频文件元信息（导入对话框只读展示采样率/通道/时长）；成功返回 true
    static bool probeAudioFile(const QString& path,
                               double* sampleRate,
                               int* channels,
                               double* durationSec,
                               QString* error);

    // ---------------- 数据帧 ----------------
    // 由数据帧创建信号（timeColumn 可为空；采样率由用户确认）
    static DASignalPtr importDataFrame(const DAData& data,
                                       const QString& timeColumn,
                                       const QStringList& valueColumns,
                                       double sampleRate,
                                       const QString& name,
                                       QString* error);
    // 由时间列推断采样率（fs = 1 / median(diff(t))）；失败时返回 false 并给 error
    static bool inferSampleRate(const DAData& data,
                                const QString& timeColumn,
                                double* sampleRate,
                                double* durationSec,
                                QString* error);

    // ---------------- 临时文件 ----------------
    // 插件专属临时目录（信号关闭/工程关闭/程序退出三处挂清理，避免残留）
    static QString tempDir();
    // 清理临时目录中本插件的全部抽取文件
    static void cleanupTempFiles();
};
}  // namespace DA

#endif  // DASIGNALIMPORTER_H
