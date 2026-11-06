#include "DASignalPlayer.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QUuid>
#include <QDebug>
#include <utility>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#endif
#include <QMediaPlayer>
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11QtCaster.hpp"
#include "pandas/DAPyDataFrame.h"
#include "Signal/DASignalImporter.h"
#include "Signal/DADataFrameSignalSource.h"

namespace DA
{
namespace
{
DAPyModule importerModule()
{
    return DAPyModule("DASignalAnalysis.importer");
}

/**
 * @brief 把数据帧来源的某通道导出为 32 位浮点 WAV（临时文件）
 * @return 成功返回 WAV 路径，失败返回空串并写 error
 */
QString exportDataFrameChannelToWav(const DASignalPtr& signal, int channel, QString* error)
{
    auto dfSource = std::dynamic_pointer_cast< DADataFrameSignalSource >(signal->source());
    if (!dfSource) {
        if (error) {
            *error = QStringLiteral("signal has no dataframe source");
        }
        return QString();
    }
    const QStringList cols = dfSource->valueColumns();
    if (channel < 0 || channel >= cols.size()) {
        if (error) {
            *error = QStringLiteral("channel index out of range");
        }
        return QString();
    }
    const QString outPath = QDir(DASignalImporter::tempDir())
                                .absoluteFilePath(QUuid::createUuid().toString(QUuid::WithoutBraces)
                                                  + QStringLiteral(".wav"));
    try {
        DAPyDataFrame df     = dfSource->data().toDataFrame();
        DAPyModule importer  = importerModule();
        pybind11::object res = importer.attr("export_column_to_wav")(
            df.object(), cols.at(channel), outPath, signal->sampleRate());
        if (!res.cast< bool >()) {
            if (error) {
                *error = QStringLiteral("failed to export dataframe column to WAV");
            }
            QFile::remove(outPath);  // 失败不留半成品临时文件（评审 I12）
            return QString();
        }
        return outPath;
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        qWarning() << "DASignalPlayer: export dataframe column failed:" << e.what();
        QFile::remove(outPath);  // 失败不留半成品临时文件（评审 I12）
        return QString();
    }
}
}  // namespace

DASignalPlayer::DASignalPlayer(QObject* parent) : QObject(parent)
{
    mPlayer = new QMediaPlayer(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6 必须同时建 QAudioOutput 并 setAudioOutput（T5）
    mAudioOutput = new QAudioOutput(this);
    mAudioOutput->setVolume(1.0);
    mPlayer->setAudioOutput(mAudioOutput);
#endif
    // 位置通知间隔调到 50 毫秒（T4）；Qt6 的 QMediaPlayer 已移除该属性，按版本条件编译（评审 D8/C8）
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mPlayer->setNotifyInterval(50);
#endif
    connect(mPlayer, &QMediaPlayer::positionChanged, this, &DASignalPlayer::positionChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(mPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& msg) {
        Q_EMIT errorOccurred(msg);
    });
#else
    connect(mPlayer, QOverload< QMediaPlayer::Error >::of(&QMediaPlayer::error), this,
            [this](QMediaPlayer::Error) { onPlayerError(); });
#endif
}

DASignalPlayer::~DASignalPlayer()
{
    if (mPlayer) {
        mPlayer->stop();
    }
    // 数据帧导出的临时 WAV 随控制器清理（T8）
    for (const QString& path : std::as_const(mTempWavCache)) {
        QFile::remove(path);
    }
    mTempWavCache.clear();
}

/**
 * @brief 设置播放源
 *
 * 文件/视频来源直接用本地文件；数据帧来源先把所选通道导出为临时 WAV（缓存到本对象）。
 */
bool DASignalPlayer::setSignal(const DASignalPtr& signal, int channel, QString* error)
{
    clearSignal();
    if (!signal) {
        if (error) {
            *error = tr("Signal is invalid.");  // cn:信号无效。
        }
        return false;
    }
    QString path;
    if (signal->sourceType() == DASignalSource::DataFrameSource) {
        // 以「信号 + 通道」为键缓存导出结果，切回同一通道不再重复整列导出（评审 C8/U9）
        const QString cacheKey =
            QString::number(reinterpret_cast< quintptr >(signal.get())) + QLatin1Char(':') + QString::number(channel);
        const QString cached = mTempWavCache.value(cacheKey);
        if (!cached.isEmpty() && QFileInfo::exists(cached)) {
            path         = cached;
            mTempWavPath = cached;
        } else {
            path = exportDataFrameChannelToWav(signal, channel, error);
            if (path.isEmpty()) {
                return false;
            }
            mTempWavCache.insert(cacheKey, path);
            mTempWavPath = path;
        }
    } else {
        path = signal->source()->playbackFilePath();
    }
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        if (error) {
            *error = tr("No playable audio file for this signal.");  // cn:该信号没有可播放的音频文件。
        }
        return false;
    }
    mMediaPath = path;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    mPlayer->setSource(QUrl::fromLocalFile(path));
#else
    mPlayer->setMedia(QMediaContent(QUrl::fromLocalFile(path)));
#endif
    return true;
}

/**
 * @brief 清除播放源（临时 WAV 保留在缓存中，切回同一信号可复用；析构时统一清理）
 */
void DASignalPlayer::clearSignal()
{
    if (mPlayer) {
        mPlayer->stop();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        mPlayer->setSource(QUrl());
#else
        mPlayer->setMedia(QMediaContent());
#endif
    }
    mTempWavPath.clear();
    mMediaPath.clear();
}

void DASignalPlayer::play()
{
    if (mPlayer) {
        mPlayer->play();
    }
}

void DASignalPlayer::pause()
{
    if (mPlayer) {
        mPlayer->pause();
    }
}

void DASignalPlayer::stop()
{
    if (mPlayer) {
        mPlayer->stop();
    }
}

/**
 * @brief 跳转到指定毫秒（拖动滑块 / 点击波形定位）
 */
void DASignalPlayer::seekMs(qint64 ms)
{
    if (mPlayer) {
        mPlayer->setPosition(qMax< qint64 >(0, ms));
    }
}

qint64 DASignalPlayer::positionMs() const
{
    return mPlayer ? mPlayer->position() : 0;
}

qint64 DASignalPlayer::durationMs() const
{
    return mPlayer ? mPlayer->duration() : 0;
}

bool DASignalPlayer::isPlaying() const
{
    return mPlayer && mPlayer->state() == QMediaPlayer::PlayingState;
}

/**
 * @brief 播放失败回调：给出明确原因（不静默）
 */
void DASignalPlayer::onPlayerError()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (mPlayer && mPlayer->error() != QMediaPlayer::NoError) {
        Q_EMIT errorOccurred(mPlayer->errorString());
    }
#endif
}
}  // namespace DA
