#include "DADataFrameSignalSource.h"
#include <cstring>
#include <QDebug>
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "pandas/DAPyDataFrame.h"
#include "DAPybind11QtCaster.hpp"

namespace DA
{
namespace
{
DAPyModule importerModule()
{
    return DAPyModule("DASignalAnalysis.importer");
}
}  // namespace

DADataFrameSignalSource::DADataFrameSignalSource(const DAData& data,
                                                 const QString& timeColumn,
                                                 const QStringList& valueColumns,
                                                 double sampleRate)
    : mData(data), mTimeColumn(timeColumn), mValueColumns(valueColumns), mSampleRate(sampleRate)
{
}

/**
 * @brief 「来源缺失」占位构造（评审 U16）
 *
 * 工程里记录的数据帧在数据管理器里找不到时，用这个构造保留数据帧名/列名/采样率，
 * 让信号标签页仍能占位显示并提示用户「重新指定数据帧来源」，而不是直接丢弃参数。
 */
DADataFrameSignalSource::DADataFrameSignalSource(const QString& dataframeName,
                                                 const QString& timeColumn,
                                                 const QStringList& valueColumns,
                                                 double sampleRate)
    : mTimeColumn(timeColumn)
    , mValueColumns(valueColumns)
    , mSampleRate(sampleRate)
    , mMissingDataFrameName(dataframeName)
{
    mSampleCount = 0;  // 缺失占位没有数据，isValid() 为 false，不参与计算
}

DADataFrameSignalSource::~DADataFrameSignalSource()
{
}

DASignalSource::SourceType DADataFrameSignalSource::type() const
{
    return DataFrameSource;
}

/**
 * @brief 从数据帧列读取一段采样（float32，按块转换）
 *
 * 引用式读取：直接引用数据帧当前内容（用户修改数据帧后重算即生效）。
 */
bool DADataFrameSignalSource::readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const
{
    out.clear();
    if (count <= 0) {
        return true;
    }
    if (isMissing()) {
        if (error) {
            *error = QStringLiteral("dataframe source is missing");
        }
        return false;
    }
    if (channel < 0 || channel >= mValueColumns.size()) {
        if (error) {
            *error = QStringLiteral("channel index out of range");
        }
        return false;
    }
    if (!mData.isDataFrame()) {
        if (error) {
            *error = QStringLiteral("source data is not a dataframe");
        }
        return false;
    }
    try {
        DAPyModule importer = importerModule();
        DAPyDataFrame df   = mData.toDataFrame();
        pybind11::object res =
            importer.attr("read_dataframe_range")(df.object(), mValueColumns.at(channel), start, count);
        pybind11::array_t< float, pybind11::array::c_style | pybind11::array::forcecast > arr(res);
        const auto n = static_cast< int >(arr.size());
        out.resize(n);
        if (n > 0) {
            std::memcpy(out.data(), arr.data(), sizeof(float) * static_cast< size_t >(n));
        }
        return true;
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        qWarning() << "DADataFrameSignalSource::readRange failed:" << e.what();
        return false;
    }
}

int DADataFrameSignalSource::channelCount() const
{
    return mValueColumns.size();
}

QStringList DADataFrameSignalSource::channelNames() const
{
    return mValueColumns;
}

qint64 DADataFrameSignalSource::sampleCount() const
{
    return mSampleCount;
}

double DADataFrameSignalSource::sampleRate() const
{
    return mSampleRate;
}

QString DADataFrameSignalSource::playbackFilePath() const
{
    // 数据帧来源没有真实音频文件（M9 播放前临时导出 WAV）
    return QString();
}

QString DADataFrameSignalSource::sourceDescription() const
{
    if (isMissing()) {
        // 缺失占位：给用户可读的来源描述（界面提示与日志都要能看懂）
        return QStringLiteral("%1[%2] (missing)").arg(mMissingDataFrameName, mValueColumns.join(QLatin1Char(',')));
    }
    return QStringLiteral("%1[%2]").arg(mData.getName(), mValueColumns.join(QLatin1Char(',')));
}

bool DADataFrameSignalSource::isMissing() const
{
    return !mData.isDataFrame();
}

QString DADataFrameSignalSource::dataframeName() const
{
    if (!mMissingDataFrameName.isEmpty()) {
        return mMissingDataFrameName;
    }
    return mData.getName();
}

void DADataFrameSignalSource::setSampleRate(double fs)
{
    mSampleRate = fs;
}

DAData DADataFrameSignalSource::data() const
{
    return mData;
}

QString DADataFrameSignalSource::timeColumn() const
{
    return mTimeColumn;
}

QStringList DADataFrameSignalSource::valueColumns() const
{
    return mValueColumns;
}
}  // namespace DA
