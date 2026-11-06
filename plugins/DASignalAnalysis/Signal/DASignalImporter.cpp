#include "DASignalImporter.h"
#include <cmath>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>
#include <QDebug>
#include <QCoreApplication>
#include "DAPybind11InQt.h"
#include <pybind11/stl.h>
#include "DADir.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "pandas/DAPyDataFrame.h"
#include "DAPybind11QtCaster.hpp"
#include "DAFileSignalSource.h"
#include "DADataFrameSignalSource.h"

namespace DA
{
namespace
{
DAPyModule importerModule()
{
    return DAPyModule("DASignalAnalysis.importer");
}

bool pythonBool(pybind11::dict d, const char* key)
{
    try {
        return d[ key ].cast< bool >();
    } catch (...) {
        return false;
    }
}

QString pythonString(pybind11::dict d, const char* key)
{
    try {
        return QString::fromStdString(d[ key ].cast< std::string >());
    } catch (...) {
        return QString();
    }
}

double pythonDouble(pybind11::dict d, const char* key)
{
    try {
        return d[ key ].cast< double >();
    } catch (...) {
        return 0.0;
    }
}

qint64 pythonInt64(pybind11::dict d, const char* key)
{
    try {
        return d[ key ].cast< qint64 >();
    } catch (...) {
        return 0;
    }
}

/// 读取插件配置文件（不存在返回空对象）
QJsonObject loadConfigObject()
{
    QFile f(DASignalImporter::configFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.object();
}

void saveConfigObject(const QJsonObject& obj)
{
    QFile f(DASignalImporter::configFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DASignalImporter: cannot write config file" << DASignalImporter::configFilePath();
        return;
    }
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

/**
 * @brief 由探测结果填充文件来源的元信息
 */
void applyProbe(DAFileSignalSource* src, pybind11::dict probe)
{
    src->mSampleRate  = pythonDouble(probe, "sample_rate");
    src->mChannelCount = static_cast< int >(pythonInt64(probe, "channels"));
    src->mSampleCount = pythonInt64(probe, "frames");
    if (src->mChannelCount <= 0) {
        src->mChannelCount = 1;
    }
    src->mChannelNames.clear();
    for (int i = 0; i < src->mChannelCount; ++i) {
        src->mChannelNames.append(QStringLiteral("Channel %1").arg(i + 1));  // cn:通道 N
    }
}

/**
 * @brief 用 FFmpeg 把任意输入转成 32 位浮点 WAV 到临时目录
 * @return 成功返回临时 WAV 路径，失败返回空串并写 error
 */
QString transcodeWithFFmpeg(const QString& inputPath, QString* error)
{
    const QString ffmpeg = DASignalImporter::ffmpegPath();
    if (ffmpeg.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter",
                                                 "FFmpeg is not available. Specify its path in the signal analysis settings.");
            // cn:未检测到 FFmpeg，请在分析设置里指定路径。
        }
        return QString();
    }
    const QString outPath =
        QDir(DASignalImporter::tempDir()).absoluteFilePath(QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".wav"));
    try {
        DAPyModule importer = importerModule();
        pybind11::dict res = importer.attr("run_ffmpeg")(ffmpeg, inputPath, outPath);
        if (!pythonBool(res, "ok")) {
            if (error) {
                *error = QCoreApplication::translate(
                             "DASignalImporter",
                             "FFmpeg failed to convert the file. Last output lines:\n%1")
                             // cn:FFmpeg 执行失败（附最后 10 行输出）
                             .arg(pythonString(res, "message"));
            }
            return QString();
        }
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        return QString();
    }
    return outPath;
}

/**
 * @brief 探测并构建文件来源信号
 */
DASignalPtr buildFileSignal(const QString& readPath,
                            const QString& originalPath,
                            bool isTemporary,
                            DASignalSource::SourceType sourceType,
                            QString* error)
{
    try {
        DAPyModule importer = importerModule();
        pybind11::dict probe = importer.attr("probe_audio")(readPath);
        if (!pythonBool(probe, "ok")) {
            if (error) {
                *error = QCoreApplication::translate("DASignalImporter", "Cannot read audio file: %1")
                             // cn:无法读取音频文件：%1
                             .arg(pythonString(probe, "error"));
            }
            return nullptr;
        }
        auto src = std::make_shared< DAFileSignalSource >(readPath, originalPath, isTemporary, sourceType);
        applyProbe(src.get(), probe);
        if (src->mSampleRate <= 0.0 || src->mSampleCount <= 0) {
            if (error) {
                *error = QCoreApplication::translate("DASignalImporter", "Audio file contains no samples.");
                // cn:音频文件不包含采样数据。
            }
            return nullptr;
        }
        const QString name = QFileInfo(originalPath).fileName();
        return std::make_shared< DASignal >(name, src);
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        return nullptr;
    }
}
}  // namespace

// ---------------------------------------------------------------- FFmpeg

/**
 * @brief 配置文件路径
 */
QString DASignalImporter::configFilePath()
{
    return DADir::getConfigPath() + QDir::separator() + QStringLiteral("signal-analysis-config.json");
}

/**
 * @brief 当前可用的 FFmpeg 路径：配置路径优先，其次系统 PATH（方案 §5.4）
 */
QString DASignalImporter::ffmpegPath()
{
    const QString configured = loadConfigObject().value(QStringLiteral("ffmpegPath")).toString();
    if (!configured.isEmpty()) {
        const QFileInfo fi(configured);
        if (fi.exists() && fi.isFile() && fi.isExecutable()) {
            return fi.absoluteFilePath();
        }
    }
    const QString inPath = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    return inPath;
}

/**
 * @brief 设置并持久化 FFmpeg 路径
 */
void DASignalImporter::setFFmpegPath(const QString& path)
{
    QJsonObject obj = loadConfigObject();
    if (path.isEmpty()) {
        obj.remove(QStringLiteral("ffmpegPath"));
    } else {
        obj[ QStringLiteral("ffmpegPath") ] = QDir::fromNativeSeparators(path);
    }
    saveConfigObject(obj);
}

/**
 * @brief FFmpeg 是否可用
 */
bool DASignalImporter::isFFmpegAvailable()
{
    return !ffmpegPath().isEmpty();
}

/**
 * @brief 插件专属临时目录（QStandardPaths::TempLocation 下的子目录）
 */
QString DASignalImporter::tempDir()
{
    return DADir::getTempPath(QStringLiteral("DASignalAnalysis"));
}

/**
 * @brief 清理临时目录中的抽取文件
 */
void DASignalImporter::cleanupTempFiles()
{
    const QString dirPath = tempDir();
    if (dirPath.isEmpty()) {
        return;
    }
    QDir dir(dirPath);
    const QFileInfoList files = dir.entryInfoList({ QStringLiteral("*.wav") }, QDir::Files);
    for (const QFileInfo& fi : files) {
        QFile::remove(fi.absoluteFilePath());
    }
}

// ---------------------------------------------------------------- 音频/视频

/**
 * @brief 文件是否需要 FFmpeg 转码（压缩音频格式或视频）
 */
bool DASignalImporter::fileNeedsFFmpeg(const QString& path)
{
    try {
        DAPyModule importer = importerModule();
        pybind11::object res = importer.attr("needs_ffmpeg")(path);
        return res.cast< bool >();
    } catch (const std::exception& e) {
        qWarning() << "DASignalImporter::fileNeedsFFmpeg:" << e.what();
        return false;
    }
}

/**
 * @brief 探测音频文件元信息（导入对话框只读展示采样率/通道/时长）
 *
 * 直读格式走 Python importer.probe_audio；需要转码的格式此处不实际转码，返回失败并给出原因，
 * 由导入流程在真正导入时再走 FFmpeg。
 */
bool DASignalImporter::probeAudioFile(const QString& path,
                                      double* sampleRate,
                                      int* channels,
                                      double* durationSec,
                                      QString* error)
{
    if (sampleRate) {
        *sampleRate = 0.0;
    }
    if (channels) {
        *channels = 0;
    }
    if (durationSec) {
        *durationSec = 0.0;
    }
    if (path.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter", "No file selected.");  // cn:未选择文件。
        }
        return false;
    }
    if (fileNeedsFFmpeg(path)) {
        if (error) {
            *error = QCoreApplication::translate(
                "DASignalImporter",
                "This format is decoded through FFmpeg; the sample rate is shown after import.");
            // cn:该格式经 FFmpeg 解码，采样率在导入后显示。
        }
        return false;
    }
    try {
        DAPyModule importer = importerModule();
        pybind11::dict probe = importer.attr("probe_audio")(path);
        if (!pythonBool(probe, "ok")) {
            if (error) {
                *error = pythonString(probe, "error");
            }
            return false;
        }
        if (sampleRate) {
            *sampleRate = pythonDouble(probe, "sample_rate");
        }
        if (channels) {
            *channels = int(pythonInt64(probe, "channels"));
        }
        if (durationSec) {
            *durationSec = pythonDouble(probe, "duration");
        }
        return true;
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        return false;
    }
}

/**
 * @brief 文件是否为视频
 */
bool DASignalImporter::isVideoFile(const QString& path)
{
    try {
        DAPyModule importer = importerModule();
        pybind11::object res = importer.attr("is_video")(path);
        return res.cast< bool >();
    } catch (const std::exception& e) {
        qWarning() << "DASignalImporter::isVideoFile:" << e.what();
        return false;
    }
}

/**
 * @brief 导入音频文件（直读或 FFmpeg 转码）
 */
DASignalPtr DASignalImporter::importAudioFile(const QString& path, QString* error)
{
    if (!QFileInfo::exists(path)) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter", "File does not exist: %1").arg(path);
            // cn:文件不存在：%1
        }
        return nullptr;
    }
    if (isVideoFile(path)) {
        return importVideoFile(path, error);
    }
    if (fileNeedsFFmpeg(path)) {
        const QString wav = transcodeWithFFmpeg(path, error);
        if (wav.isEmpty()) {
            return nullptr;
        }
        DASignalPtr sig = buildFileSignal(wav, path, true, DASignalSource::FileSource, error);
        if (!sig && error) {
            // 转码产物读不出来时给出明确原因
            *error = QCoreApplication::translate("DASignalImporter",
                                                 "The converted file could not be read: %1").arg(*error);
            // cn:转码后的文件无法读取：%1
        }
        return sig;
    }
    return buildFileSignal(path, path, false, DASignalSource::FileSource, error);
}

/**
 * @brief 导入视频文件：探测音频轨 → 抽到临时 WAV → 按文件来源建信号
 */
DASignalPtr DASignalImporter::importVideoFile(const QString& path, QString* error)
{
    if (!QFileInfo::exists(path)) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter", "File does not exist: %1").arg(path);
            // cn:文件不存在：%1
        }
        return nullptr;
    }
    const QString ffmpeg = ffmpegPath();
    if (ffmpeg.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter",
                                                 "FFmpeg is not available. Specify its path in the signal analysis settings.");
            // cn:未检测到 FFmpeg，请在分析设置里指定路径。
        }
        return nullptr;
    }
    // 先探测是否含音频轨，给出「文件不含音频轨」的明确原因
    try {
        DAPyModule importer = importerModule();
        pybind11::dict probe = importer.attr("has_audio_stream")(ffmpeg, path);
        if (pythonBool(probe, "ok") && !pythonBool(probe, "has_audio")) {
            if (error) {
                *error = QCoreApplication::translate("DASignalImporter", "The file does not contain an audio track.");
                // cn:文件不含音频轨。
            }
            return nullptr;
        }
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        return nullptr;
    }
    const QString wav = transcodeWithFFmpeg(path, error);
    if (wav.isEmpty()) {
        return nullptr;
    }
    return buildFileSignal(wav, path, true, DASignalSource::VideoSource, error);
}

// ---------------------------------------------------------------- 数据帧

/**
 * @brief 由数据帧创建信号（引用式）
 */
DASignalPtr DASignalImporter::importDataFrame(const DAData& data,
                                              const QString& timeColumn,
                                              const QStringList& valueColumns,
                                              double sampleRate,
                                              const QString& name,
                                              QString* error)
{
    if (!data.isDataFrame()) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter", "The source data is not a dataframe.");
            // cn:源数据不是数据帧。
        }
        return nullptr;
    }
    if (valueColumns.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter", "Please select at least one value column.");
            // cn:请至少选择一个幅值列。
        }
        return nullptr;
    }
    if (sampleRate <= 0.0) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter", "Sample rate must be a positive number.");
            // cn:采样率必须是正数。
        }
        return nullptr;
    }
    try {
        DAPyDataFrame df         = data.toDataFrame();
        const auto shape         = df.shape();
        auto src                 = std::make_shared< DADataFrameSignalSource >(data, timeColumn, valueColumns, sampleRate);
        src->mSampleCount        = static_cast< qint64 >(shape.first);
        if (src->mSampleCount <= 0) {
            if (error) {
                *error = QCoreApplication::translate("DASignalImporter", "The dataframe is empty.");
                // cn:数据帧为空。
            }
            return nullptr;
        }
        QString signalName = name;
        if (signalName.isEmpty()) {
            signalName = QStringLiteral("%1.%2").arg(data.getName(), valueColumns.first());
        }
        auto sig = std::make_shared< DASignal >(signalName, src);
        // 有时间列时取首值作为时间原点
        if (!timeColumn.isEmpty()) {
            try {
                pybind11::object first =
                    importerModule().attr("column_as_array")(df.object(), timeColumn);
                pybind11::array_t< double, pybind11::array::c_style | pybind11::array::forcecast > arr(first);
                if (arr.size() > 0 && std::isfinite(arr.data()[ 0 ])) {
                    sig->setTimeOrigin(arr.data()[ 0 ]);
                }
            } catch (const std::exception& e) {
                qWarning() << "DASignalImporter::importDataFrame: cannot read time origin:" << e.what();
            }
        }
        return sig;
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        return nullptr;
    }
}

/**
 * @brief 由时间列推断采样率：fs = 1 / median(diff(t))
 */
bool DASignalImporter::inferSampleRate(const DAData& data,
                                       const QString& timeColumn,
                                       double* sampleRate,
                                       double* durationSec,
                                       QString* error)
{
    if (sampleRate) {
        *sampleRate = 0.0;
    }
    if (durationSec) {
        *durationSec = 0.0;
    }
    if (!data.isDataFrame() || timeColumn.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("DASignalImporter", "No time column selected.");
            // cn:未选择时间列。
        }
        return false;
    }
    try {
        DAPyDataFrame df     = data.toDataFrame();
        DAPyModule importer  = importerModule();
        pybind11::dict res   = importer.attr("infer_sample_rate_from_column")(df.object(), timeColumn);
        if (!pythonBool(res, "ok")) {
            if (error) {
                *error = pythonString(res, "error");
            }
            return false;
        }
        if (sampleRate) {
            *sampleRate = pythonDouble(res, "sample_rate");
        }
        if (durationSec) {
            *durationSec = pythonDouble(res, "duration");
        }
        return true;
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        return false;
    }
}
}  // namespace DA
