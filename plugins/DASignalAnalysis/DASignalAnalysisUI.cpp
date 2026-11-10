#include "DASignalAnalysisUI.h"
// Qt
#include <QMenu>
#include <QActionGroup>
#include <cmath>
#include <utility>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11QtCaster.hpp"
#include "pandas/DAPyDataFrame.h"
#include "Signal/DADataFrameSignalSource.h"
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
// ADS
#include "DockManager.h"
#include "DockWidget.h"
// DA
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DARibbonAreaInterface.h"
#include "DADockingAreaInterface.h"
#include "DAActionsInterface.h"
#include "DADataManagerInterface.h"
#include "DAProjectInterface.h"
#include "DAStatusBarInterface.h"
#include "DockAreaWidget.h"
// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DAPeakDockWidget.h"
#include "Dialogs/DASignalImportDialog.h"
#include "Dialogs/DASignalAnalysisSettingDialog.h"
#include "Signal/DASignalImporter.h"
#include "Analysis/DASignalAmplitude.h"
#include "Analysis/DASignalMemoryEstimate.h"
#include "Analysis/DASignalAnalysisWorker.h"
#include "Playback/DASignalPlayer.h"
#include "Archive/DASignalArchiveTask.h"

namespace DA
{
namespace
{
/// 丝带标签页 / 面板 / 动作的 object name（唯一名，供样式与查找使用）
const char* const cCategoryObjectName      = "da-category-signal-analysis";
const char* const cPanelOpenName           = "da-panel-signal.open";
const char* const cPanelSignalName         = "da-panel-signal.signal";
const char* const cPanelPlaybackName       = "da-panel-signal.playback";
const char* const cPanelAnalysisName       = "da-panel-signal.analysis";
const char* const cPanelPeaksName          = "da-panel-signal.peaks";
const char* const cPanelDisplayName        = "da-panel-signal.display";
const char* const cPanelExportName         = "da-panel-signal.export";
const char* const cPageDockObjectName      = "da-dock-signal-analysis-page";

// 图标资源前缀
const char* const cIconPrefix = ":/DASignalAnalysis/icon/";
// 内存预算（方案 §11 / 评审 D5 / N-C1c）：频谱、峰值表、包络与热图共用同一预算门
constexpr qint64 cAnalysisBudgetBytes = qint64(1.5 * 1024 * 1024 * 1024);
// 内存预算判定（QHash 值）：0=未询问 / 1=用户已确认继续 / 2=用户拒绝
constexpr int cBudgetUnasked  = 0;
constexpr int cBudgetAllowed  = 1;
constexpr int cBudgetDeclined = 2;
}  // namespace

DASignalAnalysisUI::DASignalAnalysisUI(QObject* par) : QObject(par)
{
}

DASignalAnalysisUI::~DASignalAnalysisUI()
{
}

/**
 * @brief 初始化界面
 * @param core 核心接口
 * @return 初始化是否成功
 */
bool DASignalAnalysisUI::initialize(DACoreInterface* core)
{
    if (nullptr == core) {
        qWarning() << "DASignalAnalysisUI::initialize: core is null";
        return false;
    }
    mCore    = core;
    mUI      = core->getUiInterface();
    mActions = mUI->getActionInterface();

    // 工程生命周期（Q7）：新建工程 / 加载工程前 / 快照回滚时宿主会 clear() 工程并发
    // projectIsCleaned()，插件在这里同步清掉旧信号、峰值缓存与临时文件（评审 N-U2/P5/U15）
    if (DAProjectInterface* project = core->getProjectInterface()) {
        connect(project, &DAProjectInterface::projectIsCleaned, this, &DASignalAnalysisUI::onProjectCleaned);
    }

    // 注意：页面不在这里创建（惰性加载），见 ensurePageCreated()
    buildSignalCategory();
    retranslateUi();
    // 无信号阶段：依赖信号的动作全部置灰（方案 §8.1）
    setSignalDependentActionsEnabled(false);

    // 打开信号分析窗口：首次调用创建页面，已关闭时恢复显示
    if (actionOpenSignalPage) {
        connect(actionOpenSignalPage, &QAction::triggered, this, &DASignalAnalysisUI::openSignalAnalysisPage);
    }
    if (actionImportAudio) {
        connect(actionImportAudio, &QAction::triggered, this, &DASignalAnalysisUI::importAudioFile);
    }
    if (actionImportDataFrame) {
        connect(actionImportDataFrame, &QAction::triggered, this, &DASignalAnalysisUI::importDataFrame);
    }
    if (actionImportVideo) {
        connect(actionImportVideo, &QAction::triggered, this, &DASignalAnalysisUI::importVideoFile);
    }
    if (actionRelinkSource) {
        connect(actionRelinkSource, &QAction::triggered, this, &DASignalAnalysisUI::relinkCurrentSignalSource);
    }
    if (actionAnalysisSetting) {
        connect(actionAnalysisSetting, &QAction::triggered, this, &DASignalAnalysisUI::openAnalysisSettings);
    }
    if (actionCancelAnalysis) {
        connect(actionCancelAnalysis, &QAction::triggered, this, &DASignalAnalysisUI::cancelAllComputations);
    }
    // 导出菜单：峰值表 CSV / 频谱 CSV / 功率谱 CSV / 热图矩阵 CSV
    if (actionExportPeaks) {
        mExportMenu = new QMenu();
        mActionExportPeaksCsv = mExportMenu->addAction(tr("Peak table (CSV)"), this, &DASignalAnalysisUI::exportPeaksCsv);
        // cn:峰值表（CSV）
        mActionExportSpectrumCsv =
            mExportMenu->addAction(tr("Spectrum curve (CSV)"), this, &DASignalAnalysisUI::exportSpectrumCsv);
        // cn:频谱曲线（CSV）
        // 韦尔奇功率谱密度导出（评审 C10）：v1 至少给出口，面板保持只显示幅值谱
        mActionExportPsdCsv = mExportMenu->addAction(tr("Power spectral density (CSV)"), this, &DASignalAnalysisUI::exportPsdCsv);
        // cn:功率谱密度（CSV）
        mActionExportHeatmapCsv =
            mExportMenu->addAction(tr("Heatmap matrix (CSV)"), this, &DASignalAnalysisUI::exportHeatmapCsv);
        // cn:热图矩阵（CSV）
        actionExportPeaks->setMenu(mExportMenu);
    }
    if (actionExportImage) {
        connect(actionExportImage, &QAction::triggered, this, &DASignalAnalysisUI::exportImage);
    }
    // 瀑布模式切换（二维 / 三维 / 隐藏）
    if (actionWaterfall2D) {
        connect(actionWaterfall2D, &QAction::triggered, this, [this]() {
            if (mPage) {
                if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
                    v->setWaterfallMode(0);
                }
            }
        });
    }
    if (actionWaterfall3D) {
        connect(actionWaterfall3D, &QAction::triggered, this, [this]() {
            if (mPage) {
                if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
                    v->setWaterfallMode(1);
                }
            }
        });
    }
    if (actionWaterfallHide) {
        connect(actionWaterfallHide, &QAction::triggered, this, [this]() {
            if (mPage) {
                if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
                    v->setWaterfallMode(2);
                }
            }
        });
    }
    // 播放控制器（D26）
    mPlayer = new DASignalPlayer(this);
    connect(mPlayer, &DASignalPlayer::positionChanged, this, &DASignalAnalysisUI::onPlayerPositionChanged);
    connect(mPlayer, &DASignalPlayer::errorOccurred, this, [this](const QString& err) {
        // 播放器原始错误是 Qt 的英文诊断信息：只写日志，界面显示翻译后的可理解提示（评审 §七-9）
        qWarning() << "DASignalAnalysisUI: playback error:" << err;
        reportError(tr("Playback failed. See the message log for details."));
        // cn:播放失败，详情见消息日志。
    });
    if (actionPlay) {
        connect(actionPlay, &QAction::triggered, this, [this]() { mPlayer->play(); });
    }
    if (actionPause) {
        connect(actionPause, &QAction::triggered, this, [this]() { mPlayer->pause(); });
    }
    if (actionStop) {
        connect(actionStop, &QAction::triggered, this, [this]() {
            mPlayer->stop();
            if (mPage) {
                if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
                    v->setCurrentTime(0.0);
                }
            }
        });
    }
    // 峰值停靠窗口（右侧停靠区，与设置/智能体同区做标签页，D24）随页面一起惰性创建，
    // 页面创建前没有信号可显示，这里不建；创建逻辑见 ensurePageCreated()
    // 后台计算协调器（Python 守护线程 + 定时轮询）
    mWorker = new DASignalAnalysisWorker(this);
    connect(mWorker, &DASignalAnalysisWorker::progressChanged, this, &DASignalAnalysisUI::onWorkerProgress);
    connect(mWorker, &DASignalAnalysisWorker::spectrumReady, this, &DASignalAnalysisUI::onWorkerSpectrumReady);
    connect(mWorker, &DASignalAnalysisWorker::envelopeReady, this, &DASignalAnalysisUI::onWorkerEnvelopeReady);
    connect(mWorker, &DASignalAnalysisWorker::heatmapReady, this, &DASignalAnalysisUI::onWorkerHeatmapReady);
    connect(mWorker, &DASignalAnalysisWorker::peaksReady, this, &DASignalAnalysisUI::onWorkerPeaksReady);
    connect(mWorker, &DASignalAnalysisWorker::peakCurvesReady, this, &DASignalAnalysisUI::onWorkerPeakCurvesReady);
    connect(mWorker, &DASignalAnalysisWorker::playbackWavReady, this, &DASignalAnalysisUI::onWorkerPlaybackWavReady);
    connect(mWorker, &DASignalAnalysisWorker::importFinished, this, &DASignalAnalysisUI::onWorkerImportFinished);
    connect(mWorker, &DASignalAnalysisWorker::canceled, this, &DASignalAnalysisUI::onWorkerCanceled);
    if (actionPeakWindow) {
        connect(actionPeakWindow, &QAction::toggled, this, &DASignalAnalysisUI::onPeakWindowToggled);
    }
    if (actionPeakTable) {
        // 峰值表按钮与该停靠窗口显隐同步（评审 U14/§七-8）
        connect(actionPeakTable, &QAction::toggled, this, &DASignalAnalysisUI::onPeakWindowToggled);
    }
    if (actionPeakMarker) {
        connect(actionPeakMarker, &QAction::toggled, this, &DASignalAnalysisUI::onPeakMarkerToggled);
    }
    if (actionPeakTop10) {
        connect(actionPeakTop10, &QAction::triggered, this, &DASignalAnalysisUI::onPeakTopNChanged);
        connect(actionPeakTop20, &QAction::triggered, this, &DASignalAnalysisUI::onPeakTopNChanged);
        connect(actionPeakTop50, &QAction::triggered, this, &DASignalAnalysisUI::onPeakTopNChanged);
    }
    connect(mWorker, &DASignalAnalysisWorker::failed, this, &DASignalAnalysisUI::onWorkerFailed);
    refreshFFmpegAvailability();
    if (!DASignalImporter::isFFmpegAvailable()) {
        // 不是错误，只是能力提示：用普通消息，避免一启动就弹告警
        mUI->addInfoLogMessage(
            tr("FFmpeg was not detected; video import is unavailable until a path is specified in Settings > Signal Analysis."));
        // cn:未检测到 FFmpeg，在「设置 → 信号分析」里指定路径前视频导入不可用。
    }
    // 音频解码依赖自检（I1）：soundfile 缺失时非 WAV 直读格式给出明确提示而不是运行时崩溃
    try {
        DAPyModule importer = DAPyModule("DASignalAnalysis.importer");
        const bool available = importer.attr("soundfile_available")().cast< bool >();
        if (!available) {
            mUI->addInfoLogMessage(
                tr("soundfile is not available; only WAV files can be read directly. Install soundfile or use FFmpeg."));
            // cn:未安装 soundfile，只能直接读取 WAV；可安装 soundfile 或改用 FFmpeg。
        }
    } catch (const std::exception& e) {
        qWarning() << "DASignalAnalysisUI: soundfile self-check failed:" << e.what();
    }
    return true;
}

/**
 * @brief 获取信号分析页面
 *
 * 页面惰性创建，未打开过（且没有导入过信号）时返回 nullptr。
 */
DASignalAnalysisPage* DASignalAnalysisUI::page() const
{
    return mPage;
}

// ---------------------------------------------------------------- 导入

/**
 * @brief 统一失败上报
 *
 * 界面只显示调用方给的翻译提示；`detail` 往往是后台/导入层抛出的英文异常原文
 * （e.what()），按 i18n 规范只写日志文件，不再拼进用户可见消息（评审 §七-9）。
 */
void DASignalAnalysisUI::reportOperationFailure(const QString& userText, const QString& detail)
{
    if (!detail.isEmpty()) {
        qWarning() << "DASignalAnalysisUI::operation failed:" << userText << "| detail:" << detail;
    }
    reportError(userText);
}

/**
 * @brief 统一错误上报
 *
 * 除了写入界面消息队列（da.user，用户可见），还主动唤起界面日志窗口——
 * 后台任务失败、导入失败、导出失败都必须让用户当场看到原因，不能只剩空白视图。
 */
void DASignalAnalysisUI::reportError(const QString& text)
{
    if (!mUI) {
        return;
    }
    mUI->addWarningLogMessage(text);
    if (auto* dockingArea = mUI->getDockingArea()) {
        dockingArea->raiseDockingArea(DADockingAreaInterface::DockingAreaMessageLog);
    }
}

/**
 * @brief 按 FFmpeg 可用性刷新入口状态（方案 §5.4）
 *
 * 未检测到时视频导入置灰并给出「在 设置 → 信号分析 里指定路径」的悬浮提示；
 * 检测到后恢复可用。MP3/M4A/AAC 与音频共用同一入口，按钮保持可用，
 * 选中需转码的格式时在导入阶段给出明确错误。
 */
void DASignalAnalysisUI::refreshFFmpegAvailability()
{
    const bool available = DASignalImporter::isFFmpegAvailable();
    if (actionImportVideo) {
        actionImportVideo->setEnabled(available);
        actionImportVideo->setToolTip(
            available ? tr("Extract the audio track of a video file")  // cn:抽取视频音频轨
                      : tr("FFmpeg was not detected. Specify its path in Settings > Signal Analysis."));
        // cn:未检测到 FFmpeg，请在「设置 → 信号分析」里指定路径。
    }
}

/**
 * @brief 设置对话框应用了插件全局设置后刷新界面状态
 *
 * 目前全局设置只有 FFmpeg 路径：应用后立即按新路径刷新视频导入入口，
 * 不用重启程序；状态日志在界面消息队列里留一条，方便用户确认生效结果。
 */
void DASignalAnalysisUI::onGlobalSettingsApplied()
{
    if (nullptr == mUI) {
        return;
    }
    refreshFFmpegAvailability();
    if (actionImportVideo && actionImportVideo->isEnabled()) {
        mUI->addInfoLogMessage(tr("FFmpeg is available; video import is enabled."));
        // cn:FFmpeg 可用，视频导入已启用。
    } else {
        mUI->addInfoLogMessage(tr("FFmpeg is still not available; video import stays disabled."));
        // cn:仍未检测到 FFmpeg，视频导入保持置灰。
    }
}

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
    DASignalPtr sig = DASignalImporter::importDataFrame(dlg.selectedData(),
                                                        dlg.selectedTimeColumn(),
                                                        dlg.selectedValueColumns(),
                                                        dlg.sampleRate(),
                                                        dlg.signalName(),
                                                        &error);
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
    mUI->addInfoLogMessage(tr("Signal imported: %1 (%2 Hz, %3 s)")
                               .arg(sig->name())
                               .arg(sig->sampleRate(), 0, 'f', 3)
                               .arg(sig->duration(), 0, 'f', 3));
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
    connect(view, &DASignalAnalysisView::spectrumRequested, this, &DASignalAnalysisUI::onViewRequestSpectrum);
    connect(view, &DASignalAnalysisView::envelopeRequested, this, &DASignalAnalysisUI::onViewRequestEnvelope);
    connect(view, &DASignalAnalysisView::heatmapRequested, this, &DASignalAnalysisUI::onViewRequestHeatmap);
    connect(view, &DASignalAnalysisView::currentTimeChanged, this, &DASignalAnalysisUI::onViewTimeChanged);
    connect(view, &DASignalAnalysisView::channelChanged, this, &DASignalAnalysisUI::onViewChannelChanged);
    // 「跟随滑块」只刷新峰值表显示，不触发 seek；关闭方向要恢复整段峰值（评审 N-U4）
    connect(view, &DASignalAnalysisView::followSliderToggled, this, [this](bool on) {
        if (on) {
            updatePeaksForCurrentFrame();
            return;
        }
        DASignalPtr current = (mPage) ? mPage->currentSignal() : DASignalPtr();
        DASignalAnalysisView* currentView =
            (mPage && current) ? mPage->viewForSignal(current.get()) : nullptr;
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
        if (currentView) {
            currentView->setPeaks(peaks);
        }
    });
    // 连接就绪后再发起首次计算（视图显示早于 connect，showEvent 里的请求会丢）
    view->requestInitialAnalysis();
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
 * @brief 构建中央页面（与绘图/表格/工作流并列的标签页）
 *
 * 只在 ensurePageCreated() 中调用：页面惰性创建，插件初始化时不存在。
 */
void DASignalAnalysisUI::buildCenterPage()
{
    DADockingAreaInterface* dockingArea = mUI->getDockingArea();
    if (nullptr == dockingArea) {
        qWarning() << "DASignalAnalysisUI::buildCenterPage: docking area interface is null";
        return;
    }
    mPage = new DASignalAnalysisPage();
    mPage->setObjectName(QStringLiteral("da-signal-analysis-page"));
    mPageDock = dockingArea->createDockWidgetTabAtCenterDockArea(mPage, tr("Signal Analysis"));  // cn:信号分析
    if (mPageDock) {
        mPageDock->setObjectName(QString::fromUtf8(cPageDockObjectName));
        mPageDock->setWindowTitle(tr("Signal Analysis"));  // cn:信号分析
        // 中央区各页面 dock 都带图标（绘图/表格/工作流同款），信号分析页保持一致
        mPageDock->setIcon(QIcon(QString::fromUtf8(cIconPrefix) + QStringLiteral("showSignalAnalysis.svg")));
    } else {
        // 中心区 dock 创建失败时页面成为孤儿：删掉页面并给出可见错误，
        // 不能只留一条 qWarning 让入口静默失效（评审 N-U8）
        qWarning() << "DASignalAnalysisUI::buildCenterPage: failed to create center dock for signal page";
        delete mPage;
        mPage = nullptr;
        reportError(tr("Failed to create the signal analysis page."));  // cn:创建信号分析页面失败。
    }
}

/**
 * @brief 惰性创建页面（首次打开/导入/工程恢复时调用）
 *
 * 插件初始化只建丝带标签与动作：大部分用户用不到信号分析，不应让程序一启动就多出
 * 一个空的中央停靠标签页。页面创建时一并建好页面内的导入连接与峰值停靠窗口，
 * 并恢复插件偏好（上次使用的色带）。
 * @return 页面可用返回 true
 */
bool DASignalAnalysisUI::ensurePageCreated()
{
    if (mPage && mPageDock) {
        return true;
    }
    // 页面对象存在但 dock 没建成功（极端环境）：先把孤儿页面清掉再重建（评审 N-U8）
    if (mPage && !mPageDock) {
        delete mPage;
        mPage = nullptr;
    }
    buildCenterPage();
    if (nullptr == mPage || nullptr == mPageDock) {
        return false;  // buildCenterPage 已在失败时删页面并报可见错误
    }
    // 页面内的导入入口与页面事件（页面创建前这些连接不存在）
    connect(mPage, &DASignalAnalysisPage::requestImportAudio, this, &DASignalAnalysisUI::importAudioFile);
    connect(mPage, &DASignalAnalysisPage::requestImportDataFrame, this, &DASignalAnalysisUI::importDataFrame);
    connect(mPage, &DASignalAnalysisPage::requestImportVideo, this, &DASignalAnalysisUI::importVideoFile);
    connect(mPage, &DASignalAnalysisPage::signalRemoved, this, &DASignalAnalysisUI::onSignalRemoved);
    connect(mPage, &DASignalAnalysisPage::currentSignalChanged, this, &DASignalAnalysisUI::onCurrentSignalChanged);
    // 恢复插件偏好：上次使用的色带与峰值窗口显隐（P2/N-U1）。
    // 峰值窗口的显隐偏好先读出来，buildPeakDock() 里首次显示时按它决定是否自动切到前面。
    const QJsonObject cfg = DASignalImporter::loadPluginConfig();
    mLastColormap         = cfg.value(QStringLiteral("colormap")).toString();
    if (cfg.contains(QStringLiteral("peakDockVisible"))) {
        mPeakDockVisiblePref = cfg.value(QStringLiteral("peakDockVisible")).toBool(true) ? 1 : 0;
    }
    // 峰值停靠窗口与页面同批创建（创建后保持隐藏，首次导入信号成功时按偏好再决定）
    buildPeakDock();
    setSignalDependentActionsEnabled(mPage->signalCount() > 0);
    updateRelinkActionState();
    return true;
}

/**
 * @brief 打开信号分析页面（首次调用时惰性创建）
 *
 * 丝带「打开信号分析窗口」入口。页面关闭后再次触发会重新显示并唤起到前台，
 * 与首次打开的路径一致。
 */
void DASignalAnalysisUI::openSignalAnalysisPage()
{
    if (!ensurePageCreated()) {
        return;
    }
    if (nullptr == mUI) {
        return;
    }
    if (DADockingAreaInterface* dockingArea = mUI->getDockingArea()) {
        dockingArea->raiseDockByWidget(mPage);
    }
}

/**
 * @brief 构建「信号分析」丝带顶级标签页
 */
void DASignalAnalysisUI::buildSignalCategory()
{
    DARibbonAreaInterface* ribbonArea = mUI->getRibbonArea();
    if (nullptr == ribbonArea || nullptr == ribbonArea->ribbonBar()) {
        qWarning() << "DASignalAnalysisUI::buildSignalCategory: ribbon area is unavailable";
        return;
    }
    SARibbonBar* ribbon = ribbonArea->ribbonBar();
    // 注意：SARibbonBar 没有 addCategory()，新增标签页用 addCategoryPage()（方案 T13）
    mCategory = ribbon->addCategoryPage(tr("Signal Analysis"));  // cn:信号分析
    if (nullptr == mCategory) {
        qWarning() << "DASignalAnalysisUI::buildSignalCategory: addCategoryPage failed";
        return;
    }
    mCategory->setObjectName(QString::fromUtf8(cCategoryObjectName));

    // ---------- Window：打开信号分析窗口（页面惰性创建，未打开/被关闭时从这里进入） ----------
    mPanelOpen = mCategory->addPanel(tr("Window"));  // cn:窗口
    mPanelOpen->setObjectName(QString::fromUtf8(cPanelOpenName));
    actionOpenSignalPage = mActions->createAction(
        "actionSignalOpenPage",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("showSignalAnalysis.svg")).toUtf8().constData());
    mPanelOpen->addLargeAction(actionOpenSignalPage);

    // ---------- Signal：导入三入口 ----------
    mPanelSignal = mCategory->addPanel(tr("Signal"));  // cn:信号
    mPanelSignal->setObjectName(QString::fromUtf8(cPanelSignalName));
    actionImportAudio = mActions->createAction(
        "actionSignalImportAudio",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("importAudio.svg")).toUtf8().constData());
    actionImportDataFrame = mActions->createAction(
        "actionSignalImportDataFrame",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("importDataframe.svg")).toUtf8().constData());
    actionImportVideo = mActions->createAction(
        "actionSignalImportVideo",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("importVideo.svg")).toUtf8().constData());
    // 重新指定数据帧来源（评审 U16）：当前信号的数据帧在工程里丢失时用它重新绑定
    actionRelinkSource = mActions->createAction(
        "actionSignalRelinkSource",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("importDataframe.svg")).toUtf8().constData());
    mPanelSignal->addLargeAction(actionImportAudio);
    mPanelSignal->addLargeAction(actionImportDataFrame);
    mPanelSignal->addLargeAction(actionImportVideo);
    mPanelSignal->addMediumAction(actionRelinkSource);
    actionRelinkSource->setEnabled(false);  // 只有当前信号来源缺失时才可用

    // ---------- Playback：播放/暂停/停止 ----------
    mPanelPlayback = mCategory->addPanel(tr("Playback"));  // cn:播放
    mPanelPlayback->setObjectName(QString::fromUtf8(cPanelPlaybackName));
    actionPlay = mActions->createAction(
        "actionSignalPlay",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("play.svg")).toUtf8().constData());
    actionPause = mActions->createAction(
        "actionSignalPause",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("pause.svg")).toUtf8().constData());
    actionStop = mActions->createAction(
        "actionSignalStop",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("stop.svg")).toUtf8().constData());
    mPanelPlayback->addLargeAction(actionPlay);
    mPanelPlayback->addLargeAction(actionPause);
    mPanelPlayback->addLargeAction(actionStop);

    // ---------- Analysis：统一参数对话框 ----------
    mPanelAnalysis = mCategory->addPanel(tr("Analysis"));  // cn:分析
    mPanelAnalysis->setObjectName(QString::fromUtf8(cPanelAnalysisName));
    actionAnalysisSetting = mActions->createAction(
        "actionSignalAnalysisSetting",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("analysisSetting.svg")).toUtf8().constData());
    // 取消全部后台计算（评审 N-C7）：长任务（PSD/峰值曲线/WAV 导出）用户可随时中止
    actionCancelAnalysis = mActions->createAction(
        "actionSignalCancelAnalysis",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("stop.svg")).toUtf8().constData());
    mPanelAnalysis->addLargeAction(actionAnalysisSetting);
    mPanelAnalysis->addMediumAction(actionCancelAnalysis);
    actionCancelAnalysis->setEnabled(false);  // 有任务在跑才可用

    // ---------- Peaks：峰值标记 / 前 N / 峰值窗口 ----------
    mPanelPeaks = mCategory->addPanel(tr("Peaks"));  // cn:峰值
    mPanelPeaks->setObjectName(QString::fromUtf8(cPanelPeaksName));
    actionPeakMarker = mActions->createAction(
        "actionSignalPeakMarker",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("peaks.svg")).toUtf8().constData(),
        true,
        false,
        nullptr);
    actionPeakTopN = mActions->createAction(
        "actionSignalPeakTopN",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("peaks.svg")).toUtf8().constData());
    actionPeakWindow = mActions->createAction(
        "actionSignalPeakWindow",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("peaksWindow.svg")).toUtf8().constData(),
        true,
        false,
        nullptr);
    mPanelPeaks->addLargeAction(actionPeakMarker);
    mPanelPeaks->addLargeAction(actionPeakTopN);
    mPanelPeaks->addLargeAction(actionPeakWindow);
    buildPeakTopNMenu();

    // ---------- Display：瀑布模式 / 幅值模式 ----------
    mPanelDisplay = mCategory->addPanel(tr("Display"));  // cn:显示
    mPanelDisplay->setObjectName(QString::fromUtf8(cPanelDisplayName));
    mWaterfallModeGroup = new QActionGroup(this);
    mWaterfallModeGroup->setExclusive(true);
    actionWaterfall2D = mActions->createAction(
        "actionSignalWaterfall2D",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("waterfall2d.svg")).toUtf8().constData(),
        true,
        false,
        nullptr);
    actionWaterfall3D = mActions->createAction(
        "actionSignalWaterfall3D",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("waterfall3d.svg")).toUtf8().constData(),
        true,
        false,
        nullptr);
    actionWaterfallHide = mActions->createAction(
        "actionSignalWaterfallHide",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("waterfallHide.svg")).toUtf8().constData(),
        true,
        false,
        nullptr);
    actionWaterfall2D->setActionGroup(mWaterfallModeGroup);
    actionWaterfall3D->setActionGroup(mWaterfallModeGroup);
    actionWaterfallHide->setActionGroup(mWaterfallModeGroup);
    actionWaterfall2D->setChecked(true);  // 默认二维（D12）
    mPanelDisplay->addMediumAction(actionWaterfall2D);
    mPanelDisplay->addMediumAction(actionWaterfall3D);
    mPanelDisplay->addMediumAction(actionWaterfallHide);

    actionAmplitudeMode = mActions->createAction(
        "actionSignalAmplitudeMode",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("amplitude.svg")).toUtf8().constData());
    mPanelDisplay->addLargeAction(actionAmplitudeMode);
    // 峰值表显隐（Display 面板；评审 §七-8：actionPeakTable 之前是死成员）
    actionPeakTable = mActions->createAction(
        "actionSignalPeakTable",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("peaksWindow.svg")).toUtf8().constData(),
        true,
        false,
        nullptr);
    mPanelDisplay->addMediumAction(actionPeakTable);
    buildAmplitudeModeMenu();

    // ---------- Export：峰值表 / 图片 ----------
    mPanelExport = mCategory->addPanel(tr("Export"));  // cn:导出
    mPanelExport->setObjectName(QString::fromUtf8(cPanelExportName));
    actionExportPeaks = mActions->createAction(
        "actionSignalExportPeaks",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("exportTable.svg")).toUtf8().constData());
    actionExportImage = mActions->createAction(
        "actionSignalExportImage",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("exportImage.svg")).toUtf8().constData());
    mPanelExport->addLargeAction(actionExportPeaks);
    mPanelExport->addLargeAction(actionExportImage);
}

/**
 * @brief 构建峰值停靠窗口（右侧停靠区，与设置/智能体同区做标签页）
 */
void DASignalAnalysisUI::buildPeakDock()
{
    if (nullptr == mUI) {
        return;
    }
    DADockingAreaInterface* dockingArea = mUI->getDockingArea();
    if (nullptr == dockingArea) {
        return;
    }
    // 创建期间的显隐变化不算用户偏好（可能是创建即激活/随后隐藏）：
    // 用 mCreatingPeakDock 屏蔽，避免把偏好覆盖成「隐藏」（评审 N-U1）
    mCreatingPeakDock = true;
    mPeakPanel        = new DAPeakDockWidget();
    mPeakPanel->setObjectName(QStringLiteral("da-signal-peak-panel"));
    // 与「设置」同区作为标签页
    ads::CDockWidget* settingDock =
        dockingArea->dockingAreaToDockWidget(DADockingAreaInterface::DockingAreaSetting);
    ads::CDockAreaWidget* settingArea = settingDock ? settingDock->dockAreaWidget() : nullptr;
    if (settingArea) {
        mPeakDock = dockingArea->createDockWidgetAsTab(mPeakPanel, tr("Peaks"), settingArea);  // cn:峰值
    } else {
        mPeakDock = dockingArea->createDockWidget(mPeakPanel, ads::RightDockWidgetArea, tr("Peaks"));
        // cn:峰值
    }
    if (mPeakDock) {
        mPeakDock->setObjectName(QStringLiteral("da-dock-signal-peaks"));
        mPeakDock->setWindowTitle(tr("Peaks"));  // cn:峰值
        // 首次显示峰值窗口时按需计算每帧前 N 峰值曲线；显隐状态同步丝带按钮（评审 U14）
        connect(mPeakDock, &ads::CDockWidget::visibilityChanged, this, [this](bool visible) {
            if (actionPeakWindow) {
                const QSignalBlocker blocker(actionPeakWindow);
                actionPeakWindow->setChecked(visible);
            }
            if (actionPeakTable) {
                const QSignalBlocker blocker(actionPeakTable);
                actionPeakTable->setChecked(visible);
            }
            if (!mCreatingPeakDock) {
                mPeakDockVisiblePref = visible ? 1 : 0;  // 记录用户偏好（N-U1/P2）
            }
            if (visible && !mPeakCurvesRequested) {
                requestPeaks(true);
            }
        });
        // 创建即激活会让插件加载后右侧直接跳到空的峰值页（评审 D24/U14）：先隐藏，
        // 首次导入成功后再按偏好自动切一次（见 showPeakDockForFirstSignal）
        mPeakDock->toggleView(false);
    }
    mCreatingPeakDock = false;
    connect(mPeakPanel, &DAPeakDockWidget::sendToTableRequested, this, &DASignalAnalysisUI::sendPeaksToTable);
    connect(mPeakPanel, &DAPeakDockWidget::exportCsvRequested, this, &DASignalAnalysisUI::exportPeaksCsv);
    connect(mPeakPanel, &DAPeakDockWidget::settingsRequested, this, &DASignalAnalysisUI::openAnalysisSettings);
}

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
        mUI->addInfoLogMessage(tr("Large signal confirmed: %1 assumes about %2 GB of memory.").arg(stage).arg(estimatedGb, 0, 'f', 2));
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
    in.sampleCount         = samples;
    in.sampleRate          = sig->sampleRate();
    in.resolutionHz        = params.resolutionHz;
    in.stftWindowMs        = params.stftWindowMs;
    in.stftOverlapPercent  = params.stftOverlap;
    in.peakTopN            = params.peakTopN;
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

/**
 * @brief 清空峰值表/曲线缓存（切信号、加载工程时调用）
 */void DASignalAnalysisUI::clearPeakState()
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
 * @brief 清空页面上全部信号与相关缓存（工程加载前调用，评审 U15/P4）
 */
void DASignalAnalysisUI::clearAllSignals()
{
    if (mPlayer) {
        mPlayer->stop();
        mPlayer->clearSignal();
    }
    if (mPage) {
        mPage->clearAll();
    }
    mParamsPerSignal.clear();
    mParamsVersion.clear();
    mSpectrumCache.clear();
    mSpectrumCacheChannel.clear();
    mMemoryBudgetVerdict.clear();
    clearPeakState();
    setSignalDependentActionsEnabled(false);
    updateRelinkActionState();
    mFirstSignalActivated = false;
}

/**
 * @brief 宿主清空工程：清理本插件残留（评审 N-U2/P5/U15 / Q7）
 *
 * 宿主在新建工程、加载工程前、快照回滚时调用 DAProjectInterface::clear()，
 * 那一步不会触碰插件（DAAppProject::clear 只清数据与图表）。这里做幂等清理：
 * 页面/峰值窗口从未创建时也要安全返回，只剩缓存与临时文件需要处理。
 */
void DASignalAnalysisUI::onProjectCleaned()
{
    // 后台任务先停：旧工程的信号数据不再需要，避免继续占用内存/临时文件
    if (mWorker) {
        mWorker->cancel();
    }
    mImportTaskActive = false;
    updateImportActionsState();
    if (mPlayer) {
        mPlayer->stop();
        mPlayer->clearSignal();
    }
    if (mPage) {
        clearAllSignals();  // 清信号标签页、峰值表/曲线、频谱缓存与动作使能
    } else {
        mParamsPerSignal.clear();
        mParamsVersion.clear();
        mSpectrumCache.clear();
        mSpectrumCacheChannel.clear();
        mMemoryBudgetVerdict.clear();
        clearPeakState();
        setSignalDependentActionsEnabled(false);
        mFirstSignalActivated = false;
    }
    updateRelinkActionState();
    // 纯「关闭工程」路径也要清掉临时 WAV（评审 P5），不必等到下次加载或插件卸载
    DASignalImporter::cleanupTempFiles();
    clearProgressIfIdle();
    updateCancelActionState();
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
    bool ok = false;
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
    // 旧视图的显示状态也尽量沿用（瀑布模式/色带/对数轴/色标下限/跟随滚动）
    int oldWaterfallMode = 0;
    QString oldColormap;
    bool oldLogFrequency   = false;
    double oldDbFloor      = -80.0;
    bool oldAutoScroll     = false;
    if (DASignalAnalysisView* oldView = mPage->viewForSignal(sig.get())) {
        oldWaterfallMode = oldView->waterfallMode();
        oldColormap      = oldView->colormapName();
        oldLogFrequency  = oldView->isLogFrequency();
        oldDbFloor       = oldView->dbFloor();
        oldAutoScroll    = oldView->isHeatmapAutoScroll();
    }
    QString error;
    DASignalPtr newSig = DASignalImporter::importDataFrame(frames.at(idx),
                                                           dfSrc->timeColumn(),
                                                           dfSrc->valueColumns(),
                                                           dfSrc->sampleRate(),
                                                           signalName,
                                                           &error);
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
    const int index = mPage->indexOfSignal(sig.get());
    if (!mPage->replaceSignalAt(index, newSig)) {
        mParamsPerSignal.remove(newSig.get());
        reportError(tr("Failed to relink the data frame source."));  // cn:重新指定数据帧来源失败。
        return;
    }
    connectSignalView(newSig);
    if (DASignalAnalysisView* newView = mPage->viewForSignal(newSig.get())) {
        newView->setWaterfallMode(oldWaterfallMode);
        newView->setLogFrequency(oldLogFrequency);
        newView->setDbFloor(oldDbFloor);
        newView->setHeatmapAutoScroll(oldAutoScroll);
        if (!oldColormap.isEmpty()) {
            newView->setColormapName(oldColormap);
        }
    }
    setSignalDependentActionsEnabled(true);
    syncPlayerSource();
    updateRelinkActionState();
    mUI->addInfoLogMessage(tr("Signal \"%1\" relinked to data frame \"%2\".").arg(signalName, chosen));
    // cn:信号「%1」已重新指向数据帧「%2」。
}

/**
 * @brief 构建「前 N 个峰值」下拉菜单（10/20/50，互斥）
 */
void DASignalAnalysisUI::buildPeakTopNMenu()
{
    if (nullptr == actionPeakTopN) {
        return;
    }
    mPeakTopNMenu      = new QMenu();
    mPeakTopNGroup     = new QActionGroup(this);
    mPeakTopNGroup->setExclusive(true);
    actionPeakTop10 = mPeakTopNMenu->addAction(QStringLiteral("10"));
    actionPeakTop20 = mPeakTopNMenu->addAction(QStringLiteral("20"));
    actionPeakTop50 = mPeakTopNMenu->addAction(QStringLiteral("50"));
    for (QAction* act : { actionPeakTop10, actionPeakTop20, actionPeakTop50 }) {
        act->setCheckable(true);
        act->setActionGroup(mPeakTopNGroup);
    }
    actionPeakTop10->setChecked(true);
    actionPeakTopN->setMenu(mPeakTopNMenu);
}

/**
 * @brief 构建「幅值模式」下拉菜单（峰值幅度/均方根/dBFS/dB re 1.0，互斥）
 */
void DASignalAnalysisUI::buildAmplitudeModeMenu()
{
    if (nullptr == actionAmplitudeMode) {
        return;
    }
    mAmplitudeModeMenu  = new QMenu();
    mAmplitudeModeGroup = new QActionGroup(this);
    mAmplitudeModeGroup->setExclusive(true);
    actionAmpPeak  = mAmplitudeModeMenu->addAction(tr("Peak amplitude"));  // cn:峰值幅度
    actionAmpRms   = mAmplitudeModeMenu->addAction(tr("RMS"));             // cn:均方根
    actionAmpDbFs  = mAmplitudeModeMenu->addAction(tr("dB (FS)"));         // cn:dB（满量程）
    actionAmpDbRef1 = mAmplitudeModeMenu->addAction(tr("dB (re 1.0)"));    // cn:dB（相对 1.0）
    for (QAction* act : { actionAmpPeak, actionAmpRms, actionAmpDbFs, actionAmpDbRef1 }) {
        act->setCheckable(true);
        act->setActionGroup(mAmplitudeModeGroup);
    }
    actionAmpPeak->setChecked(true);  // 默认峰值幅度（D14）
    actionAmplitudeMode->setMenu(mAmplitudeModeMenu);
    connect(actionAmpPeak, &QAction::triggered, this, &DASignalAnalysisUI::onAmplitudeModeChanged);
    connect(actionAmpRms, &QAction::triggered, this, &DASignalAnalysisUI::onAmplitudeModeChanged);
    connect(actionAmpDbFs, &QAction::triggered, this, &DASignalAnalysisUI::onAmplitudeModeChanged);
    connect(actionAmpDbRef1, &QAction::triggered, this, &DASignalAnalysisUI::onAmplitudeModeChanged);
}

// ---------------------------------------------------------------- 导出与工程保存

namespace {
/// 调 Python 导出模块并执行一个返回 bool 的导出函数
bool callPythonExport(const char* funcName, pybind11::tuple args)
{
    try {
        DAPyModule exporter = DAPyModule("DASignalAnalysis.exporter");
        pybind11::object res = exporter.attr(funcName)(*args);
        return res.cast< bool >();
    } catch (const std::exception& e) {
        qWarning() << "signal export failed:" << e.what();
        return false;
    }
}

pybind11::tuple doubleTuple(const QVector< double >& v)
{
    pybind11::list lst;
    for (double x : v) {
        lst.append(x);
    }
    pybind11::module_ np = pybind11::module_::import("numpy");
    return pybind11::tuple(np.attr("asarray")(lst, pybind11::arg("dtype") = "float64"));
}

QString askSavePath(QWidget* parent, const QString& title, const QString& filter)
{
    return QFileDialog::getSaveFileName(parent, title, QString(), filter);
}

/// 当前幅值模式的英文列名（CSV 表头，保持英文便于二次处理）
QString dasignalAmplitudeLabel(const DASignalAnalysisParams& params)
{
    if (params.amplitudeMode == QLatin1String("rms")) {
        return QStringLiteral("RMS");
    }
    if (params.amplitudeMode == QLatin1String("dbFS")) {
        return QStringLiteral("dB (FS)");
    }
    if (params.amplitudeMode == QLatin1String("dbRef1")) {
        return QStringLiteral("dB (re 1.0)");
    }
    return QStringLiteral("Amplitude");
}

/// 峰值幅值列表 → 当前显示口径（与频谱图/峰值表同一换算函数）
QVector< double > peakMagnitudesForDisplay(const DAPeakResult& peaks,
                                           const DASignalAnalysisParams& params,
                                           double calibratedSignalPeak)
{
    QVector< double > out;
    out.reserve(peaks.magnitudes.size());
    for (int i = 0; i < peaks.magnitudes.size(); ++i) {
        out.append(DASignalAmplitude::toDisplay(peaks.magnitudes.at(i),
                                               peaks.frequencies.value(i) <= 0.0,
                                               params.amplitudeMode,
                                               params.dbReference,
                                               params.calibration,
                                               calibratedSignalPeak));
    }
    return out;
}
}  // namespace

/**
 * @brief 导出峰值表 CSV
 */
void DASignalAnalysisUI::exportPeaksCsv()
{
    if (!mCurrentPeaks.isValid()) {
        reportError(tr("No peak data to export."));  // cn:没有可导出的峰值数据。
        return;
    }
    const QString path =
        askSavePath(mPage, tr("Export Peak Table"), tr("CSV Files (*.csv)"));  // cn:导出峰值表
    if (path.isEmpty()) {
        return;
    }
    // 与屏幕一致：幅值列按当前显示口径换算（评审 I9）
    DASignalPtr sig      = mPage ? mPage->currentSignal() : DASignalPtr();
    const DASignalAnalysisParams params = paramsForSignal(sig);
    double signalPeak    = 0.0;
    if (mPage) {
        if (DASignalAnalysisView* v = mPage->viewForSignal(sig.get())) {
            signalPeak = v->signalPeak();
        }
    }
    const double peakCal = signalPeak * params.calibration;
    QVector< double > magnitudes;
    magnitudes.reserve(mCurrentPeaks.magnitudes.size());
    for (int i = 0; i < mCurrentPeaks.magnitudes.size(); ++i) {
        magnitudes.append(DASignalAmplitude::toDisplay(mCurrentPeaks.magnitudes.at(i),
                                                       mCurrentPeaks.frequencies.value(i) <= 0.0,
                                                       params.amplitudeMode,
                                                       params.dbReference,
                                                       params.calibration,
                                                       peakCal));
    }
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(mCurrentPeaks.frequencies),
                                                doubleTuple(magnitudes),
                                                doubleTuple(mCurrentPeaks.relDb),
                                                doubleTuple(mCurrentPeaks.share),
                                                dasignalAmplitudeLabel(params).toStdString());
    if (callPythonExport("write_peaks_csv", args)) {
        mUI->addInfoLogMessage(tr("Peak table exported: %1").arg(path));  // cn:峰值表已导出：%1
    } else {
        reportError(tr("Failed to export the peak table."));  // cn:峰值表导出失败。
    }
}

/**
 * @brief 导出频谱曲线 CSV（两列：频率、当前幅值模式下的幅值）
 */
void DASignalAnalysisUI::exportSpectrumCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    const DASpectrumResult spec = spectrumForSignal(sig, channelOfSignal(sig));
    if (!spec.isValid()) {
        reportError(tr("No spectrum data to export."));  // cn:没有可导出的频谱数据。
        return;
    }
    const QString path =
        askSavePath(mPage, tr("Export Spectrum"), tr("CSV Files (*.csv)"));  // cn:导出频谱
    if (path.isEmpty()) {
        return;
    }
    // 按当前幅值模式换算（与显示口径一致，评审 I9）
    const DASignalAnalysisParams params = paramsForSignal(sig);
    double signalPeak                   = 0.0;
    if (mPage) {
        if (DASignalAnalysisView* v = mPage->viewForSignal(sig.get())) {
            signalPeak = v->signalPeak();
        }
    }
    QVector< double > values;
    values.reserve(spec.amplitudes.size());
    for (int i = 0; i < spec.amplitudes.size(); ++i) {
        values.append(DASignalAmplitude::toDisplay(spec.amplitudes.at(i),
                                                   i == 0,
                                                   params.amplitudeMode,
                                                   params.dbReference,
                                                   params.calibration,
                                                   signalPeak * params.calibration));
    }
    const QString yLabel = dasignalAmplitudeLabel(params);
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(spec.freqs),
                                                doubleTuple(values),
                                                yLabel.toStdString());
    if (callPythonExport("write_spectrum_csv", args)) {
        mUI->addInfoLogMessage(tr("Spectrum exported: %1").arg(path));  // cn:频谱已导出：%1
    } else {
        reportError(tr("Failed to export the spectrum."));  // cn:频谱导出失败。
    }
}

/**
 * @brief 导出韦尔奇功率谱密度 CSV（频率、PSD(dB re 1.0/Hz)）
 *
 * D5 把韦尔奇功率谱列入 v1 算法边界，但此前算完没有任何显示或导出入口（评审 C10）。
 * v1 不为 PSD 单独加一张图，先把结果以 CSV 导出打通，口径与计算层一致：
 * P[f] 为单边功率谱密度，转 dB 用 10·log10（功率量）。
 */
void DASignalAnalysisUI::exportPsdCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    const DASpectrumResult spec = spectrumForSignal(sig, channelOfSignal(sig));
    if (!spec.isValid() || spec.psdFreqs.isEmpty() || spec.psdValues.isEmpty()) {
        reportError(tr("No power spectral density data to export."));  // cn:没有可导出的功率谱密度数据。
        return;
    }
    const QString path = askSavePath(mPage,
                                     tr("Export Power Spectral Density"),  // cn:导出功率谱密度
                                     tr("CSV Files (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    constexpr double cEps = 1e-30;
    QVector< double > values;
    values.reserve(spec.psdValues.size());
    for (double p : std::as_const(spec.psdValues)) {
        values.append(10.0 * std::log10(qMax(p, cEps)));
    }
    const QString yLabel = tr("PSD (dB re 1.0/Hz)");  // cn:功率谱密度（dB，相对 1.0/赫兹）
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(spec.psdFreqs),
                                                doubleTuple(values),
                                                yLabel.toStdString());
    if (callPythonExport("write_spectrum_csv", args)) {
        mUI->addInfoLogMessage(tr("Power spectral density exported: %1").arg(path));
        // cn:功率谱密度已导出：%1
    } else {
        reportError(tr("Failed to export the power spectral density."));  // cn:功率谱密度导出失败。
    }
}

/**
 * @brief 导出热图矩阵 CSV（行 = 频率、列 = 时间；导出前给体积提示）
 */
void DASignalAnalysisUI::exportHeatmapCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig || !mPage) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
    if (!view) {
        return;
    }
    const DAStftResult stft = view->stftResult();
    if (!stft.isValid()) {
        reportError(tr("No time-frequency data to export."));  // cn:没有可导出的时频数据。
        return;
    }
    const qint64 cells = qint64(stft.rows) * stft.columns;
    // 大矩阵导出前给体积确认，并提供 2 倍降采样（方案 §10.2 / 评审 I10）
    int downsample = 1;
    if (cells > 2000000) {
        // 文本 CSV 每单元实际约 12–16 字节（数值文本 + 分隔符 + 行尾），按 14 B/单元估算（评审 N-I10）
        constexpr double cBytesPerCsvCell = 14.0;
        const double estimatedMb          = double(cells) * cBytesPerCsvCell / (1024.0 * 1024.0);
        QMessageBox box(QMessageBox::Question,
                        tr("Export Heatmap Matrix"),  // cn:导出热图矩阵
                        tr("The heatmap matrix is %1 x %2 = %3 cells (about %4 MB as CSV).\n"
                           "Export the full matrix or downsample by 2?")
                            // cn:热图矩阵为 %1 x %2 = %3 个单元（CSV 约 %4 兆字节）。导出完整矩阵还是 2 倍降采样？
                            .arg(stft.rows)
                            .arg(stft.columns)
                            .arg(cells)
                            .arg(estimatedMb, 0, 'f', 1),
                        QMessageBox::NoButton,
                        mPage);
        QPushButton* fullBtn =
            box.addButton(tr("Export full"), QMessageBox::AcceptRole);  // cn:导出完整矩阵
        QPushButton* downBtn =
            box.addButton(tr("Downsample x2"), QMessageBox::ActionRole);  // cn:2 倍降采样
        QPushButton* cancelBtn = box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(cancelBtn);  // 安全默认：不误触大文件导出
        box.exec();
        if (box.clickedButton() == cancelBtn) {
            return;
        }
        if (box.clickedButton() == downBtn) {
            downsample = 2;
        } else if (box.clickedButton() != fullBtn) {
            return;
        }
    }
    mUI->addInfoLogMessage(tr("Heatmap matrix: %1 x %2 = %3 cells, the CSV may be large.")
                               .arg(stft.rows)
                               .arg(stft.columns)
                               .arg(cells));
    // cn:热图矩阵：%1 x %2 = %3 个单元，CSV 可能较大
    const QString path =
        askSavePath(mPage, tr("Export Heatmap Matrix"), tr("CSV Files (*.csv)"));  // cn:导出热图矩阵
    if (path.isEmpty()) {
        return;
    }
    QVector< double > outTimes = stft.times;
    QVector< double > outFreqs = stft.freqs;
    QVector< double > outValues = stft.values;
    int outRows                  = stft.rows;
    int outCols                  = stft.columns;
    if (downsample > 1) {
        // 块最大值降采样（保留峰值，不丢窄峰）
        outRows = (stft.rows + downsample - 1) / downsample;
        outCols = (stft.columns + downsample - 1) / downsample;
        QVector< double > reduced(outRows * outCols, std::numeric_limits< double >::quiet_NaN());
        for (int r = 0; r < stft.rows; ++r) {
            const int rr = r / downsample;
            for (int c = 0; c < stft.columns; ++c) {
                const double v = stft.values.value(r * stft.columns + c);
                if (!std::isfinite(v)) {
                    continue;
                }
                double& dst = reduced[ rr * outCols + (c / downsample) ];
                dst         = std::isfinite(dst) ? qMax(dst, v) : v;
            }
        }
        QVector< double > t(outCols);
        for (int c = 0; c < outCols; ++c) {
            t[ c ] = stft.times.value(qMin(stft.columns - 1, c * downsample + downsample / 2));
        }
        QVector< double > f(outRows);
        for (int r = 0; r < outRows; ++r) {
            f[ r ] = stft.freqs.value(qMin(stft.rows - 1, r * downsample + downsample / 2));
        }
        outTimes  = t;
        outFreqs  = f;
        outValues = reduced;
    }
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(outTimes),
                                                doubleTuple(outFreqs),
                                                doubleTuple(outValues),
                                                outRows,
                                                outCols);
    if (callPythonExport("write_heatmap_csv", args)) {
        mUI->addInfoLogMessage(tr("Heatmap matrix exported: %1").arg(path));  // cn:热图矩阵已导出：%1
    } else {
        reportError(tr("Failed to export the heatmap matrix."));  // cn:热图矩阵导出失败。
    }
}

/**
 * @brief 导出当前视图为位图（三维 GL 内容不保证被抓取，见 T7）
 */
void DASignalAnalysisUI::exportImage()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig || !mPage) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
    if (!view) {
        return;
    }
    const QString path =
        askSavePath(mPage, tr("Export Image"), tr("PNG Image (*.png)"));  // cn:导出图片
    if (path.isEmpty()) {
        return;
    }
    // 三维瀑布可见时把 OpenGL 帧缓冲合成进页面位图（评审 I11）
    const QPixmap pixmap = view->grabForExport();
    if (pixmap.save(path, "PNG")) {
        mUI->addInfoLogMessage(tr("Image exported: %1").arg(path));  // cn:图片已导出：%1
    } else {
        reportError(tr("Failed to export the image."));  // cn:图片导出失败。
    }
}

/**
 * @brief 峰值表送表格页（转成数据帧加入数据管理器）
 */
void DASignalAnalysisUI::sendPeaksToTable()
{
    if (!mCurrentPeaks.isValid()) {
        reportError(tr("No peak data to send."));  // cn:没有可发送的峰值数据。
        return;
    }
    if (!mCore) {
        return;
    }
    DADataManagerInterface* dataMgr = mCore->getDataManagerInterface();
    if (!dataMgr) {
        return;
    }
    try {
        DAPyModule exporter = DAPyModule("DASignalAnalysis.exporter");
        DASignalPtr sig     = (mPage && mPage->currentSignal()) ? mPage->currentSignal() : DASignalPtr();
        const DASignalAnalysisParams params = paramsForSignal(sig);
        double signalPeak                   = 0.0;
        if (mPage) {
            if (DASignalAnalysisView* v = mPage->viewForSignal(sig.get())) {
                signalPeak = v->signalPeak();
            }
        }
        // 与屏幕一致：幅值列按当前显示口径换算（评审 I9）
        const QVector< double > magnitudes =
            peakMagnitudesForDisplay(mCurrentPeaks, params, signalPeak * params.calibration);
        pybind11::object df = exporter.attr("peaks_to_dataframe")(doubleTuple(mCurrentPeaks.frequencies),
                                                                   doubleTuple(magnitudes),
                                                                   doubleTuple(mCurrentPeaks.relDb),
                                                                   doubleTuple(mCurrentPeaks.share),
                                                                   dasignalAmplitudeLabel(params).toStdString());
        DAPyDataFrame pyDf(df);
        const QString signalName = (mPage && mPage->currentSignal()) ? mPage->currentSignal()->name() : QString();
        QString tableName        = tr("Peaks - %1").arg(signalName);  // cn:峰值 - %1
        DA::DAData data(pyDf);
        data.setName(tableName);
        dataMgr->addData(data);
        mUI->addInfoLogMessage(tr("Peak table added to the data manager as \"%1\".").arg(tableName));
        // cn:峰值表已作为「%1」加入数据管理器。
    } catch (const std::exception& e) {
        // 异常原文（英文）只写日志，界面给翻译后的提示（评审 §七-9）
        reportOperationFailure(tr("Failed to send the peak table."), QString::fromUtf8(e.what()));
        // cn:峰值表发送失败。
    }
}

/**
 * @brief 序列化当前状态（信号来源引用 + 参数 + 视图状态）
 */
QByteArray DASignalAnalysisUI::serializeState() const
{
    QJsonArray arr;
    if (mPage) {
        for (const DASignalPtr& sig : mPage->allSignals()) {
            if (!sig) {
                continue;
            }
            QJsonObject o;
            o[ QStringLiteral("name") ] = sig->name();
            switch (sig->sourceType()) {
            case DASignalSource::DataFrameSource: {
                auto dfSrc = std::dynamic_pointer_cast< DADataFrameSignalSource >(sig->source());
                o[ QStringLiteral("sourceType") ]    = QStringLiteral("dataframe");
                // 缺失占位也要把工程里记录的数据帧名存回去（评审 U16）
                o[ QStringLiteral("dataframeName") ] = dfSrc ? dfSrc->dataframeName() : QString();
                o[ QStringLiteral("timeColumn") ]    = dfSrc ? dfSrc->timeColumn() : QString();
                QJsonArray cols;
                if (dfSrc) {
                    for (const QString& c : dfSrc->valueColumns()) {
                        cols.append(c);
                    }
                }
                o[ QStringLiteral("valueColumns") ] = cols;
                break;
            }
            case DASignalSource::VideoSource:
                o[ QStringLiteral("sourceType") ] = QStringLiteral("video");
                o[ QStringLiteral("path") ]       = sig->source() ? sig->source()->sourceDescription() : QString();
                break;
            case DASignalSource::FileSource:
            default:
                o[ QStringLiteral("sourceType") ] = QStringLiteral("file");
                o[ QStringLiteral("path") ]       = sig->source() ? sig->source()->sourceDescription() : QString();
                break;
            }
            o[ QStringLiteral("sampleRate") ]  = sig->sampleRate();
            o[ QStringLiteral("timeOrigin") ]  = sig->timeOrigin();
            o[ QStringLiteral("calibration") ] = sig->calibration();
            o[ QStringLiteral("unit") ]        = sig->unit();
            o[ QStringLiteral("params") ]      = paramsForSignal(sig).toJson();
            QJsonObject view;
            view[ QStringLiteral("amplitudeMode") ] = mAmplitudeMode;
            view[ QStringLiteral("dbReference") ]   = mDbReference;
            // 视图状态（P1）：瀑布模式、对数轴、色带/色标、分割比例、热图跟随
            if (DASignalAnalysisView* v = mPage ? mPage->viewForSignal(sig.get()) : nullptr) {
                view[ QStringLiteral("waterfallMode") ] = v->waterfallMode();
                view[ QStringLiteral("logFrequency") ]  = v->isLogFrequency();
                view[ QStringLiteral("colormap") ]      = v->colormapName();
                view[ QStringLiteral("dbFloor") ]       = v->dbFloor();
                view[ QStringLiteral("autoScroll") ]    = v->isHeatmapAutoScroll();
                QJsonArray vArr;
                for (int s : v->verticalSplitterSizes()) {
                    vArr.append(s);
                }
                QJsonArray hArr;
                for (int s : v->horizontalSplitterSizes()) {
                    hArr.append(s);
                }
                view[ QStringLiteral("splitterVertical") ]   = vArr;
                view[ QStringLiteral("splitterHorizontal") ] = hArr;
            }
            o[ QStringLiteral("view") ] = view;
            arr.append(o);
        }
    }
    QJsonObject root;
    root[ QStringLiteral("signals") ] = arr;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

/**
 * @brief 从 JSON 重建信号（主线程；文件缺失时提示信号源缺失并跳过）
 */
void DASignalAnalysisUI::restoreState(const QByteArray& json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        // 工程包里没有本插件的存档条目（旧工程 / 保存时插件未启用 / 条目损坏）：
        // 之前打开过页面时必须清掉上一工程残留的信号与临时文件，否则旧信号会被
        // 再次保存进新工程（评审 N-U2/U15；宿主 clear() 不触碰插件，无法兜底）
        if (mPage) {
            clearAllSignals();
        }
        DASignalImporter::cleanupTempFiles();
        return;
    }
    const QJsonArray arr = doc.object().value(QStringLiteral("signals")).toArray();
    if (arr.isEmpty()) {
        // 工程里没有信号：保持页面惰性（从未打开过就不创建，避免打开普通工程时多出空页面）；
        // 之前打开过页面的话，清掉上一工程残留的信号与临时文件
        if (mPage) {
            clearAllSignals();
            DASignalImporter::cleanupTempFiles();
        }
        return;
    }
    // 工程里可能有信号记录：惰性创建页面后才能重建信号标签页
    if (!ensurePageCreated()) {
        return;
    }
    // 打开新工程/快照回滚前先清空旧信号与缓存，避免两个工程的信号叠加（评审 U15/P4）
    clearAllSignals();
    // 旧信号的临时抽取 WAV 随旧工程一起清理（评审 P5/I12）：此时信号与播放器均已清空
    DASignalImporter::cleanupTempFiles();
    DADataManagerInterface* mgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        const QString type  = o.value(QStringLiteral("sourceType")).toString();
        const QString name  = o.value(QStringLiteral("name")).toString();
        DASignalPtr sig;
        QString error;
        if (type == QLatin1String("dataframe")) {
            const QString dfName = o.value(QStringLiteral("dataframeName")).toString();
            QStringList cols;
            const QJsonArray colsArr = o.value(QStringLiteral("valueColumns")).toArray();
            for (const QJsonValue& c : colsArr) {
                cols.append(c.toString());
            }
            const QString timeCol = o.value(QStringLiteral("timeColumn")).toString();
            const double savedFs  = o.value(QStringLiteral("sampleRate")).toDouble();
            DAData data;
            if (mgr) {
                const QList< DAData > all = mgr->getAllDatas();
                for (const DAData& d : all) {
                    if (d.isDataFrame() && d.getName() == dfName) {
                        data = d;
                        break;
                    }
                }
            }
            if (!data.isDataFrame()) {
                // 数据帧缺失（评审 U16）：不再直接跳过，而是保留「来源缺失」占位信号与参数，
                // 用户可用丝带「重新指定数据帧来源」重新绑定到现有数据帧
                auto placeholder = std::make_shared< DADataFrameSignalSource >(dfName, timeCol, cols, savedFs);
                sig              = std::make_shared< DASignal >(name, placeholder);
                qWarning() << "DASignalAnalysisUI: dataframe source missing, placeholder kept:" << dfName;
                reportError(tr("Signal source missing: data frame \"%1\" (signal \"%2\"). "
                               "Use \"Relink Data Frame Source\" to point it to an existing data frame.")
                                // cn:信号源缺失：数据帧「%1」（信号「%2」）。可用「重新指定数据帧来源」指向现有数据帧。
                                .arg(dfName, name));
            } else {
                sig = DASignalImporter::importDataFrame(data, timeCol, cols, savedFs, name, &error);
            }
        } else {
            QString path = o.value(QStringLiteral("path")).toString();
            if (!QFileInfo::exists(path)) {
                // 信号源缺失（评审 U16）：给「重新指定」入口，参数与采样率仍按工程里的记录恢复
                QMessageBox box(QMessageBox::Question,
                                tr("Signal Source Missing"),  // cn:信号源缺失
                                tr("Signal \"%1\" refers to a missing file:\n%2\n\nLocate the file again?")
                                    // cn:信号「%1」引用的文件不存在：%2。要重新指定文件吗？
                                    .arg(name, path),
                                QMessageBox::NoButton,
                                mPage);
                QPushButton* locateBtn =
                    box.addButton(tr("Locate..."), QMessageBox::AcceptRole);  // cn:重新指定...
                box.addButton(tr("Skip"), QMessageBox::RejectRole);          // cn:跳过
                box.setDefaultButton(locateBtn);
                box.exec();
                if (box.clickedButton() != locateBtn) {
                    continue;
                }
                const QString located = QFileDialog::getOpenFileName(
                    mPage,
                    tr("Locate Signal Source"),  // cn:重新指定信号源
                    QFileInfo(path).absolutePath(),
                    tr("Audio/Video files (*.wav *.aiff *.aif *.flac *.ogg *.mp3 *.m4a *.mp4 *.mkv *.avi);;All files (*)"));
                // cn:音频/视频文件
                if (located.isEmpty()) {
                    continue;  // 用户取消：保持缺失状态，不创建半成品信号
                }
                path = located;
            }
            sig = (type == QLatin1String("video")) ? DASignalImporter::importVideoFile(path, &error)
                                                   : DASignalImporter::importAudioFile(path, &error);
            if (sig) {
                // 用保存时确认过的采样率（不再回读文件头，方案 §4.4）
                const double savedFs = o.value(QStringLiteral("sampleRate")).toDouble();
                if (savedFs > 0) {
                    sig->setSampleRate(savedFs);
                }
            }
        }
        if (!sig) {
            reportOperationFailure(tr("Failed to restore signal \"%1\".").arg(name), error);
            // cn:恢复信号「%1」失败。
            continue;
        }
        sig->setName(name);
        sig->setTimeOrigin(o.value(QStringLiteral("timeOrigin")).toDouble());
        sig->setCalibration(o.value(QStringLiteral("calibration")).toDouble(1.0));
        sig->setUnit(o.value(QStringLiteral("unit")).toString());
        mParamsPerSignal[ sig.get() ] = DASignalAnalysisParams::fromJson(o.value(QStringLiteral("params")).toObject());
        const QJsonObject view        = o.value(QStringLiteral("view")).toObject();
        if (view.contains(QStringLiteral("amplitudeMode"))) {
            mAmplitudeMode = view.value(QStringLiteral("amplitudeMode")).toString(mAmplitudeMode);
        }
        if (view.contains(QStringLiteral("dbReference"))) {
            mDbReference = view.value(QStringLiteral("dbReference")).toString(mDbReference);
        }
        addSignalAndActivate(sig);
        // 恢复视图状态（P1）
        if (DASignalAnalysisView* v = mPage ? mPage->viewForSignal(sig.get()) : nullptr) {
            v->setWaterfallMode(view.value(QStringLiteral("waterfallMode")).toInt(0));
            v->setLogFrequency(view.value(QStringLiteral("logFrequency")).toBool(false));
            const QString cmap = view.value(QStringLiteral("colormap")).toString();
            if (!cmap.isEmpty()) {
                v->setColormapName(cmap);
            }
            if (view.contains(QStringLiteral("dbFloor"))) {
                v->setDbFloor(view.value(QStringLiteral("dbFloor")).toDouble(v->dbFloor()));
            }
            v->setHeatmapAutoScroll(view.value(QStringLiteral("autoScroll")).toBool(false));
            QList< int > vs;
            for (const QJsonValue& s : view.value(QStringLiteral("splitterVertical")).toArray()) {
                vs.append(s.toInt());
            }
            QList< int > hs;
            for (const QJsonValue& s : view.value(QStringLiteral("splitterHorizontal")).toArray()) {
                hs.append(s.toInt());
            }
            v->setSplitterSizes(vs, hs);
            v->setAmplitudeDisplay(mAmplitudeMode, mDbReference);
        }
    }
    // 丝带瀑布模式勾选与当前视图同步
    if (mPage && mWaterfallModeGroup) {
        if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
            switch (v->waterfallMode()) {
            case 1:
                if (actionWaterfall3D) actionWaterfall3D->setChecked(true);
                break;
            case 2:
                if (actionWaterfallHide) actionWaterfallHide->setChecked(true);
                break;
            default:
                if (actionWaterfall2D) actionWaterfall2D->setChecked(true);
                break;
            }
        }
    }
    // 恢复后当前信号可能是「来源缺失」占位：刷新重新指定入口可用性（评审 U16）
    updateRelinkActionState();
    updatePlaybackActionsState();
}

/**
 * @brief 创建工程存档任务（isSave 时收集状态；加载时在回调里重建信号）
 */
std::shared_ptr< DAAbstractArchiveTask > DASignalAnalysisUI::createArchiveTask(bool isSave)
{
    if (isSave) {
        auto task = std::make_shared< DASignalArchiveTask >(serializeState());
        return task;
    }
    auto task = std::make_shared< DASignalArchiveTask >();
    task->setLoadedCallBack([this](std::shared_ptr< DAAbstractArchiveTask > t) {
        auto sigTask = std::static_pointer_cast< DASignalArchiveTask >(t);
        if (!sigTask) {
            return;
        }
        if (!sigTask->archiveEntryFound()) {
            // 工程包里没有 signal-analysis/signals.json（旧工程/保存时插件未启用/条目损坏）：
            // restoreState 的空数据分支会清掉上一工程残留信号与临时文件（评审 N-U2）
            qInfo() << "DASignalAnalysis: project has no signal-analysis archive entry; stale signals are cleared";
        }
        restoreState(sigTask->loadedData());
    });
    return task;
}

/**
 * @brief 清理界面资源（插件热卸载前调用）
 *
 * 顺序：菜单从动作摘掉 → 丝带标签页移除 → 中央页面停靠窗口移除 → 动作销毁。
 */
void DASignalAnalysisUI::finalize()
{
    // 保存插件偏好：峰值窗口显隐与上次色带（P2）；FFmpeg 路径等其它键原样保留。
    // 页面从未创建过的会话没有可保存的偏好（不能把其它会话的 peakDockVisible 覆盖掉）
    if (mUI && mPage) {
        QJsonObject cfg      = DASignalImporter::loadPluginConfig();
        cfg[ QStringLiteral("peakDockVisible") ] = (mPeakDock && !mPeakDock->isClosed());
        if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
            cfg[ QStringLiteral("colormap") ] = v->colormapName();
        }
        DASignalImporter::savePluginConfig(cfg);
    }
    // 菜单先与动作解绑（QMenu 不随 QAction 销毁）
    if (actionPeakTopN) {
        actionPeakTopN->setMenu(nullptr);
    }
    if (actionAmplitudeMode) {
        actionAmplitudeMode->setMenu(nullptr);
    }
    if (mPeakTopNMenu) {
        delete mPeakTopNMenu;
        mPeakTopNMenu = nullptr;
    }
    if (mAmplitudeModeMenu) {
        delete mAmplitudeModeMenu;
        mAmplitudeModeMenu = nullptr;
    }
    if (actionExportPeaks) {
        actionExportPeaks->setMenu(nullptr);
    }
    if (mExportMenu) {
        delete mExportMenu;
        mExportMenu = nullptr;
    }
    mActionExportPeaksCsv    = nullptr;
    mActionExportSpectrumCsv = nullptr;
    mActionExportPsdCsv      = nullptr;
    mActionExportHeatmapCsv  = nullptr;
    // 后台任务先停：后续删视图/页面时不再有结果回填
    if (mWorker) {
        mWorker->cancel();
        delete mWorker;
        mWorker = nullptr;
    }
    if (mPlayer) {
        mPlayer->stop();
        delete mPlayer;
        mPlayer = nullptr;
    }
    // 峰值停靠窗口：创建点见 buildPeakDock，必须在这里显式移除并销毁，否则禁用插件后右侧残留
    // 「Peaks」标签页（评审 B5，M2 验收「禁用后干净移除」）
    if (mPeakDock) {
        if (mUI) {
            if (DADockingAreaInterface* dockingArea = mUI->getDockingArea()) {
                if (dockingArea->dockManager()) {
                    dockingArea->dockManager()->removeDockWidget(mPeakDock);
                }
            }
        }
        mPeakDock->deleteLater();  // CDockWidget 拥有 panel，随之一并销毁
        mPeakDock = nullptr;
    }
    mPeakPanel = nullptr;
    // 中央页面停靠窗口（dock 拥有页面部件，统一 deleteLater，不能再 delete mPage）
    if (mPageDock) {
        if (mUI) {
            if (DADockingAreaInterface* dockingArea = mUI->getDockingArea()) {
                if (dockingArea->dockManager()) {
                    dockingArea->dockManager()->removeDockWidget(mPageDock);
                }
            }
        }
        mPageDock->deleteLater();
        mPageDock = nullptr;
    }
    mPage = nullptr;
    // 丝带标签页（mUI/mCategory 任一为空时不能提前 return，后面的动作清理必须执行）
    if (mUI && mCategory) {
        if (DARibbonAreaInterface* ribbonArea = mUI->getRibbonArea()) {
            if (ribbonArea->ribbonBar()) {
                ribbonArea->ribbonBar()->removeCategory(mCategory);
            }
        }
    }
    mCategory = nullptr;
    // 经宿主接口销毁全部动作
    if (mActions) {
        const QList< QAction* > allActions = {
            actionOpenSignalPage, actionImportAudio,    actionImportDataFrame, actionImportVideo,
            actionPlay,           actionPause,          actionStop,
            actionAnalysisSetting, actionCancelAnalysis, actionPeakMarker, actionPeakTopN,
            actionPeakWindow,     actionWaterfall2D,    actionWaterfall3D,
            actionWaterfallHide,  actionAmplitudeMode,  actionExportPeaks,
            actionExportImage,    actionPeakTable,      actionRelinkSource
        };
        for (QAction* act : allActions) {
            if (act) {
                mActions->removeAction(act);
            }
        }
    }
    // 全部置空，防止悬空访问
    actionOpenSignalPage    = nullptr;
    actionImportAudio       = nullptr;
    actionImportDataFrame   = nullptr;
    actionImportVideo       = nullptr;
    actionPlay              = nullptr;
    actionPause             = nullptr;
    actionStop              = nullptr;
    actionAnalysisSetting   = nullptr;
    actionCancelAnalysis    = nullptr;
    actionPeakMarker        = nullptr;
    actionPeakTopN          = nullptr;
    actionPeakWindow        = nullptr;
    actionPeakTop10         = nullptr;
    actionPeakTop20         = nullptr;
    actionPeakTop50         = nullptr;
    actionWaterfall2D       = nullptr;
    actionWaterfall3D       = nullptr;
    actionWaterfallHide    = nullptr;
    actionAmplitudeMode     = nullptr;
    actionAmpPeak           = nullptr;
    actionAmpRms            = nullptr;
    actionAmpDbFs           = nullptr;
    actionAmpDbRef1         = nullptr;
    actionExportPeaks       = nullptr;
    actionExportImage       = nullptr;
    actionPeakTable         = nullptr;
    actionRelinkSource      = nullptr;
    mPanelOpen              = nullptr;
    mPanelSignal            = nullptr;
    mPanelPlayback          = nullptr;
    mPanelAnalysis          = nullptr;
    mPanelPeaks             = nullptr;
    mPanelDisplay           = nullptr;
    mPanelExport            = nullptr;
    mParamsPerSignal.clear();
    mParamsVersion.clear();
    mSpectrumCache.clear();
    mSpectrumCacheChannel.clear();
    mMemoryBudgetVerdict.clear();
    // 插件卸载时清理本插件的临时音频文件（P5）
    DASignalImporter::cleanupTempFiles();
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
    // 对话框里可能改了幅值口径：同步丝带勾选与全局状态，避免峰值表/导出/存档口径分叉（评审 N-U3）
    syncAmplitudeUiFromParams(params);
    // 参数变了：大信号内存预算重新询问一次（用户可能是有意用更大段长重试，评审 N-C1c）
    mMemoryBudgetVerdict.remove(sig.get());
    recomputeCurrentSignal();
}

/**
 * @brief 参数对话框改幅值口径后同步丝带勾选与全局状态（评审 N-U3）
 *
 * 幅值口径有两份状态：全局 mAmplitudeMode/mDbReference（丝带、峰值表、工程存档用）
 * 与 params.amplitudeMode/dbReference（对话框、计算、视图用）。对话框只改后者时，
 * 存档与峰值表仍按旧口径显示 → 这里按参数反查 actionAmp* 并静默同步勾选，
 * 再用同一口径刷新峰值表与视图（不重算，只重绘）。
 */
void DASignalAnalysisUI::syncAmplitudeUiFromParams(const DASignalAnalysisParams& params)
{
    if (!params.amplitudeMode.isEmpty()) {
        mAmplitudeMode = params.amplitudeMode;
    }
    if (!params.dbReference.isEmpty()) {
        mDbReference = params.dbReference;
    }
    QAction* target = actionAmpPeak;
    if (mAmplitudeMode == QLatin1String("rms")) {
        target = actionAmpRms;
    } else if (mAmplitudeMode == QLatin1String("dbFS")) {
        target = actionAmpDbFs;
    } else if (mAmplitudeMode == QLatin1String("dbRef1")) {
        target = actionAmpDbRef1;
    }
    if (target) {
        // 用 QSignalBlocker 避免勾选变化再触发 onAmplitudeModeChanged 造成反向覆盖
        const QSignalBlocker blocker(target);
        target->setChecked(true);
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
 * @brief 后台进度 → 状态栏
 */
void DASignalAnalysisUI::onWorkerProgress(double percent, const QString& stage)
{
    if (mUI) {
        mUI->getStatusBar()->showMessage(tr("Signal analysis: %1 (%2%)").arg(stage).arg(percent, 0, 'f', 0));
        // cn:信号分析：%1（%2%）
    }
    // 进度信号是「任务在跑」的最早信号：在这里刷新取消按钮可用性（评审 N-C7）
    updateCancelActionState();
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
    const DASignal* key = ctx.signal.get();
    mSpectrumCache[ key ]        = result;
    mSpectrumCacheChannel[ key ] = ctx.channel;
    if (DASignalAnalysisView* view = mPage->viewForSignal(key)) {
        view->setSpectrum(result);
    }
    if (mUI) {
        mUI->addInfoLogMessage(tr("Spectrum ready: resolution %1 Hz, %2 bins")
                                   .arg(result.resolutionHz, 0, 'f', 4)
                                   .arg(result.freqs.size()));
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
    }
    Q_UNUSED(channel)
}

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
        actionPlay->setEnabled(canPlay);
    }
    if (actionPause) {
        actionPause->setEnabled(canPlay);
    }
    if (actionStop) {
        actionStop->setEnabled(canPlay);
    }
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
 * @brief 按导入在途状态刷新三个导入入口（I2）
 */
void DASignalAnalysisUI::updateImportActionsState()
{
    const bool enabled = !mImportTaskActive;
    if (actionImportAudio) {
        actionImportAudio->setEnabled(enabled);
    }
    if (actionImportVideo) {
        // 视频入口还要看 FFmpeg 是否可用（refreshFFmpegAvailability 已负责该条件）
        actionImportVideo->setEnabled(enabled && DASignalImporter::isFFmpegAvailable());
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
 * @brief 播放位置 → 视图播放头（播放头跟随，D27）
 */
void DASignalAnalysisUI::onPlayerPositionChanged(qint64 ms)
{
    if (!mPage) {
        return;
    }
    mSyncingFromPlayer = true;
    if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
        // 播放头回写只更新显示，不再反发 currentTimeChanged（避免 seek 循环）
        v->setPlayheadFromPlayback(ms / 1000.0);
        if (v->isFollowSlider()) {
            updatePeaksForCurrentFrame();
        }
    }
    mSyncingFromPlayer = false;
}

/**
 * @brief 用户拖动滑块 / 点击波形：暂停播放并跳转（方案 §8.4）
 */
void DASignalAnalysisUI::onViewTimeChanged(double seconds)
{
    if (mSyncingFromPlayer || !mPlayer) {
        return;
    }
    if (mPlayer->isPlaying()) {
        mPlayer->pause();
    }
    mPlayer->seekMs(qint64(seconds * 1000.0));
    if (mPage && mPage->currentSignal()) {
        if (DASignalAnalysisView* view = mPage->viewForSignal(mPage->currentSignal().get())) {
            if (view->isFollowSlider()) {
                updatePeaksForCurrentFrame();
            }
        }
    }
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
    in.sampleCount        = view->signal()->sampleCount();
    in.sampleRate         = view->signal()->sampleRate();
    in.stftWindowMs       = params.stftWindowMs;
    in.stftOverlapPercent = params.stftOverlap;
    in.gridColumns        = columns;
    in.gridRows           = rows;
    qint64 startSample    = -1;
    qint64 endSample      = -1;
    int blockFrames       = 256;
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
    mWorker->computeHeatmap(makeTaskContext(view->signal(), view->currentChannel()),
                            params,
                            columns,
                            rows,
                            startSample,
                            endSample,
                            blockFrames);
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
    mCurrentPeaks         = result;
    mCurrentPeaksSignal   = ctx.signal.get();
    mCurrentPeaksChannel  = ctx.channel;
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
    const double ref = mags.first();
    double powerSum  = 0.0;
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
 * @brief 峰值标记显隐（丝带快捷键开关）
 */
void DASignalAnalysisUI::onPeakMarkerToggled(bool on)
{
    if (mPage) {
        if (DASignalAnalysisView* view = mPage->viewForSignal(mPage->currentSignal().get())) {
            view->setPeakMarkersVisible(on);
        }
    }
    if (on && mCurrentPeaks.frequencies.isEmpty()) {
        requestPeaks(false);
    }
}

/**
 * @brief 前 N 改变（10/20/50）：更新参数并重算峰值表与曲线
 */
void DASignalAnalysisUI::onPeakTopNChanged()
{
    int topN = 10;
    if (actionPeakTop20 && actionPeakTop20->isChecked()) {
        topN = 20;
    } else if (actionPeakTop50 && actionPeakTop50->isChecked()) {
        topN = 50;
    }
    if (mPage) {
        DASignalPtr sig = mPage->currentSignal();
        if (!sig) {
            return;
        }
        DASignalAnalysisParams params = paramsForSignal(sig);
        params.peakTopN               = topN;
        mParamsPerSignal[ sig.get() ] = params;
        bumpParamsVersion(sig);
    }
    mPeakCurvesRequested = false;  // 名次数改变后需要重扫曲线
    clearPeakState();
    requestPeaks(true);
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

/**
 * @brief 幅值显示模式切换：更新当前信号参数并只重绘（D14）
 */
void DASignalAnalysisUI::onAmplitudeModeChanged()
{
    if (actionAmpPeak && actionAmpPeak->isChecked()) {
        mAmplitudeMode = QStringLiteral("peak");
        mDbReference   = QStringLiteral("fullScale");
    } else if (actionAmpRms && actionAmpRms->isChecked()) {
        mAmplitudeMode = QStringLiteral("rms");
    } else if (actionAmpDbFs && actionAmpDbFs->isChecked()) {
        mAmplitudeMode = QStringLiteral("dbFS");
        mDbReference   = QStringLiteral("fullScale");
    } else if (actionAmpDbRef1 && actionAmpDbRef1->isChecked()) {
        mAmplitudeMode = QStringLiteral("dbRef1");
        mDbReference   = QStringLiteral("one");
    }
    if (!mPage) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig) {
        return;
    }
    DASignalAnalysisParams params = paramsForSignal(sig);
    params.amplitudeMode          = mAmplitudeMode;
    params.dbReference            = mDbReference;
    mParamsPerSignal[ sig.get() ] = params;
    if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
        view->setAmplitudeDisplay(mAmplitudeMode, mDbReference);
    }
    // 峰值表/曲线的显示口径也要跟着切换（只重绘，不重算；评审 §七-3）
    if (mPeakPanel) {
        if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
            mPeakPanel->setSignalPeak(view->signalPeak() * params.calibration);
        }
        mPeakPanel->setAmplitudeDisplay(mAmplitudeMode, mDbReference, params.calibration, params.unit);
        mPeakPanel->setPeaks(mCurrentPeaks);
        mPeakPanel->setPeakCurves(mCurrentCurves);
    }
}

/**
 * @brief 设置依赖信号的动作是否可用（无信号时播放/分析/峰值/显示/导出全部置灰）
 * @param on 是否可用
 */
void DASignalAnalysisUI::setSignalDependentActionsEnabled(bool on)
{
    const QList< QAction* > acts = {
        actionPlay,          actionPause,          actionStop,
        actionAnalysisSetting, actionPeakMarker,   actionPeakTopN,
        actionPeakWindow,    actionWaterfall2D,    actionWaterfall3D,
        actionWaterfallHide, actionAmplitudeMode,  actionExportPeaks,
        actionExportImage,   actionPeakTable,      actionRelinkSource
    };
    for (QAction* act : acts) {
        if (act) {
            act->setEnabled(on);
        }
    }
    // 「重新指定数据帧来源」只对来源缺失的当前信号可用（评审 U16）
    updateRelinkActionState();
    if (actionPeakTopN) {
        actionPeakTopN->setEnabled(on);
    }
    if (actionAmplitudeMode) {
        actionAmplitudeMode->setEnabled(on);
    }
}

/**
 * @brief 重译界面文本：标签页名、面板名、全部动作文本
 */
void DASignalAnalysisUI::retranslateUi()
{
    // 标签页
    if (mCategory) {
        mCategory->setCategoryName(tr("Signal Analysis"));  // cn:信号分析
    }
    if (mPageDock) {
        mPageDock->setWindowTitle(tr("Signal Analysis"));  // cn:信号分析
    }
    // 峰值停靠窗口标题必须自己重译：宿主 resetText() 只处理宿主自己的 dock（评审 §七-5）
    if (mPeakDock) {
        mPeakDock->setWindowTitle(tr("Peaks"));  // cn:峰值
    }
    // 面板
    if (mPanelOpen) {
        mPanelOpen->setPanelName(tr("Window"));  // cn:窗口
    }
    if (mPanelSignal) {
        mPanelSignal->setPanelName(tr("Signal"));  // cn:信号
    }
    if (mPanelPlayback) {
        mPanelPlayback->setPanelName(tr("Playback"));  // cn:播放
    }
    if (mPanelAnalysis) {
        mPanelAnalysis->setPanelName(tr("Analysis"));  // cn:分析
    }
    if (mPanelPeaks) {
        mPanelPeaks->setPanelName(tr("Peaks"));  // cn:峰值
    }
    if (mPanelDisplay) {
        mPanelDisplay->setPanelName(tr("Display"));  // cn:显示
    }
    if (mPanelExport) {
        mPanelExport->setPanelName(tr("Export"));  // cn:导出
    }
    // 窗口
    if (actionOpenSignalPage) {
        actionOpenSignalPage->setText(tr("Open Signal Analysis Window"));  // cn:打开信号分析窗口
        actionOpenSignalPage->setToolTip(tr("Show the signal analysis workspace; it is created on first use"));
        // cn:显示信号分析界面（首次使用时创建）
    }
    // 信号
    if (actionImportAudio) {
        actionImportAudio->setText(tr("Import Audio"));  // cn:导入音频
        actionImportAudio->setToolTip(tr("Import an audio file (WAV/AIFF/FLAC/OGG)"));  // cn:导入音频文件
    }
    if (actionImportDataFrame) {
        actionImportDataFrame->setText(tr("Import Data Frame"));  // cn:导入数据帧
        actionImportDataFrame->setToolTip(tr("Import columns from an existing data frame"));  // cn:从已有数据帧指定列导入
    }
    if (actionImportVideo) {
        actionImportVideo->setText(tr("Import Video"));  // cn:导入视频
        refreshFFmpegAvailability();  // tooltip 按 FFmpeg 是否可用给出不同说明
    }
    if (actionRelinkSource) {
        actionRelinkSource->setText(tr("Relink Data Frame Source"));  // cn:重新指定数据帧来源
        actionRelinkSource->setToolTip(tr("Point a signal whose data frame is missing to an existing data frame"));
        // cn:把数据帧缺失的信号重新指向现有数据帧
    }
    // 播放
    if (actionPlay) {
        actionPlay->setText(tr("Play"));  // cn:播放
    }
    if (actionPause) {
        actionPause->setText(tr("Pause"));  // cn:暂停
    }
    if (actionStop) {
        actionStop->setText(tr("Stop"));  // cn:停止
    }
    // 分析
    if (actionAnalysisSetting) {
        actionAnalysisSetting->setText(tr("Analysis Settings"));  // cn:分析设置
        actionAnalysisSetting->setToolTip(tr("Configure the analysis parameters of the current signal"));
        // cn:配置当前信号的分析参数
    }
    if (actionCancelAnalysis) {
        actionCancelAnalysis->setText(tr("Cancel Analysis"));  // cn:取消计算
        actionCancelAnalysis->setToolTip(tr("Cancel all running background computations"));  // cn:取消全部在跑的后台计算
    }
    // 峰值
    if (actionPeakMarker) {
        actionPeakMarker->setText(tr("Peak Markers"));  // cn:峰值标记
    }
    if (actionPeakTopN) {
        actionPeakTopN->setText(tr("Top N"));  // cn:前 N 个
        actionPeakTopN->setToolTip(tr("Number of peaks to keep (10/20/50)"));  // cn:保留的峰值数量
    }
    if (actionPeakWindow) {
        actionPeakWindow->setText(tr("Peak Window"));  // cn:峰值窗口
        actionPeakWindow->setToolTip(tr("Show or hide the peak dock window"));  // cn:显示或隐藏峰值停靠窗口
    }
    // 显示
    if (actionWaterfall2D) {
        actionWaterfall2D->setText(tr("2D"));  // cn:二维
        actionWaterfall2D->setToolTip(tr("Show the 2D time-frequency heatmap"));  // cn:显示二维时频热图
    }
    if (actionWaterfall3D) {
        actionWaterfall3D->setText(tr("3D"));  // cn:三维
        actionWaterfall3D->setToolTip(tr("Show the 3D surface waterfall"));  // cn:显示三维曲面瀑布
    }
    if (actionWaterfallHide) {
        actionWaterfallHide->setText(tr("Hide"));  // cn:隐藏
        actionWaterfallHide->setToolTip(tr("Hide the waterfall view"));  // cn:隐藏瀑布视图
    }
    if (actionAmplitudeMode) {
        actionAmplitudeMode->setText(tr("Amplitude"));  // cn:幅值模式
    }
    if (actionAmpPeak) {
        actionAmpPeak->setText(tr("Peak amplitude"));  // cn:峰值幅度
    }
    if (actionAmpRms) {
        actionAmpRms->setText(tr("RMS"));  // cn:均方根
    }
    if (actionAmpDbFs) {
        actionAmpDbFs->setText(tr("dB (FS)"));  // cn:dB（满量程）
    }
    if (actionAmpDbRef1) {
        actionAmpDbRef1->setText(tr("dB (re 1.0)"));  // cn:dB（相对 1.0）
    }
    // 导出
    if (actionExportPeaks) {
        actionExportPeaks->setText(tr("Export Peaks"));  // cn:导出峰值表
    }
    // 导出菜单项文本也随语言切换（评审 §七-5）
    if (mActionExportPeaksCsv) {
        mActionExportPeaksCsv->setText(tr("Peak table (CSV)"));  // cn:峰值表（CSV）
    }
    if (mActionExportSpectrumCsv) {
        mActionExportSpectrumCsv->setText(tr("Spectrum curve (CSV)"));  // cn:频谱曲线（CSV）
    }
    if (mActionExportPsdCsv) {
        mActionExportPsdCsv->setText(tr("Power spectral density (CSV)"));  // cn:功率谱密度（CSV）
    }
    if (mActionExportHeatmapCsv) {
        mActionExportHeatmapCsv->setText(tr("Heatmap matrix (CSV)"));  // cn:热图矩阵（CSV）
    }
    if (actionExportImage) {
        actionExportImage->setText(tr("Export Image"));  // cn:导出图片
    }
    if (actionPeakTable) {
        actionPeakTable->setText(tr("Peak Table"));  // cn:峰值表
        actionPeakTable->setToolTip(tr("Show or hide the peak table window"));  // cn:显示或隐藏峰值表窗口
    }
    // 页面
    if (mPage) {
        mPage->retranslateUi();
    }
}
}  // namespace DA
