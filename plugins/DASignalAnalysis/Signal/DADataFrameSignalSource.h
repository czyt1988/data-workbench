#ifndef DADATAFRAMESIGNALSOURCE_H
#define DADATAFRAMESIGNALSOURCE_H
#include "DASignalSource.h"
#include "DAData.h"

namespace DA
{
/**
 * @brief 数据帧来源（方案 §4.3）
 *
 * 持有 DAData 引用 + 列名；**引用式不复制**，按块把 float64 转成 float32。
 * 无真实音频文件，播放前需要临时导出 WAV（M9）。
 */
class DADataFrameSignalSource : public DASignalSource
{
public:
    DADataFrameSignalSource(const DAData& data,
                            const QString& timeColumn,
                            const QStringList& valueColumns,
                            double sampleRate);
    ~DADataFrameSignalSource() override;

    SourceType type() const override;
    bool readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const override;
    int channelCount() const override;
    QStringList channelNames() const override;
    qint64 sampleCount() const override;
    double sampleRate() const override;
    QString playbackFilePath() const override;
    QString sourceDescription() const override;
    void setSampleRate(double fs) override;

    // 数据帧（引用式）
    DAData data() const;
    // 时间列名（可为空）
    QString timeColumn() const;
    // 幅值列名（通道）
    QStringList valueColumns() const;

public:
    qint64 mSampleCount { 0 };

private:
    DAData mData;
    QString mTimeColumn;
    QStringList mValueColumns;
    double mSampleRate { 0.0 };
};
}  // namespace DA

#endif  // DADATAFRAMESIGNALSOURCE_H
