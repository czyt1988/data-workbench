#include "DASignalAnalysisUI.h"
// Qt
#include <QMenu>
#include <QActionGroup>
#include <QDebug>
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
// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"

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
    return true;
}

/**
 * @brief 获取信号分析页面
 */
DASignalAnalysisPage* DASignalAnalysisUI::page() const
{
    return mPage;
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
