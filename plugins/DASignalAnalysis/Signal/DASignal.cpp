#include "DASignal.h"
#include <atomic>

namespace DA
{
namespace
{
/**
 * @brief 进程级信号 id 计数器（单调递增）
 *
 * 所有信号创建路径（导入器、工程恢复、来源缺失占位、重新指定来源）都经过 DASignal
 * 构造函数，在这里分配会话内唯一 id；不落盘，恢复工程后按新会话重新编号即可。
 */
std::atomic< qint64 > gNextSignalId { 1 };
}  // namespace

DASignal::DASignal() : mSignalId(gNextSignalId.fetch_add(1))
{
}

DASignal::DASignal(const QString& name, const DASignalSourcePtr& source)
    : mName(name), mSignalId(gNextSignalId.fetch_add(1)), mSource(source)
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

/**
 * @brief 会话内唯一 id（进程级单调递增，不落盘）
 *
 * figure id（"sig-<id>"）与 figures.xml 的 signal-id 存档键用它区分同名信号；
 * 显示名只作可读标识，不参与存档匹配。
 */
qint64 DASignal::signalId() const
{
    return mSignalId;
}

/**
 * @brief 工程恢复时回填持久化的信号 id（BLK-02）
 *
 * signals.json 里保存了每个信号的 signalId；恢复时回填它，figures.xml 的 signal-id
 * 存档键才能在新会话里精确命中（同名信号不串档）。回填同时把进程计数器推进到
 * id+1 之后，保证本会话后续新分配的 id 不会与恢复值冲突；id<=0（旧工程无该字段）
 * 时忽略，保持构造时分配的会话内 id（匹配走 signal-name 回退）。
 */
void DASignal::restoreSignalId(qint64 id)
{
    if (id <= 0) {
        return;
    }
    mSignalId = id;
    qint64 expected = gNextSignalId.load();
    while (id >= expected && !gNextSignalId.compare_exchange_weak(expected, id + 1)) {
    }
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
