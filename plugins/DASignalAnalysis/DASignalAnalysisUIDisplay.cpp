#include "DASignalAnalysisUI.h"
#include "DASignalAnalysisUIHelpers.h"

// Qt
#include <QMenu>
#include <limits>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QIcon>
#include <QJsonObject>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>

// 3rdparty (SARibbon / ADS / qwt)
#include "qwt_colormap_preset.h"

// DA / DAFigure
#include "DAUIInterface.h"
#include "DAChartWidget.h"

// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DASignalAnalysisSettingContext.h"
#include "Page/DASignalTimeText.h"
#include "Page/DAPeakDockWidget.h"
#include "Signal/DASignalImporter.h"
#include "Playback/DASignalPlayer.h"

namespace DA
{
/**
 * @brief 「跟随滑块」开关变化：回勾底部工具栏，并在关闭时把峰值表恢复为整段
 *
 * 开启方向只刷新当前帧的峰值表；关闭方向要把整段峰值（表/曲线/频谱标记）恢复回来，
 * 否则频谱图与峰值表会停在最后一帧（D13 / 评审 N-U4）。
 */
void DASignalAnalysisUI::onViewFollowSliderToggled(bool on)
{
    if (mToolbarFollowCheck) {
        const QSignalBlocker blocker(mToolbarFollowCheck);
        mToolbarFollowCheck->setChecked(on);
    }
    if (on) {
        updatePeaksForCurrentFrame();
        // 跟随开启：峰值保持变为可用，刷新「测量 ▾」的门控
        syncSignalContextControls();
        return;
    }
    // 关闭跟随：峰值保持失去逐帧谱来源，直接关掉（缓冲由视图清空），避免留下勾选却不可用的开关
    if (actionPeakHold && actionPeakHold->isChecked()) {
        actionPeakHold->setChecked(false);  // 触发 onPeakHoldToggled(false) → 视图停用并清缓冲
    }
    const DASignalPtr current            = (mPage) ? mPage->currentSignal() : DASignalPtr();
    DASignalAnalysisView* currentSigView = (mPage && current) ? mPage->viewForSignal(current.get()) : nullptr;
    // 关闭跟随：峰值表回整段、隐藏时间列、频谱图峰值标记恢复整段（D13/评审 N-U4）
    DAPeakResult peaks;
    if (current && mCurrentPeaksSignal == current.get()) {
        peaks = mCurrentPeaks;
    }
    if (mPeakPanel) {
        mPeakPanel->setShowTimeColumn(false);
        mPeakPanel->setPeaks(peaks);
        mPeakPanel->setPeakCurves(mCurrentCurves);
    }
    if (currentSigView) {
        currentSigView->setPeaks(peaks);
    }
}

/**
 * @brief 当前 2D/3D 子图切换：同步上下文标签的工具动作/瀑布勾选
 */
void DASignalAnalysisUI::onViewSubChartChanged()
{
    syncSignalContextControls();
}

/**
 * @brief 当前视图状态 → 上下文标签控件
 *
 * 「分析设置」页的通道/参数控件与「信号显示」页的时频图设置/云图设置全部跟随当前视图；
 * 门控按对象分组：二维/隐藏时云图设置整组置灰，三维/隐藏时云图设置同样不可用（色标只属于二维谱图）。
 * 当前 2D 子图的交互工具状态（缩放/平移/十字线）只用于「点击定位」互斥（SF-12），
 * 交互工具本身由内置「绘图操作」上下文标签提供。
 * 写控件时用 QSignalBlocker 防回环（否则回填会再触发一次写回视图）。
 */
void DASignalAnalysisUI::syncSignalContextControls()
{
    DASignalAnalysisView* view = currentView();
    const bool hasView         = (view != nullptr);
    // 通道下拉（分析设置 → 频谱参数网格首行）：通道名变化时重建条目，选中项跟随视图当前通道
    if (mChannelCombo) {
        const QStringList channelNames = (view ? view->channelNames() : QStringList());
        QStringList currentNames;
        currentNames.reserve(mChannelCombo->count());
        for (int i = 0; i < mChannelCombo->count(); ++i) {
            currentNames.append(mChannelCombo->itemText(i));
        }
        if (currentNames != channelNames) {
            const QSignalBlocker blocker(mChannelCombo);
            mChannelCombo->clear();
            for (int i = 0; i < channelNames.size(); ++i) {
                mChannelCombo->addItem(channelNames.at(i), i);
            }
        }
        const QSignalBlocker blocker(mChannelCombo);
        const int channel    = (view ? view->currentChannel() : 0);
        const int comboIndex = mChannelCombo->findData(channel);
        if (comboIndex >= 0) {
            mChannelCombo->setCurrentIndex(comboIndex);
        } else if (mChannelCombo->count() > 0) {
            mChannelCombo->setCurrentIndex(0);
        }
        mChannelCombo->setEnabled(view && channelNames.size() > 1);
    }
    // 瀑布模式（时频图设置，互斥组）
    const int waterfallMode = (view ? view->waterfallMode() : 0);
    if (actionWaterfall2D) {
        const QSignalBlocker blocker(actionWaterfall2D);
        actionWaterfall2D->setEnabled(hasView);
        actionWaterfall2D->setChecked(waterfallMode == 0);
    }
    if (actionWaterfall3D) {
        const QSignalBlocker blocker(actionWaterfall3D);
        actionWaterfall3D->setEnabled(hasView);
        actionWaterfall3D->setChecked(waterfallMode == 1);
    }
    if (actionWaterfallHide) {
        const QSignalBlocker blocker(actionWaterfallHide);
        actionWaterfallHide->setEnabled(hasView);
        actionWaterfallHide->setChecked(waterfallMode == 2);
    }
    // 当前帧标记：二维云图竖线与三维当前迹线高亮共用同一开关
    if (actionTimeCursor) {
        const QSignalBlocker blocker(actionTimeCursor);
        actionTimeCursor->setEnabled(hasView);
        actionTimeCursor->setChecked(view ? view->isTimeCursorVisible() : true);
    }
    // 视图 ▾：时间格式勾选跟随视图；无视图时整组置灰
    if (actionViewOptions) {
        actionViewOptions->setEnabled(hasView);
    }
    if (actionDisplayReset) {
        actionDisplayReset->setEnabled(hasView);
    }
    const int timeFormat = (view ? view->timeFormat() : 0);
    for (QAction* formatAction : std::as_const(mTimeFormatActions)) {
        if (nullptr == formatAction) {
            continue;
        }
        const QSignalBlocker blocker(formatAction);
        formatAction->setEnabled(hasView);
        formatAction->setChecked(formatAction->data().toInt() == timeFormat);
    }
    for (QAction* presetAction : std::as_const(mLayoutPresetActions)) {
        if (presetAction) {
            presetAction->setEnabled(hasView);
        }
    }
    // 测量 ▾：联动缩放勾选跟随视图（双光标/峰值保持的勾选在各自步骤接入）
    if (actionMeasure) {
        actionMeasure->setEnabled(hasView);
    }
    if (actionLinkedZoom) {
        const QSignalBlocker blocker(actionLinkedZoom);
        actionLinkedZoom->setEnabled(hasView);
        actionLinkedZoom->setChecked(view && view->isLinkedZoom());
    }
    if (actionDualCursor) {
        const QSignalBlocker blocker(actionDualCursor);
        actionDualCursor->setEnabled(hasView);
        actionDualCursor->setChecked(view && view->isDualCursorEnabled());
    }
    // 峰值保持：只在跟随播放时可用；重置只在峰值保持开启时可用
    if (actionPeakHold) {
        const QSignalBlocker blocker(actionPeakHold);
        const bool follow = (view && view->isFollowSlider());
        actionPeakHold->setEnabled(hasView && follow);
        actionPeakHold->setChecked(view && view->isPeakHoldEnabled());
    }
    if (actionResetPeakHold) {
        actionResetPeakHold->setEnabled(hasView && view && view->isPeakHoldEnabled());
    }
    // 云图设置整组：只在二维热图可见时可用（三维/隐藏时置灰，色标也只有二维有意义）
    const bool heatmapActive = hasView && waterfallMode == 0;
    if (actionColormap) {
        actionColormap->setEnabled(heatmapActive);
    }
    if (mColormapMenu) {
        // 色带勾选回读图元当前色图（单一真相）：宿主热图面板改过起止色时返回 "custom"，
        // 菜单里没有任何预设被勾选
        const QString colormap          = (view ? view->colormapPreset() : QString());
        const QList< QAction* > presets = mColormapMenu->actions();
        for (QAction* presetAction : presets) {
            if (nullptr == presetAction || !presetAction->isCheckable()) {
                continue;
            }
            const QSignalBlocker blocker(presetAction);
            presetAction->setChecked(!colormap.isEmpty() && presetAction->data().toString() == colormap);
        }
    }
    // 色标显隐：勾选与当前视图一致（恢复工程/宿主面板改动后以视图为准）
    if (actionColorBar) {
        const QSignalBlocker blocker(actionColorBar);
        actionColorBar->setEnabled(heatmapActive);
        actionColorBar->setChecked(view ? view->isColorBarVisible() : mColorBarVisible);
    }
    // 云图 dB 范围（二维色域下限）：跟随当前视图；三维/隐藏时整组置灰
    if (mHeatmapDbFloorSpin) {
        const QSignalBlocker blocker(mHeatmapDbFloorSpin);
        mHeatmapDbFloorSpin->setValue(view ? view->dbFloor() : -80.0);
        mHeatmapDbFloorSpin->setEnabled(heatmapActive);
    }
    // 显示 ▾：显示模式与插值勾选跟随视图（显示模式的真相是谱图图元，含宿主面板改动）
    if (actionHeatmapDisplay) {
        actionHeatmapDisplay->setEnabled(heatmapActive);
    }
    const int displayMode  = (view ? view->heatmapDisplayMode() : 0);
    const int resampleMode = (view ? view->heatmapResampleMode() : 0);
    for (QAction* modeAction : std::as_const(mHeatmapDisplayModeActions)) {
        if (!modeAction) {
            continue;
        }
        const QSignalBlocker blocker(modeAction);
        modeAction->setEnabled(heatmapActive);
        modeAction->setChecked(modeAction->data().toInt() == displayMode);
    }
    for (QAction* resampleAction : std::as_const(mHeatmapResampleActions)) {
        if (!resampleAction) {
            continue;
        }
        const QSignalBlocker blocker(resampleAction);
        resampleAction->setEnabled(heatmapActive);
        resampleAction->setChecked(resampleAction->data().toInt() == resampleMode);
    }
    // 瀑布图设置整组：只在三维瀑布可见时可用
    const bool waterfallActive = hasView && waterfallMode == 1;
    if (actionWaterfallSettings) {
        actionWaterfallSettings->setEnabled(waterfallActive);
    }
    if (actionWaterfallColormap) {
        actionWaterfallColormap->setEnabled(waterfallActive);
    }
    if (mWaterfallColormapMenu) {
        const QString waterfallColormap = (view ? view->waterfallColormapName() : QString());
        const QList< QAction* > presets = mWaterfallColormapMenu->actions();
        for (QAction* presetAction : presets) {
            if (nullptr == presetAction || !presetAction->isCheckable()) {
                continue;
            }
            const QSignalBlocker blocker(presetAction);
            presetAction->setChecked(!waterfallColormap.isEmpty() && presetAction->data().toString() == waterfallColormap);
        }
    }
    if (mWaterfallDbFloorSpin) {
        const QSignalBlocker blocker(mWaterfallDbFloorSpin);
        mWaterfallDbFloorSpin->setValue(view ? view->waterfallDbFloor() : -80.0);
        mWaterfallDbFloorSpin->setEnabled(waterfallActive);
    }
    // 当前迹线高亮：单色着色下无效（tooltip 说明原因，见 retranslateUi）
    if (actionWaterfallHighlight) {
        const QSignalBlocker blocker(actionWaterfallHighlight);
        const int colorMode  = (view ? view->waterfallColorMode() : -1);
        const bool solidMode = (colorMode == 2);  // Qwt3DWaterfall::Solid
        actionWaterfallHighlight->setEnabled(waterfallActive && !solidMode);
        actionWaterfallHighlight->setChecked(view ? view->isWaterfallHighlighted() : true);
    }
    // 峰值标记：勾选状态以当前视图的 peakMarkersVisible() 为准（视图是状态源，
    // UI 层的 mPeakMarkersVisible 只作为新建视图的默认值，不再回填勾选）
    if (actionPeakMarker) {
        const QSignalBlocker blocker(actionPeakMarker);
        actionPeakMarker->setEnabled(hasView);
        actionPeakMarker->setChecked(view && view->peakMarkersVisible());
    }
    // 分析设置页参数控件（预处理/频谱/时频/峰值）跟随当前信号参数
    syncParamsRibbonControls();
    // 当前 2D 子图（「点击波形即定位」互斥用）
    DAChartWidget* chart      = (view ? view->currentChart() : nullptr);
    const bool chartAvailable = (chart != nullptr);
    // 「点击波形即定位」与交互工具互斥（SF-12）：缩放/平移/十字线任一启用、或双光标测量开启时
    // 关闭，避免按下拖动/点放游标时播放头跳位；全关时恢复。交互工具由内置「绘图操作」上下文
    // 标签负责开关，本插件只读取当前子图状态
    if (view) {
        const bool toolEnabled = chartAvailable
                                 && (chart->isZoomEnabled() || chart->isPanEnabled() || chart->isCrosshairEnabled());
        view->setSeekEnabled(!toolEnabled && !view->isDualCursorEnabled());
    }
    // 当前 2D 子图可能已被切走：重绑属性变化信号，保证用户经内置「绘图操作」标签
    // 切换缩放/平移/十字线后「点击定位」互斥仍正确（SF-12）
    bindCurrentChartSignals();
}

/**
 * @brief 绑定当前 2D 子图的 chartPropertiesChanged
 *
 * 缩放/平移/十字线改由内置「绘图操作」上下文标签开关后，本插件必须监听当前子图状态，
 * 才能在工具被打开/关闭时同步「点击定位」互斥（SF-12）。
 * 先比较成员绑定的图元，未变化则不重复连接；换图元时先断旧连接再用 QPointer 记录新图元。
 */
void DASignalAnalysisUI::bindCurrentChartSignals()
{
    DASignalAnalysisView* view = currentView();
    DAChartWidget* chart       = (view ? view->currentChart() : nullptr);
    if (mBoundChart == chart) {
        return;
    }
    unbindCurrentChartSignals();
    if (nullptr == chart) {
        return;
    }
    mBoundChart = chart;
    mChartPropertiesConnection =
        connect(chart,
                &DAChartWidget::chartPropertiesChanged,
                this,
                [ this ](DAChartWidget* changedChart, DAChartWidget::ChartPropertyChangeFlags flag) {
                    Q_UNUSED(changedChart);
                    // 只有影响 SF-12 的交互工具状态变化才需要刷新（其它属性变化不触碰点击定位）
                    if (flag.testFlag(DAChartWidget::ZoomStateChanged) || flag.testFlag(DAChartWidget::PanStateChanged)
                        || flag.testFlag(DAChartWidget::CrosshairStateChanged)) {
                        syncSignalContextControls();
                    }
                });
}

/**
 * @brief 断开当前 2D 子图的属性变化连接并清空绑定指针
 *
 * 图元可能已随视图销毁：连接对象仍可 disconnect（Qt 允许断开已失效连接），
 * QPointer 同步置空，避免后续比较/解引用悬空指针。
 */
void DASignalAnalysisUI::unbindCurrentChartSignals()
{
    if (mChartPropertiesConnection) {
        QObject::disconnect(mChartPropertiesConnection);
        mChartPropertiesConnection = QMetaObject::Connection();
    }
    mBoundChart.clear();
}

/**
 * @brief 当前视图状态 → 底部工具栏控件（QSignalBlocker 防回环）
 *
 * 时间滑块范围按当前信号时长（毫秒）设置，值取视图当前时间；
 * 无当前视图时控件全部禁用并回到默认值。
 */
void DASignalAnalysisUI::syncSignalToolbar()
{
    if (nullptr == mSignalToolbar) {
        return;
    }
    DASignalAnalysisView* view = currentView();
    const DASignalPtr sig      = (mPage ? mPage->currentSignal() : DASignalPtr());
    const bool hasView         = (view != nullptr);
    if (mToolbarFollowCheck) {
        const QSignalBlocker blocker(mToolbarFollowCheck);
        mToolbarFollowCheck->setChecked(hasView && view->isFollowSlider());
        mToolbarFollowCheck->setEnabled(hasView);
    }
    if (mToolbarTimeSlider) {
        const qint64 durationMs =
            (sig && sig->duration() > 0.0)
                ? qBound< qint64 >(0, qRound64(sig->duration() * 1000.0), std::numeric_limits< int >::max())
                : 0;
        const QSignalBlocker blocker(mToolbarTimeSlider);
        mToolbarTimeSlider->setRange(0, int(durationMs > 0 ? durationMs : 1));
        int value = 0;
        if (hasView) {
            value = int(qBound< qint64 >(0, qRound64(view->currentTime() * 1000.0), mToolbarTimeSlider->maximum()));
        }
        mToolbarTimeSlider->setValue(value);
        mToolbarTimeSlider->setEnabled(hasView && durationMs > 0);
    }
    if (mToolbarAutoScrollCheck) {
        const QSignalBlocker blocker(mToolbarAutoScrollCheck);
        mToolbarAutoScrollCheck->setChecked(hasView && view->isHeatmapAutoScroll());
        mToolbarAutoScrollCheck->setEnabled(hasView);
    }
    // 播放/暂停：勾选与图标跟随播放器真实状态（D5：▶/⏸ 单切换按钮）
    const bool hasSource = (mPlayer && mPlayer->hasSource());
    const bool playing   = (mPlayer && mPlayer->isPlaying());
    if (actionPlay) {
        const QSignalBlocker blocker(actionPlay);
        actionPlay->setChecked(hasSource && playing);
        actionPlay->setIcon(QIcon(QString::fromUtf8(cIconPrefix) + QLatin1String(playing ? "pause.svg" : "play.svg")));
        actionPlay->setText(playing ? tr("Pause") : tr("Play"));  // cn:暂停 / 播放
        actionPlay->setToolTip(playing ? tr("Pause playback") : tr("Start playback"));
        // cn:暂停播放 / 开始播放
    }
    updateToolbarTimeText();
}

/**
 * @brief 刷新底部工具栏时间文本（mm:ss.s / mm:ss.s；无播放源显示占位）
 */
void DASignalAnalysisUI::updateToolbarTimeText()
{
    if (nullptr == mToolbarTimeLabel) {
        return;
    }
    const bool hasSource  = (mPlayer && mPlayer->hasSource());
    const DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    if (!hasSource || !sig) {
        mToolbarTimeLabel->setText(daSignalFormatTimeRange(0, 0, false));
        return;
    }
    qint64 durationMs = (mPlayer ? mPlayer->durationMs() : 0);
    if (durationMs <= 0) {
        // 媒体刚装入时后端可能还没给出时长：退回信号时长（口径同为毫秒，含时间原点无关）
        durationMs = qMax< qint64 >(0, qRound64(sig->duration() * 1000.0));
    }
    const qint64 positionMs = (mPlayer ? mPlayer->positionMs() : 0);
    mToolbarTimeLabel->setText(daSignalFormatTimeRange(positionMs, durationMs, true));
}

/**
 * @brief 上下文标签通道选择：切到选中通道（视图内部重发该通道的分析请求）
 */
void DASignalAnalysisUI::onChannelComboChanged(int index)
{
    if (nullptr == mChannelCombo || index < 0) {
        return;
    }
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        return;
    }
    view->setCurrentChannel(mChannelCombo->itemData(index).toInt());
}

/**
 * @brief 「时频图设置 → 当前帧标记」开关：二维云图竖线与三维当前迹线高亮共用
 */
void DASignalAnalysisUI::onTimeCursorToggled(bool on)
{
    if (DASignalAnalysisView* view = currentView()) {
        view->setTimeCursorVisible(on);
    }
}

/**
 * @brief 「云图设置 → dB」范围：写当前视图的二维色域下限（[floor, 0]）
 *
 * 三维瀑布有自己独立的 dB 范围（D12），这里只改二维热图与色标。
 * @param value dB 下限（-200..0）
 */
void DASignalAnalysisUI::onHeatmapDbFloorChanged(double value)
{
    if (DASignalAnalysisView* view = currentView()) {
        view->setDbFloor(value);
    }
}

/**
 * @brief 「云图设置 → 显示 → 显示模式」：图像 / 等高线 / 图像+等高线
 * @param action 菜单项，data 为模式编号（0/1/2）
 */
void DASignalAnalysisUI::onHeatmapDisplayModeTriggered(QAction* action)
{
    if (nullptr == action) {
        return;
    }
    if (DASignalAnalysisView* view = currentView()) {
        view->setHeatmapDisplayMode(action->data().toInt());
    }
}

/**
 * @brief 「云图设置 → 显示 → 插值」：最近邻 / 双线性
 * @param action 菜单项，data 为插值编号（0=最近邻 / 1=双线性）
 */
void DASignalAnalysisUI::onHeatmapResampleTriggered(QAction* action)
{
    if (nullptr == action) {
        return;
    }
    if (DASignalAnalysisView* view = currentView()) {
        view->setHeatmapResampleMode(action->data().toInt());
    }
}

/**
 * @brief 「瀑布图设置 → 色带」：切三维瀑布的色带预设（只影响瀑布，不影响云图与色标）
 * @param action 菜单项，data 为 QwtColorMapPreset 预设名
 */
void DASignalAnalysisUI::onWaterfallColormapTriggered(QAction* action)
{
    if (nullptr == action) {
        return;
    }
    const QString name = action->data().toString();
    if (name.isEmpty()) {
        return;
    }
    if (DASignalAnalysisView* view = currentView()) {
        view->setWaterfallColormapName(name);
    }
}

/**
 * @brief 「瀑布图设置 → dB」：写三维瀑布的固定色域下限（[floor, 0]），与二维云图独立
 * @param value dB 下限（-200..0）
 */
void DASignalAnalysisUI::onWaterfallDbFloorChanged(double value)
{
    if (DASignalAnalysisView* view = currentView()) {
        view->setWaterfallDbFloor(value);
    }
}

/**
 * @brief 「瀑布图设置 → 当前迹线高亮」开关
 * @param on true 高亮当前帧对应迹线
 */
void DASignalAnalysisUI::onWaterfallHighlightToggled(bool on)
{
    if (DASignalAnalysisView* view = currentView()) {
        view->setWaterfallHighlight(on);
    }
}

/**
 * @brief 「瀑布图设置 → 瀑布设置…」：切右侧 3D 设置面板并定位到当前瀑布图元
 *
 * 丝带只留常用项（色带/dB/高亮），几何样式/着色/不透明度/深度方向/描边都在 3D 面板里改。
 * 时频分析还没算完（图元未生成）时给出可理解提示，不静默失败。
 */
void DASignalAnalysisUI::onWaterfallSettingsRequested()
{
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        return;
    }
    if (nullptr == view->waterfall3DItem()) {
        if (mUI) {
            mUI->addInfoLogMessage(
                tr("The 3D waterfall is not ready yet; its item appears after the time-frequency analysis finishes."));
            // cn:三维瀑布尚未生成；时频分析完成后才会出现对应图元。
        }
        return;
    }
    if (mSettingContext) {
        mSettingContext->showWaterfallItemSetting();
    }
}

/**
 * @brief 「测量 → 联动缩放」开关：波形 x ↔ 云图 x、频谱 x ↔ 云图 y 同步缩放/平移
 * @param on true 开启联动
 */
void DASignalAnalysisUI::onLinkedZoomToggled(bool on)
{
    if (DASignalAnalysisView* view = currentView()) {
        view->setLinkedZoom(on);
    }
}

/**
 * @brief 「测量 → 双光标测量」开关：在当前 2D 子图上依次点放 A/B
 * @param on true 开启
 */
void DASignalAnalysisUI::onDualCursorToggled(bool on)
{
    if (DASignalAnalysisView* view = currentView()) {
        view->setDualCursorEnabled(on);
    }
    // 开启双光标会停用「点击定位」：同步一次避免其余门控状态滞后
    syncSignalContextControls();
}

/**
 * @brief 「测量 → 峰值保持」开关：跟随播放时按频率网格累积最大值
 * @param on true 开启
 */
void DASignalAnalysisUI::onPeakHoldToggled(bool on)
{
    if (DASignalAnalysisView* view = currentView()) {
        view->setPeakHoldEnabled(on);
    }
    syncSignalContextControls();
}

/**
 * @brief 「测量 → 重置峰值保持」：清空最大值缓冲（曲线保留为空，等下一帧重新累积）
 */
void DASignalAnalysisUI::onResetPeakHold()
{
    if (DASignalAnalysisView* view = currentView()) {
        view->resetPeakHold();
    }
}

/**
 * @brief 「视图 → 时间格式」：写当前视图的时间轴标签格式（只重绘）
 * @param action 菜单项，data 为格式编号（0=秒 / 1=毫秒 / 2=mm:ss.s）
 */
void DASignalAnalysisUI::onTimeFormatTriggered(QAction* action)
{
    if (nullptr == action) {
        return;
    }
    if (DASignalAnalysisView* view = currentView()) {
        view->setTimeFormat(action->data().toInt());
    }
}

/**
 * @brief 「视图 → 布局比例」：套用 2×2 网格比例预设（只影响一次布局，不改存档语义）
 *
 * 工程里保存的仍是实际比例：预设只在点选时套用一次。
 * @param action 菜单项，data 为预设编号（0=默认 / 1=1:1 / 2=波形优先 / 3=分析优先）
 */
void DASignalAnalysisUI::onLayoutPresetTriggered(QAction* action)
{
    if (nullptr == action) {
        return;
    }
    if (DASignalAnalysisView* view = currentView()) {
        view->setGridSizes(layoutRowsForPreset(action->data().toInt()), layoutColumnsDefault());
    }
}

/**
 * @brief 重置显示：一键把显示类设置恢复成默认（不弹确认，写一条消息）
 *
 * 覆盖：显示模式、当前帧标记、联动缩放、双光标、峰值保持、时间格式、布局比例、
 * 云图色带与 dB 范围、色标、自动滚动。分析参数（窗函数/幅值口径等）不在重置范围内。
 */
void DASignalAnalysisUI::resetDisplaySettings()
{
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        return;
    }
    view->setWaterfallMode(0);
    view->setTimeCursorVisible(true);
    view->setLinkedZoom(false);
    view->setDualCursorEnabled(false);
    view->setPeakHoldEnabled(false);
    view->setTimeFormat(0);
    view->setGridSizes(layoutRowsForPreset(0), layoutColumnsDefault());
    view->setColormapName(QStringLiteral("viridis"));
    view->setDbFloor(-80.0);
    view->setColorBarVisible(true);
    view->setHeatmapDisplayMode(0);
    view->setHeatmapResampleMode(0);
    view->setHeatmapAutoScroll(false);
    // 三维瀑布：色带 / dB / 当前迹线高亮 / 图元样式回到默认
    view->setWaterfallColormapName(QStringLiteral("viridis"));
    view->setWaterfallDbFloor(-80.0);
    view->setWaterfallHighlight(true);
    view->resetWaterfallStyle();
    // 色标显隐是插件级偏好：重置后同步回偏好，避免下一个信号又回到旧值
    mColorBarVisible                         = true;
    QJsonObject cfg                          = DASignalImporter::loadPluginConfig();
    cfg[ QStringLiteral("colorBarVisible") ] = true;
    DASignalImporter::savePluginConfig(cfg);
    syncSignalContextControls();
    syncSignalToolbar();
    if (mUI) {
        mUI->addInfoLogMessage(tr("Display settings reset."));  // cn:显示设置已重置。
    }
}

/**
 * @brief 上下文标签色带菜单：切到选中的色带预设（只重绘热图）
 */
void DASignalAnalysisUI::onContextColormapTriggered(QAction* action)
{
    if (nullptr == action) {
        return;
    }
    const QString name = action->data().toString();
    if (name.isEmpty()) {
        return;
    }
    if (DASignalAnalysisView* view = currentView()) {
        view->setColormapName(name);
    }
}

/**
 * @brief 上下文标签「色标」开关：显示/隐藏时频热图右侧色标轴，并立即持久化偏好
 *
 * 偏好是全局的（不按信号区分）：先更新 UI 记录的 mColorBarVisible 并写入当前视图，
 * 再「读-改-写」整份 signal-analysis-config.json（保留 ffmpegPath/colormap/peakDockVisible
 * 等已有键），下次启动在 ensurePageCreated() 读回，finalize() 再兜底写一次。
 * @param on true 显示色标 / false 隐藏色标
 */
void DASignalAnalysisUI::onColorBarToggled(bool on)
{
    mColorBarVisible = on;
    if (DASignalAnalysisView* view = currentView()) {
        view->setColorBarVisible(on);
    }
    // 立即持久化：读-改-写整份配置，不能只写这一个键而丢掉其它键
    QJsonObject cfg                          = DASignalImporter::loadPluginConfig();
    cfg[ QStringLiteral("colorBarVisible") ] = on;
    DASignalImporter::savePluginConfig(cfg);
}

/**
 * @brief 幅值口径同步：全局状态 + 频谱 panel 的幅值 combo + 视图/峰值表重绘
 *
 * 幅值口径有两份状态：全局 mAmplitudeMode/mDbReference（峰值表、工程存档用）与
 * params.amplitudeMode/dbReference（计算、视图用）。丝带 combo 与对话框都只改后者，
 * 这里统一把前者同步成同一口径，并只重绘（不重算）。
 * dB 参考由幅值口径派生（D7）：dbRef1 → 相对 1.0，其余 → 相对满量程。
 */
void DASignalAnalysisUI::syncAmplitudeUiFromParams(const DASignalAnalysisParams& params)
{
    if (!params.amplitudeMode.isEmpty()) {
        mAmplitudeMode = params.amplitudeMode;
    }
    if (!params.dbReference.isEmpty()) {
        mDbReference = params.dbReference;
    }
    if (mAmplitudeModeCombo) {
        const QSignalBlocker blocker(mAmplitudeModeCombo);
        const int comboIndex = mAmplitudeModeCombo->findData(mAmplitudeMode);
        if (comboIndex >= 0) {
            mAmplitudeModeCombo->setCurrentIndex(comboIndex);
        }
    }
    if (!mPage) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (DASignalAnalysisView* view = (sig ? mPage->viewForSignal(sig.get()) : nullptr)) {
        view->setAmplitudeDisplay(mAmplitudeMode, mDbReference);
    }
    if (mPeakPanel) {
        mPeakPanel->setAmplitudeDisplay(mAmplitudeMode, mDbReference, params.calibration, params.unit);
        mPeakPanel->setPeaks(mCurrentPeaks);
        mPeakPanel->setPeakCurves(mCurrentCurves);
    }
}

/**
 * @brief 刷新时频 panel 的 Δf 只读提示（按采样率与实际段长反算，与 Python 同口径）
 */
void DASignalAnalysisUI::updateDeltaFLabel(const DASignalAnalysisParams& params)
{
    if (nullptr == mDeltaFLabel) {
        return;
    }
    double sampleRate = 0.0;
    if (mPage) {
        if (DASignalPtr sig = mPage->currentSignal()) {
            sampleRate = sig->sampleRate();
        }
    }
    const double resolution = (sampleRate > 0.0) ? signalResolutionFromStftWindowMs(params.stftWindowMs) : 0.0;
    if (resolution <= 0.0) {
        mDeltaFLabel->setText(QStringLiteral("Δf —"));
        mDeltaFLabel->setToolTip(tr("Frequency resolution of the time-frequency view"));  // cn:时频视图的频率分辨率
        return;
    }
    // 实际分辨率按整数段长回算（与 spectral.py 的段长取整口径一致）
    const int segLen     = qMax(2, int(qRound(qMax(params.stftWindowMs, 1.0) * sampleRate / 1000.0)));
    const double realRes = sampleRate / double(segLen);
    Q_UNUSED(resolution)
    mDeltaFLabel->setText(tr("Δf ≈ %1 Hz").arg(realRes, 0, 'f', 2));  // cn:Δf ≈ %1 赫兹
    mDeltaFLabel->setToolTip(tr("Frequency resolution derived from the window length"));
    // cn:由窗长派生的频率分辨率
}

/**
 * @brief 分析设置页参数控件回填当前信号参数（预处理/频谱/时频/峰值）
 *
 * 切信号、参数变化、页面未创建都要能安全调用：控件写值一律用 QSignalBlocker，
 * 避免回填触发各 on*Changed 造成反向参数覆盖；无当前信号时全部置灰。
 */
void DASignalAnalysisUI::syncParamsRibbonControls()
{
    const DASignalPtr sig               = (mPage ? mPage->currentSignal() : DASignalPtr());
    const bool hasSignal                = static_cast< bool >(sig);
    const DASignalAnalysisParams params = hasSignal ? paramsForSignal(sig) : DASignalAnalysisParams::defaults();
    // ---- 预处理 ----
    if (mPreprocessDcCheck) {
        const QSignalBlocker blocker(mPreprocessDcCheck);
        mPreprocessDcCheck->setChecked(params.removeDc);
        mPreprocessDcCheck->setEnabled(hasSignal);
    }
    if (mDetrendCombo) {
        const QSignalBlocker blocker(mDetrendCombo);
        const int idx = qMax(0, mDetrendCombo->findData(params.detrend));
        mDetrendCombo->setCurrentIndex(idx);
        mDetrendCombo->setEnabled(hasSignal);
    }
    // ---- 频谱 ----
    if (mWindowCombo) {
        const QSignalBlocker blocker(mWindowCombo);
        const int idx = qMax(0, mWindowCombo->findData(params.window));
        mWindowCombo->setCurrentIndex(idx);
        mWindowCombo->setEnabled(hasSignal);
    }
    if (mAmplitudeModeCombo) {
        const QSignalBlocker blocker(mAmplitudeModeCombo);
        const int idx = qMax(0, mAmplitudeModeCombo->findData(params.amplitudeMode));
        mAmplitudeModeCombo->setCurrentIndex(idx);
        mAmplitudeModeCombo->setEnabled(hasSignal);
    }
    if (mResolutionSpin) {
        const QSignalBlocker blocker(mResolutionSpin);
        mResolutionSpin->setValue(params.resolutionHz);
        mResolutionSpin->setEnabled(hasSignal);
    }
    // ---- 时频 ----
    if (mStftWindowSpin) {
        const QSignalBlocker blocker(mStftWindowSpin);
        mStftWindowSpin->setValue(params.stftWindowMs);
        mStftWindowSpin->setEnabled(hasSignal);
    }
    if (mStftOverlapSpin) {
        const QSignalBlocker blocker(mStftOverlapSpin);
        mStftOverlapSpin->setValue(params.stftOverlap);
        mStftOverlapSpin->setEnabled(hasSignal);
    }
    // ---- 峰值 ----
    if (mPeakModeCombo) {
        const QSignalBlocker blocker(mPeakModeCombo);
        const int comboIndex = qMax(0, mPeakModeCombo->findData(params.peakMode));
        mPeakModeCombo->setCurrentIndex(comboIndex);
        mPeakModeCombo->setEnabled(hasSignal);
    }
    if (mPeakRelDbSpin) {
        const QSignalBlocker blocker(mPeakRelDbSpin);
        mPeakRelDbSpin->setValue(params.peakRelDb);
        mPeakRelDbSpin->setEnabled(hasSignal);
    }
    // Δf 只读提示跟随窗长（与上面的回填同一口径）
    updateDeltaFLabel(params);
}

}  // namespace DA
