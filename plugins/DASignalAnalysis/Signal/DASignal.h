#ifndef DASIGNAL_H
#define DASIGNAL_H
#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>
#include "DASignalSource.h"

namespace DA
{
/**
 * @brief 信号对象（方案 §4.1）
 *
 * 只描述数据，不持有界面。采样率/通道/采样点数等由来源（DASignalSource）持有，
 * DASignal 转发访问，避免同一信息出现两个真相。
 */
class DASignal
{
public:
    DASignal();
    DASignal(const QString& name, const DASignalSourcePtr& source);

    bool isValid() const;
    // 显示名（导入时取文件名或「数据帧名.列名」）
    QString name() const;
    void setName(const QString& n);
    // 采样率（赫兹，用户确认后的值）
    double sampleRate() const;
    void setSampleRate(double fs);
    // 通道数
    int channelCount() const;
    // 通道名（缺省「通道 N」由 channelNamesOrDefaults 补齐）
    QStringList channelNames() const;
    QStringList channelNamesOrDefaults() const;
    // 每通道采样点数
    qint64 sampleCount() const;
    // 时长（秒）= sampleCount / sampleRate
    double duration() const;
    // 时间原点偏移（秒）
    double timeOrigin() const;
    void setTimeOrigin(double t);
    // 幅值单位文本
    QString unit() const;
    void setUnit(const QString& u);
    // 标定系数
    double calibration() const;
    void setCalibration(double c);
    // 来源
    DASignalSourcePtr source() const;
    // 来源类型
    DASignalSource::SourceType sourceType() const;
    // 按通道读一段（转发来源）
    bool readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const;

private:
    QString mName;
    double mTimeOrigin { 0.0 };
    QString mUnit;
    double mCalibration { 1.0 };
    DASignalSourcePtr mSource;
};

using DASignalPtr = std::shared_ptr< DASignal >;
}  // namespace DA

#endif  // DASIGNAL_H
