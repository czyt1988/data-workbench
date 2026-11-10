#ifndef DASIGNALANALYSISWORKER_H
#define DASIGNALANALYSISWORKER_H
#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASignalTaskContext.h"
#include "Analysis/DASpectrumResult.h"
#include "Analysis/DAStftResult.h"
#include "Analysis/DAPeakResult.h"

class QTimer;

namespace DA
{
/**
 * @brief 信号分析后台任务协调器
 *
 * 并发约定（方案 §6.7）：Python 侧起守护线程执行长任务，本类用 QTimer 定时轮询
 * DAWorkbench.DAPyBase.thread_status_manager 的任务状态，把进度写回界面；
 * 不在 C++ 侧新起 QThread 自造并发。
 *
 * 每个任务在发起时携带 DASignalTaskContext（信号 + 通道 + 参数版本），结果回传时
 * 原样带回，界面层据此校验后回填，避免 A 信号的结果写到 B 信号/别的通道上。
 */
class DASignalAnalysisWorker : public QObject
{
    Q_OBJECT
public:
    explicit DASignalAnalysisWorker(QObject* par = nullptr);
    ~DASignalAnalysisWorker() override;

    // 发起频谱计算（异步）
    bool computeSpectrum(const DA::DASignalTaskContext& ctx, const DASignalAnalysisParams& params);
    // 发起波形包络计算（异步）；start/end 为采样点，width 为目标列数
    bool computeEnvelope(const DA::DASignalTaskContext& ctx, qint64 start, qint64 end, int width);
    // 发起时频热图计算（异步）；columns/rows 为显示栅格的目标列数与行数
    bool computeHeatmap(const DA::DASignalTaskContext& ctx,
                        const DASignalAnalysisParams& params,
                        int columns,
                        int rows);
    // 发起整段峰值表计算（异步）
    bool computePeaks(const DA::DASignalTaskContext& ctx, const DASignalAnalysisParams& params);
    // 发起每帧前 N 峰值曲线扫描（异步，单趟扫描，可能耗时几十秒）
    bool computePeakCurves(const DA::DASignalTaskContext& ctx, const DASignalAnalysisParams& params);
    // 发起数据帧列 → 会话临时 WAV 的后台导出（U9：数据帧来源播放前准备）
    bool computePlaybackWav(const DA::DASignalTaskContext& ctx);
    // 发起音视频导入的后台准备（I2：视频/压缩音频转码搬进 Python 守护线程，界面不阻塞）
    bool importMediaAsync(const QString& path, bool isVideo);
    // 取消单个在跑任务（含结果表清理与未完成临时 WAV 删除）；任务不存在返回 false
    bool cancelTask(const QString& taskId);
    // 取消全部在跑任务并清理（finalize/切工程时调用）
    void cancelAll();
    // 兼容旧入口：等价于 cancelAll()
    void cancel();
    // 在跑任务数（供界面控制取消按钮可用性）
    int runningTaskCount() const;
    // 在跑任务 id 快照
    QStringList runningTaskIds() const;
    // 是否有任务在跑
    bool isRunning() const;

Q_SIGNALS:
    void progressChanged(double percent, const QString& stage);
    void spectrumReady(const DA::DASignalTaskContext& ctx, const DA::DASpectrumResult& result);
    // 包络结果（x 为采样点下标，mins/maxs 为每列极值）
    void envelopeReady(const DA::DASignalTaskContext& ctx,
                       const QVector< double >& xs,
                       const QVector< double >& mins,
                       const QVector< double >& maxs);
    // 时频显示栅格结果
    void heatmapReady(const DA::DASignalTaskContext& ctx, const DA::DAStftResult& result);
    // 峰值表结果
    void peaksReady(const DA::DASignalTaskContext& ctx, const DA::DAPeakResult& result);
    // 每帧前 N 峰值曲线结果
    void peakCurvesReady(const DA::DASignalTaskContext& ctx, const DA::DAPeakCurveResult& result);
    // 数据帧播放 WAV 后台导出完成（U9）；wavPath 为会话临时目录内的可播放 WAV 绝对路径
    void playbackWavReady(const DA::DASignalTaskContext& ctx, const QString& wavPath);
    // 后台导入准备完成（I2）：成功时 signal 非空；失败/取消时 signal 为空、error 为英文细节（只写日志）
    void importFinished(const DA::DASignalPtr& signal, const QString& error);
    // 任务被主动取消（供界面清进度用；不弹「失败」提示）
    void canceled(const QString& taskId);
    void failed(const QString& error);

private Q_SLOTS:
    void poll();

private:
    enum TaskKind
    {
        SpectrumTask,
        EnvelopeTask,
        HeatmapTask,
        PeaksTask,
        PeakCurvesTask,
        PlaybackWavTask,
        ImportTask
    };
    struct TaskEntry
    {
        QString id;
        TaskKind kind { SpectrumTask };
        DASignalTaskContext ctx;
        // 仅 PlaybackWavTask：会话临时目录内的目标 WAV 路径（取消/失败时删除）
        QString wavPath;
        // 仅 ImportTask：源文件、预分配的转码目标 WAV、是否视频（取消/失败时删临时产物）
        QString importPath;
        QString importTempWav;
        bool importIsVideo { false };
    };

    void updateTimer();
    // 取数可用性前置检查（D1：来源缺失/数据帧被改名或移除/文件不可用时给出明确失败，而非静默空结果）
    bool ensureAnalyzable(const DASignalTaskContext& ctx, QString* detail);
    // 按任务类型调用对应 Python 模块的 cancel_task / take_result（C7）
    void cancelPythonTask(TaskKind kind, const QString& id);
    void discardPythonResult(TaskKind kind, const QString& id);

private:
    QTimer* mTimer { nullptr };
    QHash< QString, TaskEntry > mTasks;
};
}  // namespace DA

#endif  // DASIGNALANALYSISWORKER_H
