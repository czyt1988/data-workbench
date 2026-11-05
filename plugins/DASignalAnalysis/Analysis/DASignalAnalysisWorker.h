#ifndef DASIGNALANALYSISWORKER_H
#define DASIGNALANALYSISWORKER_H
#include <QObject>
#include <QString>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"

class QTimer;

namespace DA
{
/**
 * @brief 信号分析后台任务协调器
 *
 * 并发约定（方案 §6.7）：Python 侧起守护线程执行长任务，本类用 QTimer 定时轮询
 * DAWorkbench.DAPyBase.thread_status_manager 的任务状态，把进度写回界面；
 * 不在 C++ 侧新起 QThread 自造并发。
 */
class DASignalAnalysisWorker : public QObject
{
    Q_OBJECT
public:
    explicit DASignalAnalysisWorker(QObject* par = nullptr);
    ~DASignalAnalysisWorker() override;

    // 发起频谱计算（异步）；同一时刻只跑一个任务，重复调用会取消旧任务
    bool computeSpectrum(const DASignalPtr& signal, int channel, const DASignalAnalysisParams& params);
    // 取消当前任务
    void cancel();
    // 是否有任务在跑
    bool isRunning() const;

Q_SIGNALS:
    // 进度（0~100，阶段描述）
    void progressChanged(double percent, const QString& stage);
    // 频谱计算完成
    void spectrumReady(const DA::DASpectrumResult& result);
    // 任务失败（附原因）
    void failed(const QString& error);

private Q_SLOTS:
    void poll();

private:
    QTimer* mTimer { nullptr };
    QString mTaskId;
    bool mRunning { false };
};
}  // namespace DA

#endif  // DASIGNALANALYSISWORKER_H
