#ifndef DAFILESIGNALSOURCE_H
#define DAFILESIGNALSOURCE_H
#include "DASignalSource.h"

namespace DA
{
/**
 * @brief 文件来源：音频文件或视频抽取出的临时 WAV（方案 §4.3）
 *
 * 播放直接用本地文件路径；分析按块读（Python soundfile 读取，不一次性读入整文件）。
 */
class DAFileSignalSource : public DASignalSource
{
public:
    /**
     * @param readPath 实际读取路径（视频为抽取出的临时 WAV）
     * @param originalPath 用户选择的原始文件路径（工程保存与重新指定用）
     * @param isTemporary readPath 是临时抽取文件（信号销毁时清理）
     */
    DAFileSignalSource(const QString& readPath,
                       const QString& originalPath,
                       bool isTemporary = false,
                       SourceType sourceType = FileSource);
    ~DAFileSignalSource() override;

    SourceType type() const override;
    bool readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const override;
    int channelCount() const override;
    QStringList channelNames() const override;
    qint64 sampleCount() const override;
    double sampleRate() const override;
    QString playbackFilePath() const override;
    QString sourceDescription() const override;
    void setSampleRate(double fs) override;

    // 实际读取路径（视频为临时 WAV）
    QString readPath() const;
    // 用户选择的原始文件路径
    QString originalPath() const;
    // 是否为临时抽取文件
    bool isTemporary() const;

public:
    // 音频元信息由导入器探测后写入
    int mChannelCount { 1 };
    QStringList mChannelNames;
    qint64 mSampleCount { 0 };
    double mSampleRate { 0.0 };

private:
    QString mReadPath;
    QString mOriginalPath;
    bool mIsTemporary { false };
    SourceType mSourceType { FileSource };
};
}  // namespace DA

#endif  // DAFILESIGNALSOURCE_H
