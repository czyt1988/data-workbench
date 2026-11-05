#include "DASignalAnalysisWorker.h"
#include <cstring>
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
bool DASignalAnalysisWorker::computeSpectrum(const DASignalPtr& signal,
                                             int channel,
                                             const DASignalAnalysisParams& params)
{
    if (!signal || !signal->isValid()) {
        Q_EMIT failed(tr("Signal is invalid or not loaded."));  // cn:信号无效或未载入。
        return false;
    }
    try {
        DAPyModule spectrum   = namedModule("DASignalAnalysis.spectrum");
        pybind11::dict source = buildSourceDict(signal, channel);
        pybind11::dict p      = paramsToDict(params);
        pybind11::object res  = spectrum.attr("compute_spectrum_async")(source, p);
        TaskEntry entry;
        entry.id   = QString::fromStdString(res.cast< std::string >());
        entry.kind = SpectrumTask;
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
bool DASignalAnalysisWorker::computeEnvelope(const DASignalPtr& signal,
                                             int channel,
                                             qint64 start,
                                             qint64 end,
                                             int width)
{
    if (!signal || !signal->isValid()) {
        return false;
    }
    try {
        DAPyModule envelope   = namedModule("DASignalAnalysis.envelope");
        pybind11::dict source = buildSourceDict(signal, channel);
        pybind11::object res  = envelope.attr("compute_envelope_async")(source, channel, start, end, width);
        TaskEntry entry;
        entry.id   = QString::fromStdString(res.cast< std::string >());
        entry.kind = EnvelopeTask;
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
 * @brief 取消全部任务
 */
void DASignalAnalysisWorker::cancel()
{
    if (mTasks.isEmpty()) {
        return;
    }
    try {
        DAPyModule spectrum = namedModule("DASignalAnalysis.spectrum");
        for (const TaskEntry& entry : mTasks) {
            if (entry.kind == SpectrumTask) {
                spectrum.attr("cancel_task")(entry.id.toStdString());
            }
            // 包络任务结果很小，交给 task 自然结束即可
        }
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisWorker::cancel:" << e.what();
    }
    mTasks.clear();
    mTimer->stop();
}

/**
 * @brief 轮询所有任务的运行状态
 */
void DASignalAnalysisWorker::poll()
{
    if (mTasks.isEmpty()) {
        mTimer->stop();
        return;
    }
    try {
        DAPyModule statusMgr = statusModule();
        QVector< QString > finishedIds;
        for (const TaskEntry& entry : mTasks) {
            pybind11::object info = statusMgr.attr("get_task_status")(entry.id.toStdString());
            if (info.is_none()) {
                finishedIds.append(entry.id);
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
            finishedIds.append(entry.id);
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
                pybind11::object raw = spectrum.attr("take_result")(entry.id.toStdString());
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
                Q_EMIT spectrumReady(result);
            } else {
                DAPyModule envelope  = namedModule("DASignalAnalysis.envelope");
                pybind11::object raw = envelope.attr("take_result")(entry.id.toStdString());
                if (raw.is_none()) {
                    continue;
                }
                pybind11::dict r = pybind11::cast< pybind11::dict >(raw);
                Q_EMIT envelopeReady(toDoubleVector(r[ "x" ]),
                                     toDoubleVector(r[ "mins" ]),
                                     toDoubleVector(r[ "maxs" ]));
            }
        }
        for (const QString& id : finishedIds) {
            mTasks.remove(id);
        }
        updateTimer();
    } catch (const std::exception& e) {
        mTasks.clear();
        mTimer->stop();
        qWarning() << "DASignalAnalysisWorker::poll:" << e.what();
        Q_EMIT failed(QString::fromUtf8(e.what()));
    }
}
}  // namespace DA
