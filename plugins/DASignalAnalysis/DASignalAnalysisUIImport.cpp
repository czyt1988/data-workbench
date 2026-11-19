#include "DASignalAnalysisUI.h"

// Qt
#include <QDebug>
#include <QInputDialog>
#include <QJsonObject>
#include <QUndoStack>

// 3rdparty (SARibbon / ADS / qwt)
#include "DockWidget.h"

// DA / DAFigure
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DACommandInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
#include "DAFigureWidget.h"

// DASignalAnalysis
#include "Signal/DADataFrameSignalSource.h"
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DAPeakDockWidget.h"
#include "Dialogs/DASignalImportDialog.h"
#include "Signal/DASignalImporter.h"
#include "Analysis/DASignalAnalysisWorker.h"

namespace DA
{
/**
 * @brief 导入音频文件（WAV/AIFF/FLAC/OGG 直读；MP3/M4A/AAC 经 FFmpeg）
 */
void DASignalAnalysisUI::importAudioFile()
{
    // 导入即代表用户要用信号分析：先惰性创建页面（对话框也以页面为父）
    if (!ensurePageCreated()) {
        return;
    }
    DADataManagerInterface* dataMgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    DASignalImportDialog dlg(DASignalImportDialog::AudioMode, dataMgr, mPage);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    if (mImportTaskActive) {
        return;  // 已有导入在跑：入口已置灰，这里是防御
    }
    // I2：视频/压缩音频的转码与探测在 Python 守护线程做，界面立即返回；
    // 成功走 importFinished → onWorkerImportFinished，进度/取消走既有 worker 通道。
    mPendingImportIsVideo = false;
    mImportTaskActive     = true;
    updateImportActionsState();
    if (!mWorker || !mWorker->importMediaAsync(dlg.filePath(), false)) {
        mImportTaskActive = false;
        updateImportActionsState();
        // 启动失败原因由 worker failed 信号负责提示（已翻译文案）
    }
}

/**
 * @brief 从数据帧导入（时间列 + 幅值列 + 采样率）
 */
void DASignalAnalysisUI::importDataFrame()
{
    // 导入即代表用户要用信号分析：先惰性创建页面（对话框也以页面为父）
    if (!ensurePageCreated()) {
        return;
    }
    DADataManagerInterface* dataMgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    DASignalImportDialog dlg(DASignalImportDialog::DataFrameMode, dataMgr, mPage);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    QString error;
    DASignalPtr sig = DASignalImporter::importDataFrame(
        dlg.selectedData(), dlg.selectedTimeColumn(), dlg.selectedValueColumns(), dlg.sampleRate(), dlg.signalName(), &error);
    if (!sig) {
        reportOperationFailure(tr("Failed to import data frame."), error);  // cn:导入数据帧失败。
        return;
    }
    addSignalAndActivate(sig);
}

/**
 * @brief 导入视频（FFmpeg 抽取音频轨到临时 WAV）
 */
void DASignalAnalysisUI::importVideoFile()
{
    // 导入即代表用户要用信号分析：先惰性创建页面（对话框也以页面为父）
    if (!ensurePageCreated()) {
        return;
    }
    DADataManagerInterface* dataMgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    DASignalImportDialog dlg(DASignalImportDialog::VideoMode, dataMgr, mPage);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    if (mImportTaskActive) {
        return;  // 已有导入在跑：入口已置灰，这里是防御
    }
    mPendingImportIsVideo = true;
    mImportTaskActive     = true;
    updateImportActionsState();
    if (!mWorker || !mWorker->importMediaAsync(dlg.filePath(), true)) {
        mImportTaskActive = false;
        updateImportActionsState();
    }
}

/**
 * @brief 导入成功：加入页面并激活依赖信号的动作
 */
void DASignalAnalysisUI::addSignalAndActivate(const DASignalPtr& sig)
{
    if (!sig) {
        return;
    }
    // 工程恢复等路径可能绕过导入入口，这里兜底惰性创建页面
    if (!ensurePageCreated()) {
        return;
    }
    mPage->addSignal(sig);
    setSignalDependentActionsEnabled(true);
    syncPlayerSource();
    connectSignalView(sig);
    // 切到「信号分析」页面标签（导入后用户应立刻看到信号视图，而不是停在原页面）
    if (auto* dockingArea = mUI->getDockingArea()) {
        dockingArea->raiseDockByWidget(mPage);
    }
    // 首次导入信号成功后按偏好自动切到峰值窗口一次（D24/N-U1）
    if (!mFirstSignalActivated) {
        mFirstSignalActivated = true;
        showPeakDockForFirstSignal();
    }
    updateRelinkActionState();
    if (sig->source() && sig->source()->isMissing()) {
        // 来源缺失占位（工程恢复路径，评审 U16）：不谎报「已导入」，提示用重新指定入口
        mUI->addInfoLogMessage(tr("Signal \"%1\" is a placeholder: its data frame is missing.").arg(sig->name()));
        // cn:信号「%1」为占位：其数据帧缺失。
        return;
    }
    mUI->addInfoLogMessage(
        tr("Signal imported: %1 (%2 Hz, %3 s)").arg(sig->name()).arg(sig->sampleRate(), 0, 'f', 3).arg(sig->duration(), 0, 'f', 3));
    // cn:信号已导入：%1（%2 赫兹，%3 秒）
}

/**
 * @brief 首次导入信号后按偏好决定是否显示峰值窗口（D24 / 评审 N-U1）
 *
 * 用户此前明确关掉过峰值窗口（配置 peakDockVisible=false）时不再强制显示，
 * 否则重启/加载工程后偏好会被「首次导入自动显示」覆盖；从未有过偏好时按 D24 显示一次。
 */
void DASignalAnalysisUI::showPeakDockForFirstSignal()
{
    if (!mPeakDock) {
        return;
    }
    const bool preferVisible = (mPeakDockVisiblePref >= 0) ? (mPeakDockVisiblePref == 1) : true;
    if (!preferVisible) {
        return;
    }
    mPeakDock->toggleView(true);
    mPeakDock->setAsCurrentTab();
}

/**
 * @brief 连接单个信号视图的请求/事件信号并发起首次计算
 *
 * 导入（addSignalAndActivate）与「重新指定数据帧来源」（relinkCurrentSignalSource）共用，
 * 避免两条路径的连接逻辑分叉。
 */
void DASignalAnalysisUI::connectSignalView(const DASignalPtr& sig)
{
    if (!mPage || !sig) {
        return;
    }
    DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
    if (!view) {
        return;
    }
    view->setParams(paramsForSignal(sig));
    view->setTimeOrigin(sig->timeOrigin());
    if (!mLastColormap.isEmpty()) {
        view->setColormapName(mLastColormap);  // 插件偏好：上次色带（P2）
    }
    // 色标显隐偏好是插件级的（各信号共用）：新视图按当前偏好应用，之后以视图状态为准
    view->setColorBarVisible(mColorBarVisible);
    // 峰值标记开关由 UI 记录（视图无 getter）：新视图按当前开关状态应用
    view->setPeakMarkersVisible(mPeakMarkersVisible);
    // 峰值标记文本模板是全局显示偏好：新视图直接应用（之后的模板修改由对话框统一广播）
    view->setPeakMarkerText(mPeakMarkerText);
    // 把信号 figure 的撤销栈加入宿主 QUndoGroup（SF-11）：全局撤销/重做才能作用于信号图。
    // 栈由 figure 持有（值成员），figure 销毁时栈随 QObject 析构被 QUndoGroup 自动摘除
    if (DAFigureWidget* fig = view->figure()) {
        if (DACommandInterface* cmd = (mUI ? mUI->getCommandInterface() : nullptr)) {
            cmd->addStack(fig->getUndoStack());
        }
    }
    // 同一视图可能因重新指定来源/重复激活多次连接：成员槽用 UniqueConnection 防重复触发
    connect(view, &DASignalAnalysisView::spectrumRequested, this, &DASignalAnalysisUI::onViewRequestSpectrum, Qt::UniqueConnection);
    connect(view, &DASignalAnalysisView::envelopeRequested, this, &DASignalAnalysisUI::onViewRequestEnvelope, Qt::UniqueConnection);
    connect(view, &DASignalAnalysisView::heatmapRequested, this, &DASignalAnalysisUI::onViewRequestHeatmap, Qt::UniqueConnection);
    connect(view, &DASignalAnalysisView::currentTimeChanged, this, &DASignalAnalysisUI::onViewTimeChanged, Qt::UniqueConnection);
    connect(view, &DASignalAnalysisView::channelChanged, this, &DASignalAnalysisUI::onViewChannelChanged, Qt::UniqueConnection);
    // 「跟随滑块」只刷新峰值表显示，不触发 seek；同时回勾底部工具栏的「跟随播放」
    connect(view,
            &DASignalAnalysisView::followSliderToggled,
            this,
            &DASignalAnalysisUI::onViewFollowSliderToggled,
            Qt::UniqueConnection);
    // 跟随播放：视图按当前时间请求「当前帧真实幅值谱」，这里同步算完回填（与峰值曲线同口径）
    connect(view,
            &DASignalAnalysisView::followFrameSpectrumRequested,
            this,
            &DASignalAnalysisUI::onViewFollowFrameSpectrumRequested,
            Qt::UniqueConnection);
    // 当前 2D/3D 子图切换：上下文标签的工具动作/瀑布勾选跟随
    connect(view,
            &DASignalAnalysisView::currentSubChartChanged,
            this,
            &DASignalAnalysisUI::onViewSubChartChanged,
            Qt::UniqueConnection);
    // 视图创建/切换后与上下文标签、窗口底部工具栏对齐
    syncSignalContextControls();
    syncSignalToolbar();
    // 连接就绪后再发起首次计算（视图显示早于 connect，showEvent 里的请求会丢）
    view->requestInitialAnalysis();
}

/**
 * @brief 视图请求「跟随帧幅值谱」：界面线程同步短调用，成功回填、失败静默
 *
 * 跟随播放的频谱必须与峰值曲线同口径（同段长/跳步/窗/预处理），否则峰值标记会落在
 * 曲线下降沿（此前用 STFT 显示栅格当前列，频率方向池化到画布行数）。这里直接调用
 * worker 的同步方法（单帧 nperseg 5000 点量级，<10 毫秒），**不新起任务/线程**：
 * - 不走 ensureAnalyzable（失败会发 failed 信号，跟随播放每换一帧都会调用 → 刷界面消息）；
 * - 失败静默（不调用 reportOperationFailure），视图侧回退 STFT 栅格列显示；
 * - 不做参数版本校验：参数取当前值、同步返回，不存在迟到结果。
 * 峰值曲线（updatePeaksForCurrentFrame）不动：现在标记与真实帧谱同口径，会自然落在曲线上。
 */
void DASignalAnalysisUI::onViewFollowFrameSpectrumRequested(qint64 frameIndex)
{
    auto* view = qobject_cast< DASignalAnalysisView* >(sender());
    if (!view || !mWorker) {
        return;
    }
    const DASignalPtr sig = view->signal();
    if (!sig) {
        return;
    }
    DASpectrumResult result;
    const DASignalTaskContext ctx       = makeTaskContext(sig, view->currentChannel());
    const DASignalAnalysisParams params = paramsForSignal(sig);
    if (mWorker->computeFrameSpectrumSync(ctx, params, frameIndex, &result)) {
        view->setFollowFrameSpectrum(result.freqs, result.amplitudes);
    }
    // 失败静默：只写 worker 内的英文 qWarning，界面不回灌消息（跟随播放是每帧调用）
}

/**
 * @brief 信号标签页已关闭：清理该信号的缓存与待回填目标
 *
 * 页面已摘除视图并延后销毁，这里必须在同一轮事件循环内清掉待回填指针，
 * 否则后台任务回填时会写进即将销毁的视图。峰值表/曲线只属于当前信号，
 * 信号一走就清空；还有其它信号时按新的当前信号重算。
 */
void DASignalAnalysisUI::onSignalRemoved(const DASignalPtr& signal)
{
    if (!signal) {
        return;
    }
    const DASignal* key = signal.get();
    mParamsPerSignal.remove(key);
    mParamsVersion.remove(key);
    mSpectrumCache.remove(key);
    mSpectrumCacheChannel.remove(key);
    mMemoryBudgetVerdict.remove(key);
    if (mCurrentPeaksSignal == key) {
        clearPeakState();
    }

    const bool hasSignal = (mPage && mPage->signalCount() > 0);
    setSignalDependentActionsEnabled(hasSignal);
    if (hasSignal) {
        // 剩余信号接替为当前信号：峰值按它重算（峰值窗口还开着时连曲线一起）
        const bool peakViewOpen = (mPeakDock && !mPeakDock->isClosed());
        requestPeaks(peakViewOpen);
    }
    syncPlayerSource();
    updateRelinkActionState();
    // 关闭的是当前信号时当前视图已变化：上下文标签与底部工具栏跟随
    syncSignalContextControls();
    syncSignalToolbar();
}

/**
 * @brief 当前信号标签页切换：峰值数据只属于当前信号，切走后必须清空重算（评审 U4）
 *
 * 峰值表与曲线是由「当时的当前信号」算出来的；不发请求时旧表会停在新信号名下，
 * 数值与信号错配且没有任何提示。这里统一：同步播放源 → 若峰值数据不属于新信号
 * 则清空 → 峰值窗口可见时按新信号重算表与曲线；同时刷新时间原点与幅值口径。
 */
void DASignalAnalysisUI::onCurrentSignalChanged()
{
    updateRelinkActionState();
    syncPlayerSource();
    // 激活新当前视图 figure 的撤销栈（SF-11）：QUndoGroup 的 undo/redo 动作跟随当前信号；
    // 上一信号标签页关闭后其栈随 figure 销毁被 QUndoGroup 自动摘除
    if (DASignalAnalysisView* view = currentView()) {
        if (DAFigureWidget* fig = view->figure()) {
            if (QUndoStack* stack = fig->getUndoStack()) {
                stack->setActive();
            }
        }
    }
    // 切信号后当前视图整体变化：上下文标签控件与底部工具栏跟随（阻断信号防回环）
    syncSignalContextControls();
    syncSignalToolbar();
    if (!mPage || !mPeakPanel) {
        return;
    }
    DASignalPtr sig     = mPage->currentSignal();
    const DASignal* key = sig.get();
    if (!sig) {
        clearPeakState();
        return;
    }
    if (mCurrentPeaksSignal != key) {
        clearPeakState();
        // 峰值窗口可见时立即按新信号重算（表 2/3 + 曲线 1/3 同一停靠窗口，D23）
        if (mPeakDock && !mPeakDock->isClosed()) {
            requestPeaks(true);
        }
    }
    mPeakPanel->setTimeOrigin(sig->timeOrigin());
    const DASignalAnalysisParams params = paramsForSignal(sig);
    mPeakPanel->setAmplitudeDisplay(mAmplitudeMode, mDbReference, params.calibration, params.unit);
    updateRelinkActionState();
}

/**
 * @brief 刷新「重新指定数据帧来源」动作可用性（评审 U16）
 *
 * 只有当前信号的数据帧来源缺失时才可用；其余情况置灰，避免用户误以为是「换数据帧」。
 */
void DASignalAnalysisUI::updateRelinkActionState()
{
    if (!actionRelinkSource) {
        return;
    }
    bool missing = false;
    if (mPage) {
        DASignalPtr sig = mPage->currentSignal();
        if (sig && sig->source()) {
            auto dfSrc = std::dynamic_pointer_cast< DADataFrameSignalSource >(sig->source());
            missing    = (dfSrc && dfSrc->isMissing());
        }
    }
    actionRelinkSource->setEnabled(missing);
}

/**
 * @brief 重新指定数据帧来源（当前信号来源缺失时可用，评审 U16）
 *
 * 占位信号保留了工程里记录的列名/采样率/参数：这里让用户从数据管理器里挑一个数据帧，
 * 用保存的列名重新建信号并在同一标签位置替换，参数与视图状态尽量沿用。
 */
void DASignalAnalysisUI::relinkCurrentSignalSource()
{
    if (!mPage || !mCore) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig) {
        return;
    }
    auto dfSrc = std::dynamic_pointer_cast< DADataFrameSignalSource >(sig->source());
    if (!dfSrc || !dfSrc->isMissing()) {
        if (mUI) {
            mUI->addInfoLogMessage(tr("The current signal's source is available; there is nothing to relink."));
            // cn:当前信号的来源可用，无需重新指定。
        }
        return;
    }
    DADataManagerInterface* mgr = mCore->getDataManagerInterface();
    if (!mgr) {
        return;
    }
    const QList< DAData > all = mgr->getAllDatas();
    QStringList names;
    QList< DAData > frames;
    for (const DAData& d : all) {
        if (d.isDataFrame()) {
            names.append(d.getName());
            frames.append(d);
        }
    }
    if (names.isEmpty()) {
        reportError(tr("No data frame is available. Import or create a data frame first."));
        // cn:没有可用的数据帧，请先导入或创建数据帧。
        return;
    }
    int initial = names.indexOf(dfSrc->dataframeName());
    if (initial < 0) {
        initial = 0;
    }
    bool ok              = false;
    const QString chosen = QInputDialog::getItem(mPage,
                                                 tr("Relink Data Frame Source"),  // cn:重新指定数据帧来源
                                                 tr("Data frame:"),               // cn:数据帧：
                                                 names,
                                                 initial,
                                                 false,
                                                 &ok);
    if (!ok || chosen.isEmpty()) {
        return;
    }
    const int idx = names.indexOf(chosen);
    if (idx < 0 || idx >= frames.size()) {
        return;
    }
    // 保留原信号的显示元数据与参数（占位信号带着工程里的记录，评审 U16）
    const DASignalAnalysisParams params = paramsForSignal(sig);
    const double timeOrigin             = sig->timeOrigin();
    const double calibration            = sig->calibration();
    const QString unit                  = sig->unit();
    const QString signalName            = sig->name();
    // 旧视图的显示状态也尽量沿用（瀑布模式/色带/色标下限/跟随滚动/三维瀑布显示状态）
    int oldWaterfallMode = 0;
    QString oldColormap;
    double oldDbFloor  = -80.0;
    bool oldAutoScroll = false;
    QString oldWaterfallColormap;
    double oldWaterfallDbFloor = -80.0;
    bool oldWaterfallHighlight = true;
    QJsonObject oldWaterfallStyle;
    if (DASignalAnalysisView* oldView = mPage->viewForSignal(sig.get())) {
        oldWaterfallMode      = oldView->waterfallMode();
        oldColormap           = oldView->colormapName();
        oldDbFloor            = oldView->dbFloor();
        oldAutoScroll         = oldView->isHeatmapAutoScroll();
        oldWaterfallColormap  = oldView->waterfallColormapName();
        oldWaterfallDbFloor   = oldView->waterfallDbFloor();
        oldWaterfallHighlight = oldView->isWaterfallHighlighted();
        oldWaterfallStyle     = oldView->waterfallStyle();
    }
    QString error;
    DASignalPtr newSig = DASignalImporter::importDataFrame(
        frames.at(idx), dfSrc->timeColumn(), dfSrc->valueColumns(), dfSrc->sampleRate(), signalName, &error);
    if (!newSig) {
        reportOperationFailure(tr("Failed to relink the data frame source."), error);
        // cn:重新指定数据帧来源失败。
        return;
    }
    newSig->setName(signalName);
    newSig->setTimeOrigin(timeOrigin);
    newSig->setCalibration(calibration);
    newSig->setUnit(unit);
    // 参数先挂到新信号上：替换过程会 emit signalRemoved → 峰值重算，
    // 此时 paramsForSignal(新信号) 必须已经拿到原参数而不是默认值（评审 U16）
    mParamsPerSignal[ newSig.get() ] = params;
    const int index                  = mPage->indexOfSignal(sig.get());
    if (!mPage->replaceSignalAt(index, newSig)) {
        mParamsPerSignal.remove(newSig.get());
        reportError(tr("Failed to relink the data frame source."));  // cn:重新指定数据帧来源失败。
        return;
    }
    connectSignalView(newSig);
    if (DASignalAnalysisView* newView = mPage->viewForSignal(newSig.get())) {
        newView->setWaterfallMode(oldWaterfallMode);
        newView->setDbFloor(oldDbFloor);
        newView->setHeatmapAutoScroll(oldAutoScroll);
        if (!oldColormap.isEmpty()) {
            newView->setColormapName(oldColormap);
        }
        if (!oldWaterfallColormap.isEmpty()) {
            newView->setWaterfallColormapName(oldWaterfallColormap);
        }
        newView->setWaterfallDbFloor(oldWaterfallDbFloor);
        newView->setWaterfallHighlight(oldWaterfallHighlight);
        newView->applyWaterfallStyle(oldWaterfallStyle);
    }
    setSignalDependentActionsEnabled(true);
    syncPlayerSource();
    updateRelinkActionState();
    mUI->addInfoLogMessage(tr("Signal \"%1\" relinked to data frame \"%2\".").arg(signalName, chosen));
    // cn:信号「%1」已重新指向数据帧「%2」。
}

/**
 * @brief 按导入在途状态刷新三个导入入口（I2）
 */
void DASignalAnalysisUI::updateImportActionsState()
{
    const bool enabled = !mImportTaskActive;
    // 父动作（导入大按钮）随在途状态置灰：菜单整体不可打开，避免并发导入互相覆盖
    if (actionImport) {
        actionImport->setEnabled(enabled);
    }
    if (actionImportAudio) {
        actionImportAudio->setEnabled(enabled);
    }
    if (actionImportDataFrame) {
        actionImportDataFrame->setEnabled(enabled);
    }
    if (actionImportVideo) {
        // 视频入口还要看 FFmpeg 是否可用（refreshFFmpegAvailability 已负责该条件）
        actionImportVideo->setEnabled(enabled && DASignalImporter::isFFmpegAvailable());
    }
}

/**
 * @brief 后台导入准备完成（I2）
 *
 * signal 非空 = 成功：加入页面并激活（与同步导入同一收尾入口）；
 * signal 为空且 error 为空 = 用户取消：只清进度、不弹失败；
 * signal 为空且 error 非空 = 失败：界面显示可翻译提示，英文细节只写日志（§七-9）。
 */
void DASignalAnalysisUI::onWorkerImportFinished(const DA::DASignalPtr& signal, const QString& error)
{
    const bool wasVideo = mPendingImportIsVideo;
    mImportTaskActive   = false;
    updateImportActionsState();
    if (signal) {
        addSignalAndActivate(signal);
        if (mUI) {
            mUI->addInfoLogMessage(tr("Signal imported."));  // cn:信号导入完成。
        }
    } else if (!error.isEmpty()) {
        reportOperationFailure(wasVideo ? tr("Failed to import video.") : tr("Failed to import audio."), error);
        // cn:导入视频失败。/导入音频失败。
    }
    clearProgressIfIdle();
    updateCancelActionState();
}

}  // namespace DA
