#ifndef DASIGNALANALYSISWORKER_H
#define DASIGNALANALYSISWORKER_H
#include <QObject>
#include <QHash>
#include <QString>
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
    // 取消全部任务
    void cancel();
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
        PeakCurvesTask
    };
    struct TaskEntry
    {
        QString id;
        TaskKind kind { SpectrumTask };
        DASignalTaskContext ctx;
    };

    void updateTimer();

private:
    QTimer* mTimer { nullptr };
    QHash< QString, TaskEntry > mTasks;
};
}  // namespace DA

#endif  // DASIGNALANALYSISWORKER_H
