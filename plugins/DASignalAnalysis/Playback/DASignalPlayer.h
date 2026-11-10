#ifndef DASIGNALPLAYER_H
#define DASIGNALPLAYER_H
#include <QObject>
#include <QHash>
#include <QString>
#include "Signal/DASignal.h"

class QMediaPlayer;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QAudioOutput;
#endif

namespace DA
{
/**
 * @brief 信号播放控制器（方案 D26 / §8.5）
 *
 * 封装 QMediaPlayer，对外只暴露 play/pause/stop/seekMs/positionMs/durationMs
 * 与 positionChanged(qint64)，便于将来替换后端（例如 QAudioOutput 喂 PCM）。
 *
 * - 位置通知间隔调为 50 毫秒（默认 1 秒太粗，播放头会一跳一跳）；
 * - 数据帧来源没有真实音频文件，播放前把所选通道导出为临时 WAV（缓存，随控制器清理）。
 */
class DASignalPlayer : public QObject
{
    Q_OBJECT
public:
    explicit DASignalPlayer(QObject* parent = nullptr);
    ~DASignalPlayer() override;

    // 设置播放源（数据帧来源会先导出临时 WAV）；失败返回 false 并写 error
    bool setSignal(const DASignalPtr& signal, int channel, QString* error);
    // 清除播放源
    void clearSignal();

    // 数据帧来源的临时 WAV 缓存路径（未导出返回空串）；后台导出流程据此判断是否需要导出
    QString cachedWavPath(const DASignalPtr& signal, int channel) const;
    // 登记后台导出的临时 WAV 并切为该信号的播放源（导出在工作线程完成后调用，评审 U9）
    bool setPreparedDataFrameWav(const DASignalPtr& signal, int channel, const QString& wavPath, QString* error);
    // 是否已装入可播放的媒体源（播放动作可用性用）
    bool hasSource() const;

    void play();
    void pause();
    void stop();
    // 跳转到指定毫秒
    void seekMs(qint64 ms);

    qint64 positionMs() const;
    qint64 durationMs() const;
    bool isPlaying() const;

Q_SIGNALS:
    // 播放位置（毫秒，约 50 毫秒一次）
    void positionChanged(qint64 ms);
    // 播放失败（格式不支持、后端缺失等）
    void errorOccurred(const QString& error);

private Q_SLOTS:
    void onPlayerError();

private:
    // 切当前播放源（本地文件路径）
    void applyMediaPath(const QString& path);
    // 「信号 + 通道」缓存键
    static QString wavCacheKey(const DASignalPtr& signal, int channel);

private:
    QMediaPlayer* mPlayer { nullptr };
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioOutput* mAudioOutput { nullptr };
#endif
    QString mMediaPath;        ///< 当前播放的本地文件
    QString mTempWavPath;      ///< 数据帧来源导出的临时 WAV（当前）
    /// 数据帧通道临时 WAV 缓存：键 = 信号指针:通道，避免切回同一信号时重复整列导出（评审 C8）
    QHash< QString, QString > mTempWavCache;
};
}  // namespace DA

#endif  // DASIGNALPLAYER_H
