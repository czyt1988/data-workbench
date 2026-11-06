#include "DASignalAnalysisWorker.h"
#include <cstring>
#include <utility>
#include <QTimer>
#include <QDebug>
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11QtCaster.hpp"
#include "Signal/DADataFrameSignalSource.h"

namespace DA
{
namespace
{
/// 轮询间隔（毫秒）
constexpr int cPollIntervalMs = 100;

DAPyModule namedModule(const char* name)
{
    return DAPyModule(name);
}

DAPyModule statusModule()
{
    return DAPyModule("DAWorkbench.DAPyBase.thread_status_manager");
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
 */
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
    return source;
}

/**
 * @brief 参数结构体 → Python 参数字典（键名与 spectrum.py / stft.py / peaks.py 一致）
 */
pybind11::dict paramsToDict(const DASignalAnalysisParams& params)
{
    pybind11::dict p;
    p[ "removeDc" ]         = params.removeDc;
    p[ "detrend" ]          = params.detrend.toStdString();
    p[ "window" ]           = params.window.toStdString();
    p[ "amplitudeMode" ]    = params.amplitudeMode.toStdString();
    p[ "dbReference" ]      = params.dbReference.toStdString();
    p[ "resolutionHz" ]     = params.resolutionHz;
    p[ "zeroPadding" ]      = params.zeroPadding;
    p[ "average" ]          = params.average.toStdString();
    p[ "overlap" ]          = params.overlap;
    p[ "stftWindowMs" ]     = params.stftWindowMs;
    p[ "stftOverlap" ]      = params.stftOverlap;
    p[ "stftZeroPadding" ]  = params.stftZeroPadding;
    p[ "peakMode" ]         = params.peakMode.toStdString();
    p[ "peakTopN" ]         = params.peakTopN;
    p[ "peakRelDb" ]        = params.peakRelDb;
    p[ "peakProminence" ]   = params.peakProminence;
    p[ "peakMinSpacingHz" ] = params.peakMinSpacingHz;
    p[ "peakMinHeight" ]    = params.peakMinHeight;
    p[ "calibration" ]      = params.calibration;
    p[ "unit" ]             = params.unit.toStdString();
    return p;
}
}  // namespace

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
    if (!ctx.signal || !ctx.signal->isValid()) {
        Q_EMIT failed(tr("Signal is invalid or not loaded."));  // cn:信号无效或未载入。
        return false;
    }
    try {
        DAPyModule spectrum   = namedModule("DASignalAnalysis.spectrum");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        pybind11::object res  = spectrum.attr("compute_spectrum_async")(source, p);
        TaskEntry entry;
        entry.id   = QString::fromStdString(res.cast< std::string >());
        entry.kind = SpectrumTask;
        entry.ctx  = ctx;
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
        Q_EMIT failed(QString::fromUtf8(e.what()));
        return false;
    }
}

/**
 * @brief 发起波形包络计算
 */
bool DASignalAnalysisWorker::computeEnvelope(const DASignalTaskContext& ctx, qint64 start, qint64 end, int width)
{
    if (!ctx.signal || !ctx.signal->isValid()) {
        return false;
    }
    try {
        DAPyModule envelope   = namedModule("DASignalAnalysis.envelope");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::object res = envelope.attr("compute_envelope_async")(source, ctx.channel, start, end, width);
        TaskEntry entry;
        entry.id   = QString::fromStdString(res.cast< std::string >());
        entry.kind = EnvelopeTask;
        entry.ctx  = ctx;
        if (entry.id.isEmpty()) {
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computeEnvelope:" << e.what();
        Q_EMIT failed(QString::fromUtf8(e.what()));
        return false;
    }
}

/**
 * @brief 发起时频热图计算（显示栅格，Python 侧分块计算并池化）
 */
bool DASignalAnalysisWorker::computeHeatmap(const DASignalTaskContext& ctx,
                                            const DASignalAnalysisParams& params,
                                            int columns,
                                            int rows)
{
    if (!ctx.signal || !ctx.signal->isValid()) {
        Q_EMIT failed(tr("Signal is invalid or not loaded."));  // cn:信号无效或未载入。
        return false;
    }
    try {
        DAPyModule stft       = namedModule("DASignalAnalysis.stft");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        pybind11::object res  = stft.attr("compute_heatmap_async")(source, p, columns, rows);
        TaskEntry entry;
        entry.id   = QString::fromStdString(res.cast< std::string >());
        entry.kind = HeatmapTask;
        entry.ctx  = ctx;
        if (entry.id.isEmpty()) {
            Q_EMIT failed(tr("Failed to start the time-frequency task."));  // cn:时频任务启动失败。
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computeHeatmap:" << e.what();
        Q_EMIT failed(QString::fromUtf8(e.what()));
        return false;
    }
}

/**
 * @brief 发起整段峰值表计算
 */
bool DASignalAnalysisWorker::computePeaks(const DASignalTaskContext& ctx, const DASignalAnalysisParams& params)
{
    if (!ctx.signal || !ctx.signal->isValid()) {
        Q_EMIT failed(tr("Signal is invalid or not loaded."));  // cn:信号无效或未载入。
        return false;
    }
    try {
        DAPyModule peaks      = namedModule("DASignalAnalysis.peaks");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        pybind11::object res  = peaks.attr("compute_peaks_async")(source, p);
        TaskEntry entry;
        entry.id   = QString::fromStdString(res.cast< std::string >());
        entry.kind = PeaksTask;
        entry.ctx  = ctx;
        if (entry.id.isEmpty()) {
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computePeaks:" << e.what();
        Q_EMIT failed(QString::fromUtf8(e.what()));
        return false;
    }
}

/**
 * @brief 发起每帧前 N 峰值曲线扫描
 */
bool DASignalAnalysisWorker::computePeakCurves(const DASignalTaskContext& ctx, const DASignalAnalysisParams& params)
{
    if (!ctx.signal || !ctx.signal->isValid()) {
        Q_EMIT failed(tr("Signal is invalid or not loaded."));  // cn:信号无效或未载入。
        return false;
    }
    try {
        DAPyModule peaks      = namedModule("DASignalAnalysis.peaks");
        pybind11::dict source = buildSourceDict(ctx.signal, ctx.channel);
        pybind11::dict p      = paramsToDict(params);
        pybind11::object res  = peaks.attr("compute_peak_curves_async")(source, p);
        TaskEntry entry;
        entry.id   = QString::fromStdString(res.cast< std::string >());
        entry.kind = PeakCurvesTask;
        entry.ctx  = ctx;
        if (entry.id.isEmpty()) {
            return false;
        }
        mTasks[ entry.id ] = entry;
        updateTimer();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::computePeakCurves:" << e.what();
        Q_EMIT failed(QString::fromUtf8(e.what()));
        return false;
    }
}

/**
 * @brief 取消全部任务
 */
void DASignalAnalysisWorker::cancel()
{
    if (mTasks.isEmpty()) {
        return;
    }
    try {
        DAPyModule spectrum = namedModule("DASignalAnalysis.spectrum");
        for (const TaskEntry& entry : std::as_const(mTasks)) {
            if (entry.kind == SpectrumTask) {
                spectrum.attr("cancel_task")(entry.id.toStdString());
            }
            // 包络结果很小；热图/峰值在 Python 侧分块检查取消标志，交给任务自然结束
        }
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::cancel:" << e.what();
    }
    mTasks.clear();
    mTimer->stop();
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
        mTasks.clear();
        mTimer->stop();
        qWarning() << "DASignalAnalysisWorker::poll:" << e.what();
        Q_EMIT failed(QString::fromUtf8(e.what()));
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
                Q_EMIT failed(tr("Computation was cancelled."));  // cn:计算已取消。
                continue;
            }
            if (!isSuccess) {
                const QString msg = QString::fromStdString(d[ "message" ].cast< std::string >());
                Q_EMIT failed(msg.isEmpty() ? tr("Computation failed.") : msg);  // cn:计算失败。
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
            Q_EMIT failed(QString::fromUtf8(e.what()));
        }
    }
    updateTimer();
}
}  // namespace DA
