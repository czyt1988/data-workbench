#include "DASignalAnalysisUI.h"
#include "DASignalAnalysisUIHelpers.h"

// Qt
#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

// 3rdparty (SARibbon / ADS / qwt)
#include "DockWidget.h"

// DA / DAFigure
#include "DAUIInterface.h"
#include "DAStatusBarInterface.h"

// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DAPeakDockWidget.h"
#include "Dialogs/DASignalAnalysisSettingDialog.h"
#include "Analysis/DASignalMemoryEstimate.h"
#include "Analysis/DASignalAnalysisWorker.h"

namespace DA
{
/**
 * @brief 大信号内存预算门（评审 N-C1c / D19）
 *
 * 频谱、峰值表、包络与热图共用同一预算（1.5 GB）：超预算时只提示一次，
 * 用户确认后本信号不再重复弹窗；拒绝后本次会话对该信号跳过计算。
 * @param stage 已翻译的环节名（如「频谱」「峰值表」），用于消息正文
 * @return false 表示用户取消本次计算
 */
bool DASignalAnalysisUI::checkMemoryBudget(const DASignalPtr& sig, const QString& stage, qint64 estimatedBytes)
{
    if (!sig || estimatedBytes <= cAnalysisBudgetBytes) {
        return true;
    }
    const int verdict = mMemoryBudgetVerdict.value(sig.get(), cBudgetUnasked);
    if (verdict == cBudgetAllowed) {
        return true;
    }
    if (verdict == cBudgetDeclined) {
        return false;  // 该信号已被拒绝过，不再逐次弹窗
    }
    const double estimatedGb = double(estimatedBytes) / (1024.0 * 1024.0 * 1024.0);
    QMessageBox box(QMessageBox::Question,
                    tr("Large Signal"),  // cn:大信号
                    tr("The estimated peak memory for the %1 is about %2 GB, above the 1.5 GB budget.\n"
                       "Continue or cancel?")
                        // cn:%1 的预计峰值内存约 %2 GB，超过 1.5 GB 预算。继续还是取消？
                        .arg(stage)
                        .arg(estimatedGb, 0, 'f', 2),
                    QMessageBox::NoButton,
                    mPage);
    QPushButton* continueBtn = box.addButton(tr("Continue"), QMessageBox::AcceptRole);  // cn:继续
    QPushButton* cancelBtn   = box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(cancelBtn);  // 默认安全：不误触大内存计算
    box.exec();
    if (box.clickedButton() != continueBtn) {
        mMemoryBudgetVerdict[ sig.get() ] = cBudgetDeclined;
        if (mUI) {
            mUI->addInfoLogMessage(tr("Signal analysis cancelled: %1 needs more than the memory budget.").arg(stage));
            // cn:已取消信号分析：%1 超出内存预算。
        }
        return false;
    }
    mMemoryBudgetVerdict[ sig.get() ] = cBudgetAllowed;
    if (mUI) {
        mUI->addInfoLogMessage(
            tr("Large signal confirmed: %1 assumes about %2 GB of memory.").arg(stage).arg(estimatedGb, 0, 'f', 2));
        // cn:大信号已确认：%1 预计占用约 %2 GB 内存。
    }
    return true;
}

/**
 * @brief D5：内存超预算时的降级选择
 *
 * 三选：降低精度（热图减半栅格/块帧数，峰值曲线隔帧）/ 只扫可见区段 / 取消；
 * allowVisibleRange=false 时只给「降低精度 / 取消」两选。
 */
DASignalAnalysisUI::BudgetChoice DASignalAnalysisUI::promptBudgetChoice(const DASignalPtr& sig,
                                                                        const QString& stage,
                                                                        qint64 estimatedBytes,
                                                                        bool allowVisibleRange)
{
    Q_UNUSED(sig)
    const double estimatedGb = double(estimatedBytes) / (1024.0 * 1024.0 * 1024.0);
    QMessageBox box(QMessageBox::Question,
                    tr("Large Signal"),  // cn:大信号
                    tr("The estimated peak memory for the %1 is about %2 GB, above the 1.5 GB budget.\n"
                       "Reduce the precision, scan only the visible range, or cancel?")
                        // cn:%1 的预计峰值内存约 %2 GB，超过 1.5 GB 预算。降低精度、只扫可见区段，还是取消？
                        .arg(stage)
                        .arg(estimatedGb, 0, 'f', 2),
                    QMessageBox::NoButton,
                    mPage);
    QPushButton* reduceBtn  = box.addButton(tr("Reduce precision"), QMessageBox::ActionRole);  // cn:降低精度
    QPushButton* visibleBtn = nullptr;
    if (allowVisibleRange) {
        visibleBtn = box.addButton(tr("Scan visible range"), QMessageBox::ActionRole);  // cn:只扫可见区段
    }
    QPushButton* cancelBtn = box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(cancelBtn);  // 默认安全：不误触大内存计算
    box.exec();
    if (box.clickedButton() == nullptr || box.clickedButton() == cancelBtn) {
        return BudgetCancel;
    }
    if (visibleBtn && box.clickedButton() == visibleBtn) {
        return BudgetVisibleRangeOnly;
    }
    if (box.clickedButton() == reduceBtn) {
        return BudgetReducePrecision;
    }
    return BudgetProceed;
}

/**
 * @brief 触发当前信号的峰值表（与可选峰值曲线）计算
 */
void DASignalAnalysisUI::requestPeaks(bool withCurves)
{
    if (nullptr == mPage || nullptr == mWorker) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig || !sig->isValid()) {
        return;
    }
    // 内存预算（D5）：按真实 dtype/并发副本估算；峰值表与频谱共用同一预算
    const qint64 samples                = sig->sampleCount();
    const DASignalAnalysisParams params = paramsForSignal(sig);
    DASignalAnalysisView* view          = mPage->viewForSignal(sig.get());
    const int channel                   = view ? view->currentChannel() : 0;
    DASignalMemoryInputs in;
    in.sampleCount        = samples;
    in.sampleRate         = sig->sampleRate();
    in.resolutionHz       = params.resolutionHz;
    in.stftWindowMs       = params.stftWindowMs;
    in.stftOverlapPercent = params.stftOverlap;
    in.peakTopN           = params.peakTopN;
    if (!checkMemoryBudget(sig,
                           tr("peak table"),  // cn:峰值表
                           dasignalEstimateAnalysisBytes(DASignalAnalysisStage::PeakTable, in))) {
        return;
    }
    mWorker->computePeaks(makeTaskContext(sig, channel), params);
    if (!withCurves) {
        return;
    }
    // 峰值曲线：超预算时三选降级（D5）——只扫可见区段 / 隔帧降精度 / 取消
    const qint64 curveEstimated = dasignalEstimateAnalysisBytes(DASignalAnalysisStage::PeakCurves, in);
    qint64 scanStart            = -1;
    qint64 scanEnd              = -1;
    int frameStep               = 1;
    bool local                  = false;
    if (curveEstimated > kDASignalAnalysisBudgetBytes) {
        const BudgetChoice choice = promptBudgetChoice(sig, tr("peak curves"), curveEstimated, true);  // cn:峰值曲线
        if (choice == BudgetCancel) {
            mPeakCurvesRequested = false;
            return;
        }
        if (choice == BudgetVisibleRangeOnly) {
            double startSec = 0.0;
            double endSec   = 0.0;
            if (view && view->visibleTimeRange(&startSec, &endSec) && sig->sampleRate() > 0.0) {
                const double fs = sig->sampleRate();
                scanStart       = qint64(qMax(0.0, (startSec - sig->timeOrigin()) * fs));
                scanEnd         = qint64(qMin< double >(double(samples), (endSec - sig->timeOrigin()) * fs));
                local           = (scanStart > 0 || scanEnd < samples);
            }
            if (mUI) {
                mUI->addInfoLogMessage(tr("Peak curves were limited to the visible time range (local scan)."));
                // cn:峰值曲线已限制为可见时间范围（局部扫描）。
            }
        } else if (choice == BudgetReducePrecision) {
            frameStep = 2;
            local     = true;
            if (mUI) {
                mUI->addInfoLogMessage(tr("Peak curves use a coarser frame step (local scan)."));
                // cn:峰值曲线改用更粗的帧步进（局部扫描）。
            }
        }
    }
    if (!mPeakCurvesRequested) {
        mPeakCurvesRequested = true;
        mPeakCurvesLocal     = local;
        if (mPeakPanel) {
            mPeakPanel->setPartialScan(false);  // 新结果到达前先隐藏，避免上一信号的标注残留
        }
        mWorker->computePeakCurves(makeTaskContext(sig, channel), params, scanStart, scanEnd, frameStep);
    }
}

/**
 * @brief 构造后台任务上下文：信号 + 通道 + 当前参数版本
 */
DASignalTaskContext DASignalAnalysisUI::makeTaskContext(const DASignalPtr& sig, int channel) const
{
    DASignalTaskContext ctx;
    ctx.signal        = sig;
    ctx.channel       = channel;
    ctx.paramsVersion = mParamsVersion.value(sig.get(), 0);
    return ctx;
}

/**
 * @brief 参数版本 +1：迟到的旧参数结果会因版本不一致被丢弃（评审 U3）
 */
void DASignalAnalysisUI::bumpParamsVersion(const DASignalPtr& sig)
{
    if (!sig) {
        return;
    }
    ++mParamsVersionCounter;
    mParamsVersion[ sig.get() ] = mParamsVersionCounter;
}

/**
 * @brief 校验后台结果是否仍属于当前信号/通道/参数版本
 * @param ctx 任务上下文
 * @param checkChannel true 时还要求视图当前通道与任务通道一致
 */
bool DASignalAnalysisUI::isContextCurrent(const DASignalTaskContext& ctx, bool checkChannel) const
{
    if (!ctx.signal || !mPage) {
        return false;
    }
    if (!mPage->viewForSignal(ctx.signal.get())) {
        return false;  // 信号标签已关闭，结果丢弃
    }
    if (mParamsVersion.value(ctx.signal.get(), 0) != ctx.paramsVersion) {
        return false;  // 参数已变化，旧结果丢弃
    }
    if (checkChannel) {
        if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
            if (view->currentChannel() != ctx.channel) {
                return false;  // 已切到别的通道
            }
        }
    }
    return true;
}

/**
 * @brief 后台任务全部结束时清空状态栏进度（评审 §七-4）
 */
void DASignalAnalysisUI::clearProgressIfIdle()
{
    if (mUI && mWorker && !mWorker->isRunning()) {
        hideProgressIndicator();
        mUI->getStatusBar()->clearMessage();
    }
}

/**
 * @brief 信号的当前视图通道（无视图时 0）
 */
int DASignalAnalysisUI::channelOfSignal(const DASignalPtr& sig) const
{
    if (mPage && sig) {
        if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
            return view->currentChannel();
        }
    }
    return 0;
}

// ---------------------------------------------------------------- 分析

/**
 * @brief 当前信号的参数（无记录时返回默认参数）
 */
DASignalAnalysisParams DASignalAnalysisUI::paramsForSignal(const DASignalPtr& sig) const
{
    if (!sig) {
        return DASignalAnalysisParams::defaults();
    }
    DASignalAnalysisParams p = mParamsPerSignal.value(sig.get(), DASignalAnalysisParams::defaults());
    // 标定系数与单位以信号对象为唯一真相（评审 P3）：参数里的同名字段只作为表单回显
    p.calibration = sig->calibration();
    p.unit        = sig->unit();
    return p;
}

/**
 * @brief 指定信号/通道最近一次频谱结果（通道不匹配视为无缓存）
 */
DASpectrumResult DASignalAnalysisUI::spectrumForSignal(const DASignalPtr& sig, int channel) const
{
    if (!sig) {
        return DASpectrumResult();
    }
    if (mSpectrumCacheChannel.value(sig.get(), -1) != channel) {
        return DASpectrumResult();
    }
    return mSpectrumCache.value(sig.get(), DASpectrumResult());
}

/**
 * @brief 打开统一参数对话框，确定后立即重算一次（D20 / D21）
 */
void DASignalAnalysisUI::openAnalysisSettings()
{
    if (nullptr == mPage) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    DASignalAnalysisParams params = paramsForSignal(sig);
    if (!DASignalAnalysisSettingDialog::edit(&params, sig->sampleRate(), mPage)) {
        return;
    }
    mParamsPerSignal[ sig.get() ] = params;
    // 标定/单位写回信号对象（唯一真相，评审 P3），参数里的同名字段仅用于表单回显
    sig->setCalibration(params.calibration);
    sig->setUnit(params.unit);
    bumpParamsVersion(sig);
    // 对话框里可能改了任意参数（含幅值口径）：全量回填丝带控件与全局状态，避免口径分叉（评审 N-U3）
    syncParamsRibbonControls();
    // 参数变了：大信号内存预算重新询问一次（用户可能是有意用更大段长重试，评审 N-C1c）
    mMemoryBudgetVerdict.remove(sig.get());
    recomputeCurrentSignal();
}

/**
 * @brief 参数统一写回入口（方案 §5）：改字段 → 存表 → 版本 +1 → 按 scope 分级重算
 *
 * 不要所有改动都走全量重算：大信号改 STFT 跳步时重算整段频谱会明显变慢。
 * DisplayOnly（幅值口径）只更新显示状态与峰值表重绘，不递增参数版本、不提交计算。
 */
void DASignalAnalysisUI::applyParamsPatch(const DASignalPtr& sig,
                                          const std::function< void(DASignalAnalysisParams&) >& patch,
                                          ParamScope scope)
{
    if (!sig) {
        return;
    }
    DASignalAnalysisParams next = paramsForSignal(sig);
    if (patch) {
        patch(next);
    }
    mParamsPerSignal[ sig.get() ] = next;
    if (scope == ParamScope::DisplayOnly) {
        syncAmplitudeUiFromParams(next);
        updateDeltaFLabel(next);
        return;
    }
    // 参数变化：版本 +1（迟到结果丢弃），预算判定与旧峰值结果作废
    bumpParamsVersion(sig);
    mPeakCurvesRequested = false;  // 帧口径/名次可能变化，峰值曲线需要重扫
    mMemoryBudgetVerdict.remove(sig.get());
    if (DASignalAnalysisView* view = (mPage ? mPage->viewForSignal(sig.get()) : nullptr)) {
        view->setParams(next);
    }
    clearPeakState();
    recomputeScoped(scope);
    updateDeltaFLabel(next);
}

/**
 * @brief 按参数重算范围提交后台任务（方案 §5 的分级重算）
 *
 * - Preprocess：预处理影响频谱/热图/峰值与 dBFS 参考峰值
 * - Spectrum：窗函数/频率分辨率影响频谱、热图与峰值 → 三者都重算
 * - TimeFreq：只改 STFT 跳步重叠 → 热图 + 峰值曲线（不改频谱）
 * - Peaks：只重算峰值表与曲线
 * 包络只依赖原始波形，任何参数变化都不重算。
 */
void DASignalAnalysisUI::recomputeScoped(ParamScope scope)
{
    if (nullptr == mPage || nullptr == mWorker) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig || !sig->isValid()) {
        return;
    }
    DASignalAnalysisView* view          = mPage->viewForSignal(sig.get());
    const int channel                   = view ? view->currentChannel() : 0;
    const DASignalAnalysisParams params = paramsForSignal(sig);
    const bool peakCurves               = (mPeakDock && !mPeakDock->isClosed());
    switch (scope) {
    case ParamScope::Preprocess:
    case ParamScope::Spectrum: {
        mWorker->computeSpectrum(makeTaskContext(sig, channel), params);
        if (view) {
            view->requestHeatmapRefresh();
        }
        requestPeaks(peakCurves);
        break;
    }
    case ParamScope::TimeFreq: {
        // 跳步只影响时频栅格与峰值曲线；频谱段长没变，不重算整段频谱
        if (view) {
            view->requestHeatmapRefresh();
        }
        requestPeaks(peakCurves);
        break;
    }
    case ParamScope::Peaks: {
        requestPeaks(true);
        break;
    }
    case ParamScope::DisplayOnly:
        break;
    }
    updateCancelActionState();
}

/**
 * @brief 重新计算当前信号：频谱、热图、峰值表（必要时连曲线）统一按新参数提交一次（D21）
 */
void DASignalAnalysisUI::recomputeCurrentSignal()
{
    if (nullptr == mPage || nullptr == mWorker) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig || !sig->isValid()) {
        return;
    }
    DASignalAnalysisView* view          = mPage->viewForSignal(sig.get());
    const int channel                   = view ? view->currentChannel() : 0;
    const DASignalAnalysisParams params = paramsForSignal(sig);
    // 参数已变化：版本 +1（迟到结果丢弃），峰值曲线标志重置，旧峰值清空
    bumpParamsVersion(sig);
    clearPeakState();
    if (view) {
        view->setParams(params);
    }
    mWorker->computeSpectrum(makeTaskContext(sig, channel), params);
    if (view) {
        view->requestEnvelopeRefresh();
        view->requestHeatmapRefresh();
    }
    const bool peakCurves = (mPeakDock && !mPeakDock->isClosed());
    requestPeaks(peakCurves);
    if (mUI) {
        mUI->getStatusBar()->showMessage(tr("Signal analysis: parameters applied, recomputing."));
        // cn:信号分析：参数已应用，正在重算。
    }
    updateCancelActionState();
    updatePlaybackActionsState();
}

/**
 * @brief 后台进度 → 状态栏进度条（带防抖）与停止按钮可用性
 *
 * 多个任务并发时 worker 上报的是平均进度与任务数，进度文本显示「N 项任务」；
 * 只有进度条真正显示期间才刷新界面，短任务不会让状态栏闪一下。
 */
void DASignalAnalysisUI::onWorkerProgress(double percent, const QString& stage, int runningTaskCount)
{
    mProgressPercent   = qIsFinite(percent) ? qBound(0.0, percent, 100.0) : 0.0;
    mProgressStage     = stage;
    mProgressTaskCount = qMax(1, runningTaskCount);
    // 进度信号是「任务在跑」的最早信号：在这里刷新停止按钮可用性（评审 N-C7）
    updateCancelActionState();
    if (mProgressVisible) {
        applyProgressIndicator();
        return;
    }
    // 防抖：只在首次进入「有任务在跑」时起表；后续进度更新不重置定时器，
    // 否则连续进度会让显示时间被无限推迟
    if (mProgressDelayTimer && !mProgressDelayTimer->isActive() && mWorker && mWorker->isRunning()) {
        mProgressDelayTimer->start();
    }
}

/**
 * @brief 把缓存的进度/阶段写到状态栏进度条（含进度条右侧的停止按钮）
 */
void DASignalAnalysisUI::applyProgressIndicator()
{
    if (!mUI || !mWorker || !mWorker->isRunning()) {
        hideProgressIndicator();
        return;
    }
    DAStatusBarInterface* statusBar = mUI->getStatusBar();
    if (!statusBar) {
        return;
    }
    mProgressVisible = true;
    statusBar->setProgressCancelVisible(true);  // 只在本插件进度显示期间带出停止按钮
    statusBar->showProgressBar();
    statusBar->setProgress(qRound(mProgressPercent));
    statusBar->setProgressText(progressStageText());
}

/**
 * @brief 清掉本插件显示的状态栏进度与停止按钮
 *
 * 只有本插件确实显示过进度条时才隐藏：避免在防抖窗口内结束的小计算去关掉
 * 其它调用方（例如工程保存/加载）正在显示的进度条。
 */
void DASignalAnalysisUI::hideProgressIndicator()
{
    if (mProgressDelayTimer) {
        mProgressDelayTimer->stop();
    }
    if (!mProgressVisible) {
        return;
    }
    mProgressVisible = false;
    if (!mUI) {
        return;
    }
    if (DAStatusBarInterface* statusBar = mUI->getStatusBar()) {
        statusBar->setProgressCancelVisible(false);
        statusBar->hideProgressBar();
        statusBar->clearProgressText();
    }
}

/**
 * @brief 进度文本：单任务显示阶段名，多任务显示任务数（进度百分比由进度条自身显示）
 */
QString DASignalAnalysisUI::progressStageText() const
{
    if (mProgressTaskCount > 1) {
        return tr("%1 tasks").arg(mProgressTaskCount);  // cn:%1 项任务
    }
    return mProgressStage;
}

/**
 * @brief 频谱计算完成：按任务上下文校验后缓存并回填到对应信号视图（评审 U3）
 */
void DASignalAnalysisUI::onWorkerSpectrumReady(const DASignalTaskContext& ctx, const DASpectrumResult& result)
{
    if (!result.isValid()) {
        clearProgressIfIdle();
        updateCancelActionState();
        return;
    }
    if (!isContextCurrent(ctx, true)) {
        // 信号已关闭 / 已切通道 / 参数已过期：丢弃迟到结果，但仍要清状态栏进度（评审 N-U7）
        clearProgressIfIdle();
        updateCancelActionState();
        return;
    }
    const DASignal* key          = ctx.signal.get();
    mSpectrumCache[ key ]        = result;
    mSpectrumCacheChannel[ key ] = ctx.channel;
    if (DASignalAnalysisView* view = mPage->viewForSignal(key)) {
        view->setSpectrum(result);
    }
    if (mUI) {
        mUI->addInfoLogMessage(
            tr("Spectrum ready: resolution %1 Hz, %2 bins").arg(result.resolutionHz, 0, 'f', 4).arg(result.freqs.size()));
        // cn:频谱计算完成：分辨率 %1 赫兹，%2 个频点
    }
    Q_EMIT spectrumUpdated();
    // 只有当前信号才刷新峰值表（整段频谱的前 N 峰）
    DASignalPtr current = mPage->currentSignal();
    if (current && current.get() == key) {
        requestPeaks(false);
    }
    clearProgressIfIdle();
    updateCancelActionState();
}

/**
 * @brief 后台任务失败
 */
void DASignalAnalysisUI::onWorkerFailed(const QString& error)
{
    // 异常原文是英文诊断信息（e.what()），只写日志；界面给翻译后的可理解提示（评审 §七-9）
    reportOperationFailure(tr("Signal analysis failed. See the message log for details."), error);
    // cn:信号分析失败，详情见消息日志。
    mPlaybackWavPendingKey.clear();  // 播放 WAV 导出失败也要解除「导出中」状态
    updatePlaybackActionsState();
    clearProgressIfIdle();
    updateCancelActionState();
}

/**
 * @brief 视图请求计算频谱
 */
void DASignalAnalysisUI::onViewRequestSpectrum(const DASignalPtr& signal, int channel)
{
    if (!mWorker || !signal) {
        return;
    }
    // 内存预算门（D5）：频谱/PSD 为流式取数，峰值内存只与段长相关（不随信号总长增长）
    const DASignalAnalysisParams sp = paramsForSignal(signal);
    DASignalMemoryInputs spIn;
    spIn.sampleCount  = signal->sampleCount();
    spIn.sampleRate   = signal->sampleRate();
    spIn.resolutionHz = sp.resolutionHz;
    if (!checkMemoryBudget(signal,
                           tr("spectrum"),  // cn:频谱
                           dasignalEstimateAnalysisBytes(DASignalAnalysisStage::Spectrum, spIn))) {
        return;
    }
    mWorker->computeSpectrum(makeTaskContext(signal, channel), paramsForSignal(signal));
}

/**
 * @brief 视图请求计算波形包络
 */
void DASignalAnalysisUI::onViewRequestEnvelope(qint64 start, qint64 end, int width)
{
    auto* view = qobject_cast< DASignalAnalysisView* >(sender());
    if (!mWorker || !view || !view->signal()) {
        return;
    }
    // 内存预算门（D5）：包络按可见区段取数，峰值内存只与区段长度/目标列数相关
    DASignalMemoryInputs envIn;
    envIn.sampleCount  = view->signal()->sampleCount();
    envIn.sampleRate   = view->signal()->sampleRate();
    envIn.rangeSamples = qMax< qint64 >(0, end - start);
    envIn.gridColumns  = width;
    if (!checkMemoryBudget(view->signal(),
                           tr("waveform envelope"),  // cn:波形包络
                           dasignalEstimateAnalysisBytes(DASignalAnalysisStage::Envelope, envIn))) {
        return;
    }
    mWorker->computeEnvelope(makeTaskContext(view->signal(), view->currentChannel()), start, end, width);
}

/**
 * @brief 通道切换：失效频谱缓存与峰值，按新通道重算峰值
 *
 * 包络/频谱/热图的重新请求由视图自己发出（顺序在 channelChanged 之前），
 * 因此这里不能再改参数版本号，否则那几次请求的迟到结果会被误判为过期。
 */
void DASignalAnalysisUI::onViewChannelChanged(int channel)
{
    auto* view = qobject_cast< DASignalAnalysisView* >(sender());
    if (!view || !view->signal()) {
        return;
    }
    const DASignal* key = view->signal().get();
    mSpectrumCache.remove(key);
    mSpectrumCacheChannel.remove(key);
    if (mCurrentPeaksSignal == key) {
        clearPeakState();
    }
    DASignalPtr current = mPage ? mPage->currentSignal() : DASignalPtr();
    if (current && current.get() == key) {
        const bool peakViewOpen = (mPeakDock && !mPeakDock->isClosed());
        requestPeaks(peakViewOpen);
        // 通道切换后上下文标签/峰值标记状态跟随（视图内部已重发分析请求）
        syncSignalContextControls();
    }
    Q_UNUSED(channel)
}

/**
 * @brief 按在跑任务数刷新「取消计算」动作可用性（评审 N-C7）
 */
void DASignalAnalysisUI::updateCancelActionState()
{
    if (actionCancelAnalysis) {
        actionCancelAnalysis->setEnabled(mWorker && mWorker->runningTaskCount() > 0);
    }
}

/**
 * @brief 取消全部后台计算（评审 N-C7）
 */
void DASignalAnalysisUI::cancelAllComputations()
{
    if (!mWorker) {
        return;
    }
    mWorker->cancelAll();  // 每个被取消的任务会发 canceled → onWorkerCanceled
    mPlaybackWavPendingKey.clear();
    clearProgressIfIdle();
    updateCancelActionState();
    updatePlaybackActionsState();
}

/**
 * @brief 后台任务被用户取消：只清进度，不弹失败（评审 N-C7）
 */
void DASignalAnalysisUI::onWorkerCanceled(const QString& taskId)
{
    Q_UNUSED(taskId)
    // 导入任务被取消：worker 会补发 importFinished(空信号, 空 error)；这里先复位入口状态，
    // 保证即使信号时序变化，导入按钮也不会永久置灰。
    if (mImportTaskActive) {
        mImportTaskActive = false;
        updateImportActionsState();
    }
    mPlaybackWavPendingKey.clear();
    clearProgressIfIdle();
    updateCancelActionState();
    updatePlaybackActionsState();
}

/**
 * @brief 视图请求计算时频热图
 */
void DASignalAnalysisUI::onViewRequestHeatmap(int columns, int rows)
{
    auto* view = qobject_cast< DASignalAnalysisView* >(sender());
    if (!mWorker || !view || !view->signal()) {
        return;
    }
    // 内存预算（D5）：热图为流式取数，峰值内存 = 单块 float64 帧矩阵（∝ STFT 段长 × 块帧数）
    // + 显示栅格；超限时三选：降低精度（栅格减半 + 块帧数 64）/ 只扫可见区段 / 取消
    const DASignalAnalysisParams params = view->params();
    DASignalMemoryInputs in;
    in.sampleCount              = view->signal()->sampleCount();
    in.sampleRate               = view->signal()->sampleRate();
    in.stftWindowMs             = params.stftWindowMs;
    in.stftOverlapPercent       = params.stftOverlap;
    in.gridColumns              = columns;
    in.gridRows                 = rows;
    qint64 startSample          = -1;
    qint64 endSample            = -1;
    int blockFrames             = 256;
    const qint64 estimatedBytes = dasignalEstimateAnalysisBytes(DASignalAnalysisStage::Heatmap, in);
    if (estimatedBytes > kDASignalAnalysisBudgetBytes) {
        const BudgetChoice choice = promptBudgetChoice(view->signal(), tr("heatmap"), estimatedBytes, true);  // cn:热图
        if (choice == BudgetCancel) {
            return;
        }
        if (choice == BudgetReducePrecision) {
            columns     = qMax(64, columns / 2);
            rows        = qMax(64, rows / 2);
            blockFrames = 64;  // 单块 float64 帧矩阵随块帧数线性下降（真正的内存降级）
            if (mUI) {
                mUI->addInfoLogMessage(
                    tr("Heatmap precision reduced to %1 x %2 (smaller block size).").arg(columns).arg(rows));
                // cn:热图精度已降至 %1 x %2（更小的分块）。
            }
        } else if (choice == BudgetVisibleRangeOnly) {
            double startSec = 0.0;
            double endSec   = 0.0;
            if (view->visibleTimeRange(&startSec, &endSec) && view->signal()->sampleRate() > 0.0) {
                const double fs = view->signal()->sampleRate();
                startSample     = qint64(qMax(0.0, (startSec - view->signal()->timeOrigin()) * fs));
                endSample       = qint64(qMin< double >(double(view->signal()->sampleCount()),
                                                        (endSec - view->signal()->timeOrigin()) * fs));
            }
            if (mUI) {
                mUI->addInfoLogMessage(tr("Heatmap was limited to the visible time range."));
                // cn:热图已限制为可见时间范围。
            }
        }
    }
    mWorker->computeHeatmap(
        makeTaskContext(view->signal(), view->currentChannel()), params, columns, rows, startSample, endSample, blockFrames);
}

/**
 * @brief 时频栅格结果回填到发起请求的信号视图（按上下文校验）
 */
void DASignalAnalysisUI::onWorkerHeatmapReady(const DASignalTaskContext& ctx, const DAStftResult& result)
{
    if (!isContextCurrent(ctx, true)) {
        clearProgressIfIdle();  // 丢弃过期结果也要清进度（评审 N-U7）
        return;
    }
    if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
        view->setStft(result);
    }
    clearProgressIfIdle();
    updateCancelActionState();
}

/**
 * @brief 预处理「去直流」开关：写参数并重算频谱/热图/峰值
 */
void DASignalAnalysisUI::onPreprocessDcToggled(bool on)
{
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig || paramsForSignal(sig).removeDc == on) {
        return;  // 程序化回填不重算
    }
    applyParamsPatch(sig, [ on ](DASignalAnalysisParams& params) { params.removeDc = on; }, ParamScope::Preprocess);
}

/**
 * @brief 预处理「去趋势」改变：写参数并重算频谱/热图/峰值
 */
void DASignalAnalysisUI::onDetrendChanged(int index)
{
    Q_UNUSED(index)
    if (nullptr == mDetrendCombo) {
        return;
    }
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig) {
        return;
    }
    const QString detrend = mDetrendCombo->currentData().toString();
    if (detrend.isEmpty() || paramsForSignal(sig).detrend == detrend) {
        return;
    }
    applyParamsPatch(sig, [ detrend ](DASignalAnalysisParams& params) { params.detrend = detrend; }, ParamScope::Preprocess);
}

/**
 * @brief 频谱「窗函数」改变：写参数并重算频谱/热图/峰值（窗函数与 STFT 共用）
 */
void DASignalAnalysisUI::onWindowChanged(int index)
{
    Q_UNUSED(index)
    if (nullptr == mWindowCombo) {
        return;
    }
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig) {
        return;
    }
    const QString window = mWindowCombo->currentData().toString();
    if (window.isEmpty() || paramsForSignal(sig).window == window) {
        return;
    }
    applyParamsPatch(sig, [ window ](DASignalAnalysisParams& params) { params.window = window; }, ParamScope::Spectrum);
}

/**
 * @brief 频谱「幅值口径」改变：只改显示口径（dB 参考由它派生），只重绘不重算
 */
void DASignalAnalysisUI::onAmplitudeModeComboChanged(int index)
{
    Q_UNUSED(index)
    if (nullptr == mAmplitudeModeCombo) {
        return;
    }
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig) {
        return;
    }
    const QString mode = mAmplitudeModeCombo->currentData().toString();
    if (mode.isEmpty() || paramsForSignal(sig).amplitudeMode == mode) {
        return;
    }
    applyParamsPatch(
        sig,
        [ mode ](DASignalAnalysisParams& params) {
            params.amplitudeMode = mode;
            // D7：不再有独立「dB 参考」入口，参考口径由幅值口径派生（字段保留兼容旧档）
            params.dbReference = (mode == QLatin1String("dbRef1")) ? QStringLiteral("one") : QStringLiteral("fullScale");
        },
        ParamScope::DisplayOnly);
}

/**
 * @brief 频谱「频率分辨率」改变：写参数并联动 STFT 窗长，重算频谱/热图/峰值
 */
void DASignalAnalysisUI::onResolutionChanged(double value)
{
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig || !(value > 0.0)) {
        return;
    }
    if (qFuzzyCompare(paramsForSignal(sig).resolutionHz + 1.0, value + 1.0)) {
        return;  // 程序化回填（含 STFT 窗长联动）不重算
    }
    const double windowMs = signalStftWindowMsFromResolution(value);
    applyParamsPatch(
        sig,
        [ value, windowMs ](DASignalAnalysisParams& params) {
            params.resolutionHz = value;
            // 分辨率 ↔ STFT 窗长联动（Δf = 1/窗长），同一组布局参数只有一份真相
            if (windowMs >= 1.0 && windowMs <= 10000.0) {
                params.stftWindowMs = windowMs;
            }
        },
        ParamScope::Spectrum);
    syncParamsRibbonControls();  // 联动回填窗长/Δf 控件（阻断信号）
}

/**
 * @brief 时频「窗长」改变：写参数并联动频率分辨率，重算频谱/热图/峰值
 */
void DASignalAnalysisUI::onStftWindowChanged(double value)
{
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig || !(value > 0.0)) {
        return;
    }
    if (qFuzzyCompare(paramsForSignal(sig).stftWindowMs + 1.0, value + 1.0)) {
        return;
    }
    const double resolution = signalResolutionFromStftWindowMs(value);
    applyParamsPatch(
        sig,
        [ value, resolution ](DASignalAnalysisParams& params) {
            params.stftWindowMs = value;
            if (resolution >= 0.001 && resolution <= 1.0e6) {
                params.resolutionHz = resolution;  // 与频谱分辨率联动
            }
        },
        ParamScope::Spectrum);
    syncParamsRibbonControls();
}

/**
 * @brief 时频「跳步重叠」改变：只重算热图与峰值曲线（频谱段长没变）
 */
void DASignalAnalysisUI::onStftOverlapChanged(double value)
{
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig) {
        return;
    }
    if (qFuzzyCompare(paramsForSignal(sig).stftOverlap + 1.0, value + 1.0)) {
        return;
    }
    applyParamsPatch(sig, [ value ](DASignalAnalysisParams& params) { params.stftOverlap = value; }, ParamScope::TimeFreq);
}

/**
 * @brief 包络结果回填到发起请求的信号视图（按上下文校验）
 */
void DASignalAnalysisUI::onWorkerEnvelopeReady(const DASignalTaskContext& ctx,
                                               const QVector< double >& xs,
                                               const QVector< double >& mins,
                                               const QVector< double >& maxs)
{
    if (!isContextCurrent(ctx, true)) {
        clearProgressIfIdle();  // 丢弃过期结果也要清进度（评审 N-U7）
        return;
    }
    if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
        view->setEnvelope(xs, mins, maxs);
    }
    clearProgressIfIdle();
    updateCancelActionState();
}

}  // namespace DA
