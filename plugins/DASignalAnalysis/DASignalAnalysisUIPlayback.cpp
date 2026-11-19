#include "DASignalAnalysisUI.h"
#include "DASignalAnalysisUIHelpers.h"

// Qt
#include <QIcon>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>

// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DAPeakDockWidget.h"
#include "Analysis/DASignalAnalysisWorker.h"
#include "Playback/DASignalPlayer.h"

namespace DA
{
/**
 * @brief 当前信号的播放源切换
 *
 * 文件/视频来源直接用本地文件；数据帧来源没有真实音频文件，需把所选通道导出成临时 WAV：
 * 优先用本会话缓存，未缓存则在后台导出（评审 U9：不在界面线程整列导出），
 * 导出期间禁用播放动作，完成后走 onWorkerPlaybackWavReady 切播放源。
 */
void DASignalAnalysisUI::syncPlayerSource()
{
    if (!mPlayer || !mPage) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig || !sig->isValid()) {
        mPlayer->clearSignal();
        mPlaybackWavPendingKey.clear();
        updatePlaybackActionsState();
        return;
    }
    const int channel = channelOfSignal(sig);
    if (sig->sourceType() == DASignalSource::DataFrameSource) {
        const QString cached = mPlayer->cachedWavPath(sig, channel);
        if (cached.isEmpty()) {
            // 未缓存：后台导出该通道 WAV（worker 的 Python 守护线程 + QTimer 轮询）
            const QString key = QStringLiteral("%1:%2").arg(reinterpret_cast< quintptr >(sig.get())).arg(channel);
            if (mWorker && mPlaybackWavPendingKey != key) {
                if (mWorker->computePlaybackWav(makeTaskContext(sig, channel))) {
                    mPlaybackWavPendingKey = key;
                }
            }
            updatePlaybackActionsState();
            updateCancelActionState();
            if (mPeakPanel) {
                mPeakPanel->setTimeOrigin(sig->timeOrigin());
            }
            return;
        }
        QString error;
        if (!mPlayer->setPreparedDataFrameWav(sig, channel, cached, &error)) {
            reportOperationFailure(tr("Failed to prepare the playback audio for this signal."), error);
            // cn:准备该信号的播放音频失败。
        }
        mPlaybackWavPendingKey.clear();
    } else {
        QString error;
        if (!mPlayer->setSignal(sig, channel, &error)) {
            reportOperationFailure(tr("Cannot start playback for this signal."), error);
            // cn:无法开始播放该信号。
            updatePlaybackActionsState();
            return;
        }
        mPlaybackWavPendingKey.clear();
    }
    if (mPeakPanel) {
        mPeakPanel->setTimeOrigin(sig->timeOrigin());  // 峰值曲线时间轴与波形/热图对齐（U12）
    }
    updatePlaybackActionsState();
}

/**
 * @brief 按播放源与导出状态刷新播放动作可用性（评审 U9）
 */
void DASignalAnalysisUI::updatePlaybackActionsState()
{
    const bool hasSignal = (mPage && mPage->currentSignal());
    if (!hasSignal) {
        return;  // 无信号时由 setSignalDependentActionsEnabled(false) 统一置灰
    }
    const bool exporting = !mPlaybackWavPendingKey.isEmpty();
    const bool canPlay   = !exporting && mPlayer && mPlayer->hasSource();
    if (actionPlay) {
        const QSignalBlocker blocker(actionPlay);
        actionPlay->setEnabled(canPlay);
        if (!canPlay) {
            // 源不可用/导出期间一定不在播放态：勾选复位，避免按钮停在 ⏸
            actionPlay->setChecked(false);
        }
    }
    if (actionStop) {
        actionStop->setEnabled(canPlay);
    }
}

/**
 * @brief 数据帧播放 WAV 后台导出完成（评审 U9）
 *
 * 导出期间用户可能已切信号/通道，迟到的结果不能覆盖播放器：先做上下文校验。
 */
void DASignalAnalysisUI::onWorkerPlaybackWavReady(const DASignalTaskContext& ctx, const QString& wavPath)
{
    mPlaybackWavPendingKey.clear();
    if (!isContextCurrent(ctx, true)) {
        clearProgressIfIdle();
        updateCancelActionState();
        updatePlaybackActionsState();
        return;
    }
    QString error;
    if (mPlayer && !mPlayer->setPreparedDataFrameWav(ctx.signal, ctx.channel, wavPath, &error)) {
        reportOperationFailure(tr("Failed to prepare the playback audio for this signal."), error);
        // cn:准备该信号的播放音频失败。
    } else if (mPeakPanel && ctx.signal) {
        mPeakPanel->setTimeOrigin(ctx.signal->timeOrigin());
    }
    clearProgressIfIdle();
    updateCancelActionState();
    updatePlaybackActionsState();
}

/**
 * @brief 播放位置 → 视图播放头（播放头跟随，D27）
 */
void DASignalAnalysisUI::onPlayerPositionChanged(qint64 ms)
{
    if (!mPage) {
        return;
    }
    mSyncingFromPlayer = true;
    if (DASignalAnalysisView* v = currentView()) {
        // 播放头回写只更新显示，不再反发 currentTimeChanged（避免 seek 循环）
        v->setPlayheadFromPlayback(ms / 1000.0);
        if (v->isFollowSlider()) {
            updatePeaksForCurrentFrame();
        }
    }
    mSyncingFromPlayer = false;
    // 播放推进时回填底部工具栏时间滑块与时间文本（阻断信号防回环；此时未走 currentTimeChanged）
    if (mToolbarTimeSlider) {
        DASignalAnalysisView* view = currentView();
        if (view) {
            const QSignalBlocker blocker(mToolbarTimeSlider);
            const qint64 value = qBound< qint64 >(0, qRound64(view->currentTime() * 1000.0), mToolbarTimeSlider->maximum());
            mToolbarTimeSlider->setValue(int(value));
        }
    }
    updateToolbarTimeText();
}

/**
 * @brief 播放器播放/暂停状态变化：同步 ▶/⏸ 图标、勾选与时间文本
 *
 * 勾选写回一律 QSignalBlocker 阻断，否则会再触发 onPlayPauseToggled 反向调用播放器。
 */
void DASignalAnalysisUI::onPlayerStateChanged(bool playing)
{
    if (actionPlay) {
        const QSignalBlocker blocker(actionPlay);
        actionPlay->setChecked(playing);
        actionPlay->setIcon(QIcon(QString::fromUtf8(cIconPrefix) + QLatin1String(playing ? "pause.svg" : "play.svg")));
        actionPlay->setText(playing ? tr("Pause") : tr("Play"));  // cn:暂停 / 播放
        actionPlay->setToolTip(playing ? tr("Pause playback") : tr("Start playback"));
        // cn:暂停播放 / 开始播放
    }
    updateToolbarTimeText();
}

/**
 * @brief 「播放/暂停」动作切换：勾选 = 开始播放，取消勾选 = 暂停
 */
void DASignalAnalysisUI::onPlayPauseToggled(bool on)
{
    if (!mPlayer) {
        return;
    }
    if (on) {
        mPlayer->play();
    } else {
        mPlayer->pause();
    }
}

/**
 * @brief 停止播放并把播放头归零（■ 按钮与页面失活时共用）
 */
void DASignalAnalysisUI::stopPlayback()
{
    if (mPlayer) {
        mPlayer->stop();
    }
    if (DASignalAnalysisView* view = currentView()) {
        view->setCurrentTime(0.0);
    }
    syncSignalToolbar();
    updatePlaybackActionsState();
}

/**
 * @brief 用户拖动滑块 / 点击波形：暂停播放并跳转（方案 §8.4）
 */
void DASignalAnalysisUI::onViewTimeChanged(double seconds)
{
    // 视图时间变化（上下文/底部工具栏滑块、波形点击、停止）→ 回填底部工具栏（阻断信号防回环）
    syncSignalToolbar();
    if (mSyncingFromPlayer || !mPlayer) {
        return;
    }
    if (mPlayer->isPlaying()) {
        mPlayer->pause();
    }
    mPlayer->seekMs(qint64(seconds * 1000.0));
    if (DASignalAnalysisView* view = currentView()) {
        if (view->isFollowSlider()) {
            updatePeaksForCurrentFrame();
        }
    }
}

}  // namespace DA
