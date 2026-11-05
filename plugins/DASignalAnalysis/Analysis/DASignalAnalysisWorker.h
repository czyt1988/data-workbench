#ifndef DASIGNALANALYSISWORKER_H
#define DASIGNALANALYSISWORKER_H
#include <QObject>
#include <QHash>
#include <QString>
#include <QVector>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"
#include "Analysis/DAStftResult.h"

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
 * 支持两类任务并存：频谱计算与波形包络（视图尺寸/参数变化时）。
 */
class DASignalAnalysisWorker : public QObject
{
    Q_OBJECT
public:
    explicit DASignalAnalysisWorker(QObject* par = nullptr);
    ~DASignalAnalysisWorker() override;

    // 发起频谱计算（异步）
    bool computeSpectrum(const DASignalPtr& signal, int channel, const DASignalAnalysisParams& params);
    // 发起波形包络计算（异步）；start/end 为采样点，width 为目标列数
    bool computeEnvelope(const DASignalPtr& signal, int channel, qint64 start, qint64 end, int width);
    // 发起时频热图计算（异步）；columns/rows 为显示栅格的目标列数与行数
    bool computeHeatmap(const DASignalPtr& signal,
                        int channel,
                        const DASignalAnalysisParams& params,
                        int columns,
                        int rows);
    // 取消全部任务
    void cancel();
    // 是否有任务在跑
    bool isRunning() const;

Q_SIGNALS:
    void progressChanged(double percent, const QString& stage);
    void spectrumReady(const DA::DASpectrumResult& result);
    // 包络结果（x 为采样点下标，mins/maxs 为每列极值）
    void envelopeReady(const QVector< double >& xs, const QVector< double >& mins, const QVector< double >& maxs);
    // 时频显示栅格结果
    void heatmapReady(const DA::DAStftResult& result);
    void failed(const QString& error);

private Q_SLOTS:
    void poll();

private:
    enum TaskKind
    {
        SpectrumTask,
        EnvelopeTask,
        HeatmapTask
    };
    struct TaskEntry
    {
        QString id;
        TaskKind kind { SpectrumTask };
    };

    void updateTimer();

private:
    QTimer* mTimer { nullptr };
    QHash< QString, TaskEntry > mTasks;
};
}  // namespace DA

#endif  // DASIGNALANALYSISWORKER_H
