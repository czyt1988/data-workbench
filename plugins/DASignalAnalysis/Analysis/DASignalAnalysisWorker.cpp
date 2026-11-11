#include "DASignalAnalysisWorker.h"
#include <cstring>
#include <stdexcept>
#include <utility>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11QtCaster.hpp"
#include "Signal/DADataFrameSignalSource.h"
#include "Signal/DASignalImporter.h"

namespace DA
{
namespace
{
/// 轮询间隔（毫秒）：既决定结果回填延迟，也决定每轮让出 GIL 的频率（见 cGilYieldMs）
constexpr int cPollIntervalMs = 20;
/**
 * @brief 每轮轮询后让出 GIL 的时长（毫秒）
 *
 * 内嵌解释器由 GUI 线程初始化后一直持有 GIL，Python 守护线程只能在 GUI 线程进入 Python
 * 的那一瞬间拿到 GIL。只轮询不让出时，分析任务几乎每帧都要等一个轮询周期：实测 9M 点
 * @32kHz 的频谱（约 1.1 万帧）要 20 分钟才回填，且频谱/时频/峰值表长时间空白。
 * 这里与 DataframeIOWorker 的后台等待口径一致：释放 GIL + 短暂休眠，把 GIL 真正让给
 * Python 任务线程；GUI 线程只阻塞轮询周期的一部分，期间仍能处理取消/重绘事件。
 */
constexpr int cGilYieldMs = 12;

DAPyModule namedModule(const char* name)
{
    return DAPyModule(name);
}

DAPyModule statusModule()
{
    return DAPyModule("DAWorkbench.DAPyBase.thread_status_manager");
}

/**
 * @brief 任务去重键：同类任务 + 信号 + 通道 + 参数版本，再拼调用方的请求特征
 *
 * 视图会在首次显示、外部连接完成后、resize 防抖、峰值窗口显示等时机重复请求同一份分析；
 * 同一键的任务在跑时不再重复提交，避免把 O(N) 计算跑多遍（大信号时成倍拖慢出图）。
 */
QString makeDedupeKey(const char* kind, const DASignalTaskContext& ctx, const QString& extra = QString())
{
    return QStringLiteral("%1|%2|%3|%4|%5")
        .arg(QString::fromLatin1(kind))
        .arg(QString::number(reinterpret_cast< quintptr >(ctx.signal.get())))
        .arg(ctx.channel)
        .arg(ctx.paramsVersion)
        .arg(extra);
}

/**
 * @brief numpy 一维数组 → QVector<double>
 */
QVector< double > toDoubleVector(pybind11::object arr)
{
    pybind11::array_t< double, pybind11::array::c_style | pybind11::array::forcecast > a(arr);
    QVector< double > v(static_cast< int >(a.size()));
    if (a.size() > 0) {
        std::memcpy(v.data(), a.data(), sizeof(double) * static_cast< size_t >(a.size()));
    }
    return v;
}

/**
 * @brief 构造 Python 编排需要的来源描述
 *
 * D1-a（Q3①）：额外注入 `"read"` 可调用对象，把 DASignalSource::readRange 按块暴露给
 * Python（read(channel, start, count) -> float32 ndarray）。Python 侧所有取数都优先走它，
 * 不再按 path/dataframeName 自己整段读取，从而避免「整段 float32 + 预处理副本」的峰值内存，
 * 也让数据帧改名不再断链（按 source 对象引用取数）。
 *
 * 生命周期：callable 捕获 shared_ptr<DASignal>，随任务结果/任务表引用释放；readRange 契约
 * 保证可跨线程调用、不碰界面对象，因此在 Python 守护线程里直接调用是安全的。
 */
pybind11::object makeSourceReader(const DASignalPtr& signal)
{
    return pybind11::cpp_function([signal](int channel, qint64 start, qint64 count) -> pybind11::array_t< float > {
        QVector< float > out;
        if (count <= 0) {
            return pybind11::array_t< float >(0);
        }
        QString error;
        if (!signal->readRange(channel, start, count, out, &error)) {
            // pybind11 会把 std::runtime_error 转成 Python RuntimeError，Python 编排层原样上抛；
            // worker 的 failed 分支只把英文细节写日志、界面显示可翻译消息（§七-9）。
            throw std::runtime_error(error.isEmpty() ? std::string("readRange failed") : error.toStdString());
        }
        pybind11::array_t< float > arr(static_cast< pybind11::ssize_t >(out.size()));
        if (!out.isEmpty()) {
            std::memcpy(arr.mutable_data(), out.constData(), sizeof(float) * static_cast< size_t >(out.size()));
        }
        return arr;
    });
}

pybind11::dict buildSourceDict(const DASignalPtr& signal, int channel)
{
    pybind11::dict source;
    if (signal->sourceType() == DASignalSource::DataFrameSource) {
        auto dfSource = std::dynamic_pointer_cast< DADataFrameSignalSource >(signal->source());
        source[ "type" ]          = "dataframe";
        source[ "dataframeName" ] = dfSource ? dfSource->data().getName().toStdString() : std::string();
        const QStringList cols    = dfSource ? dfSource->valueColumns() : QStringList();
        source[ "column" ] = (channel >= 0 && channel < cols.size()) ? cols.at(channel).toStdString() : std::string();
    } else {
        source[ "type" ] = "file";
        source[ "path" ] = signal->source()->playbackFilePath().toStdString();
    }
    source[ "channel" ]     = channel;
    source[ "sampleRate" ]  = signal->sampleRate();
    source[ "sampleCount" ] = signal->sampleCount();
    source[ "read" ]        = makeSourceReader(signal);
    return source;
}

/**
 * @brief 参数结构体 → Python 参数字典（键名与 spectrum.py / stft.py / peaks.py 一致）
 *
 * 键名/类型契约唯一真相在 DASignalAnalysisParams::toPythonDict()（纯逻辑、可单测）；
 * 这里只做 QVariant → Python 原生对象的显式搬运，不再重复一遍键名映射（避免两套真相）。
 */
pybind11::dict paramsToDict(const DASignalAnalysisParams& params)
{
    pybind11::dict p;
    const QVariantMap map = params.toPythonDict();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const QVariant& v = it.value();
        pybind11::object pyValue;
        switch (v.userType()) {
        case QMetaType::Bool:
            pyValue = pybind11::bool_(v.toBool());
            break;
        case QMetaType::Int:
            pyValue = pybind11::int_(v.toInt());
            break;
        case QMetaType::Double:
            pyValue = pybind11::float_(v.toDouble());
            break;
        case QMetaType::QString:
            pyValue = pybind11::str(v.toString().toStdString());
            break;
        default:
            pyValue = pybind11::cast(v);  // 兜底交给 DAPybind11QtCaster 的 QVariant 转换器
            break;
        }
        const QByteArray key = it.key().toUtf8();
        p[ key.constData() ] = pyValue;
    }
    return p;
}
}  // namespace

/**
 * @brief 取数可用性前置检查（D1-a / D2）
 *
 * 分析任务发起前先探测一次单样本取数：
 * - 来源缺失（数据帧被移出管理器后仍是占位状态）→ 明确提示重新指定/重新导入；
 * - readRange 失败（数据帧改名/移除后引用失效、文件不可用）→ 明确提示「无法读取」；
 * 而不是让 Python 侧静默拿到空数组或抛异常后只显示「计算失败」。英文细节只写日志（§七-9）。
 */
bool DASignalAnalysisWorker::ensureAnalyzable(const DASignalTaskContext& ctx, QString* detail)
{
    if (detail) {
        detail->clear();
    }
    if (!ctx.signal || !ctx.signal->isValid()) {
        if (detail) {
            *detail = QStringLiteral("signal is invalid or not loaded");
        }
        Q_EMIT failed(tr("Signal is invalid or not loaded."));  // cn:信号无效或未载入。
        return false;
    }
    if (ctx.signal->source() && ctx.signal->source()->isMissing()) {
        if (detail) {
            *detail = QStringLiteral("signal source is missing (placeholder)");
        }
        Q_EMIT failed(tr("The signal source is missing. Relink or re-import it before analyzing."));
        // cn:信号源缺失，请重新指定来源或重新导入后再分析。
        return false;
    }
    QVector< float > probe;
    QString error;
    if (!ctx.signal->readRange(ctx.channel, 0, 1, probe, &error)) {
        if (detail) {
            *detail = error;
        }
        qWarning() << "DASignalAnalysisWorker::ensureAnalyzable channel" << ctx.channel << ":" << error;
        Q_EMIT failed(
            tr("The signal source cannot be read (it may have been renamed or removed, or the file is unavailable)."));
        // cn:信号源无法读取（可能已改名或移除，或文件不可用）。
        return false;
    }
    return true;
}

DASignalAnalysisWorker::DASignalAnalysisWorker(QObject* par) : QObject(par)
{
    mTimer = new QTimer(this);
    mTimer->setInterval(cPollIntervalMs);
    connect(mTimer, &QTimer::timeout, this, &DASignalAnalysisWorker::poll);
}

DASignalAnalysisWorker::~DASignalAnalysisWorker()
{
    cancel();
}

bool DASignalAnalysisWorker::isRunning() const
{
    return !mTasks.isEmpty();
}

/**
 * @brief 更新轮询定时器状态（有任务就跑，没任务就停）
 */
void DASignalAnalysisWorker::updateTimer()
{
    if (mTasks.isEmpty()) {
        mTimer->stop();
    } else if (!mTimer->isActive()) {
        mTimer->start();
    }
}

/**
 * @brief 发起频谱计算（Python 守护线程执行，QTimer 轮询状态）
 */
bool DASignalAnalysisWorker::computeSpectrum(const DASignalTaskContext& ctx, const DASignalAnalysisParams& params)
{
    const QString key = makeDedupeKey("spectrum", ctx);
    if (hasRunningTask(key)) {
        return true;  // 同一份频谱已在计算：直接复用，不再提交重复任务
    }
    if (!ensureAnalyzable(ctx, nullptr)) {
        return false;
    }
    try {
        DAPyModule spectrum   = namedModule("DASignalAnalysis.spectrum");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        pybind11::object res  = spectrum.attr("compute_spectrum_async")(source, p);
        TaskEntry entry;
        entry.id        = QString::fromStdString(res.cast< std::string >());
        entry.kind      = SpectrumTask;
        entry.ctx       = ctx;
        entry.dedupeKey = key;
        if (entry.id.isEmpty()) {
            Q_EMIT failed(tr("Failed to start the spectrum task."));  // cn:频谱任务启动失败。
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        Q_EMIT progressChanged(0.0, tr("Computing spectrum..."));  // cn:正在计算频谱...
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computeSpectrum:" << e.what();
        Q_EMIT failed(tr("Failed to start the spectrum task."));  // cn:频谱任务启动失败。
        return false;
    }
}

/**
 * @brief 发起波形包络计算
 */
bool DASignalAnalysisWorker::computeEnvelope(const DASignalTaskContext& ctx, qint64 start, qint64 end, int width)
{
    const QString key = makeDedupeKey("envelope", ctx, QStringLiteral("%1|%2|%3").arg(start).arg(end).arg(width));
    if (hasRunningTask(key)) {
        return true;  // 同一区段/同一目标宽度的包络已在计算
    }
    if (!ensureAnalyzable(ctx, nullptr)) {
        return false;
    }
    try {
        DAPyModule envelope   = namedModule("DASignalAnalysis.envelope");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::object res = envelope.attr("compute_envelope_async")(source, ctx.channel, start, end, width);
        TaskEntry entry;
        entry.id        = QString::fromStdString(res.cast< std::string >());
        entry.kind      = EnvelopeTask;
        entry.ctx       = ctx;
        entry.dedupeKey = key;
        if (entry.id.isEmpty()) {
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computeEnvelope:" << e.what();
        Q_EMIT failed(tr("Failed to start the envelope task."));  // cn:包络任务启动失败。
        return false;
    }
}

/**
 * @brief 发起时频热图计算（显示栅格，Python 侧分块计算并池化）
 */
bool DASignalAnalysisWorker::computeHeatmap(const DASignalTaskContext& ctx,
                                            const DASignalAnalysisParams& params,
                                            int columns,
                                            int rows,
                                            qint64 startSample,
                                            qint64 endSample,
                                            int blockFrames)
{
    const QString key = makeDedupeKey(
        "heatmap",
        ctx,
        QStringLiteral("%1|%2|%3|%4|%5").arg(columns).arg(rows).arg(startSample).arg(endSample).arg(blockFrames));
    if (hasRunningTask(key)) {
        return true;  // 同一栅格尺寸/同一区段的热图已在计算
    }
    if (!ensureAnalyzable(ctx, nullptr)) {
        return false;
    }
    try {
        DAPyModule stft       = namedModule("DASignalAnalysis.stft");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        // D5：<0 表示整段（传 None），否则传采样点区间；blockFrames 只影响临时内存
        pybind11::object pyStart = (startSample >= 0) ? pybind11::cast(startSample) : pybind11::none();
        pybind11::object pyEnd   = (endSample >= 0) ? pybind11::cast(endSample) : pybind11::none();
        pybind11::object pyBlock = (blockFrames > 0) ? pybind11::cast(blockFrames) : pybind11::none();
        pybind11::object res     = stft.attr("compute_heatmap_async")(source, p, columns, rows, pyStart, pyEnd, pyBlock);
        TaskEntry entry;
        entry.id        = QString::fromStdString(res.cast< std::string >());
        entry.kind      = HeatmapTask;
        entry.ctx       = ctx;
        entry.dedupeKey = key;
        if (entry.id.isEmpty()) {
            Q_EMIT failed(tr("Failed to start the time-frequency task."));  // cn:时频任务启动失败。
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computeHeatmap:" << e.what();
        Q_EMIT failed(tr("Failed to start the time-frequency task."));  // cn:时频任务启动失败。
        return false;
    }
}

/**
 * @brief 发起整段峰值表计算
 */
bool DASignalAnalysisWorker::computePeaks(const DASignalTaskContext& ctx, const DASignalAnalysisParams& params)
{
    const QString key = makeDedupeKey("peaks", ctx);
    if (hasRunningTask(key)) {
        return true;  // 同一份峰值表已在计算
    }
    if (!ensureAnalyzable(ctx, nullptr)) {
        return false;
    }
    try {
        DAPyModule peaks      = namedModule("DASignalAnalysis.peaks");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        pybind11::object res  = peaks.attr("compute_peaks_async")(source, p);
        TaskEntry entry;
        entry.id        = QString::fromStdString(res.cast< std::string >());
        entry.kind      = PeaksTask;
        entry.ctx       = ctx;
        entry.dedupeKey = key;
        if (entry.id.isEmpty()) {
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computePeaks:" << e.what();
        Q_EMIT failed(tr("Failed to start the peak table task."));  // cn:峰值表任务启动失败。
        return false;
    }
}

/**
 * @brief 发起每帧前 N 峰值曲线扫描
 */
bool DASignalAnalysisWorker::computePeakCurves(const DASignalTaskContext& ctx,
                                               const DASignalAnalysisParams& params,
                                               qint64 startSample,
                                               qint64 endSample,
                                               int frameStep)
{
    const QString key = makeDedupeKey("peakcurves", ctx, QStringLiteral("%1|%2|%3").arg(startSample).arg(endSample).arg(frameStep));
    if (hasRunningTask(key)) {
        return true;  // 同一区段/同一帧步进的峰值曲线已在扫描
    }
    if (!ensureAnalyzable(ctx, nullptr)) {
        return false;
    }
    try {
        DAPyModule peaks      = namedModule("DASignalAnalysis.peaks");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        // D5：局部扫描（None=整段）与隔帧降精度
        pybind11::object pyStart = (startSample >= 0) ? pybind11::cast(startSample) : pybind11::none();
        pybind11::object pyEnd   = (endSample >= 0) ? pybind11::cast(endSample) : pybind11::none();
        pybind11::object pyStep  = pybind11::cast(std::max(1, frameStep));
        pybind11::object res     = peaks.attr("compute_peak_curves_async")(source, p, pyStart, pyEnd, pyStep);
        TaskEntry entry;
        entry.id        = QString::fromStdString(res.cast< std::string >());
        entry.kind      = PeakCurvesTask;
        entry.ctx       = ctx;
        entry.dedupeKey = key;
        if (entry.id.isEmpty()) {
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computePeakCurves:" << e.what();
        Q_EMIT failed(tr("Failed to start the peak curve task."));  // cn:峰值曲线任务启动失败。
        return false;
    }
}

/**
 * @brief 发起数据帧列 → 会话临时 WAV 的后台导出（U9）
 *
 * Python 守护线程导出 + 现有 QTimer 轮询，不新起 QThread；完成后 playbackWavReady
 * 带回 ctx 与路径，界面层校验 ctx 后再交给播放器。临时文件登记在会话目录，
 * 取消/失败/结果解析失败都会删除。
 */
bool DASignalAnalysisWorker::computePlaybackWav(const DASignalTaskContext& ctx)
{
    const QString key = makeDedupeKey("playbackwav", ctx);
    if (hasRunningTask(key)) {
        return true;  // 同一通道的播放音频已在导出
    }
    if (!ensureAnalyzable(ctx, nullptr)) {
        return false;
    }
    if (ctx.signal->sourceType() != DASignalSource::DataFrameSource) {
        Q_EMIT failed(tr("Playback audio export is only available for dataframe signals."));
        // cn:只有数据帧来源的信号需要导出播放音频。
        return false;
    }
    const QString wavPath = DASignalImporter::newSessionTempWavPath();
    if (wavPath.isEmpty()) {
        Q_EMIT failed(tr("Failed to create the temporary audio file."));  // cn:创建临时音频文件失败。
        return false;
    }
    try {
        DAPyModule importer   = namedModule("DASignalAnalysis.importer");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::object res  = importer.attr("export_playback_wav_async")(source, wavPath.toStdString());
        TaskEntry entry;
        entry.id        = QString::fromStdString(res.cast< std::string >());
        entry.kind      = PlaybackWavTask;
        entry.ctx       = ctx;
        entry.dedupeKey = key;
        entry.wavPath   = wavPath;
        if (entry.id.isEmpty()) {
            QFile::remove(wavPath);
            Q_EMIT failed(tr("Failed to start the playback audio export."));  // cn:播放音频导出任务启动失败。
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        Q_EMIT progressChanged(0.0, tr("Preparing playback audio..."));  // cn:正在准备播放音频...
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computePlaybackWav:" << e.what();
        QFile::remove(wavPath);
        Q_EMIT failed(tr("Failed to start the playback audio export."));  // cn:播放音频导出任务启动失败。
        return false;
    }
}

/**
 * @brief 发起音视频导入的后台准备（I2）
 *
 * 视频/压缩音频导入的耗时点在 FFmpeg 转码与探测；这里只把「准备」交给 Python 守护线程
 * （prepare_import_async：探测音频轨 → 转码到会话临时 WAV → 可读性校验），界面线程
 * 立即返回，进度经 progressChanged 上报、取消走 cancelAll。完成后 poll() 在界面线程
 * 用 DASignalImporter::importPreparedMedia 做文件头 probe 与信号构造（轻量）。
 */
bool DASignalAnalysisWorker::importMediaAsync(const QString& path, bool isVideo)
{
    if (path.isEmpty()) {
        Q_EMIT failed(tr("No file selected."));  // cn:未选择文件。
        return false;
    }
    if (!QFileInfo::exists(path)) {
        Q_EMIT failed(tr("File does not exist."));  // cn:文件不存在。
        return false;
    }
    const QString ffmpeg = DASignalImporter::ffmpegPath();
    if (isVideo && ffmpeg.isEmpty()) {
        Q_EMIT failed(tr("FFmpeg is not available. Specify its path in the signal analysis settings."));
        // cn:未检测到 FFmpeg，请在信号分析设置里指定路径。
        return false;
    }
    const QString tempWav = DASignalImporter::newSessionTempWavPath();
    if (tempWav.isEmpty()) {
        Q_EMIT failed(tr("Failed to create the temporary audio file."));  // cn:创建临时音频文件失败。
        return false;
    }
    try {
        DAPyModule importer = namedModule("DASignalAnalysis.importer");
        pybind11::object res = importer.attr("prepare_import_async")(
            path.toStdString(),
            std::string(isVideo ? "video" : "audio"),
            ffmpeg.toStdString(),
            tempWav.toStdString());
        TaskEntry entry;
        entry.id            = QString::fromStdString(res.cast< std::string >());
        entry.kind          = ImportTask;
        entry.importPath    = path;
        entry.importTempWav = tempWav;
        entry.importIsVideo = isVideo;
        if (entry.id.isEmpty()) {
            QFile::remove(tempWav);
            Q_EMIT failed(tr("Failed to start the import task."));  // cn:导入任务启动失败。
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        Q_EMIT progressChanged(0.0, tr("Preparing import..."));  // cn:正在准备导入...
        return true;
    } catch (const std::exception& e) {
        QFile::remove(tempWav);
        qWarning() << "DASignalAnalysisWorker::importMediaAsync:" << e.what();
        Q_EMIT failed(tr("Failed to start the import task."));  // cn:导入任务启动失败。
        return false;
    }
}

/**
 * @brief 按任务类型调用对应 Python 模块的 cancel_task（C7：取消链路按 kind 分发）
 */
void DASignalAnalysisWorker::cancelPythonTask(TaskKind kind, const QString& id)
{
    const std::string taskId = id.toStdString();
    try {
        switch (kind) {
        case SpectrumTask:
            namedModule("DASignalAnalysis.spectrum").attr("cancel_task")(taskId);
            break;
        case EnvelopeTask:
            namedModule("DASignalAnalysis.envelope").attr("cancel_task")(taskId);
            break;
        case HeatmapTask:
            namedModule("DASignalAnalysis.stft").attr("cancel_task")(taskId);
            break;
        case PeaksTask:
        case PeakCurvesTask:
            namedModule("DASignalAnalysis.peaks").attr("cancel_task")(taskId);
            break;
        case PlaybackWavTask:
            namedModule("DASignalAnalysis.importer").attr("cancel_task")(taskId);
            break;
        }
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::cancelPythonTask task" << id << ":" << e.what();
    }
}

/**
 * @brief 丢弃某任务在 Python 结果表中的条目（C7：取消后不让大数组滞留）
 */
void DASignalAnalysisWorker::discardPythonResult(TaskKind kind, const QString& id)
{
    const std::string taskId = id.toStdString();
    try {
        switch (kind) {
        case SpectrumTask:
            namedModule("DASignalAnalysis.spectrum").attr("take_result")(taskId);
            break;
        case EnvelopeTask:
            namedModule("DASignalAnalysis.envelope").attr("take_result")(taskId);
            break;
        case HeatmapTask:
            namedModule("DASignalAnalysis.stft").attr("take_result")(taskId);
            break;
        case PeaksTask:
        case PeakCurvesTask:
            namedModule("DASignalAnalysis.peaks").attr("take_result")(taskId);
            break;
        case PlaybackWavTask:
            namedModule("DASignalAnalysis.importer").attr("take_result")(taskId);
            break;
        }
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::discardPythonResult task" << id << ":" << e.what();
    }
}

/**
 * @brief 取消单个在跑任务
 */
bool DASignalAnalysisWorker::cancelTask(const QString& taskId)
{
    auto it = mTasks.find(taskId);
    if (it == mTasks.end()) {
        return false;
    }
    const TaskEntry entry = it.value();
    mTasks.remove(taskId);
    // 先置取消标志，再丢弃结果表里可能已完成的条目（C7）
    cancelPythonTask(entry.kind, entry.id);
    discardPythonResult(entry.kind, entry.id);
    if (entry.kind == PlaybackWavTask && !entry.wavPath.isEmpty()) {
        QFile::remove(entry.wavPath);
    }
    if (entry.kind == ImportTask && !entry.importTempWav.isEmpty()) {
        QFile::remove(entry.importTempWav);
    }
    updateTimer();
    Q_EMIT canceled(taskId);
    if (entry.kind == ImportTask) {
        // 空 error + 空信号 = 用户取消（界面据此复位导入状态，不弹失败）
        Q_EMIT importFinished(DA::DASignalPtr(), QString());
    }
    return true;
}

/**
 * @brief 取消全部在跑任务并清理结果表/未完成临时文件（C7）
 *
 * 收尾用 updateTimer() 而不是无条件 mTimer->stop()：canceled 是直连信号，
 * 界面槽可能在回调里同步发起新任务（切工程/finalize 前后都有清理后重算的路径），
 * 若在遍历结束后强制停表，新任务会永远不被轮询、进度与结果都回不来（D2 幂等性复核）。
 */
void DASignalAnalysisWorker::cancelAll()
{
    if (mTasks.isEmpty()) {
        updateTimer();
        return;
    }
    const QList< QString > ids = mTasks.keys();
    for (const QString& id : ids) {
        auto it = mTasks.find(id);
        if (it == mTasks.end()) {
            continue;
        }
        const TaskEntry entry = it.value();
        mTasks.remove(id);
        cancelPythonTask(entry.kind, entry.id);
        discardPythonResult(entry.kind, entry.id);
        if (entry.kind == PlaybackWavTask && !entry.wavPath.isEmpty()) {
            QFile::remove(entry.wavPath);
        }
        if (entry.kind == ImportTask && !entry.importTempWav.isEmpty()) {
            QFile::remove(entry.importTempWav);
        }
        Q_EMIT canceled(id);
        if (entry.kind == ImportTask) {
            Q_EMIT importFinished(DA::DASignalPtr(), QString());
        }
    }
    updateTimer();
}

/**
 * @brief 兼容旧入口：等价 cancelAll()
 */
void DASignalAnalysisWorker::cancel()
{
    cancelAll();
}

int DASignalAnalysisWorker::runningTaskCount() const
{
    return mTasks.size();
}

QStringList DASignalAnalysisWorker::runningTaskIds() const
{
    return mTasks.keys();
}

/**
 * @brief 是否已有同 dedupeKey 的任务在跑
 *
 * 命中时不重复提交（调用方按「已受理」处理）：同一份 O(N) 计算只跑一遍。
 */
bool DASignalAnalysisWorker::hasRunningTask(const QString& dedupeKey) const
{
    if (dedupeKey.isEmpty()) {
        return false;
    }
    for (auto it = mTasks.constBegin(); it != mTasks.constEnd(); ++it) {
        if (it.value().dedupeKey == dedupeKey) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 轮询所有任务的运行状态
 *
 * 注意（评审 B2）：本轮先对任务表做 id 快照，完成的任务**先摘除再发信号**。
 * 信号是直连的，UI 槽可能在回调里同步发起新任务（频谱完成 → 请求峰值），
 * 若在遍历 mTasks 期间插入/移除会触发 QHash 重哈希、迭代器失效。
 */
void DASignalAnalysisWorker::poll()
{
    if (mTasks.isEmpty()) {
        mTimer->stop();
        return;
    }
    const QList< QString > ids = mTasks.keys();
    DAPyModule statusMgr;
    try {
        statusMgr = statusModule();
    } catch (const std::exception& e) {
        // 状态模块不可用：先按 kind 尽力取消 Python 任务并丢弃结果表，避免守护线程
        // 继续跑、大数组滞留在结果表（D2 幂等性复核），再清空任务表。
        const QList< QString > ids = mTasks.keys();
        for (const QString& id : ids) {
            auto it = mTasks.find(id);
            if (it == mTasks.end()) {
                continue;
            }
            const TaskEntry entry = it.value();
            cancelPythonTask(entry.kind, entry.id);
            discardPythonResult(entry.kind, entry.id);
            if (entry.kind == PlaybackWavTask && !entry.wavPath.isEmpty()) {
                QFile::remove(entry.wavPath);
            }
            if (entry.kind == ImportTask && !entry.importTempWav.isEmpty()) {
                QFile::remove(entry.importTempWav);
            }
            Q_EMIT canceled(id);
            if (entry.kind == ImportTask) {
                Q_EMIT importFinished(DA::DASignalPtr(), QString());
            }
        }
        mTasks.clear();
        updateTimer();
        qWarning() << "DASignalAnalysisWorker::poll:" << e.what();
        Q_EMIT failed(tr("Failed to query background tasks."));  // cn:查询后台任务失败。
        return;
    }
    for (const QString& id : ids) {
        auto it = mTasks.find(id);
        if (it == mTasks.end()) {
            continue;  // 已被 cancel() 清掉
        }
        const TaskEntry entry = it.value();  // 复制：随后可能从表里摘除
        try {
            pybind11::object info = statusMgr.attr("get_task_status")(id.toStdString());
            if (info.is_none()) {
                mTasks.remove(id);
                Q_EMIT failed(tr("Background task disappeared."));  // cn:后台任务丢失。
                continue;
            }
            pybind11::dict d     = pybind11::cast< pybind11::dict >(info);
            const bool isRunning = d[ "is_running" ].cast< bool >();
            if (isRunning) {
                const double percent = d[ "progress" ].cast< double >();
                const QString stage  = QString::fromStdString(d[ "current_stage" ].cast< std::string >());
                Q_EMIT progressChanged(percent, stage);
                continue;
            }
            // 先摘除，再发信号：回调里同步发起的新任务只会插入新条目，不影响本轮快照遍历
            mTasks.remove(id);
            const bool isCanceled = d[ "is_canceled" ].cast< bool >();
            const bool isSuccess  = d[ "is_success" ].cast< bool >();
            if (isCanceled) {
                // 取消属于用户主动行为：走 canceled 信号让界面清进度，不再弹「失败」（C7）
                if (entry.kind == PlaybackWavTask && !entry.wavPath.isEmpty()) {
                    QFile::remove(entry.wavPath);
                }
                if (entry.kind == ImportTask && !entry.importTempWav.isEmpty()) {
                    QFile::remove(entry.importTempWav);
                }
                Q_EMIT canceled(id);
                if (entry.kind == ImportTask) {
                    Q_EMIT importFinished(DA::DASignalPtr(), QString());
                }
                continue;
            }
            if (!isSuccess) {
                // Python 侧错误文本（含 traceback）只进英文日志，界面显示可翻译消息（§七-9）
                const QString msg = QString::fromStdString(d[ "message" ].cast< std::string >());
                qWarning() << "DASignalAnalysisWorker::poll task" << id << "failed:" << msg;
                if (entry.kind == ImportTask) {
                    if (!entry.importTempWav.isEmpty()) {
                        QFile::remove(entry.importTempWav);
                    }
                    Q_EMIT importFinished(DA::DASignalPtr(), msg);
                    continue;
                }
                Q_EMIT failed(tr("Computation failed. See the message log for details."));
                // cn:计算失败，详见消息日志。
                continue;
            }
            if (entry.kind == SpectrumTask) {
                DAPyModule spectrum  = namedModule("DASignalAnalysis.spectrum");
                pybind11::object raw = spectrum.attr("take_result")(id.toStdString());
                if (raw.is_none()) {
                    Q_EMIT failed(tr("Spectrum result is empty."));  // cn:频谱结果为空。
                    continue;
                }
                pybind11::dict r = pybind11::cast< pybind11::dict >(raw);
                DASpectrumResult result;
                result.freqs        = toDoubleVector(r[ "freqs" ]);
                result.amplitudes   = toDoubleVector(r[ "amplitudes" ]);
                result.psdFreqs     = toDoubleVector(r[ "psd_freqs" ]);
                result.psdValues    = toDoubleVector(r[ "psd_values" ]);
                result.segLen       = r[ "seg_len" ].cast< int >();
                result.resolutionHz = r[ "resolution_hz" ].cast< double >();
                result.window       = QString::fromStdString(r[ "window" ].cast< std::string >());
                result.rms          = r[ "rms" ].cast< bool >();
                Q_EMIT spectrumReady(entry.ctx, result);
            } else if (entry.kind == EnvelopeTask) {
                DAPyModule envelope  = namedModule("DASignalAnalysis.envelope");
                pybind11::object raw = envelope.attr("take_result")(id.toStdString());
                if (raw.is_none()) {
                    continue;
                }
                pybind11::dict r = pybind11::cast< pybind11::dict >(raw);
                Q_EMIT envelopeReady(entry.ctx,
                                     toDoubleVector(r[ "x" ]),
                                     toDoubleVector(r[ "mins" ]),
                                     toDoubleVector(r[ "maxs" ]));
            } else if (entry.kind == PeaksTask) {
                DAPyModule peaks     = namedModule("DASignalAnalysis.peaks");
                pybind11::object raw = peaks.attr("take_result")(id.toStdString());
                if (raw.is_none()) {
                    continue;
                }
                pybind11::dict r = pybind11::cast< pybind11::dict >(raw);
                DAPeakResult peakResult;
                peakResult.frequencies  = toDoubleVector(r[ "frequencies" ]);
                peakResult.magnitudes   = toDoubleVector(r[ "magnitudes" ]);
                peakResult.relDb        = toDoubleVector(r[ "rel_db" ]);
                peakResult.share        = toDoubleVector(r[ "share" ]);
                peakResult.resolutionHz = r[ "resolution_hz" ].cast< double >();
                Q_EMIT peaksReady(entry.ctx, peakResult);
            } else if (entry.kind == PeakCurvesTask) {
                DAPyModule peaks     = namedModule("DASignalAnalysis.peaks");
                pybind11::object raw = peaks.attr("take_result")(id.toStdString());
                if (raw.is_none()) {
                    continue;
                }
                pybind11::dict r = pybind11::cast< pybind11::dict >(raw);
                DAPeakCurveResult curves;
                curves.times      = toDoubleVector(r[ "times" ]);
                curves.freqByRank = toDoubleVector(r[ "freq_by_rank" ]);
                curves.magByRank  = toDoubleVector(r[ "mag_by_rank" ]);
                curves.topN       = r[ "top_n" ].cast< int >();
                curves.nFrames    = r[ "n_frames" ].cast< int >();
                curves.nperseg    = r[ "nperseg" ].cast< int >();
                Q_EMIT peakCurvesReady(entry.ctx, curves);
            } else if (entry.kind == ImportTask) {
                DAPyModule importer  = namedModule("DASignalAnalysis.importer");
                pybind11::object raw = importer.attr("take_result")(id.toStdString());
                if (raw.is_none()) {
                    if (!entry.importTempWav.isEmpty()) {
                        QFile::remove(entry.importTempWav);
                    }
                    Q_EMIT importFinished(DA::DASignalPtr(), QStringLiteral("import task produced no result"));
                    continue;
                }
                pybind11::dict r   = pybind11::cast< pybind11::dict >(raw);
                const QString readPath = QString::fromStdString(r[ "read_path" ].cast< std::string >());
                const bool isTemporary = r[ "is_temporary" ].cast< bool >();
                QString buildError;
                DASignalPtr sig = DASignalImporter::importPreparedMedia(
                    readPath,
                    entry.importPath,
                    isTemporary,
                    entry.importIsVideo ? DASignalSource::VideoSource : DASignalSource::FileSource,
                    &buildError);
                if (!sig) {
                    if (isTemporary && !readPath.isEmpty()) {
                        QFile::remove(readPath);
                    }
                    Q_EMIT importFinished(DA::DASignalPtr(),
                                          buildError.isEmpty()
                                              ? QStringLiteral("failed to build a signal from the prepared file")
                                              : buildError);
                } else {
                    Q_EMIT importFinished(sig, QString());
                }
            } else if (entry.kind == PlaybackWavTask) {
                DAPyModule importer  = namedModule("DASignalAnalysis.importer");
                pybind11::object raw = importer.attr("take_result")(id.toStdString());
                QString wavPath       = entry.wavPath;
                if (raw.is_none()) {
                    // 结果缺失（异常/被清理）：删掉可能存在的半成品并报错
                    if (!entry.wavPath.isEmpty()) {
                        QFile::remove(entry.wavPath);
                    }
                    Q_EMIT failed(tr("Playback audio export produced no result."));  // cn:播放音频导出没有结果。
                    continue;
                }
                pybind11::dict r = pybind11::cast< pybind11::dict >(raw);
                wavPath          = QString::fromStdString(r[ "path" ].cast< std::string >());
                if (wavPath.isEmpty()) {
                    wavPath = entry.wavPath;
                }
                if (wavPath.isEmpty() || !QFile::exists(wavPath)) {
                    Q_EMIT failed(tr("Playback audio file was not created."));  // cn:播放音频文件未生成。
                    continue;
                }
                Q_EMIT playbackWavReady(entry.ctx, wavPath);
            } else {
                DAPyModule stft      = namedModule("DASignalAnalysis.stft");
                pybind11::object raw = stft.attr("take_result")(id.toStdString());
                if (raw.is_none()) {
                    continue;
                }
                pybind11::dict r = pybind11::cast< pybind11::dict >(raw);
                DAStftResult result;
                result.times         = toDoubleVector(r[ "times" ]);
                result.freqs         = toDoubleVector(r[ "freqs" ]);
                result.values        = toDoubleVector(r[ "values" ]);
                result.rows          = r[ "rows" ].cast< int >();
                result.columns       = r[ "columns" ].cast< int >();
                result.nperseg       = r[ "nperseg" ].cast< int >();
                result.hop           = r[ "hop" ].cast< int >();
                result.windowSeconds = r[ "window_seconds" ].cast< double >();
                Q_EMIT heatmapReady(entry.ctx, result);
            }
        } catch (const std::exception& e) {
            // 单个任务解析失败只丢弃它自己，不再清空整张任务表（评审 B1 的放大后果）
            mTasks.remove(id);
            qWarning() << "DASignalAnalysisWorker::poll task" << id << ":" << e.what();
            Q_EMIT failed(tr("Failed to read the background result."));  // cn:读取后台结果失败。
        }
    }
    // 让出 GIL 给 Python 守护线程：任务在跑时，GUI 线程若不主动释放，分析线程只能等到
    // 下一次进入 Python 的瞬间，几乎每帧一次轮询周期（大信号会从秒级拖到几十分钟）。
    // 释放 + 短暂休眠让守护线程连续计算；任务全部完成则不再休眠，正常停表。
    if (!mTasks.isEmpty()) {
        pybind11::gil_scoped_release release;
        QThread::msleep(static_cast< unsigned long >(cGilYieldMs));
    }
    updateTimer();
}
}  // namespace DA
