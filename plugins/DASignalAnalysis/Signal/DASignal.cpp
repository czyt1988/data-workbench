#include "DASignal.h"

namespace DA
{
DASignal::DASignal()
{
}

DASignal::DASignal(const QString& name, const DASignalSourcePtr& source) : mName(name), mSource(source)
{
}

/**
 * @brief 信号是否有效（有来源且采样率大于 0）
 */
bool DASignal::isValid() const
{
    return mSource && mSource->sampleRate() > 0.0 && mSource->sampleCount() > 0;
}

QString DASignal::name() const
{
    return mName;
}

void DASignal::setName(const QString& n)
{
    mName = n;
}

double DASignal::sampleRate() const
{
    return mSource ? mSource->sampleRate() : 0.0;
}

void DASignal::setSampleRate(double fs)
{
    if (mSource) {
        mSource->setSampleRate(fs);
    }
}

int DASignal::channelCount() const
{
    return mSource ? mSource->channelCount() : 0;
}

QStringList DASignal::channelNames() const
{
    return mSource ? mSource->channelNames() : QStringList();
}

/**
 * @brief 通道名（缺省补成「Channel N」/ cn:通道 N）
 */
QStringList DASignal::channelNamesOrDefaults() const
{
    QStringList names = channelNames();
    const int cnt     = channelCount();
    for (int i = names.size(); i < cnt; ++i) {
        // 缺省通道名：英文源 + 中文注释（不翻译，通道数据标签沿用「Channel N」的英文形式即可）
        names.append(QStringLiteral("Channel %1").arg(i + 1));  // cn:通道 N
    }
    return names;
}

qint64 DASignal::sampleCount() const
{
    return mSource ? mSource->sampleCount() : 0;
}

/**
 * @brief 信号时长（秒）
 */
double DASignal::duration() const
{
    const double fs = sampleRate();
    if (fs <= 0.0) {
        return 0.0;
    }
    return static_cast< double >(sampleCount()) / fs;
}

double DASignal::timeOrigin() const
{
    return mTimeOrigin;
}

void DASignal::setTimeOrigin(double t)
{
    mTimeOrigin = t;
}

QString DASignal::unit() const
{
    return mUnit;
}

void DASignal::setUnit(const QString& u)
{
    mUnit = u;
}

double DASignal::calibration() const
{
    return mCalibration;
}

void DASignal::setCalibration(double c)
{
    mCalibration = c;
}

DASignalSourcePtr DASignal::source() const
{
    return mSource;
}

DASignalSource::SourceType DASignal::sourceType() const
{
    return mSource ? mSource->type() : DASignalSource::FileSource;
}

bool DASignal::readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const
{
    if (!mSource) {
        if (error) {
            *error = QStringLiteral("signal has no source");
        }
        return false;
    }
    return mSource->readRange(channel, start, count, out, error);
}
}  // namespace DA
