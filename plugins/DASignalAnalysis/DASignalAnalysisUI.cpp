#include "DASignalAnalysisUI.h"
#include "DASignalAnalysisUIHelpers.h"

// Qt
#include <QMenu>
#include <QDebug>
#include <QIcon>
#include <QJsonObject>
#include <QMainWindow>
#include <QTimer>
#include <QToolBar>

// 3rdparty (SARibbon / ADS / qwt)
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonParamGrid.h"
#include "SARibbonTabBar.h"
#include "DockManager.h"
#include "DockWidget.h"

// DA / DAFigure
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DARibbonAreaInterface.h"
#include "DADockingAreaInterface.h"
#include "DAActionsInterface.h"
#include "DAProjectInterface.h"
#include "DAStatusBarInterface.h"
#include "DAUiObjectNames.h"
#include "DAFigureWidget.h"

// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DASignalAnalysisSettingContext.h"
#include "Page/DAPeakMarkerLabel.h"
#include "Signal/DASignalImporter.h"
#include "Analysis/DASignalAnalysisWorker.h"
#include "Playback/DASignalPlayer.h"

namespace DA
{
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
    // 全局显示偏好：峰值标记文本模板（缺省用默认模板；显式空串表示不显示标记文本）
    const QJsonObject initialConfig = DASignalImporter::loadPluginConfig();
    mPeakMarkerText                 = initialConfig.contains(QStringLiteral("peakMarkerText"))
                                          ? initialConfig.value(QStringLiteral("peakMarkerText")).toString()
                                          : daDefaultPeakMarkerText();
    // 「取消计算」动作只挂状态栏进度条右侧（不进丝带）：与丝带布局无关，先建出来，
    // 保证丝带区域不可用/上下文构建提前返回时状态栏仍能绑定它（不得替换/删除/重建）
    actionCancelAnalysis = mActions->createAction(
        "actionSignalCancelAnalysis",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("cancelAnalysis.svg")).toUtf8().constData());
    actionCancelAnalysis->setEnabled(false);  // 有任务在跑才可用
    // 主页「创建」panel 一个入口 + 上下文标签两页（插件不再占用顶级丝带标签页）
    installHomeEntry();
    buildSignalContextCategory();
    retranslateUi();
    // 无信号阶段：依赖信号的动作全部置灰（方案 §8.1）
    setSignalDependentActionsEnabled(false);

    // 状态栏计算进度：防抖定时器到点才显示进度条/停止按钮，短任务不闪烁
    mProgressDelayTimer = new QTimer(this);
    mProgressDelayTimer->setSingleShot(true);
    mProgressDelayTimer->setInterval(cProgressIndicatorDelayMs);
    connect(mProgressDelayTimer, &QTimer::timeout, this, &DASignalAnalysisUI::applyProgressIndicator);
    // 「取消计算」入口放在状态栏进度条右侧（不占丝带面板）：动作由宿主管理，按钮随进度条显示
    if (DAStatusBarInterface* statusBar = mUI->getStatusBar()) {
        statusBar->setProgressCancelAction(actionCancelAnalysis);
    }

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
        mActionExportPsdCsv =
            mExportMenu->addAction(tr("Power spectral density (CSV)"), this, &DASignalAnalysisUI::exportPsdCsv);
        // cn:功率谱密度（CSV）
        mActionExportHeatmapCsv =
            mExportMenu->addAction(tr("Heatmap matrix (CSV)"), this, &DASignalAnalysisUI::exportHeatmapCsv);
        // cn:热图矩阵（CSV）
        actionExportPeaks->setMenu(mExportMenu);
    }
    if (actionExportImage) {
        // 图片导出菜单（T9）：整页（2D+3D 合帧）/ 当前子图（PNG/SVG/PDF）/ 复制到剪贴板
        mExportImageMenu = new QMenu();
        mActionExportWholePage =
            mExportImageMenu->addAction(tr("Whole page (PNG)..."), this, &DASignalAnalysisUI::exportWholePageImage);
        // cn:整页（PNG）...
        mActionExportCurrentSubChart = mExportImageMenu->addAction(
            tr("Current sub-chart (PNG/SVG/PDF)..."), this, &DASignalAnalysisUI::exportCurrentSubChartImage);
        // cn:当前子图（PNG/SVG/PDF）...
        mExportImageMenu->addSeparator();
        mActionCopyWholePage = mExportImageMenu->addAction(
            tr("Copy whole page to clipboard"), this, &DASignalAnalysisUI::copyWholePageToClipboard);
        // cn:复制整页到剪贴板
        mActionCopyCurrentSubChart = mExportImageMenu->addAction(
            tr("Copy current sub-chart to clipboard"), this, &DASignalAnalysisUI::copyCurrentSubChartToClipboard);
        // cn:复制当前子图到剪贴板
        actionExportImage->setMenu(mExportImageMenu);
    }
    // 瀑布模式切换（二维 / 三维 / 隐藏；动作位于上下文标签 Display 组）
    if (actionWaterfall2D) {
        connect(actionWaterfall2D, &QAction::triggered, this, [ this ]() {
            if (DASignalAnalysisView* view = currentView()) {
                view->setWaterfallMode(0);
            }
        });
    }
    if (actionWaterfall3D) {
        connect(actionWaterfall3D, &QAction::triggered, this, [ this ]() {
            if (DASignalAnalysisView* view = currentView()) {
                view->setWaterfallMode(1);
            }
        });
    }
    if (actionWaterfallHide) {
        connect(actionWaterfallHide, &QAction::triggered, this, [ this ]() {
            if (DASignalAnalysisView* view = currentView()) {
                view->setWaterfallMode(2);
            }
        });
    }
    // 播放控制器（D26）
    mPlayer = new DASignalPlayer(this);
    connect(mPlayer, &DASignalPlayer::positionChanged, this, &DASignalAnalysisUI::onPlayerPositionChanged);
    // 播放状态（播放/暂停/停止/自然结束）→ 播放器控制条的 ▶/⏸ 勾选与图标
    connect(mPlayer, &DASignalPlayer::stateChanged, this, &DASignalAnalysisUI::onPlayerStateChanged);
    connect(mPlayer, &DASignalPlayer::errorOccurred, this, [ this ](const QString& err) {
        // 播放器原始错误是 Qt 的英文诊断信息：只写日志，界面显示翻译后的可理解提示（评审 §七-9）
        qWarning() << "DASignalAnalysisUI: playback error:" << err;
        reportError(tr("Playback failed. See the message log for details."));
        // cn:播放失败，详情见消息日志。
    });
    if (actionPlay) {
        connect(actionPlay, &QAction::toggled, this, &DASignalAnalysisUI::onPlayPauseToggled);
    }
    if (actionStop) {
        connect(actionStop, &QAction::triggered, this, &DASignalAnalysisUI::stopPlayback);
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
    // 峰值标记 / 标记显示 / 峰值窗口的动作在 buildAnalysisSettingsPage() 创建时已连接
    if (actionPeakTop10) {
        connect(actionPeakTop10, &QAction::triggered, this, &DASignalAnalysisUI::onPeakTopNChanged);
        connect(actionPeakTop20, &QAction::triggered, this, &DASignalAnalysisUI::onPeakTopNChanged);
        connect(actionPeakTop50, &QAction::triggered, this, &DASignalAnalysisUI::onPeakTopNChanged);
    }
    // 当前帧标记（信号显示 → 时频图设置）
    if (actionTimeCursor) {
        connect(actionTimeCursor, &QAction::toggled, this, &DASignalAnalysisUI::onTimeCursorToggled);
    }
    // 丝带样式切换后重新给「信号显示」页下发面板标题（D9）；页切换时刷新一次上下文控件
    // （宿主设置面板改色/改显示模式后切回丝带需要一个刷新点）
    if (mUI) {
        if (DARibbonAreaInterface* ribbonArea = mUI->getRibbonArea()) {
            if (SARibbonBar* ribbon = ribbonArea->ribbonBar()) {
                mRibbonStyleConnection =
                    connect(ribbon, &SARibbonBar::ribbonStyleChanged, this, [ this ](SARibbonBar::RibbonStyles) {
                        applyDisplayPagePanelTitles();
                    });
                mRibbonTabChangedConnection =
                    connect(ribbon, &SARibbonBar::currentRibbonTabChanged, this, [ this, ribbon ](int index) {
                        // 只在本插件两页被切到时刷新（其它标签页与本插件无关）
                        SARibbonCategory* current = ribbon->categoryByIndex(index);
                        if (current && (current == mContextAnalysisPage || current == mContextDisplayPage)) {
                            syncSignalContextControls();
                        }
                    });
            }
        }
    }
    connect(mWorker, &DASignalAnalysisWorker::failed, this, &DASignalAnalysisUI::onWorkerFailed);
    refreshFFmpegAvailability();
    if (!DASignalImporter::isFFmpegAvailable()) {
        // 不是错误，只是能力提示：用普通消息，避免一启动就弹告警
        mUI->addInfoLogMessage(tr("FFmpeg was not detected; video import is unavailable until a path is specified in "
                                  "Settings > Signal Analysis."));
        // cn:未检测到 FFmpeg，在「设置 → 信号分析」里指定路径前视频导入不可用。
    }
    // 音频解码依赖自检（I1）：soundfile 缺失时非 WAV 直读格式给出明确提示而不是运行时崩溃
    try {
        DAPyModule importer  = DAPyModule("DASignalAnalysis.importer");
        const bool available = importer.attr("soundfile_available")().cast< bool >();
        if (!available) {
            mUI->addInfoLogMessage(tr(
                "soundfile is not available; only WAV files can be read directly. Install soundfile or use FFmpeg."));
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

/**
 * @brief 当前信号对应的视图（页面未创建/无当前信号时返回 nullptr）
 *
 * provider 与 Ribbon 上下文标签、底部工具栏共用此入口；所有跨指针访问都要判空。
 */
DASignalAnalysisView* DASignalAnalysisUI::currentView() const
{
    if (nullptr == mPage) {
        return nullptr;
    }
    const DASignalPtr sig = mPage->currentSignal();
    if (!sig) {
        return nullptr;
    }
    return mPage->viewForSignal(sig.get());
}

/**
 * @brief 全部信号的 figure（设置上下文图元树用）
 *
 * 无页面/无信号时返回空列表；figure 的所有权仍在各视图，调用方不得释放。
 */
QList< DAFigureWidget* > DASignalAnalysisUI::signalFigures() const
{
    QList< DAFigureWidget* > figures;
    if (nullptr == mPage) {
        return figures;
    }
    const QList< DASignalPtr > signalList = mPage->allSignals();
    for (const DASignalPtr& sig : signalList) {
        if (!sig) {
            continue;
        }
        if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
            if (DAFigureWidget* figure = view->figure()) {
                figures.append(figure);
            }
        }
    }
    return figures;
}

/**
 * @brief 宿主停靠区接口（provider 注册设置上下文/图元树/设置面板用）
 * @return 未初始化界面时返回 nullptr
 */
DADockingAreaInterface* DASignalAnalysisUI::getDockingArea() const
{
    return mUI ? mUI->getDockingArea() : nullptr;
}

/**
 * @brief 显示/隐藏插件 Ribbon 上下文标签
 *
 * 优先走宿主接口 DARibbonAreaInterface::showContextCategory/hideContextCategory(objectName)
 * （上下文标签只能按 objectName 查找），失败时退回 SARibbonBar 的指针版本。
 * 显示时把上下文页设为当前页（showContextCategory 只追加 tab、不切换当前页），
 * 并同步一次控件状态，保证与当前视图一致。
 * @param on true 显示，false 隐藏
 */
void DASignalAnalysisUI::setSignalContextCategoryVisible(bool on)
{
    if (nullptr == mContextCategory || nullptr == mUI) {
        return;
    }
    DARibbonAreaInterface* ribbonArea = mUI->getRibbonArea();
    if (nullptr == ribbonArea) {
        return;
    }
    SARibbonBar* ribbon = ribbonArea->ribbonBar();
    if (nullptr == ribbon) {
        return;
    }
    const QString objName = QString::fromUtf8(cContextCategoryObjectName);
    if (!on) {
        if (!ribbonArea->hideContextCategory(objName)) {
            ribbon->hideContextCategory(mContextCategory);
        }
        return;
    }
    const bool alreadyVisible = ribbon->isContextCategoryVisible(mContextCategory);
    const QList< int > before = ribbon->currentVisibleContextCategoryTabIndexs();
    if (!ribbonArea->showContextCategory(objName)) {
        ribbon->showContextCategory(mContextCategory);
    }
    if (!alreadyVisible) {
        const QList< int > after = ribbon->currentVisibleContextCategoryTabIndexs();
        for (int index : after) {
            if (!before.contains(index)) {
                ribbon->ribbonTabBar()->setCurrentIndex(index);
                break;
            }
        }
    }
    syncSignalContextControls();
}

/**
 * @brief 显示/隐藏窗口底部播放工具栏
 *
 * 只切可见性；显示前从当前视图回填控件状态（阻断信号防回环）。工具栏在
 * 页面惰性创建时建立，未创建页面时调用是安全的空操作。
 * @param on true 显示，false 隐藏
 */
void DASignalAnalysisUI::setSignalToolbarVisible(bool on)
{
    if (nullptr == mSignalToolbar) {
        return;
    }
    if (on) {
        syncSignalToolbar();
    }
    mSignalToolbar->setVisible(on);
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
        actionImportVideo->setEnabled(available && !mImportTaskActive);
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
    // 页面对象存在但 dock 没建成功（极端环境）：先把孤儿页面清掉再重建（评审 N-U8）。
    // 旧页面的设置上下文提供者必须先注销并销毁，否则注册表里会留下指向旧页面的条目。
    if (mPage && !mPageDock) {
        if (mUI) {
            if (DADockingAreaInterface* dockingArea = mUI->getDockingArea()) {
                dockingArea->unregisterSettingContextProvider(mPage);
            }
        }
        if (mSettingContext) {
            delete mSettingContext;
            mSettingContext = nullptr;
        }
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
    // 色标显隐偏好：缺省（旧配置/首次运行）按显示处理；动作勾选在 buildSignalContextCategory 建立、
    // 视图生效在 connectSignalView → syncSignalContextControls
    mColorBarVisible = cfg.value(QStringLiteral("colorBarVisible")).toBool(true);
    // 峰值停靠窗口与页面同批创建（创建后保持隐藏，首次导入信号成功时按偏好再决定）
    buildPeakDock();
    // 窗口底部播放工具栏同批创建（默认隐藏，provider 激活信号分析页时显示）
    buildSignalToolbar();
    // 注册设置上下文提供者：该页 dock 激活时由 provider 接管设置面板/图元树/上下文标签/工具栏
    if (DADockingAreaInterface* dockingArea = (mUI ? mUI->getDockingArea() : nullptr)) {
        mSettingContext = new DASignalAnalysisSettingContext(this);
        dockingArea->registerSettingContextProvider(mPage, mSettingContext);
    }
    setSignalDependentActionsEnabled(mPage->signalCount() > 0);
    updateRelinkActionState();
    return true;
}

/**
 * @brief 打开信号分析页面（首次调用时惰性创建）
 *
 * 主页「创建」panel 的「信号分析」入口。页面关闭后再次触发会重新显示并唤起到前台，
 * 与首次打开的路径一致；打开后丝带停在「分析设置」页（D1/D2）。
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
    // 上下文标签已显示（页面已激活过）时切回页 1「分析设置」：默认页就是它，重新打开保持一致
    if (mContextCategory && mContextAnalysisPage) {
        if (DARibbonAreaInterface* ribbonArea = mUI->getRibbonArea()) {
            if (SARibbonBar* ribbon = ribbonArea->ribbonBar()) {
                if (ribbon->isContextCategoryVisible(mContextCategory)) {
                    const QList< int > tabs = ribbon->currentVisibleContextCategoryTabIndexs();
                    for (int index : tabs) {
                        if (ribbon->categoryByIndex(index) == mContextAnalysisPage) {
                            ribbon->ribbonTabBar()->setCurrentIndex(index);
                            break;
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief 清理界面资源（插件热卸载前调用）
 *
 * 顺序：菜单从动作摘掉 → 丝带标签页移除 → 中央页面停靠窗口移除 → 动作销毁。
 */
void DASignalAnalysisUI::finalize()
{
    // 状态栏进度入口：先清本插件的进度（只在自己显示过时隐藏），再解绑取消按钮；
    // 取消动作随后由宿主动作管理器销毁，状态栏按钮不能再引用它
    hideProgressIndicator();
    if (mUI) {
        if (DAStatusBarInterface* statusBar = mUI->getStatusBar()) {
            statusBar->setProgressCancelVisible(false);
            statusBar->setProgressCancelAction(nullptr);
        }
    }
    // 先解绑当前 2D 子图的属性变化连接：页面/dock 即将销毁，避免残留连接与悬空 QPointer
    unbindCurrentChartSignals();
    // 保存插件偏好：峰值窗口显隐与上次色带（P2）；FFmpeg 路径等其它键原样保留。
    // 页面从未创建过的会话没有可保存的偏好（不能把其它会话的 peakDockVisible 覆盖掉）
    if (mUI && mPage) {
        QJsonObject cfg                          = DASignalImporter::loadPluginConfig();
        cfg[ QStringLiteral("peakDockVisible") ] = (mPeakDock && !mPeakDock->isClosed());
        // 色标显隐兜底写回（onColorBarToggled 已即时写；这里防未触发 toggled 的退出路径丢失）
        cfg[ QStringLiteral("colorBarVisible") ] = mColorBarVisible;
        if (DASignalAnalysisView* v = currentView()) {
            cfg[ QStringLiteral("colormap") ] = v->colormapName();
        }
        DASignalImporter::savePluginConfig(cfg);
    }
    // 设置上下文提供者先注销（注销会触发 deactivateSettingContext：隐藏上下文标签/底部工具栏、
    // 还原宿主设置面板与图元树），再销毁；必须在页面 dock 移除之前完成
    if (mUI && mPage) {
        if (DADockingAreaInterface* dockingArea = mUI->getDockingArea()) {
            dockingArea->unregisterSettingContextProvider(mPage);
        }
    }
    if (mSettingContext) {
        delete mSettingContext;
        mSettingContext = nullptr;
    }
    // 菜单先与动作解绑（QMenu 不随 QAction 销毁）
    if (actionPeakTopN) {
        actionPeakTopN->setMenu(nullptr);
    }
    if (actionImport) {
        actionImport->setMenu(nullptr);
    }
    if (mImportMenu) {
        delete mImportMenu;
        mImportMenu = nullptr;
    }
    if (mPeakTopNMenu) {
        delete mPeakTopNMenu;
        mPeakTopNMenu = nullptr;
    }
    // 色带菜单（云图设置）
    if (actionColormap) {
        actionColormap->setMenu(nullptr);
    }
    if (mColormapMenu) {
        delete mColormapMenu;
        mColormapMenu = nullptr;
    }
    mColormapGroup = nullptr;
    // 瀑布色带菜单（瀑布图设置）
    if (actionWaterfallColormap) {
        actionWaterfallColormap->setMenu(nullptr);
    }
    if (mWaterfallColormapMenu) {
        delete mWaterfallColormapMenu;
        mWaterfallColormapMenu = nullptr;
    }
    mWaterfallColormapGroup = nullptr;
    // 云图「显示 ▾」菜单（显示模式/插值）
    if (actionHeatmapDisplay) {
        actionHeatmapDisplay->setMenu(nullptr);
    }
    if (mHeatmapDisplayMenu) {
        delete mHeatmapDisplayMenu;
        mHeatmapDisplayMenu = nullptr;
    }
    mHeatmapDisplayModeGroup   = nullptr;
    mHeatmapResampleGroup      = nullptr;
    mHeatmapDisplayModeSection = nullptr;
    mHeatmapResampleSection    = nullptr;
    mHeatmapDisplayModeActions.clear();
    mHeatmapResampleActions.clear();
    // 「视图 ▾」菜单：子菜单/菜单项随父菜单销毁，这里只释放父菜单并清空指针
    if (actionViewOptions) {
        actionViewOptions->setMenu(nullptr);
    }
    if (mViewOptionsMenu) {
        delete mViewOptionsMenu;
        mViewOptionsMenu = nullptr;
    }
    mTimeFormatMenu    = nullptr;
    mLayoutPresetMenu  = nullptr;
    mTimeFormatGroup   = nullptr;
    actionDisplayReset = nullptr;
    mTimeFormatActions.clear();
    mLayoutPresetActions.clear();
    // 「测量 ▾」菜单：菜单项随父菜单销毁
    if (actionMeasure) {
        actionMeasure->setMenu(nullptr);
    }
    if (mMeasureMenu) {
        delete mMeasureMenu;
        mMeasureMenu = nullptr;
    }
    actionLinkedZoom    = nullptr;
    actionDualCursor    = nullptr;
    actionPeakHold      = nullptr;
    actionResetPeakHold = nullptr;
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
    // 图片导出菜单（整页/当前子图/复制到剪贴板，T9）
    if (actionExportImage) {
        actionExportImage->setMenu(nullptr);
    }
    if (mExportImageMenu) {
        delete mExportImageMenu;
        mExportImageMenu = nullptr;
    }
    mActionExportWholePage       = nullptr;
    mActionExportCurrentSubChart = nullptr;
    mActionCopyWholePage         = nullptr;
    mActionCopyCurrentSubChart   = nullptr;
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
    // 窗口底部播放工具栏：从主窗口移除并销毁（必须在页面/停靠窗口销毁之前）
    if (mSignalToolbar) {
        if (mUI) {
            if (QMainWindow* mainWindow = mUI->getMainWindow()) {
                mainWindow->removeToolBar(mSignalToolbar);
            }
        }
        delete mSignalToolbar;
        mSignalToolbar = nullptr;
    }
    mToolbarFollowCheck     = nullptr;
    mToolbarPlayButton      = nullptr;
    mToolbarStopButton      = nullptr;
    mToolbarTimeSlider      = nullptr;
    mToolbarTimeLabel       = nullptr;
    mToolbarAutoScrollCheck = nullptr;
    // 插件 Ribbon 上下文标签：先隐藏再销毁（SARibbonBar 拥有上下文标签的管理权）
    if (mContextCategory) {
        DARibbonAreaInterface* ribbonArea = (mUI ? mUI->getRibbonArea() : nullptr);
        SARibbonBar* ribbon               = (ribbonArea ? ribbonArea->ribbonBar() : nullptr);
        if (mRibbonStyleConnection) {
            QObject::disconnect(mRibbonStyleConnection);
            mRibbonStyleConnection = QMetaObject::Connection();
        }
        if (mRibbonTabChangedConnection) {
            QObject::disconnect(mRibbonTabChangedConnection);
            mRibbonTabChangedConnection = QMetaObject::Connection();
        }
        if (ribbon) {
            if (ribbon->isContextCategoryVisible(mContextCategory)) {
                ribbon->hideContextCategory(mContextCategory);
            }
            ribbon->destroyContextCategory(mContextCategory);
        } else {
            qWarning() << "DASignalAnalysisUI::finalize: ribbon bar unavailable, context category left to host";
        }
        mContextCategory = nullptr;
    }
    mContextDisplayPage  = nullptr;
    mContextAnalysisPage = nullptr;
    mPanelTimeFreqView   = nullptr;
    mPanelHeatmap        = nullptr;
    mPanelWaterfall      = nullptr;
    mChannelCombo        = nullptr;
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
    // 主页「创建」panel：只摘插件注入的按钮，不删宿主 panel（面板归宿主所有）。
    // mUI 为空时不能提前 return，后面的动作清理必须执行（铁律 11）
    if (mUI && actionOpenSignalPage) {
        if (DARibbonAreaInterface* ribbonArea = mUI->getRibbonArea()) {
            SARibbonPanel* createPanel =
                ribbonArea->getPanelByObjectName(QString::fromUtf8(UiNames::Ribbon::MainCreatePanel));
            removeActionFromPanel(createPanel, actionOpenSignalPage);
        }
    }
    // 经宿主接口销毁全部动作（含上下文标签新增动作）
    if (mActions) {
        const QList< QAction* > allActions = { actionOpenSignalPage,
                                               actionImport,
                                               actionImportAudio,
                                               actionImportDataFrame,
                                               actionImportVideo,
                                               actionPlay,
                                               actionStop,
                                               actionAnalysisSetting,
                                               actionCancelAnalysis,
                                               actionPeakMarker,
                                               actionPeakTopN,
                                               actionPeakMarkerDisplay,
                                               actionPeakWindow,
                                               actionWaterfall2D,
                                               actionWaterfall3D,
                                               actionWaterfallHide,
                                               actionExportPeaks,
                                               actionExportImage,
                                               actionRelinkSource,
                                               actionTimeCursor,
                                               actionViewOptions,
                                               actionColormap,
                                               actionColorBar,
                                               actionWaterfallColormap,
                                               actionWaterfallHighlight,
                                               actionWaterfallSettings,
                                               actionHeatmapDisplay,
                                               actionMeasure };
        for (QAction* act : allActions) {
            if (act) {
                mActions->removeAction(act);
            }
        }
    }
    // 全部置空，防止悬空访问
    actionOpenSignalPage     = nullptr;
    actionImport             = nullptr;
    actionImportAudio        = nullptr;
    actionImportDataFrame    = nullptr;
    actionImportVideo        = nullptr;
    actionPlay               = nullptr;
    actionStop               = nullptr;
    actionAnalysisSetting    = nullptr;
    actionCancelAnalysis     = nullptr;
    actionPeakMarker         = nullptr;
    actionPeakTopN           = nullptr;
    actionPeakMarkerDisplay  = nullptr;
    actionPeakWindow         = nullptr;
    actionPeakTop10          = nullptr;
    actionPeakTop20          = nullptr;
    actionPeakTop50          = nullptr;
    actionWaterfall2D        = nullptr;
    actionWaterfall3D        = nullptr;
    actionWaterfallHide      = nullptr;
    actionTimeCursor         = nullptr;
    actionViewOptions        = nullptr;
    actionDisplayReset       = nullptr;
    actionExportPeaks        = nullptr;
    actionExportImage        = nullptr;
    actionRelinkSource       = nullptr;
    actionColormap           = nullptr;
    actionColorBar           = nullptr;
    actionWaterfallColormap  = nullptr;
    actionWaterfallHighlight = nullptr;
    actionWaterfallSettings  = nullptr;
    actionHeatmapDisplay     = nullptr;
    actionMeasure            = nullptr;
    actionLinkedZoom         = nullptr;
    mPanelImportExport       = nullptr;
    mPanelPreprocess         = nullptr;
    mPanelSpectrum           = nullptr;
    mPanelTimeFreq           = nullptr;
    mPanelPeaks              = nullptr;
    mPanelMore               = nullptr;
    // 峰值 panel 控件随所属面板/category 一并销毁，这里只置空指针
    mPeakModeCombo         = nullptr;
    mPeakRelDbSpin         = nullptr;
    mPeakModeLabel         = nullptr;
    mPeakRelDbLabel        = nullptr;
    mHeatmapDbFloorSpin    = nullptr;
    mHeatmapDbFloorLabel   = nullptr;
    mWaterfallDbFloorSpin  = nullptr;
    mWaterfallDbFloorLabel = nullptr;
    // 参数网格的标签归 SARibbonParamGrid 所有，随面板销毁；指针统一置空，避免重译时命中悬空指针
    mDetrendLabel       = nullptr;
    mChannelLabel       = nullptr;
    mWindowLabel        = nullptr;
    mAmplitudeModeLabel = nullptr;
    mResolutionLabel    = nullptr;
    mStftWindowLabel    = nullptr;
    mStftOverlapLabel   = nullptr;
    mDeltaFLabel        = nullptr;
    mPeakMarkersVisible = false;
    mParamsPerSignal.clear();
    mParamsVersion.clear();
    mSpectrumCache.clear();
    mSpectrumCacheChannel.clear();
    mMemoryBudgetVerdict.clear();
    // 插件卸载时清理本插件的临时音频文件（P5）
    DASignalImporter::cleanupTempFiles();
}

/**
 * @brief 设置依赖信号的动作是否可用（无信号时播放/分析/峰值/显示/工具/导出全部置灰）
 * @param on 是否可用
 */
void DASignalAnalysisUI::setSignalDependentActionsEnabled(bool on)
{
    const QList< QAction* > acts = { actionPlay,
                                     actionStop,
                                     actionAnalysisSetting,
                                     actionPeakMarker,
                                     actionPeakTopN,
                                     actionPeakMarkerDisplay,
                                     actionPeakWindow,
                                     actionWaterfall2D,
                                     actionWaterfall3D,
                                     actionWaterfallHide,
                                     actionExportPeaks,
                                     actionTimeCursor,
                                     actionViewOptions,
                                     actionDisplayReset,
                                     actionWaterfallColormap,
                                     actionWaterfallHighlight,
                                     actionWaterfallSettings,
                                     actionExportImage,
                                     actionRelinkSource,
                                     actionHeatmapDisplay,
                                     actionMeasure,
                                     actionLinkedZoom,
                                     actionDualCursor,
                                     actionPeakHold,
                                     actionResetPeakHold,
                                     actionColormap,
                                     actionColorBar };
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
    // 工具类动作还取决于当前 2D 子图是否存在：按当前视图状态再精化一次
    syncSignalContextControls();
}

}  // namespace DA
