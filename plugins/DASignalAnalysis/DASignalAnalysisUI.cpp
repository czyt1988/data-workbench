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
        mUI->addWarningLogMessage(tr("Playback failed: %1").arg(err));  // cn:播放失败：%1
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
        mUI->addWarningLogMessage(tr("Failed to import audio: %1").arg(error));  // cn:导入音频失败：%1
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
        mUI->addWarningLogMessage(tr("Failed to import data frame: %1").arg(error));  // cn:导入数据帧失败：%1
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
        mUI->addWarningLogMessage(tr("Failed to import video: %1").arg(error));  // cn:导入视频失败：%1
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
        connect(view, &DASignalAnalysisView::spectrumRequested, this, &DASignalAnalysisUI::onViewRequestSpectrum);
        connect(view, &DASignalAnalysisView::envelopeRequested, this, &DASignalAnalysisUI::onViewRequestEnvelope);
        connect(view, &DASignalAnalysisView::heatmapRequested, this, &DASignalAnalysisUI::onViewRequestHeatmap);
        connect(view, &DASignalAnalysisView::currentTimeChanged, this, &DASignalAnalysisUI::onViewTimeChanged);
        connect(view, &DASignalAnalysisView::channelChanged, this, [this](int) {
            // 通道变了：峰值与曲线需要按新通道重算
            mPeakCurvesRequested = false;
            mCurrentCurves       = DAPeakCurveResult();
            requestPeaks(true);
        });
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
        // 首次显示峰值窗口时按需计算每帧前 N 峰值曲线
        connect(mPeakDock, &ads::CDockWidget::visibilityChanged, this, [this](bool visible) {
            if (visible && !mPeakCurvesRequested) {
                requestPeaks(true);
            }
        });
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
    const int channel = (mPage && mPage->viewForSignal(sig.get())) ? mPage->viewForSignal(sig.get())->currentChannel() : 0;
    mWorker->computePeaks(sig, channel, params);
    if (withCurves && !mPeakCurvesRequested) {
        mPeakCurvesRequested = true;
        mWorker->computePeakCurves(sig, channel, params);
    }
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
}  // namespace

/**
 * @brief 导出峰值表 CSV
 */
void DASignalAnalysisUI::exportPeaksCsv()
{
    if (!mCurrentPeaks.isValid()) {
        mUI->addWarningLogMessage(tr("No peak data to export."));  // cn:没有可导出的峰值数据。
        return;
    }
    const QString path =
        askSavePath(mPage, tr("Export Peak Table"), tr("CSV Files (*.csv)"));  // cn:导出峰值表
    if (path.isEmpty()) {
        return;
    }
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(mCurrentPeaks.frequencies),
                                                doubleTuple(mCurrentPeaks.magnitudes),
                                                doubleTuple(mCurrentPeaks.relDb),
                                                doubleTuple(mCurrentPeaks.share));
    if (callPythonExport("write_peaks_csv", args)) {
        mUI->addInfoLogMessage(tr("Peak table exported: %1").arg(path));  // cn:峰值表已导出：%1
    } else {
        mUI->addWarningLogMessage(tr("Failed to export the peak table."));  // cn:峰值表导出失败。
    }
}

/**
 * @brief 导出频谱曲线 CSV（两列：频率、当前幅值模式下的幅值）
 */
void DASignalAnalysisUI::exportSpectrumCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig) {
        mUI->addWarningLogMessage(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    const DASpectrumResult spec = spectrumForSignal(sig);
    if (!spec.isValid()) {
        mUI->addWarningLogMessage(tr("No spectrum data to export."));  // cn:没有可导出的频谱数据。
        return;
    }
    const QString path =
        askSavePath(mPage, tr("Export Spectrum"), tr("CSV Files (*.csv)"));  // cn:导出频谱
    if (path.isEmpty()) {
        return;
    }
    // 按当前幅值模式换算（与显示口径一致）
    const DASignalAnalysisParams params = paramsForSignal(sig);
    QVector< double > values;
    values.reserve(spec.amplitudes.size());
    const bool isDb = (params.amplitudeMode == QLatin1String("dbFS")
                       || params.amplitudeMode == QLatin1String("dbRef1"));
    double peak = 0.0;
    for (double a : spec.amplitudes) {
        peak = qMax(peak, std::fabs(a));
    }
    for (double a : spec.amplitudes) {
        if (isDb) {
            const double ref = (params.dbReference == QLatin1String("one")) ? 1.0 : qMax(peak, 1e-30);
            values.append(20.0 * std::log10(qMax(std::fabs(a * params.calibration), 1e-30) / ref));
        } else {
            values.append(a * params.calibration);
        }
    }
    QString yLabel = (params.amplitudeMode == QLatin1String("rms")) ? QStringLiteral("RMS") : QStringLiteral("Amplitude");
    if (isDb) {
        yLabel = QStringLiteral("dB");
    }
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(spec.freqs),
                                                doubleTuple(values),
                                                yLabel.toStdString());
    if (callPythonExport("write_spectrum_csv", args)) {
        mUI->addInfoLogMessage(tr("Spectrum exported: %1").arg(path));  // cn:频谱已导出：%1
    } else {
        mUI->addWarningLogMessage(tr("Failed to export the spectrum."));  // cn:频谱导出失败。
    }
}

/**
 * @brief 导出热图矩阵 CSV（行 = 频率、列 = 时间；导出前给体积提示）
 */
void DASignalAnalysisUI::exportHeatmapCsv()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig || !mPage) {
        mUI->addWarningLogMessage(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
    if (!view) {
        return;
    }
    const DAStftResult stft = view->stftResult();
    if (!stft.isValid()) {
        mUI->addWarningLogMessage(tr("No time-frequency data to export."));  // cn:没有可导出的时频数据。
        return;
    }
    const qint64 cells = qint64(stft.rows) * stft.columns;
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
    pybind11::tuple args = pybind11::make_tuple(path.toStdString(),
                                                doubleTuple(stft.times),
                                                doubleTuple(stft.freqs),
                                                doubleTuple(stft.values),
                                                stft.rows,
                                                stft.columns);
    if (callPythonExport("write_heatmap_csv", args)) {
        mUI->addInfoLogMessage(tr("Heatmap matrix exported: %1").arg(path));  // cn:热图矩阵已导出：%1
    } else {
        mUI->addWarningLogMessage(tr("Failed to export the heatmap matrix."));  // cn:热图矩阵导出失败。
    }
}

/**
 * @brief 导出当前视图为位图（三维 GL 内容不保证被抓取，见 T7）
 */
void DASignalAnalysisUI::exportImage()
{
    DASignalPtr sig = mPage ? mPage->currentSignal() : DASignalPtr();
    if (!sig || !mPage) {
        mUI->addWarningLogMessage(tr("No signal is selected."));  // cn:未选中信号。
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
        mUI->addWarningLogMessage(tr("Failed to export the image."));  // cn:图片导出失败。
    }
}

/**
 * @brief 峰值表送表格页（转成数据帧加入数据管理器）
 */
void DASignalAnalysisUI::sendPeaksToTable()
{
    if (!mCurrentPeaks.isValid()) {
        mUI->addWarningLogMessage(tr("No peak data to send."));  // cn:没有可发送的峰值数据。
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
        pybind11::object df  = exporter.attr("peaks_to_dataframe")(doubleTuple(mCurrentPeaks.frequencies),
                                                                  doubleTuple(mCurrentPeaks.magnitudes),
                                                                  doubleTuple(mCurrentPeaks.relDb),
                                                                  doubleTuple(mCurrentPeaks.share));
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
        mUI->addWarningLogMessage(tr("Failed to send the peak table: %1").arg(QString::fromUtf8(e.what())));
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
            o[ QStringLiteral("view") ]             = view;
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
                mUI->addWarningLogMessage(tr("Signal source missing: dataframe \"%1\".").arg(dfName));
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
                mUI->addWarningLogMessage(tr("Signal source missing: %1").arg(path));
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
            mUI->addWarningLogMessage(tr("Failed to restore signal \"%1\": %2").arg(name, error));
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
    // 丝带标签页
    if (nullptr == mUI || nullptr == mCategory) {
        return;
    }
    DARibbonAreaInterface* ribbonArea = mUI->getRibbonArea();
    if (ribbonArea && ribbonArea->ribbonBar()) {
        ribbonArea->ribbonBar()->removeCategory(mCategory);
    }
    mCategory = nullptr;
    // 中央页面停靠窗口
    if (mPageDock) {
        DADockingAreaInterface* dockingArea = mUI->getDockingArea();
        if (dockingArea && dockingArea->dockManager()) {
            dockingArea->dockManager()->removeDockWidget(mPageDock);
        }
        mPageDock = nullptr;
    }
    if (mPlayer) {
        mPlayer->stop();
        delete mPlayer;
        mPlayer = nullptr;
    }
    if (mWorker) {
        mWorker->cancel();
        delete mWorker;
        mWorker = nullptr;
    }
    if (mPage) {
        delete mPage;
        mPage = nullptr;
    }
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
 * @brief 最近一次频谱结果（无结果返回无效对象）
 */
DASpectrumResult DASignalAnalysisUI::spectrumForSignal(const DASignalPtr& sig) const
{
    if (!sig) {
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
        mUI->addWarningLogMessage(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    DASignalAnalysisParams params = paramsForSignal(sig);
    if (!DASignalAnalysisSettingDialog::edit(&params, sig->sampleRate(), mPage)) {
        return;
    }
    mParamsPerSignal[ sig.get() ] = params;
    recomputeCurrentSignal();
}

/**
 * @brief 重新计算当前信号的频谱（参数变化后调用）
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
    mPendingSignal = sig.get();
    mWorker->computeSpectrum(sig, 0, paramsForSignal(sig));
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
 * @brief 频谱计算完成：缓存结果（显示层在后续里程碑接入）
 */
void DASignalAnalysisUI::onWorkerSpectrumReady(const DASpectrumResult& result)
{
    if (mPendingSignal && result.isValid()) {
        mSpectrumCache[ mPendingSignal ] = result;
        if (mPage) {
            if (DASignalAnalysisView* view = mPage->viewForSignal(mPendingSignal)) {
                view->setSpectrum(result);
            }
        }
    }
    if (mUI) {
        mUI->addInfoLogMessage(tr("Spectrum ready: resolution %1 Hz, %2 bins")
                                   .arg(result.resolutionHz, 0, 'f', 4)
                                   .arg(result.freqs.size()));
        // cn:频谱计算完成：分辨率 %1 赫兹，%2 个频点
    }
    Q_EMIT spectrumUpdated();
    // 同时刷新峰值表（整段频谱的前 N 峰）
    requestPeaks(false);
}

/**
 * @brief 后台任务失败
 */
void DASignalAnalysisUI::onWorkerFailed(const QString& error)
{
    if (mUI) {
        mUI->addWarningLogMessage(tr("Signal analysis failed: %1").arg(error));  // cn:信号分析失败：%1
    }
}

/**
 * @brief 视图请求计算频谱
 */
void DASignalAnalysisUI::onViewRequestSpectrum(const DASignalPtr& signal, int channel)
{
    if (!mWorker || !signal) {
        return;
    }
    mPendingSignal = signal.get();
    mWorker->computeSpectrum(signal, channel, paramsForSignal(signal));
}

/**
 * @brief 视图请求计算波形包络
 */
void DASignalAnalysisUI::onViewRequestEnvelope(qint64 start, qint64 end, int width)
{
    auto* view = qobject_cast< DASignalAnalysisView* >(sender());
    if (!mWorker || !view) {
        return;
    }
    mPendingEnvelopeView = view;
    mWorker->computeEnvelope(view->signal(), view->currentChannel(), start, end, width);
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
    if (!mPlayer->setSignal(sig, 0, &error)) {
        if (actionPlay) {
            actionPlay->setEnabled(false);
        }
        if (!error.isEmpty()) {
            mUI->addWarningLogMessage(error);
        }
        return;
    }
    if (actionPlay) {
        actionPlay->setEnabled(true);
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
        v->setCurrentTime(ms / 1000.0);
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
            mUI->addWarningLogMessage(
                tr("Signal is very large (> 1.5 GB as float32); the heatmap uses display-precision data.")
                // cn:信号非常大（按 float32 计超过 1.5 吉字节），热图按显示精度降级计算。
            );
        }
    }
    mPendingHeatmapView = view;
    mWorker->computeHeatmap(view->signal(), view->currentChannel(), view->params(), columns, rows);
}

/**
 * @brief 时频栅格结果回填到请求它的视图
 */
void DASignalAnalysisUI::onWorkerHeatmapReady(const DAStftResult& result)
{
    if (mPendingHeatmapView) {
        mPendingHeatmapView->setStft(result);
    }
}

/**
 * @brief 峰值表结果回填（表模型 + 频谱图标记）
 */
void DASignalAnalysisUI::onWorkerPeaksReady(const DAPeakResult& result)
{
    mCurrentPeaks = result;
    if (mPeakPanel) {
        const DASignalAnalysisParams params =
            (mPage && mPage->currentSignal()) ? paramsForSignal(mPage->currentSignal())
                                              : DASignalAnalysisParams::defaults();
        mPeakPanel->setAmplitudeDisplay(mAmplitudeMode, mDbReference, params.calibration, params.unit);
        mPeakPanel->setPeaks(result);
        mPeakPanel->setPeakCurves(mCurrentCurves);
    }
    if (mPage) {
        if (DASignalAnalysisView* view = mPage->viewForSignal(mPendingSignal)) {
            view->setPeaks(result);
        }
    }
}

/**
 * @brief 每帧前 N 峰值曲线结果回填
 */
void DASignalAnalysisUI::onWorkerPeakCurvesReady(const DAPeakCurveResult& result)
{
    mCurrentCurves = result;
    if (mPeakPanel) {
        mPeakPanel->setPeakCurves(result);
    }
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
    }
    mPeakCurvesRequested = false;  // 名次数改变后需要重扫曲线
    requestPeaks(true);
}

/**
 * @brief 包络结果回填到请求它的视图
 */
void DASignalAnalysisUI::onWorkerEnvelopeReady(const QVector< double >& xs,
                                               const QVector< double >& mins,
                                               const QVector< double >& maxs)
{
    if (mPendingEnvelopeView) {
        mPendingEnvelopeView->setEnvelope(xs, mins, maxs);
    }
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
        actionImportVideo->setToolTip(tr("Extract the audio track of a video file (requires FFmpeg)"));  // cn:抽取视频音频轨（需 FFmpeg）
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
