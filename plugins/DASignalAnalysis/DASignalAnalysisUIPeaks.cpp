#include "DASignalAnalysisUI.h"

// Qt
#include <cmath>
#include <limits>
#include <QComboBox>
#include <QJsonObject>
#include <QMainWindow>

// 3rdparty (SARibbon / ADS / qwt)
#include "DockWidget.h"

// DA / DAFigure
#include "DAUIInterface.h"

// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DAPeakDockWidget.h"
#include "Dialogs/DAPeakMarkerDisplayDialog.h"
#include "Signal/DASignalImporter.h"
#include "Analysis/DASignalAmplitude.h"

namespace DA
{
/**
 * @brief 清空峰值表/曲线缓存（切信号、加载工程时调用）
 */
void DASignalAnalysisUI::clearPeakState()
{
    mCurrentPeaks        = DAPeakResult();
    mCurrentCurves       = DAPeakCurveResult();
    mCurrentPeaksSignal  = nullptr;
    mCurrentPeaksChannel = 0;
    mPeakCurvesRequested = false;
    mPeakCurvesLocal     = false;
    if (mPeakPanel) {
        mPeakPanel->setPartialScan(false);
        mPeakPanel->setShowTimeColumn(false);
        mPeakPanel->setPeaks(mCurrentPeaks);
        mPeakPanel->setPeakCurves(mCurrentCurves);
    }
}

/**
 * @brief 峰值表结果回填（表模型 + 频谱图标记）；只接受当前信号/通道的结果
 */
void DASignalAnalysisUI::onWorkerPeaksReady(const DASignalTaskContext& ctx, const DAPeakResult& result)
{
    if (!isContextCurrent(ctx, false)) {
        clearProgressIfIdle();  // 丢弃过期结果也要清进度（评审 N-U7）
        return;
    }
    DASignalPtr current = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!current || current.get() != ctx.signal.get()) {
        clearProgressIfIdle();  // 峰值表/标记只属于当前信号
        return;
    }
    if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
        if (view->currentChannel() != ctx.channel) {
            clearProgressIfIdle();
            updateCancelActionState();
            return;
        }
    }
    mCurrentPeaks        = result;
    mCurrentPeaksSignal  = ctx.signal.get();
    mCurrentPeaksChannel = ctx.channel;
    if (mPeakPanel) {
        const DASignalAnalysisParams params = paramsForSignal(current);
        mPeakPanel->setAmplitudeDisplay(mAmplitudeMode, mDbReference, params.calibration, params.unit);
        if (DASignalAnalysisView* v = mPage->viewForSignal(ctx.signal.get())) {
            mPeakPanel->setSignalPeak(v->signalPeak() * params.calibration);
        }
        mPeakPanel->setPeaks(result);
        mPeakPanel->setPeakCurves(mCurrentCurves);
    }
    if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
        view->setPeaks(result);
    }
    clearProgressIfIdle();
    updateCancelActionState();
}

/**
 * @brief 每帧前 N 峰值曲线结果回填（只接受当前信号/通道）
 */
void DASignalAnalysisUI::onWorkerPeakCurvesReady(const DASignalTaskContext& ctx, const DAPeakCurveResult& result)
{
    if (!isContextCurrent(ctx, false)) {
        clearProgressIfIdle();  // 丢弃过期结果也要清进度（评审 N-U7）
        return;
    }
    DASignalPtr current = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!current || current.get() != ctx.signal.get()) {
        clearProgressIfIdle();
        updateCancelActionState();
        return;
    }
    mCurrentCurves = result;
    if (mPeakPanel) {
        mPeakPanel->setPartialScan(mPeakCurvesLocal);
    }
    if (mPeakPanel) {
        mPeakPanel->setPeakCurves(result);
    }
    clearProgressIfIdle();
    updateCancelActionState();
}

/**
 * @brief 跟随滑块时按当前帧刷新峰值表与峰值标记（D13）
 *
 * 用每帧前 N 扫描结果里当前时间最近的一帧重建峰值表（幅值、相对主峰、占比现场算）。
 */
void DASignalAnalysisUI::updatePeaksForCurrentFrame()
{
    if (!mPage || !mCurrentCurves.isValid()) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig) {
        return;
    }
    DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
    if (!view) {
        return;
    }
    // 找当前时间最近的帧
    const double t = view->currentTime();
    int frame      = 0;
    double bestD   = std::numeric_limits< double >::infinity();
    for (int i = 0; i < mCurrentCurves.times.size(); ++i) {
        const double d = std::fabs(mCurrentCurves.times.at(i) - t);
        if (d < bestD) {
            bestD = d;
            frame = i;
        }
    }
    DAPeakResult peaks;
    QVector< double > mags;
    for (int rank = 0; rank < mCurrentCurves.topN; ++rank) {
        const double f = mCurrentCurves.at(rank, frame);
        const double m = mCurrentCurves.magnitudeAt(rank, frame);
        if (std::isfinite(f) && std::isfinite(m)) {
            peaks.frequencies.append(f);
            mags.append(m);
        }
    }
    if (peaks.frequencies.isEmpty()) {
        return;
    }
    peaks.magnitudes = mags;
    // STFT 模式峰值表带时间列（帧中心时间 + 时间原点，评审 U11/U12）
    peaks.frameTime    = mCurrentCurves.times.value(frame) + sig->timeOrigin();
    peaks.hasFrameTime = true;
    const double ref   = mags.first();
    double powerSum    = 0.0;
    for (double m : mags) {
        powerSum += m * m;
    }
    for (double m : mags) {
        peaks.relDb.append(20.0 * std::log10(qMax(m, 1e-30) / qMax(ref, 1e-30)));
        peaks.share.append(powerSum > 0.0 ? (m * m / powerSum * 100.0) : 0.0);
    }
    if (mPeakPanel) {
        mPeakPanel->setShowTimeColumn(true);  // 跟随模式：峰值表带帧时间列（U11）
        mPeakPanel->setPeaks(peaks);
    }
    view->setPeaks(peaks);
}

/**
 * @brief 峰值窗口显隐切换（丝带「峰值窗口」按钮）
 */
void DASignalAnalysisUI::onPeakWindowToggled(bool on)
{
    if (!mPeakDock) {
        return;
    }
    mPeakDock->toggleView(on);
    if (on && !mPeakCurvesRequested) {
        requestPeaks(true);
    }
}

/**
 * @brief 峰值标记显隐（峰值 panel 第一个动作）
 *
 * 切通道路径：勾选状态从视图 peakMarkersVisible() 回填（视图是状态源）。UI 层仍保留
 * mPeakMarkersVisible，只作为「新建视图/未打开页面时」的默认值（比如恢复工程前先建视图）。
 */
void DASignalAnalysisUI::onPeakMarkerToggled(bool on)
{
    mPeakMarkersVisible = on;
    if (DASignalAnalysisView* view = currentView()) {
        view->setPeakMarkersVisible(on);
    }
    if (on && mCurrentPeaks.frequencies.isEmpty()) {
        requestPeaks(false);
    }
}

/**
 * @brief 标记数量改变（10/20/50）：更新参数并重算峰值表与曲线
 */
void DASignalAnalysisUI::onPeakTopNChanged()
{
    int topN = 10;
    if (actionPeakTop20 && actionPeakTop20->isChecked()) {
        topN = 20;
    } else if (actionPeakTop50 && actionPeakTop50->isChecked()) {
        topN = 50;
    }
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig) {
        return;
    }
    applyParamsPatch(sig, [ topN ](DASignalAnalysisParams& params) { params.peakTopN = topN; }, ParamScope::Peaks);
}

/**
 * @brief 峰值 panel「模式」改变（simple/advanced）：更新当前信号参数并重算峰值
 *
 * 模式只影响峰值查找（peaks.py 的高级选项是否生效），不影响频谱/热图，
 * 因此只重新提交峰值表与峰值曲线，不触发整段频谱重算。
 */
void DASignalAnalysisUI::onPeakModeChanged(int index)
{
    Q_UNUSED(index)
    if (nullptr == mPeakModeCombo) {
        return;
    }
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig) {
        return;
    }
    const QString mode = mPeakModeCombo->currentData().toString();
    if (mode.isEmpty() || paramsForSignal(sig).peakMode == mode) {
        return;  // 程序化回填触发的信号：参数没变不重算
    }
    applyParamsPatch(sig, [ mode ](DASignalAnalysisParams& params) { params.peakMode = mode; }, ParamScope::Peaks);
    syncParamsRibbonControls();
}

/**
 * @brief 峰值 panel「相对门限」改变（dB，相对最强峰）：更新当前信号参数并重算峰值
 */
void DASignalAnalysisUI::onPeakRelDbChanged(double value)
{
    DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!sig) {
        return;
    }
    if (qFuzzyCompare(paramsForSignal(sig).peakRelDb + 1.0, value + 1.0)) {
        return;  // 程序化回填触发的信号：参数没变不重算
    }
    applyParamsPatch(sig, [ value ](DASignalAnalysisParams& params) { params.peakRelDb = value; }, ParamScope::Peaks);
    syncParamsRibbonControls();
}

/**
 * @brief 峰值 panel「标记显示」：弹出峰值标记文本模板编辑对话框
 *
 * 模板是全局显示偏好（所有信号共用）：确定后立即写入配置并广播到已创建的全部视图；
 * 新建视图在 connectSignalView 里应用。模板只影响标记上方文本，不改标记位置与峰值数据。
 */
void DASignalAnalysisUI::openPeakMarkerDisplaySettings()
{
    QString textTemplate = mPeakMarkerText;
    // 预览幅值口径取当前信号的幅值显示模式（没有信号时用全局记录）
    bool dbMode = DASignalAmplitude::isDbMode(mAmplitudeMode);
    if (mPage) {
        if (DASignalPtr sig = mPage->currentSignal()) {
            dbMode = DASignalAmplitude::isDbMode(paramsForSignal(sig).amplitudeMode);
        }
    }
    QWidget* parent = (mPage ? static_cast< QWidget* >(mPage) : (mUI ? mUI->getMainWindow() : nullptr));
    if (!DAPeakMarkerDisplayDialog::edit(&textTemplate, dbMode, parent)) {
        return;
    }
    mPeakMarkerText = textTemplate;
    // 立即落盘（与色标显隐同一「读-改-写」模式，保留 ffmpegPath/colormap 等其它键）
    QJsonObject cfg                         = DASignalImporter::loadPluginConfig();
    cfg[ QStringLiteral("peakMarkerText") ] = mPeakMarkerText;
    DASignalImporter::savePluginConfig(cfg);
    applyPeakMarkerTextToViews();
}

/**
 * @brief 把全局峰值标记文本模板应用到已创建的全部信号视图
 */
void DASignalAnalysisUI::applyPeakMarkerTextToViews()
{
    if (nullptr == mPage) {
        return;
    }
    const QList< DASignalPtr > signalList = mPage->allSignals();
    for (const DASignalPtr& sig : signalList) {
        if (!sig) {
            continue;
        }
        if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
            view->setPeakMarkerText(mPeakMarkerText);
        }
    }
}

}  // namespace DA
