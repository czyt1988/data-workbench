#include "DAFileSignalSource.h"
#include <cstring>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11QtCaster.hpp"

namespace DA
{
namespace
{
/**
 * @brief 导入插件 Python 包并返回 importer 模块
 *
 * 每次调用都新建 DAPyModule（Python import 有模块缓存，开销很小）；
 * 不使用静态对象，避免解释器关闭后静态析构访问已销毁的 Python 对象。
 */
DAPyModule importerModule()
{
    return DAPyModule("DASignalAnalysis.importer");
}
}  // namespace

DAFileSignalSource::DAFileSignalSource(const QString& readPath,
                                       const QString& originalPath,
                                       bool isTemporary,
                                       SourceType sourceType)
    : mReadPath(readPath), mOriginalPath(originalPath), mIsTemporary(isTemporary), mSourceType(sourceType)
{
}

DAFileSignalSource::~DAFileSignalSource()
{
    // 临时抽取文件随信号生命周期清理（方案 §5.3）
    if (mIsTemporary && !mReadPath.isEmpty()) {
        QFile::remove(mReadPath);
    }
}

DASignalSource::SourceType DAFileSignalSource::type() const
{
    return mSourceType;
}

/**
 * @brief 按通道读取一段采样（float32）
 *
 * 经 Python soundfile 分块读取；每次调用独立打开文件头（成本很低），
 * 不持有跨线程文件句柄。
 */
bool DAFileSignalSource::readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const
{
    out.clear();
    if (count <= 0) {
        return true;
    }
    try {
        DAPyModule importer = importerModule();
        pybind11::object res = importer.attr("read_range")(mReadPath, channel, start, count);
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
        qWarning() << "DAFileSignalSource::readRange failed:" << e.what();
        return false;
    }
}

int DAFileSignalSource::channelCount() const
{
    return mChannelCount;
}

QStringList DAFileSignalSource::channelNames() const
{
    return mChannelNames;
}

qint64 DAFileSignalSource::sampleCount() const
{
    return mSampleCount;
}

double DAFileSignalSource::sampleRate() const
{
    return mSampleRate;
}

QString DAFileSignalSource::playbackFilePath() const
{
    return mReadPath;
}

QString DAFileSignalSource::sourceDescription() const
{
    return mOriginalPath;
}

void DAFileSignalSource::setSampleRate(double fs)
{
    mSampleRate = fs;
}

QString DAFileSignalSource::readPath() const
{
    return mReadPath;
}

QString DAFileSignalSource::originalPath() const
{
    return mOriginalPath;
}

bool DAFileSignalSource::isTemporary() const
{
    return mIsTemporary;
}
}  // namespace DA
