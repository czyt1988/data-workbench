#include "DASignalAnalysisUI.h"
// Qt
#include <QMenu>
#include <QActionGroup>
#include <cmath>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
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
#include "Analysis/DASignalAnalysisWorker.h"
#include "Playback/DASignalPlayer.h"
#include "Archive/DASignalArchiveTask.h"

namespace DA
{
namespace
{
/// 丝带标签页 / 面板 / 动作的 object name（唯一名，供样式与查找使用）
const char* const cCategoryObjectName      = "da-category-signal-analysis";
const char* const cPanelSignalName         = "da-panel-signal.signal";
const char* const cPanelPlaybackName       = "da-panel-signal.playback";
const char* const cPanelAnalysisName       = "da-panel-signal.analysis";
const char* const cPanelPeaksName          = "da-panel-signal.peaks";
const char* const cPanelDisplayName        = "da-panel-signal.display";
const char* const cPanelExportName         = "da-panel-signal.export";
const char* const cPageDockObjectName      = "da-dock-signal-analysis-page";

// 图标资源前缀
const char* const cIconPrefix = ":/DASignalAnalysis/icon/";
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

    buildCenterPage();
    buildSignalCategory();
    retranslateUi();
    // 无信号阶段：依赖信号的动作全部置灰（方案 §8.1）
    setSignalDependentActionsEnabled(false);

    // 导入入口：丝带按钮与空状态按钮共用同一槽
    if (mPage) {
        connect(mPage, &DASignalAnalysisPage::requestImportAudio, this, &DASignalAnalysisUI::importAudioFile);
        connect(mPage, &DASignalAnalysisPage::requestImportDataFrame, this, &DASignalAnalysisUI::importDataFrame);
        connect(mPage, &DASignalAnalysisPage::requestImportVideo, this, &DASignalAnalysisUI::importVideoFile);
        connect(mPage, &DASignalAnalysisPage::signalRemoved, this, &DASignalAnalysisUI::onSignalRemoved);
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
    if (actionAnalysisSetting) {
        connect(actionAnalysisSetting, &QAction::triggered, this, &DASignalAnalysisUI::openAnalysisSettings);
    }
    // 导出菜单：峰值表 CSV / 频谱 CSV / 热图矩阵 CSV
    if (actionExportPeaks) {
        mExportMenu = new QMenu();
        mExportMenu->addAction(tr("Peak table (CSV)"), this, &DASignalAnalysisUI::exportPeaksCsv);
        // cn:峰值表（CSV）
        mExportMenu->addAction(tr("Spectrum curve (CSV)"), this, &DASignalAnalysisUI::exportSpectrumCsv);
        // cn:频谱曲线（CSV）
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
        reportError(tr("Playback failed: %1").arg(err));  // cn:播放失败：%1
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
    if (mPage) {
        connect(mPage, &DASignalAnalysisPage::currentSignalChanged, this, &DASignalAnalysisUI::syncPlayerSource);
    }
    // 峰值停靠窗口（右侧停靠区，与设置/智能体同区做标签页，D24）
    buildPeakDock();
    // 恢复插件偏好：停靠窗口显隐与上次色带（P2）
    {
        const QJsonObject cfg = DASignalImporter::loadPluginConfig();
        mLastColormap         = cfg.value(QStringLiteral("colormap")).toString();
        if (mPeakDock && cfg.value(QStringLiteral("peakDockVisible")).toBool(false)) {
            if (actionPeakWindow) {
                actionPeakWindow->setChecked(true);
            }
            mPeakDock->toggleView(true);
        }
    }
    // 后台计算协调器（Python 守护线程 + 定时轮询）
    mWorker = new DASignalAnalysisWorker(this);
    connect(mWorker, &DASignalAnalysisWorker::progressChanged, this, &DASignalAnalysisUI::onWorkerProgress);
    connect(mWorker, &DASignalAnalysisWorker::spectrumReady, this, &DASignalAnalysisUI::onWorkerSpectrumReady);
    connect(mWorker, &DASignalAnalysisWorker::envelopeReady, this, &DASignalAnalysisUI::onWorkerEnvelopeReady);
    connect(mWorker, &DASignalAnalysisWorker::heatmapReady, this, &DASignalAnalysisUI::onWorkerHeatmapReady);
    connect(mWorker, &DASignalAnalysisWorker::peaksReady, this, &DASignalAnalysisUI::onWorkerPeaksReady);
    connect(mWorker, &DASignalAnalysisWorker::peakCurvesReady, this, &DASignalAnalysisUI::onWorkerPeakCurvesReady);
    if (actionPeakWindow) {
        connect(actionPeakWindow, &QAction::toggled, this, &DASignalAnalysisUI::onPeakWindowToggled);
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
        mUI->addInfoLogMessage(tr("FFmpeg was not detected; video import is unavailable until a path is specified."));
        // cn:未检测到 FFmpeg，视频导入在指定路径前不可用。
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
 */
DASignalAnalysisPage* DASignalAnalysisUI::page() const
{
    return mPage;
}

// ---------------------------------------------------------------- 导入

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
 * 未检测到时视频导入置灰并给出「在分析设置里指定路径」的悬浮提示；
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
                      : tr("FFmpeg was not detected. Specify its path in the signal analysis settings."));
        // cn:未检测到 FFmpeg，请在分析设置里指定路径。
    }
}

/**
 * @brief 导入音频文件（WAV/AIFF/FLAC/OGG 直读；MP3/M4A/AAC 经 FFmpeg）
 */
void DASignalAnalysisUI::importAudioFile()
{
    DADataManagerInterface* dataMgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    DASignalImportDialog dlg(DASignalImportDialog::AudioMode, dataMgr, mPage);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    QString error;
    DASignalPtr sig = DASignalImporter::importAudioFile(dlg.filePath(), &error);
    if (!sig) {
        reportError(tr("Failed to import audio: %1").arg(error));  // cn:导入音频失败：%1
        return;
    }
    addSignalAndActivate(sig);
}

/**
 * @brief 从数据帧导入（时间列 + 幅值列 + 采样率）
 */
void DASignalAnalysisUI::importDataFrame()
{
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
        reportError(tr("Failed to import data frame: %1").arg(error));  // cn:导入数据帧失败：%1
        return;
    }
    addSignalAndActivate(sig);
}

/**
 * @brief 导入视频（FFmpeg 抽取音频轨到临时 WAV）
 */
void DASignalAnalysisUI::importVideoFile()
{
    DADataManagerInterface* dataMgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    DASignalImportDialog dlg(DASignalImportDialog::VideoMode, dataMgr, mPage);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    QString error;
    DASignalPtr sig = DASignalImporter::importVideoFile(dlg.filePath(), &error);
    if (!sig) {
        reportError(tr("Failed to import video: %1").arg(error));  // cn:导入视频失败：%1
        return;
    }
    addSignalAndActivate(sig);
}

/**
 * @brief 导入成功：加入页面并激活依赖信号的动作
 */
void DASignalAnalysisUI::addSignalAndActivate(const DASignalPtr& sig)
{
    if (nullptr == mPage || !sig) {
        return;
    }
    mPage->addSignal(sig);
    setSignalDependentActionsEnabled(true);
    syncPlayerSource();
    if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
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
        // 「跟随滑块」只刷新峰值表显示，不触发 seek
        connect(view, &DASignalAnalysisView::followSliderToggled, this, [this](bool) {
            updatePeaksForCurrentFrame();
        });
        // 连接就绪后再发起首次计算（视图显示早于 connect，showEvent 里的请求会丢）
        view->requestInitialAnalysis();
    }
    // 切到「信号分析」页面标签（导入后用户应立刻看到信号视图，而不是停在原页面）
    if (auto* dockingArea = mUI->getDockingArea()) {
        dockingArea->raiseDockByWidget(mPage);
    }
    // 首次导入信号成功后自动切到峰值窗口一次（D24），之后尊重用户开关
    if (!mFirstSignalActivated) {
        mFirstSignalActivated = true;
        if (mPeakDock) {
            mPeakDock->toggleView(true);
            mPeakDock->setAsCurrentTab();
        }
    }
    mUI->addInfoLogMessage(tr("Signal imported: %1 (%2 Hz, %3 s)")
                               .arg(sig->name())
                               .arg(sig->sampleRate(), 0, 'f', 3)
                               .arg(sig->duration(), 0, 'f', 3));
    // cn:信号已导入：%1（%2 赫兹，%3 秒）
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
    mLargeSignalWarned.remove(key);
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
}

/**
 * @brief 构建中央页面（与绘图/表格/工作流并列的标签页）
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
    } else {
        qWarning() << "DASignalAnalysisUI::buildCenterPage: failed to create center dock for signal page";
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
    mPanelSignal->addLargeAction(actionImportAudio);
    mPanelSignal->addLargeAction(actionImportDataFrame);
    mPanelSignal->addLargeAction(actionImportVideo);

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
    mPanelAnalysis->addLargeAction(actionAnalysisSetting);

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
    mPeakPanel = new DAPeakDockWidget();
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
            if (visible && !mPeakCurvesRequested) {
                requestPeaks(true);
            }
        });
        // 创建即激活会让插件加载后右侧直接跳到空的峰值页（评审 D24/U14）：先隐藏，
        // 首次导入成功后再自动切一次（见 addSignalAndActivate）
        mPeakDock->toggleView(false);
    }
    connect(mPeakPanel, &DAPeakDockWidget::sendToTableRequested, this, &DASignalAnalysisUI::sendPeaksToTable);
    connect(mPeakPanel, &DAPeakDockWidget::exportCsvRequested, this, &DASignalAnalysisUI::exportPeaksCsv);
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
    const DASignalAnalysisParams params = paramsForSignal(sig);
    DASignalAnalysisView* view         = mPage->viewForSignal(sig.get());
    const int channel                  = view ? view->currentChannel() : 0;
    mWorker->computePeaks(makeTaskContext(sig, channel), params);
    if (withCurves && !mPeakCurvesRequested) {
        mPeakCurvesRequested = true;
        mWorker->computePeakCurves(makeTaskContext(sig, channel), params);
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
    if (mPeakPanel) {
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
    mLargeSignalWarned.clear();
    clearPeakState();
    setSignalDependentActionsEnabled(false);
    mFirstSignalActivated = false;
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
        const double estimatedMb = double(cells) * 8.0 / (1024.0 * 1024.0);
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
    const QPixmap pixmap = view->grab();
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
        qWarning() << "sendPeaksToTable failed:" << e.what();
        reportError(tr("Failed to send the peak table: %1").arg(QString::fromUtf8(e.what())));
        // cn:峰值表发送失败：%1
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
                o[ QStringLiteral("dataframeName") ] = dfSrc ? dfSrc->data().getName() : QString();
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
        return;
    }
    // 打开新工程/快照回滚前先清空旧信号与缓存，避免两个工程的信号叠加（评审 U15/P4）
    clearAllSignals();
    const QJsonArray arr        = doc.object().value(QStringLiteral("signals")).toArray();
    DADataManagerInterface* mgr = mCore ? mCore->getDataManagerInterface() : nullptr;
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        const QString type  = o.value(QStringLiteral("sourceType")).toString();
        const QString name  = o.value(QStringLiteral("name")).toString();
        DASignalPtr sig;
        QString error;
        if (type == QLatin1String("dataframe")) {
            const QString dfName = o.value(QStringLiteral("dataframeName")).toString();
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
                reportError(tr("Signal source missing: dataframe \"%1\".").arg(dfName));
                // cn:信号源缺失：数据帧「%1」。
                continue;
            }
            QStringList cols;
            const QJsonArray colsArr = o.value(QStringLiteral("valueColumns")).toArray();
            for (const QJsonValue& c : colsArr) {
                cols.append(c.toString());
            }
            sig = DASignalImporter::importDataFrame(data,
                                                    o.value(QStringLiteral("timeColumn")).toString(),
                                                    cols,
                                                    o.value(QStringLiteral("sampleRate")).toDouble(),
                                                    name,
                                                    &error);
        } else {
            const QString path = o.value(QStringLiteral("path")).toString();
            if (!QFileInfo::exists(path)) {
                reportError(tr("Signal source missing: %1").arg(path));
                // cn:信号源缺失：%1
                continue;
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
            reportError(tr("Failed to restore signal \"%1\": %2").arg(name, error));
            // cn:恢复信号「%1」失败：%2
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
        if (sigTask) {
            restoreState(sigTask->loadedData());
        }
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
    // 保存插件偏好：峰值窗口显隐与上次色带（P2）；FFmpeg 路径等其它键原样保留
    if (mUI) {
        QJsonObject cfg      = DASignalImporter::loadPluginConfig();
        cfg[ QStringLiteral("peakDockVisible") ] = (mPeakDock && !mPeakDock->isClosed());
        if (mPage) {
            if (DASignalAnalysisView* v = mPage->viewForSignal(mPage->currentSignal().get())) {
                cfg[ QStringLiteral("colormap") ] = v->colormapName();
            }
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
            actionImportAudio,    actionImportDataFrame, actionImportVideo,
            actionPlay,           actionPause,           actionStop,
            actionAnalysisSetting, actionPeakMarker,     actionPeakTopN,
            actionPeakWindow,     actionWaterfall2D,     actionWaterfall3D,
            actionWaterfallHide,  actionAmplitudeMode,   actionExportPeaks,
            actionExportImage
        };
        for (QAction* act : allActions) {
            if (act) {
                mActions->removeAction(act);
            }
        }
    }
    // 全部置空，防止悬空访问
    actionImportAudio       = nullptr;
    actionImportDataFrame   = nullptr;
    actionImportVideo       = nullptr;
    actionPlay              = nullptr;
    actionPause             = nullptr;
    actionStop              = nullptr;
    actionAnalysisSetting   = nullptr;
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
    mLargeSignalWarned.clear();
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
    return mParamsPerSignal.value(sig.get(), DASignalAnalysisParams::defaults());
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
    refreshFFmpegAvailability();  // 对话框里可能改了 FFmpeg 路径（D7）
    recomputeCurrentSignal();
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
}

/**
 * @brief 频谱计算完成：按任务上下文校验后缓存并回填到对应信号视图（评审 U3）
 */
void DASignalAnalysisUI::onWorkerSpectrumReady(const DASignalTaskContext& ctx, const DASpectrumResult& result)
{
    if (!result.isValid()) {
        return;
    }
    if (!isContextCurrent(ctx, true)) {
        return;  // 信号已关闭 / 已切通道 / 参数已过期：丢弃迟到结果
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
}

/**
 * @brief 后台任务失败
 */
void DASignalAnalysisUI::onWorkerFailed(const QString& error)
{
    if (mUI) {
        reportError(tr("Signal analysis failed: %1").arg(error));  // cn:信号分析失败：%1
    }
    clearProgressIfIdle();
}

/**
 * @brief 视图请求计算频谱
 */
void DASignalAnalysisUI::onViewRequestSpectrum(const DASignalPtr& signal, int channel)
{
    if (!mWorker || !signal) {
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
 * @brief 当前信号的播放源切换（数据帧来源会先导出临时 WAV）
 */
void DASignalAnalysisUI::syncPlayerSource()
{
    if (!mPlayer || !mPage) {
        return;
    }
    DASignalPtr sig = mPage->currentSignal();
    if (!sig) {
        mPlayer->clearSignal();
        return;
    }
    QString error;
    // 播放当前查看的通道（评审 U9）；数据帧来源的临时 WAV 由播放器按「信号+通道」缓存
    if (!mPlayer->setSignal(sig, channelOfSignal(sig), &error)) {
        if (actionPlay) {
            actionPlay->setEnabled(false);
        }
        if (!error.isEmpty()) {
            reportError(error);
        }
        return;
    }
    if (actionPlay) {
        actionPlay->setEnabled(true);
    }
    if (mPeakPanel) {
        mPeakPanel->setTimeOrigin(sig->timeOrigin());  // 峰值曲线时间轴与波形/热图对齐（U12）
    }
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
    // 内存预算（方案 §11）：float32 全量超过 1.5 吉字节时提示一次，热图按显示精度降级
    constexpr qint64 cLargeSignalBytes = qint64(1.5 * 1024 * 1024 * 1024);
    if (view->signal()->sampleCount() * qint64(sizeof(float)) > cLargeSignalBytes) {
        const DA::DASignal* key = view->signal().get();
        if (!mLargeSignalWarned.contains(key)) {
            mLargeSignalWarned.insert(key);
            reportError(
                tr("Signal is very large (> 1.5 GB as float32); the heatmap uses display-precision data.")
                // cn:信号非常大（按 float32 计超过 1.5 吉字节），热图按显示精度降级计算。
            );
        }
    }
    mWorker->computeHeatmap(makeTaskContext(view->signal(), view->currentChannel()), view->params(), columns, rows);
}

/**
 * @brief 时频栅格结果回填到发起请求的信号视图（按上下文校验）
 */
void DASignalAnalysisUI::onWorkerHeatmapReady(const DASignalTaskContext& ctx, const DAStftResult& result)
{
    if (!isContextCurrent(ctx, true)) {
        return;
    }
    if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
        view->setStft(result);
    }
    clearProgressIfIdle();
}

/**
 * @brief 峰值表结果回填（表模型 + 频谱图标记）；只接受当前信号/通道的结果
 */
void DASignalAnalysisUI::onWorkerPeaksReady(const DASignalTaskContext& ctx, const DAPeakResult& result)
{
    if (!isContextCurrent(ctx, false)) {
        return;
    }
    DASignalPtr current = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!current || current.get() != ctx.signal.get()) {
        return;  // 峰值表/标记只属于当前信号
    }
    if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
        if (view->currentChannel() != ctx.channel) {
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
}

/**
 * @brief 每帧前 N 峰值曲线结果回填（只接受当前信号/通道）
 */
void DASignalAnalysisUI::onWorkerPeakCurvesReady(const DASignalTaskContext& ctx, const DAPeakCurveResult& result)
{
    if (!isContextCurrent(ctx, false)) {
        return;
    }
    DASignalPtr current = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!current || current.get() != ctx.signal.get()) {
        return;
    }
    mCurrentCurves = result;
    if (mPeakPanel) {
        mPeakPanel->setPeakCurves(result);
    }
    clearProgressIfIdle();
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
        return;
    }
    if (DASignalAnalysisView* view = mPage->viewForSignal(ctx.signal.get())) {
        view->setEnvelope(xs, mins, maxs);
    }
    clearProgressIfIdle();
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
        actionExportImage
    };
    for (QAction* act : acts) {
        if (act) {
            act->setEnabled(on);
        }
    }
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
    // 面板
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
        actionAnalysisSetting->setToolTip(tr("Open the unified analysis parameter dialog"));  // cn:打开统一参数对话框
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
    if (actionExportImage) {
        actionExportImage->setText(tr("Export Image"));  // cn:导出图片
    }
    // 页面
    if (mPage) {
        mPage->retranslateUi();
    }
}
}  // namespace DA
