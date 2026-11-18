#include "DASignalAnalysisUI.h"
// Qt
#include <QMenu>
#include <QActionGroup>
#include <cmath>
#include <limits>
#include <utility>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStandardPaths>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include "DAPybind11InQt.h"
#include "DAPyModule.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11QtCaster.hpp"
#include "pandas/DAPyDataFrame.h"
#include "Signal/DADataFrameSignalSource.h"
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonContextCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonPanelLayout.h"
#include "SARibbonTabBar.h"
// ADS
#include "DockManager.h"
#include "DockWidget.h"
// DA
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DACommandInterface.h"
#include "DARibbonAreaInterface.h"
#include "DADockingAreaInterface.h"
#include "DAActionsInterface.h"
#include "DADataManagerInterface.h"
#include "DAProjectInterface.h"
#include "DAStatusBarInterface.h"
#include "DAUiObjectNames.h"
#include "DockAreaWidget.h"
// DAFigure / DAGui / qwt
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "qwt_colormap_preset.h"
#include "qwt_plot_item.h"
#include <QUndoStack>
// DASignalAnalysis
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DASignalAnalysisSettingContext.h"
#include "Page/DASignalTimeText.h"
#include "Page/DAPeakDockWidget.h"
#include "Page/DAPeakMarkerLabel.h"
#include "Dialogs/DASignalImportDialog.h"
#include "Dialogs/DASignalAnalysisSettingDialog.h"
#include "Dialogs/DAPeakMarkerDisplayDialog.h"
#include "Signal/DASignalImporter.h"
#include "Analysis/DASignalAmplitude.h"
#include "Analysis/DASignalMemoryEstimate.h"
#include "Analysis/DASignalAnalysisWorker.h"
#include "Playback/DASignalPlayer.h"
#include "Archive/DASignalArchiveTask.h"
#include "Archive/DASignalFigureArchive.h"
#include "Page/DASignalChartExport.h"
#include "DAChartItemsManager.h"
#include "Archive/DAChartItemArchive.h"
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
namespace
{
/// 丝带标签页 / 面板 / 动作的 object name（唯一名，供样式与查找使用）
// 插件不再占用顶级丝带标签页：主页「创建」panel 一个入口 + 上下文标签两页。
const char* const cPanelImportExportName   = "da-panel-signal.importExport";
const char* const cPanelPreprocessName     = "da-panel-signal.preprocess";
const char* const cPanelSpectrumName       = "da-panel-signal.spectrum";
const char* const cPanelTimeFreqName       = "da-panel-signal.timefreq";
const char* const cPanelPeaksName          = "da-panel-signal.peaks";
const char* const cPanelMoreName           = "da-panel-signal.more";
const char* const cPageDockObjectName      = "da-dock-signal-analysis-page";
// 插件自建的 Ribbon 上下文标签（SARibbon 按 objectName 查找，任务卡固定值）
const char* const cContextCategoryObjectName     = "ContextCategorySignalAnalysis";
const char* const cContextAnalysisPageObjectName = "da-category-signal-analysis";
const char* const cContextDisplayPageObjectName  = "da-category-signal-context";
const char* const cPanelContextTimeFreqViewName  = "da-panel-signal.display.view";
const char* const cPanelContextHeatmapName       = "da-panel-signal.display.heatmap";
const char* const cPanelContextWaterfallName     = "da-panel-signal.display.waterfall";
// 窗口底部播放工具栏
const char* const cToolbarObjectName = "da-signal-playback-toolbar";

// 图标资源前缀
const char* const cIconPrefix = ":/DASignalAnalysis/icon/";
// 宿主应用级图标前缀（复用宿主已有图标，不为单个动作新增 SVG）
const char* const cAppIconPrefix = ":/app/bright/Icon/";
// 内存预算（方案 §11 / 评审 D5 / N-C1c）：频谱、峰值表、包络与热图共用同一预算门
constexpr qint64 cAnalysisBudgetBytes = qint64(1.5 * 1024 * 1024 * 1024);
// 内存预算判定（QHash 值）：0=未询问 / 1=用户已确认继续 / 2=用户拒绝
constexpr int cBudgetUnasked  = 0;
constexpr int cBudgetAllowed  = 1;
constexpr int cBudgetDeclined = 2;
// 状态栏计算进度的防抖窗口（毫秒）：短任务在窗口内结束就完全不显示进度条与停止按钮，
// 避免小计算让状态栏闪一下；超过窗口仍在算才显示。
constexpr int cProgressIndicatorDelayMs = 500;

/// 插件自带图标路径
QString signalPluginIcon(const char* file)
{
    return QString::fromUtf8(cIconPrefix) + QString::fromUtf8(file);
}

/**
 * @brief 布局比例预设的行高（归一化，和≈1）
 *
 * 2×2 网格：第 1 行是波形（跨两列）、第 2 行是频谱 / 时频图。
 * 0=默认（与 buildFigure 一致）/ 1=1:1 / 2=波形优先 / 3=分析优先。
 * @param preset 预设编号，越界按默认处理
 * @return 两行比例
 */
QList< qreal > layoutRowsForPreset(int preset)
{
    switch (preset) {
    case 1:
        return { 0.50, 0.50 };
    case 2:
        return { 0.65, 0.35 };
    case 3:
        return { 0.35, 0.65 };
    case 0:
    default:
        return { 0.56, 0.44 };
    }
}

/// 布局预设的列宽（左右 1:1，所有预设一致）
QList< qreal > layoutColumnsDefault()
{
    return { 0.50, 0.50 };
}

/// 宿主应用级图标路径（复用宿主通用图标）
QString signalAppIcon(const char* file)
{
    return QString::fromUtf8(cAppIconPrefix) + QString::fromUtf8(file);
}

/**
 * @brief 用预设色带生成菜单用的渐变预览图标（64×16）
 *
 * 色带菜单每项带一条真实渐变色条（取自 QwtColorMapPreset::create 的 rgb 输出），
 * 用户不必先应用再判断颜色。未知预设给透明图标（菜单项仍可用）。
 * @param preset QwtColorMapPreset 预设名
 * @return 64×16 图标的 QIcon
 */
QIcon makeColormapPreviewIcon(const QString& preset)
{
    constexpr int cPreviewWidth  = 64;
    constexpr int cPreviewHeight = 16;
    QImage image(cPreviewWidth, cPreviewHeight, QImage::Format_ARGB32);
    const std::unique_ptr< QwtLinearColorMap > map = QwtColorMapPreset::create(preset);
    if (!map) {
        image.fill(Qt::transparent);
        return QIcon(QPixmap::fromImage(image));
    }
    const double span = double(cPreviewWidth > 1 ? cPreviewWidth - 1 : 1);
    for (int x = 0; x < cPreviewWidth; ++x) {
        const double value = double(x) / span;
        const QRgb color     = map->rgb(0.0, 1.0, value);
        for (int y = 0; y < cPreviewHeight; ++y) {
            image.setPixel(x, y, color);
        }
    }
    return QIcon(QPixmap::fromImage(image));
}

/**
 * @brief 丝带参数网格：把一组「标签 + 控件」行放进共享列宽的 QGridLayout
 *
 * 丝带的 small 项逐行独立排布，每行标签宽度按自身文本自适应，同一组内的标签与
 * 控件左边界因此参差不齐。整组参数装进同一个网格控件、以 Large 占比加入面板后，
 * 标签列与控件列共享列宽：标签左对齐、控件列统一左边界，行距与丝带 small 行一致。
 * 末尾压一个弹性空白行，参数行保持自然高度、贴顶排列，不被面板高度均分拉伸。
 */
class DASignalRibbonParamGrid : public QWidget
{
public:
    explicit DASignalRibbonParamGrid(QWidget* parent) : QWidget(parent), mGrid(new QGridLayout(this))
    {
        mGrid->setContentsMargins(0, 0, 0, 0);
        mGrid->setHorizontalSpacing(4);
        mGrid->setVerticalSpacing(2);
        mGrid->setColumnStretch(1, 1);
    }

    /// 追加一行「标签 + 控件」，返回标签指针供语言切换时重译
    QLabel* addRow(const QString& text, QWidget* field)
    {
        auto* label = new QLabel(text, this);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        const int row = mGrid->rowCount();
        mGrid->addWidget(label, row, 0);
        mGrid->addWidget(field, row, 1);
        return label;
    }

    /// 追加一行没有标签的控件（复选框），与上面的输入控件左边界对齐
    void addFieldOnly(QWidget* field)
    {
        mGrid->addWidget(field, mGrid->rowCount(), 1);
    }

    /// 追加一行跨两列的只读提示（如 Δf），文本自带标签
    void addHint(QWidget* hint)
    {
        mGrid->addWidget(hint, mGrid->rowCount(), 0, 1, 2);
    }

    /// 收尾：压弹性空白行，避免参数行被面板高度均分拉伸
    void finish()
    {
        const int row = mGrid->rowCount();
        mGrid->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), row, 0, 1, 2);
        mGrid->setRowStretch(row, 1);
    }

private:
    QGridLayout* mGrid { nullptr };
};

/**
 * @brief 从宿主 ribbon 面板中摘除某个动作对应的按钮（不删宿主面板）
 *
 * 插件往宿主主页「创建」panel 注入过「信号分析」大按钮；热卸载时只摘按钮、
 * 不删宿主 panel（面板归宿主所有）。写法与 plugins/DataAnalysis/DataAnalysisUI.cpp 一致。
 * @param panel 宿主面板，可为 nullptr
 * @param act 待摘除的动作，可为 nullptr
 */
void removeActionFromPanel(SARibbonPanel* panel, QAction* act)
{
    if (nullptr == panel || nullptr == act) {
        return;
    }
    SARibbonPanelLayout* lay = panel->panelLayout();
    if (nullptr == lay) {
        return;
    }
    const int idx = lay->indexByAction(act);
    if (idx >= 0) {
        // takeAt 内部已对按钮控件 hide + deleteLater，item 本身需手动释放
        QLayoutItem* item = lay->takeAt(idx);
        delete item;
    }
}
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
        mActionExportPsdCsv = mExportMenu->addAction(tr("Power spectral density (CSV)"), this, &DASignalAnalysisUI::exportPsdCsv);
        // cn:功率谱密度（CSV）
        mActionExportHeatmapCsv =
            mExportMenu->addAction(tr("Heatmap matrix (CSV)"), this, &DASignalAnalysisUI::exportHeatmapCsv);
        // cn:热图矩阵（CSV）
        actionExportPeaks->setMenu(mExportMenu);
    }
    if (actionExportImage) {
        // 图片导出菜单（T9）：整页（2D+3D 合帧）/ 当前子图（PNG/SVG/PDF）/ 复制到剪贴板
        mExportImageMenu       = new QMenu();
        mActionExportWholePage = mExportImageMenu->addAction(tr("Whole page (PNG)..."),
                                                             this,
                                                             &DASignalAnalysisUI::exportWholePageImage);
        // cn:整页（PNG）...
        mActionExportCurrentSubChart =
            mExportImageMenu->addAction(tr("Current sub-chart (PNG/SVG/PDF)..."),
                                        this,
                                        &DASignalAnalysisUI::exportCurrentSubChartImage);
        // cn:当前子图（PNG/SVG/PDF）...
        mExportImageMenu->addSeparator();
        mActionCopyWholePage =
            mExportImageMenu->addAction(tr("Copy whole page to clipboard"), this, &DASignalAnalysisUI::copyWholePageToClipboard);
        // cn:复制整页到剪贴板
        mActionCopyCurrentSubChart = mExportImageMenu->addAction(tr("Copy current sub-chart to clipboard"),
                                                                 this,
                                                                 &DASignalAnalysisUI::copyCurrentSubChartToClipboard);
        // cn:复制当前子图到剪贴板
        actionExportImage->setMenu(mExportImageMenu);
    }
    // 瀑布模式切换（二维 / 三维 / 隐藏；动作位于上下文标签 Display 组）
    if (actionWaterfall2D) {
        connect(actionWaterfall2D, &QAction::triggered, this, [this]() {
            if (DASignalAnalysisView* view = currentView()) {
                view->setWaterfallMode(0);
            }
        });
    }
    if (actionWaterfall3D) {
        connect(actionWaterfall3D, &QAction::triggered, this, [this]() {
            if (DASignalAnalysisView* view = currentView()) {
                view->setWaterfallMode(1);
            }
        });
    }
    if (actionWaterfallHide) {
        connect(actionWaterfallHide, &QAction::triggered, this, [this]() {
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
    connect(mPlayer, &DASignalPlayer::errorOccurred, this, [this](const QString& err) {
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
                mRibbonStyleConnection = connect(ribbon,
                                                 &SARibbonBar::ribbonStyleChanged,
                                                 this,
                                                 [this](SARibbonBar::RibbonStyles) { applyDisplayPagePanelTitles(); });
                mRibbonTabChangedConnection = connect(ribbon,
                                                      &SARibbonBar::currentRibbonTabChanged,
                                                      this,
                                                      [this, ribbon](int index) {
                                                          // 只在本插件两页被切到时刷新（其它标签页与本插件无关）
                                                          SARibbonCategory* current = ribbon->categoryByIndex(index);
                                                          if (current
                                                              && (current == mContextAnalysisPage
                                                                  || current == mContextDisplayPage)) {
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
    connect(view, &DASignalAnalysisView::followSliderToggled, this, &DASignalAnalysisUI::onViewFollowSliderToggled, Qt::UniqueConnection);
    // 跟随播放：视图按当前时间请求「当前帧真实幅值谱」，这里同步算完回填（与峰值曲线同口径）
    connect(view, &DASignalAnalysisView::followFrameSpectrumRequested, this, &DASignalAnalysisUI::onViewFollowFrameSpectrumRequested, Qt::UniqueConnection);
    // 当前 2D/3D 子图切换：上下文标签的工具动作/瀑布勾选跟随
    connect(view, &DASignalAnalysisView::currentSubChartChanged, this, &DASignalAnalysisUI::onViewSubChartChanged, Qt::UniqueConnection);
    // 视图创建/切换后与上下文标签、窗口底部工具栏对齐
    syncSignalContextControls();
    syncSignalToolbar();
    // 连接就绪后再发起首次计算（视图显示早于 connect，showEvent 里的请求会丢）
    view->requestInitialAnalysis();
}

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
        return;
    }
    const DASignalPtr current = (mPage) ? mPage->currentSignal() : DASignalPtr();
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
    const DASignalTaskContext ctx        = makeTaskContext(sig, view->currentChannel());
    const DASignalAnalysisParams params  = paramsForSignal(sig);
    if (mWorker->computeFrameSpectrumSync(ctx, params, frameIndex, &result)) {
        view->setFollowFrameSpectrum(result.freqs, result.amplitudes);
    }
    // 失败静默：只写 worker 内的英文 qWarning，界面不回灌消息（跟随播放是每帧调用）
}

/**
 * @brief 当前 2D/3D 子图切换：同步上下文标签的工具动作/瀑布勾选
 */
void DASignalAnalysisUI::onViewSubChartChanged()
{
    syncSignalContextControls();
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
 * @brief 经宿主动作管理器创建带图标的动作
 *
 * 动作归宿主 DAActionsInterface 所有（parent 是宿主接口），插件 finalize() 统一
 * removeAction 销毁；丝带顶级标签页与上下文标签页共用本入口，避免各写一份。
 * @param objectName 动作 objectName（样式/查找用，全插件唯一）
 * @param iconPath 图标资源路径（插件 icon/ 或宿主通用图标）
 * @param checkable 是否可勾选（勾选初值由同步逻辑回填）
 */
QAction* DASignalAnalysisUI::createRibbonIconAction(const char* objectName, const QString& iconPath, bool checkable)
{
    if (nullptr == mActions) {
        qWarning() << "DASignalAnalysisUI::createRibbonIconAction: action interface is unavailable";
        return nullptr;
    }
    const QByteArray iconPathUtf8 = iconPath.toUtf8();
    return mActions->createAction(objectName, iconPathUtf8.constData(), checkable, false, nullptr);
}

/**
 * @brief 主页「创建」panel 注入「信号分析」入口（插件不再占用顶级丝带标签页）
 *
 * 动作 objectName 沿用 actionSignalOpenPage，点击走 openSignalAnalysisPage()（页面惰性创建）；
 * 宿主面板按 DA::UiNames::Ribbon::MainCreatePanel 查找，找不到时只告警、不影响其它功能
 * （工程恢复等路径仍可创建页面）。卸载时只摘按钮，不删宿主 panel（见 finalize()）。
 */
void DASignalAnalysisUI::installHomeEntry()
{
    if (nullptr == mActions) {
        qWarning() << "DASignalAnalysisUI::installHomeEntry: action interface is unavailable";
        return;
    }
    actionOpenSignalPage = mActions->createAction(
        "actionSignalOpenPage",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("showSignalAnalysis.svg")).toUtf8().constData());
    DARibbonAreaInterface* ribbonArea = (mUI ? mUI->getRibbonArea() : nullptr);
    SARibbonPanel* createPanel        = (ribbonArea
                                             ? ribbonArea->getPanelByObjectName(
                                                   QString::fromUtf8(UiNames::Ribbon::MainCreatePanel))
                                             : nullptr);
    if (nullptr == createPanel) {
        qWarning() << "DASignalAnalysisUI::installHomeEntry: host main create panel not found";
        return;
    }
    createPanel->addLargeAction(actionOpenSignalPage);
}

/**
 * @brief 构建上下文页 1「分析设置」（常用分析参数）
 *
 * 该页必须排在「信号显示」页之前：宿主把上下文标签里「新出现页中的第一个」设为当前页，
 * 排前面的才是默认页。峰值相关动作在步骤 2 拆到两页：常见项留本页、标注类去显示页。
 * @param contextCategory 插件自建的上下文标签（由 buildSignalContextCategory 创建）
 */
void DASignalAnalysisUI::buildAnalysisSettingsPage(SARibbonContextCategory* contextCategory)
{
    if (nullptr == contextCategory) {
        qWarning() << "DASignalAnalysisUI::buildAnalysisSettingsPage: context category is null";
        return;
    }
    mContextAnalysisPage = contextCategory->addCategoryPage(tr("Analysis Settings"));  // cn:分析设置
    if (nullptr == mContextAnalysisPage) {
        qWarning() << "DASignalAnalysisUI::buildAnalysisSettingsPage: addCategoryPage failed";
        return;
    }
    mContextAnalysisPage->setObjectName(QString::fromUtf8(cContextAnalysisPageObjectName));

    // ---------- 导入导出：导入▾ / 导出数据▾ / 导出图片▾ ----------
    // 步骤 3 把原「信号」「导出」两个 panel 合并：导入 4 个入口收进统一「导入」菜单，
    // 与两类导出共用本 panel（导入是进入信号分析的第一步，导出是分析结果的出口）。
    mPanelImportExport = mContextAnalysisPage->addPanel(tr("Import / Export"));  // cn:导入导出
    mPanelImportExport->setObjectName(QString::fromUtf8(cPanelImportExportName));
    // 导入父动作（大按钮，点击展开菜单）
    actionImport = mActions->createAction(
        "actionSignalImport",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("import.svg")).toUtf8().constData());
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
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("relinkSource.svg")).toUtf8().constData());
    actionRelinkSource->setEnabled(false);  // 只有当前信号来源缺失时才可用
    // 导入菜单：工具提示必须可见（FFmpeg 缺失时菜单里的视频项要靠 tooltip 说明原因）
    mImportMenu = new QMenu();
    mImportMenu->setToolTipsVisible(true);
    mImportMenu->addAction(actionImportAudio);
    mImportMenu->addAction(actionImportDataFrame);
    mImportMenu->addAction(actionImportVideo);
    mImportMenu->addSeparator();
    mImportMenu->addAction(actionRelinkSource);
    actionImport->setMenu(mImportMenu);
    mPanelImportExport->addLargeAction(actionImport);
    // 导出：峰值/频谱/功率谱/热图 CSV（actionExportPeaks 的菜单在 initialize 里建）
    actionExportPeaks = mActions->createAction(
        "actionSignalExportPeaks",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("exportTable.svg")).toUtf8().constData());
    // 导出图片：整页 / 当前子图 / 复制剪贴板（actionExportImage 的菜单在 initialize 里建）
    actionExportImage = mActions->createAction(
        "actionSignalExportImage",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("exportImage.svg")).toUtf8().constData());
    mPanelImportExport->addLargeAction(actionExportPeaks);
    mPanelImportExport->addLargeAction(actionExportImage);

    // ---------- 播放：动作只绑到底部播放器控制条（D5，不再占丝带 panel） ----------
    // 播放/暂停升格为唯一动作（可勾选：勾选=正在播放），暂停动作已删除；
    // 播放器控制条由 buildSignalToolbar() 在页面创建时建好，动作先在这里创建以便 initialize 连接。
    actionPlay = mActions->createAction(
        "actionSignalPlay",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("play.svg")).toUtf8().constData(),
        true);
    actionStop = mActions->createAction(
        "actionSignalStop",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("stop.svg")).toUtf8().constData());

    // ---------- 预处理：去直流 / 去趋势（S） ----------
    // 影响频谱、热图、峰值与 dBFS 参考峰值（§5），改后按 ParamScope::Preprocess 分级重算
    mPanelPreprocess = mContextAnalysisPage->addPanel(tr("Preprocess"));  // cn:预处理
    mPanelPreprocess->setObjectName(QString::fromUtf8(cPanelPreprocessName));
    mPreprocessDcCheck = new QCheckBox(tr("Remove DC"), mPanelPreprocess);  // cn:去直流
    mPreprocessDcCheck->setObjectName(QStringLiteral("da-signal-preprocess-removedc"));
    connect(mPreprocessDcCheck, &QCheckBox::toggled, this, &DASignalAnalysisUI::onPreprocessDcToggled);
    mDetrendCombo = new QComboBox(mPanelPreprocess);
    mDetrendCombo->setObjectName(QStringLiteral("da-signal-preprocess-detrend"));
    mDetrendCombo->setMinimumWidth(96);
    mDetrendCombo->addItem(tr("None"), QStringLiteral("none"));          // cn:无
    mDetrendCombo->addItem(tr("Constant"), QStringLiteral("constant"));  // cn:常数
    mDetrendCombo->addItem(tr("Linear"), QStringLiteral("linear"));      // cn:线性
    connect(mDetrendCombo,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASignalAnalysisUI::onDetrendChanged);
    // 去直流复选框与去趋势下拉框放进同一个参数网格：复选框左边界与下拉框对齐
    auto* preprocessGrid = new DASignalRibbonParamGrid(mPanelPreprocess);
    preprocessGrid->addFieldOnly(mPreprocessDcCheck);
    mDetrendLabel = preprocessGrid->addRow(tr("Detrend"), mDetrendCombo);  // cn:去趋势
    preprocessGrid->finish();
    mPanelPreprocess->addLargeWidget(preprocessGrid);

    // ---------- 频谱：通道 / 窗函数 / 幅值口径 / 频率分辨率（S） ----------
    // 通道下拉从「信号显示」页迁入本页（D8）：通道本就是分析输入，显示页要腾宽度；
    // 幅值口径从「信号显示」页迁入本页（D7：dB 参考由幅值口径派生，不再单独入口）；
    // 频率分辨率与「时频」页的窗长联动（Δf = 1/窗长）。
    mPanelSpectrum = mContextAnalysisPage->addPanel(tr("Spectrum"));  // cn:频谱
    mPanelSpectrum->setObjectName(QString::fromUtf8(cPanelSpectrumName));
    // 通道选择：一次分析一个通道（多通道信号），单通道时置灰
    mChannelCombo = new QComboBox(mPanelSpectrum);
    mChannelCombo->setObjectName(QStringLiteral("da-signal-spectrum-channel"));
    mChannelCombo->setMinimumWidth(120);
    mChannelCombo->setToolTip(tr("Channel"));  // cn:通道
    connect(mChannelCombo,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASignalAnalysisUI::onChannelComboChanged);
    mWindowCombo = new QComboBox(mPanelSpectrum);
    mWindowCombo->setObjectName(QStringLiteral("da-signal-spectrum-window"));
    mWindowCombo->setMinimumWidth(96);
    mWindowCombo->addItem(tr("Rectangular"), QStringLiteral("rectangular"));  // cn:矩形
    mWindowCombo->addItem(tr("Hann"), QStringLiteral("hann"));                // cn:汉宁
    mWindowCombo->addItem(tr("Hamming"), QStringLiteral("hamming"));          // cn:汉明
    mWindowCombo->addItem(tr("Blackman"), QStringLiteral("blackman"));        // cn:布莱克曼
    mWindowCombo->addItem(tr("Flat top"), QStringLiteral("flattop"));         // cn:平顶
    connect(mWindowCombo,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASignalAnalysisUI::onWindowChanged);
    mAmplitudeModeCombo = new QComboBox(mPanelSpectrum);
    mAmplitudeModeCombo->setObjectName(QStringLiteral("da-signal-spectrum-amplitude"));
    mAmplitudeModeCombo->setMinimumWidth(118);
    mAmplitudeModeCombo->addItem(tr("Peak amplitude"), QStringLiteral("peak"));   // cn:峰值幅度
    mAmplitudeModeCombo->addItem(tr("RMS"), QStringLiteral("rms"));               // cn:均方根
    mAmplitudeModeCombo->addItem(tr("dB (FS)"), QStringLiteral("dbFS"));          // cn:dB（满量程）
    mAmplitudeModeCombo->addItem(tr("dB (re 1.0)"), QStringLiteral("dbRef1"));    // cn:dB（相对 1.0）
    connect(mAmplitudeModeCombo,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASignalAnalysisUI::onAmplitudeModeComboChanged);
    mResolutionSpin = new QDoubleSpinBox(mPanelSpectrum);
    mResolutionSpin->setObjectName(QStringLiteral("da-signal-spectrum-resolution"));
    mResolutionSpin->setRange(0.01, 100000.0);
    mResolutionSpin->setDecimals(2);
    mResolutionSpin->setSingleStep(1.0);
    mResolutionSpin->setSuffix(QStringLiteral(" Hz"));
    mResolutionSpin->setMinimumWidth(112);
    mResolutionSpin->setKeyboardTracking(false);  // 输入过程中不逐字符触发重算
    connect(mResolutionSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onResolutionChanged);
    // 通道 / 窗函数 / 幅值口径 / 频率分辨率四行放进同一个参数网格：标签列与控件列共享列宽
    auto* spectrumGrid  = new DASignalRibbonParamGrid(mPanelSpectrum);
    mChannelLabel       = spectrumGrid->addRow(tr("Channel"), mChannelCombo);           // cn:通道
    mWindowLabel        = spectrumGrid->addRow(tr("Window"), mWindowCombo);            // cn:窗函数
    mAmplitudeModeLabel = spectrumGrid->addRow(tr("Amplitude"), mAmplitudeModeCombo);  // cn:幅值口径
    mResolutionLabel    = spectrumGrid->addRow(tr("Resolution"), mResolutionSpin);     // cn:频率分辨率
    spectrumGrid->finish();
    mPanelSpectrum->addLargeWidget(spectrumGrid);

    // ---------- 时频：窗长 / 跳步重叠 / Δf（S） ----------
    // 窗长与频谱页的频率分辨率联动（改一个同步另一个）；Δf 是只读提示，不参与输入。
    mPanelTimeFreq = mContextAnalysisPage->addPanel(tr("Time-Frequency"));  // cn:时频
    mPanelTimeFreq->setObjectName(QString::fromUtf8(cPanelTimeFreqName));
    mStftWindowSpin = new QDoubleSpinBox(mPanelTimeFreq);
    mStftWindowSpin->setObjectName(QStringLiteral("da-signal-stft-window"));
    mStftWindowSpin->setRange(1.0, 10000.0);
    mStftWindowSpin->setDecimals(1);
    mStftWindowSpin->setSingleStep(10.0);
    mStftWindowSpin->setSuffix(QStringLiteral(" ms"));
    mStftWindowSpin->setMinimumWidth(108);
    mStftWindowSpin->setKeyboardTracking(false);
    connect(mStftWindowSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onStftWindowChanged);
    mStftOverlapSpin = new QDoubleSpinBox(mPanelTimeFreq);
    mStftOverlapSpin->setObjectName(QStringLiteral("da-signal-stft-overlap"));
    mStftOverlapSpin->setRange(0.0, 95.0);
    mStftOverlapSpin->setDecimals(0);
    mStftOverlapSpin->setSingleStep(5.0);
    mStftOverlapSpin->setSuffix(QStringLiteral(" %"));
    mStftOverlapSpin->setMinimumWidth(96);
    mStftOverlapSpin->setKeyboardTracking(false);
    connect(mStftOverlapSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onStftOverlapChanged);
    mDeltaFLabel = new QLabel(mPanelTimeFreq);
    mDeltaFLabel->setObjectName(QStringLiteral("da-signal-stft-deltaf"));
    mDeltaFLabel->setText(QStringLiteral("Δf —"));
    // 窗长 / 跳步重叠 / Δf 提示放进同一个参数网格（Δf 是跨两列的只读提示）
    auto* timeFreqGrid = new DASignalRibbonParamGrid(mPanelTimeFreq);
    mStftWindowLabel   = timeFreqGrid->addRow(tr("Window length"), mStftWindowSpin);  // cn:窗长
    mStftOverlapLabel  = timeFreqGrid->addRow(tr("Hop overlap"), mStftOverlapSpin);   // cn:跳步重叠
    timeFreqGrid->addHint(mDeltaFLabel);
    timeFreqGrid->finish();
    mPanelTimeFreq->addLargeWidget(timeFreqGrid);

    // ---------- Peaks：峰值（全项目唯一峰值入口，D13） ----------
    // 峰值相关入口全部在本面板，不再拆到「信号显示」页：
    // 峰值标记（L）/ 标记数量（L）/ 模式+相对门限（参数网格）/ 标记显示（S）/ 峰值窗口（L）。
    mPanelPeaks = mContextAnalysisPage->addPanel(tr("Peaks"));  // cn:峰值
    mPanelPeaks->setObjectName(QString::fromUtf8(cPanelPeaksName));
    // 峰值标记：显示/隐藏频谱图上的峰值标记（状态源仍是 mPeakMarkersVisible）
    actionPeakMarker = createRibbonIconAction("actionSignalPeakMarker", signalPluginIcon("peaks.svg"), true);
    connect(actionPeakMarker, &QAction::toggled, this, &DASignalAnalysisUI::onPeakMarkerToggled);
    mPanelPeaks->addLargeAction(actionPeakMarker);
    actionPeakTopN = createRibbonIconAction("actionSignalPeakTopN", signalPluginIcon("peakCount.svg"), false);
    mPanelPeaks->addLargeAction(actionPeakTopN);
    // 峰值查找：模式 / 相对门限（参数网格，标签列与控件列共享列宽；标签随语言切换重译）
    mPeakModeCombo = new QComboBox(mPanelPeaks);
    mPeakModeCombo->setObjectName(QStringLiteral("da-signal-peak-mode"));
    mPeakModeCombo->setMinimumWidth(84);
    mPeakModeCombo->addItem(tr("Simple"), QStringLiteral("simple"));      // cn:简单
    mPeakModeCombo->addItem(tr("Advanced"), QStringLiteral("advanced"));  // cn:高级
    connect(mPeakModeCombo,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASignalAnalysisUI::onPeakModeChanged);
    mPeakRelDbSpin = new QDoubleSpinBox(mPanelPeaks);
    mPeakRelDbSpin->setObjectName(QStringLiteral("da-signal-peak-reldb"));
    mPeakRelDbSpin->setRange(0.0, 200.0);
    mPeakRelDbSpin->setDecimals(1);
    mPeakRelDbSpin->setSingleStep(5.0);
    mPeakRelDbSpin->setSuffix(QStringLiteral(" dB"));
    mPeakRelDbSpin->setMinimumWidth(96);
    // 关闭键盘跟踪：输入过程中不逐字符触发重算，回车/失焦或点上下箭头时才提交
    mPeakRelDbSpin->setKeyboardTracking(false);
    connect(mPeakRelDbSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onPeakRelDbChanged);
    // 模式 / 相对门限两行放进同一个参数网格：标签列与控件列共享列宽
    auto* peaksGrid = new DASignalRibbonParamGrid(mPanelPeaks);
    mPeakModeLabel  = peaksGrid->addRow(tr("Mode"), mPeakModeCombo);       // cn:模式
    mPeakRelDbLabel = peaksGrid->addRow(tr("Threshold"), mPeakRelDbSpin);  // cn:相对门限
    peaksGrid->finish();
    mPanelPeaks->addLargeWidget(peaksGrid);
    // 标记显示：半高按钮，弹出峰值标记文本模板编辑对话框（刻意保持 Small 尺寸，D13）
    actionPeakMarkerDisplay =
        createRibbonIconAction("actionSignalPeakMarkerDisplay", signalPluginIcon("peakLabel.svg"), false);
    connect(actionPeakMarkerDisplay, &QAction::triggered, this, &DASignalAnalysisUI::openPeakMarkerDisplaySettings);
    mPanelPeaks->addSmallAction(actionPeakMarkerDisplay);
    // 峰值窗口：峰值表 + 每帧前 N 曲线停靠窗口的唯一开关
    actionPeakWindow = createRibbonIconAction("actionSignalPeakWindow", signalPluginIcon("peaksWindow.svg"), true);
    connect(actionPeakWindow, &QAction::toggled, this, &DASignalAnalysisUI::onPeakWindowToggled);
    mPanelPeaks->addLargeAction(actionPeakWindow);
    buildPeakTopNMenu();

    // ---------- 更多：高级设置…（对话框唯一入口） ----------
    // 常用参数已全部上丝带，本 panel 只留「高级设置…」打开对话框（零填充/功率谱/峰值高级/标定）。
    mPanelMore = mContextAnalysisPage->addPanel(tr("More"));  // cn:更多
    mPanelMore->setObjectName(QString::fromUtf8(cPanelMoreName));
    actionAnalysisSetting = mActions->createAction(
        "actionSignalAnalysisSetting",
        (QString::fromUtf8(cIconPrefix) + QStringLiteral("analysisSetting.svg")).toUtf8().constData());
    mPanelMore->addLargeAction(actionAnalysisSetting);
}

/**
 * @brief 构建插件自建的 Ribbon 上下文标签（「分析设置」「信号显示」两页）
 *
 * 上下文标签只在信号分析页 dock 激活时显示（provider 调 setSignalContextCategoryVisible），
 * objectName 固定为 cContextCategoryObjectName，供宿主接口按名字 show/hide。
 * 「信号显示」页按对象分组（时频图设置 / 云图设置 / 瀑布图设置），不再把显示项堆进一个面板；
 * 该页单独开启面板标题（D9），样式切换后由 ribbonStyleChanged 重新下发。
 */
void DASignalAnalysisUI::buildSignalContextCategory()
{
    DARibbonAreaInterface* ribbonArea = (mUI ? mUI->getRibbonArea() : nullptr);
    SARibbonBar* ribbon               = (ribbonArea ? ribbonArea->ribbonBar() : nullptr);
    if (nullptr == ribbon) {
        qWarning() << "DASignalAnalysisUI::buildSignalContextCategory: ribbon bar is unavailable";
        return;
    }
    // 图标路径辅助：插件自带图标（文件列表见 icon/）
    auto pluginIcon = [](const char* file) -> QString {
        return QString::fromUtf8(cIconPrefix) + QString::fromUtf8(file);
    };
    // 经宿主动作管理器创建带图标的动作（checkable 动作初始未勾选，状态由同步逻辑回填）
    auto createIconAction = [this](const char* objName, const QString& iconPath, bool checkable) -> QAction* {
        return createRibbonIconAction(objName, iconPath, checkable);
    };
    // 色带菜单：QwtColorMapPreset::availablePresets() 动态填充。二维云图与三维瀑布各一份，
    // 两者是独立视图状态（D12），不能共用同一个勾选组
    auto makeColormapMenu = [this](QActionGroup** groupOut) -> QMenu* {
        QMenu* menu = new QMenu();
        auto* group = new QActionGroup(this);
        group->setExclusive(true);
        const QStringList presets = QwtColorMapPreset::availablePresets();
        for (const QString& preset : presets) {
            QAction* presetAction = menu->addAction(preset);
            presetAction->setCheckable(true);
            presetAction->setData(preset);
            presetAction->setActionGroup(group);
            // 渐变预览：菜单项直接显示该预设的真实色带
            presetAction->setIcon(makeColormapPreviewIcon(preset));
            if (preset == QLatin1String("viridis")) {
                presetAction->setChecked(true);  // 默认色带与视图默认一致
            }
        }
        *groupOut = group;
        return menu;
    };

    mContextCategory = ribbon->addContextCategory(tr("Signal Analysis"), QColor(), QVariant());  // cn:信号分析
    if (nullptr == mContextCategory) {
        qWarning() << "DASignalAnalysisUI::buildSignalContextCategory: addContextCategory failed";
        return;
    }
    mContextCategory->setObjectName(QString::fromUtf8(cContextCategoryObjectName));
    // 页 1「分析设置」必须排在最前：showContextCategory 会把「新出现页中的第一个」设为当前页
    buildAnalysisSettingsPage(mContextCategory);
    mContextDisplayPage = mContextCategory->addCategoryPage(tr("Signal Display"));  // cn:信号显示
    if (nullptr == mContextDisplayPage) {
        qWarning() << "DASignalAnalysisUI::buildSignalContextCategory: addCategoryPage failed";
        return;
    }
    mContextDisplayPage->setObjectName(QString::fromUtf8(cContextDisplayPageObjectName));
    // D9：只给本页开启面板标题（SARibbonBar 在两行/单行紧凑样式下会强制关闭全部页的标题，
    // 样式切换后由 ribbonStyleChanged 重新下发，见 applyDisplayPagePanelTitles()）
    mContextDisplayPage->setEnableShowPanelTitle(true);

    // ---------- 时频图设置：显示模式 + 当前帧标记 ----------
    // 二维/三维/隐藏互斥：右下单元格在二维云图与三维瀑布之间切换（D12）
    mPanelTimeFreqView = mContextDisplayPage->addPanel(tr("Time-Frequency View"));  // cn:时频图设置
    mPanelTimeFreqView->setObjectName(QString::fromUtf8(cPanelContextTimeFreqViewName));
    mWaterfallModeGroup = new QActionGroup(this);
    mWaterfallModeGroup->setExclusive(true);
    actionWaterfall2D = createIconAction("actionSignalWaterfall2D", pluginIcon("waterfall2d.svg"), true);
    actionWaterfall3D = createIconAction("actionSignalWaterfall3D", pluginIcon("waterfall3d.svg"), true);
    actionWaterfallHide = createIconAction("actionSignalWaterfallHide", pluginIcon("waterfallHide.svg"), true);
    actionWaterfall2D->setActionGroup(mWaterfallModeGroup);
    actionWaterfall3D->setActionGroup(mWaterfallModeGroup);
    actionWaterfallHide->setActionGroup(mWaterfallModeGroup);
    actionWaterfall2D->setChecked(true);  // 默认二维（D12）
    mPanelTimeFreqView->addMediumAction(actionWaterfall2D);
    mPanelTimeFreqView->addMediumAction(actionWaterfall3D);
    mPanelTimeFreqView->addMediumAction(actionWaterfallHide);
    // 当前帧标记：二维云图的半透明虚线竖线 + 三维瀑布的当前迹线高亮，独立开关（默认开）
    actionTimeCursor = createIconAction("actionSignalTimeCursor", signalAppIcon("chart-vline-marker.svg"), true);
    connect(actionTimeCursor, &QAction::toggled, this, &DASignalAnalysisUI::onTimeCursorToggled);
    mPanelTimeFreqView->addLargeAction(actionTimeCursor);
    // 测量 ▾：联动缩放（双光标/峰值保持在后续步骤加入同一菜单）
    actionMeasure = createIconAction("actionSignalMeasure", pluginIcon("measure.svg"), false);
    mMeasureMenu  = new QMenu();
    actionLinkedZoom = mMeasureMenu->addAction(tr("Linked zoom"));  // cn:联动缩放
    actionLinkedZoom->setObjectName(QStringLiteral("actionSignalLinkedZoom"));
    actionLinkedZoom->setCheckable(true);
    connect(actionLinkedZoom, &QAction::toggled, this, &DASignalAnalysisUI::onLinkedZoomToggled);
    actionDualCursor = mMeasureMenu->addAction(tr("Dual cursors"));  // cn:双光标测量
    actionDualCursor->setObjectName(QStringLiteral("actionSignalDualCursor"));
    actionDualCursor->setCheckable(true);
    connect(actionDualCursor, &QAction::toggled, this, &DASignalAnalysisUI::onDualCursorToggled);
    actionMeasure->setMenu(mMeasureMenu);
    mPanelTimeFreqView->addLargeAction(actionMeasure);
    // 视图 ▾：时间格式 / 布局比例 / 重置显示（都只影响显示，不重算分析）
    actionViewOptions = createIconAction("actionSignalViewOptions", pluginIcon("viewOptions.svg"), false);
    mViewOptionsMenu  = new QMenu();
    mTimeFormatMenu   = mViewOptionsMenu->addMenu(tr("Time format"));  // cn:时间格式
    mTimeFormatGroup  = new QActionGroup(this);
    mTimeFormatGroup->setExclusive(true);
    // 时间格式三态：0=秒 / 1=毫秒 / 2=mm:ss.s（data 里存格式编号）
    const QStringList timeFormatNames = { tr("Seconds"), tr("Milliseconds"), tr("mm:ss.s") };
    // cn:秒 / 毫秒 / 分:秒.十分位
    for (int i = 0; i < timeFormatNames.size(); ++i) {
        QAction* act = mTimeFormatMenu->addAction(timeFormatNames.at(i));
        act->setCheckable(true);
        act->setData(i);
        act->setActionGroup(mTimeFormatGroup);
        mTimeFormatActions.append(act);
    }
    connect(mTimeFormatGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onTimeFormatTriggered);
    mLayoutPresetMenu = mViewOptionsMenu->addMenu(tr("Layout"));  // cn:布局比例
    // 布局比例预设：0=1:1 / 1=波形优先 / 2=分析优先 / 3=恢复默认（data 里存预设编号）
    const QStringList layoutPresetNames = { tr("1:1"), tr("Waveform priority"), tr("Analysis priority"), tr("Restore default") };
    // cn:1:1 / 波形优先 / 分析优先 / 恢复默认
    for (int i = 0; i < layoutPresetNames.size(); ++i) {
        QAction* act = mLayoutPresetMenu->addAction(layoutPresetNames.at(i));
        act->setData(i);
        mLayoutPresetActions.append(act);
    }
    connect(mLayoutPresetMenu, &QMenu::triggered, this, &DASignalAnalysisUI::onLayoutPresetTriggered);
    mViewOptionsMenu->addSeparator();
    actionDisplayReset = mViewOptionsMenu->addAction(tr("Reset display"));  // cn:重置显示
    actionDisplayReset->setObjectName(QStringLiteral("actionSignalDisplayReset"));
    connect(actionDisplayReset, &QAction::triggered, this, &DASignalAnalysisUI::resetDisplaySettings);
    actionViewOptions->setMenu(mViewOptionsMenu);
    mPanelTimeFreqView->addMediumAction(actionViewOptions);

    // ---------- 云图设置：色带 / 色标（只影响二维时频热图） ----------
    mPanelHeatmap = mContextDisplayPage->addPanel(tr("Heatmap"));  // cn:云图设置
    mPanelHeatmap->setObjectName(QString::fromUtf8(cPanelContextHeatmapName));
    actionColormap = createIconAction("actionSignalColormap", pluginIcon("colorMap.svg"), false);
    mColormapMenu  = makeColormapMenu(&mColormapGroup);
    connect(mColormapGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onContextColormapTriggered);
    actionColormap->setMenu(mColormapMenu);
    mPanelHeatmap->addLargeAction(actionColormap);
    // 色标（colorbar）显隐：时频热图右侧的色标轴，紧挨「色带」放置；只影响显示，不重算
    actionColorBar = createIconAction("actionSignalColorBar", pluginIcon("colorBar.svg"), true);
    actionColorBar->setChecked(mColorBarVisible);  // 初始勾选跟随已加载的偏好
    connect(actionColorBar, &QAction::toggled, this, &DASignalAnalysisUI::onColorBarToggled);
    mPanelHeatmap->addLargeAction(actionColorBar);
    // dB 范围：二维色域下限 [floor, 0]（D5：从窗口底部播放器控制条迁入，显示设置归显示页）
    mHeatmapDbFloorSpin = new QDoubleSpinBox(mPanelHeatmap);
    mHeatmapDbFloorSpin->setObjectName(QStringLiteral("da-signal-heatmap-dbfloor"));
    mHeatmapDbFloorSpin->setRange(-200.0, 0.0);
    mHeatmapDbFloorSpin->setDecimals(0);
    mHeatmapDbFloorSpin->setSingleStep(5.0);
    mHeatmapDbFloorSpin->setValue(-80.0);
    mHeatmapDbFloorSpin->setSuffix(QStringLiteral(" dB"));
    mHeatmapDbFloorSpin->setMinimumWidth(96);
    mHeatmapDbFloorSpin->setKeyboardTracking(false);  // 输入过程中不逐字符重绘
    connect(mHeatmapDbFloorSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onHeatmapDbFloorChanged);
    auto* heatmapGrid      = new DASignalRibbonParamGrid(mPanelHeatmap);
    mHeatmapDbFloorLabel   = heatmapGrid->addRow(tr("dB"), mHeatmapDbFloorSpin);
    heatmapGrid->finish();
    mPanelHeatmap->addLargeWidget(heatmapGrid);
    // 显示 ▾：图像/等高线/图像+等高线 与 最近邻/双线性（只影响二维云图的绘制方式）
    actionHeatmapDisplay =
        createIconAction("actionSignalHeatmapDisplay", QStringLiteral(":/DAGui/icon/chart-type/chart-spectrogram.svg"), false);
    mHeatmapDisplayMenu       = new QMenu();
    mHeatmapDisplayModeGroup  = new QActionGroup(this);
    mHeatmapDisplayModeGroup->setExclusive(true);
    mHeatmapDisplayMenu->addSection(tr("Display mode"));  // cn:显示模式
    mHeatmapDisplayModeSection = mHeatmapDisplayMenu->actions().last();
    const QStringList displayModeNames = { tr("Image"), tr("Contour"), tr("Image + Contour") };
    // cn:图像 / 等高线 / 图像+等高线
    for (int i = 0; i < displayModeNames.size(); ++i) {
        QAction* act = mHeatmapDisplayMenu->addAction(displayModeNames.at(i));
        act->setCheckable(true);
        act->setData(i);
        act->setActionGroup(mHeatmapDisplayModeGroup);
        mHeatmapDisplayModeActions.append(act);
    }
    mHeatmapResampleGroup = new QActionGroup(this);
    mHeatmapResampleGroup->setExclusive(true);
    mHeatmapDisplayMenu->addSection(tr("Interpolation"));  // cn:插值
    mHeatmapResampleSection = mHeatmapDisplayMenu->actions().last();
    const QStringList resampleNames = { tr("Nearest neighbour"), tr("Bilinear") };
    // cn:最近邻 / 双线性
    for (int i = 0; i < resampleNames.size(); ++i) {
        QAction* act = mHeatmapDisplayMenu->addAction(resampleNames.at(i));
        act->setCheckable(true);
        act->setData(i);
        act->setActionGroup(mHeatmapResampleGroup);
        mHeatmapResampleActions.append(act);
    }
    connect(mHeatmapDisplayModeGroup,
            &QActionGroup::triggered,
            this,
            &DASignalAnalysisUI::onHeatmapDisplayModeTriggered);
    connect(mHeatmapResampleGroup, &QActionGroup::triggered, this, &DASignalAnalysisUI::onHeatmapResampleTriggered);
    actionHeatmapDisplay->setMenu(mHeatmapDisplayMenu);
    mPanelHeatmap->addMediumAction(actionHeatmapDisplay);

    // ---------- 瀑布图设置：色带 / dB / 当前迹线高亮（只影响三维瀑布） ----------
    // 三维瀑布有自己独立的色带与 dB 色域（D12），与二维云图互不影响
    mPanelWaterfall = mContextDisplayPage->addPanel(tr("Waterfall"));  // cn:瀑布图设置
    mPanelWaterfall->setObjectName(QString::fromUtf8(cPanelContextWaterfallName));
    // 瀑布设置…：切右侧 3D 设置面板并定位到瀑布图元（详细样式在那里改）
    actionWaterfallSettings =
        createIconAction("actionSignalWaterfallSettings", pluginIcon("analysisSetting.svg"), false);
    connect(actionWaterfallSettings,
            &QAction::triggered,
            this,
            &DASignalAnalysisUI::onWaterfallSettingsRequested);
    mPanelWaterfall->addLargeAction(actionWaterfallSettings);
    actionWaterfallColormap =
        createIconAction("actionSignalWaterfallColormap", pluginIcon("colorMap.svg"), false);
    mWaterfallColormapMenu  = makeColormapMenu(&mWaterfallColormapGroup);
    connect(mWaterfallColormapGroup,
            &QActionGroup::triggered,
            this,
            &DASignalAnalysisUI::onWaterfallColormapTriggered);
    actionWaterfallColormap->setMenu(mWaterfallColormapMenu);
    mPanelWaterfall->addLargeAction(actionWaterfallColormap);
    // dB 范围：三维固定色域 [floor, 0]（与二维云图的 dB 相互独立）
    mWaterfallDbFloorSpin = new QDoubleSpinBox(mPanelWaterfall);
    mWaterfallDbFloorSpin->setObjectName(QStringLiteral("da-signal-waterfall-dbfloor"));
    mWaterfallDbFloorSpin->setRange(-200.0, 0.0);
    mWaterfallDbFloorSpin->setDecimals(0);
    mWaterfallDbFloorSpin->setSingleStep(5.0);
    mWaterfallDbFloorSpin->setValue(-80.0);
    mWaterfallDbFloorSpin->setSuffix(QStringLiteral(" dB"));
    mWaterfallDbFloorSpin->setMinimumWidth(96);
    mWaterfallDbFloorSpin->setKeyboardTracking(false);
    connect(mWaterfallDbFloorSpin,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASignalAnalysisUI::onWaterfallDbFloorChanged);
    auto* waterfallGrid        = new DASignalRibbonParamGrid(mPanelWaterfall);
    mWaterfallDbFloorLabel     = waterfallGrid->addRow(tr("dB"), mWaterfallDbFloorSpin);
    waterfallGrid->finish();
    mPanelWaterfall->addLargeWidget(waterfallGrid);
    // 当前迹线高亮：三维瀑布里把当前帧对应的迹线着色（单色着色时无效并置灰）
    actionWaterfallHighlight =
        createIconAction("actionSignalWaterfallHighlight", pluginIcon("waterfallHighlight.svg"), true);
    connect(actionWaterfallHighlight, &QAction::toggled, this, &DASignalAnalysisUI::onWaterfallHighlightToggled);
    mPanelWaterfall->addLargeAction(actionWaterfallHighlight);

    // 初始不显示：由 provider 在信号分析页激活时显示
    ribbon->hideContextCategory(mContextCategory);
}

/**
 * @brief 给「信号显示」页重新下发面板标题开关（D9 spike）
 *
 * SARibbonBar::setRibbonStyle() 会按样式统一改写所有页的面板标题（两行/单行样式强制关闭），
 * 因此样式切换后必须把本页的开关再打开一次；页不存在时是安全的空操作。
 */
void DASignalAnalysisUI::applyDisplayPagePanelTitles()
{
    if (mContextDisplayPage) {
        mContextDisplayPage->setEnableShowPanelTitle(true);
    }
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
 * @brief 构建窗口底部播放器控制条
 *
 * 顺序：跟随播放 → ▶/⏸ → ■ → 时间滑块 → 时间文本 → 自动滚动。
 * 只留播放相关控件：dB 范围已归「信号显示 → 云图设置」（D5），显示设置不再留在播放条上。
 * 只在信号分析页 dock 激活时显示（provider 调 setSignalToolbarVisible）；
 * 控件只是当前视图/播放器状态的视图层：控件变化写回视图，状态变化经 syncSignalToolbar() 回填。
 * ▶/⏸ 按钮绑 actionPlay（可勾选 = 正在播放），■ 绑 actionStop；时间文本是纯函数格式化结果。
 */
void DASignalAnalysisUI::buildSignalToolbar()
{
    if (mSignalToolbar || nullptr == mUI) {
        return;
    }
    QMainWindow* mainWindow = mUI->getMainWindow();
    if (nullptr == mainWindow) {
        qWarning() << "DASignalAnalysisUI::buildSignalToolbar: main window is unavailable";
        return;
    }
    mSignalToolbar = new QToolBar(mainWindow);
    mSignalToolbar->setObjectName(QString::fromUtf8(cToolbarObjectName));
    mSignalToolbar->setMovable(false);
    mSignalToolbar->setFloatable(false);

    // 跟随播放：频谱/峰值表显示当前帧而不是整段
    mToolbarFollowCheck = new QCheckBox(mSignalToolbar);
    mToolbarFollowCheck->setObjectName(QStringLiteral("da-signal-toolbar-follow"));
    mToolbarFollowCheck->setText(tr("Follow playback"));  // cn:跟随播放
    mToolbarFollowCheck->setToolTip(tr("Show the current frame while playback advances"));
    // cn:播放推进时显示当前帧
    connect(mToolbarFollowCheck, &QCheckBox::toggled, this, [this](bool on) {
        if (DASignalAnalysisView* view = currentView()) {
            view->setFollowSlider(on);
        }
    });
    mSignalToolbar->addWidget(mToolbarFollowCheck);
    mSignalToolbar->addSeparator();

    // 播放/暂停（单切换按钮，勾选=正在播放）与停止：动作是唯一状态源，
    // 按钮经 setDefaultAction 跟随动作的 enabled/checked/icon/text
    mToolbarPlayButton = new QToolButton(mSignalToolbar);
    mToolbarPlayButton->setObjectName(QStringLiteral("da-signal-toolbar-play"));
    mToolbarPlayButton->setDefaultAction(actionPlay);
    mToolbarPlayButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mSignalToolbar->addWidget(mToolbarPlayButton);
    mToolbarStopButton = new QToolButton(mSignalToolbar);
    mToolbarStopButton->setObjectName(QStringLiteral("da-signal-toolbar-stop"));
    mToolbarStopButton->setDefaultAction(actionStop);
    mToolbarStopButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mSignalToolbar->addWidget(mToolbarStopButton);
    mSignalToolbar->addSeparator();

    // 时间滑块：毫秒范围（0..信号时长），与视图 currentTime/时间原点口径一致
    mToolbarTimeSlider = new QSlider(Qt::Horizontal, mSignalToolbar);
    mToolbarTimeSlider->setObjectName(QStringLiteral("da-signal-toolbar-time"));
    mToolbarTimeSlider->setRange(0, 1000);
    mToolbarTimeSlider->setMinimumWidth(180);
    mToolbarTimeSlider->setToolTip(tr("Playback position"));  // cn:播放位置
    connect(mToolbarTimeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (DASignalAnalysisView* view = currentView()) {
            view->setCurrentTime(value / 1000.0);
        }
    });
    mSignalToolbar->addWidget(mToolbarTimeSlider);

    // 时间文本：当前位置 / 总时长（mm:ss.s，无源显示 --:--.-）
    mToolbarTimeLabel = new QLabel(mSignalToolbar);
    mToolbarTimeLabel->setObjectName(QStringLiteral("da-signal-toolbar-timetext"));
    mToolbarTimeLabel->setToolTip(tr("Playback position / total duration"));  // cn:播放位置 / 总时长
    mSignalToolbar->addWidget(mToolbarTimeLabel);
    mSignalToolbar->addSeparator();

    // 自动滚动：热图随播放推进水平滚动
    mToolbarAutoScrollCheck = new QCheckBox(mSignalToolbar);
    mToolbarAutoScrollCheck->setObjectName(QStringLiteral("da-signal-toolbar-autoscroll"));
    mToolbarAutoScrollCheck->setText(tr("Auto scroll"));  // cn:自动滚动
    mToolbarAutoScrollCheck->setToolTip(tr("Scroll the time-frequency view while playback advances"));
    // cn:播放推进时滚动时频视图
    connect(mToolbarAutoScrollCheck, &QCheckBox::toggled, this, [this](bool on) {
        if (DASignalAnalysisView* view = currentView()) {
            view->setHeatmapAutoScroll(on);
        }
    });
    mSignalToolbar->addWidget(mToolbarAutoScrollCheck);

    mainWindow->addToolBar(Qt::BottomToolBarArea, mSignalToolbar);
    mSignalToolbar->setVisible(false);  // 只在信号分析页激活时由 provider 显示
    syncSignalToolbar();
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
    const bool hasView          = (view != nullptr);
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
        const QString waterfallColormap     = (view ? view->waterfallColormapName() : QString());
        const QList< QAction* > presets     = mWaterfallColormapMenu->actions();
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
        const int colorMode    = (view ? view->waterfallColorMode() : -1);
        const bool solidMode   = (colorMode == 2);  // Qwt3DWaterfall::Solid
        actionWaterfallHighlight->setEnabled(waterfallActive && !solidMode);
        actionWaterfallHighlight->setChecked(view ? view->isWaterfallHighlighted() : true);
    }
    // 峰值标记（视图无 getter：以 UI 记录的开关状态为准；动作在「分析设置 → 峰值」panel）
    if (actionPeakMarker) {
        const QSignalBlocker blocker(actionPeakMarker);
        actionPeakMarker->setEnabled(hasView);
        actionPeakMarker->setChecked(view && mPeakMarkersVisible);
    }
    // 分析设置页参数控件（预处理/频谱/时频/峰值）跟随当前信号参数
    syncParamsRibbonControls();
    // 当前 2D 子图（「点击波形即定位」互斥用）
    DAChartWidget* chart = (view ? view->currentChart() : nullptr);
    const bool chartAvailable = (chart != nullptr);
    // 「点击波形即定位」与交互工具互斥（SF-12）：缩放/平移/十字线任一启用、或双光标测量开启时
    // 关闭，避免按下拖动/点放游标时播放头跳位；全关时恢复。交互工具由内置「绘图操作」上下文
    // 标签负责开关，本插件只读取当前子图状态
    if (view) {
        const bool toolEnabled =
            chartAvailable && (chart->isZoomEnabled() || chart->isPanEnabled() || chart->isCrosshairEnabled());
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
        connect(chart, &DAChartWidget::chartPropertiesChanged, this, [this](DAChartWidget* changedChart,
                                                                           DAChartWidget::ChartPropertyChangeFlags flag) {
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
        const qint64 durationMs = (sig && sig->duration() > 0.0)
                                      ? qBound< qint64 >(0,
                                                         qRound64(sig->duration() * 1000.0),
                                                         std::numeric_limits< int >::max())
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
        actionPlay->setIcon(QIcon(QString::fromUtf8(cIconPrefix)
                                  + QLatin1String(playing ? "pause.svg" : "play.svg")));
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
    const bool hasSource = (mPlayer && mPlayer->hasSource());
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
    mColorBarVisible = true;
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
    // 清空后无当前视图：上下文标签/底部工具栏回到空状态
    mPeakMarkersVisible = false;
    syncSignalContextControls();
    syncSignalToolbar();
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
    // 工程清空后无当前视图：上下文标签/底部工具栏回到空状态（页面从未创建时也是安全空操作）
    syncSignalContextControls();
    syncSignalToolbar();
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
        oldWaterfallMode     = oldView->waterfallMode();
        oldColormap          = oldView->colormapName();
        oldDbFloor           = oldView->dbFloor();
        oldAutoScroll        = oldView->isHeatmapAutoScroll();
        oldWaterfallColormap = oldView->waterfallColormapName();
        oldWaterfallDbFloor  = oldView->waterfallDbFloor();
        oldWaterfallHighlight = oldView->isWaterfallHighlighted();
        oldWaterfallStyle     = oldView->waterfallStyle();
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
 * @brief 构建「标记数量」下拉菜单（10/20/50，互斥）
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
 * @brief 导出整页（2D 网格 + 三维帧缓冲合帧）为 PNG
 *
 * 整页位图走 DASignalChartExport::grabWholePage：三维瀑布可见时把 OpenGL 帧缓冲
 * 按子图位置合成到页面位图（沿用工程评审 I11 的 grabForExport 思路）。
 */
void DASignalAnalysisUI::exportWholePageImage()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    const QString pngFilter = tr("PNG Image (*.png)");  // cn:PNG 图片 (*.png)
    QString path = askSavePath(mPage, tr("Export Whole Page"), pngFilter);  // cn:导出整页
    if (path.isEmpty()) {
        return;
    }
    // 整页固定 PNG：按过滤器补齐后缀（F-07，与子图导出同一段规范化逻辑；不动 askSavePath
    // 本体，避免影响 CSV 等其它调用方）
    path = DAFigureWidget::applyExportFilterSuffix(path, pngFilter);
    if (DASignalChartExport::saveWholePage(view, path)) {
        mUI->addInfoLogMessage(tr("Whole page exported: %1").arg(path));  // cn:整页已导出：%1
    } else {
        reportError(tr("Failed to export the whole page."));  // cn:整页导出失败。
    }
}

/**
 * @brief 导出当前子图（2D 支持 PNG/SVG/PDF；三维子图本期固定 PNG）
 *
 * 后缀口径与绘图区「导出当前子图」一致：用户输入未知后缀时按所选过滤器补真实后缀，
 * 避免「内容与后缀不一致」。3D 子图激活时文件对话框过滤器收窄为 PNG（SF-14）：
 * qwt3d 提供 saveVector（gl2ps 已编入）但依赖 GL 上下文、目标环境可用性未验证，
 * 本期固定 PNG，矢量导出留作后续项。降级分支保留作防御：万一拿到非 PNG 请求
 * （如旧工程/程序化调用），改存 PNG 并给用户可见提示（不只写开发日志）。
 */
void DASignalAnalysisUI::exportCurrentSubChartImage()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    // 3D 子图只有 PNG 位图路径：过滤器收窄，用户从一开始就选不到 SVG/PDF（SF-14）
    const bool is3D = view->is3DChartActive();
    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(mPage,
                                                tr("Export Sub-chart"),  // cn:导出当前子图
                                                QStringLiteral("chart.png"),
                                                is3D ? tr("PNG Image (*.png)")  // cn:PNG 图片 (*.png)
                                                     : DAFigureWidget::exportFileFilter(),
                                                &selectedFilter);
    if (path.isEmpty()) {
        return;
    }
    // 以所选过滤器为权威修正后缀（SF-05）：默认名 chart.png 在用户切到 SVG/PDF 过滤器后
    // 仍保留 .png，只按后缀推断格式会「选 SVG 却存成 PNG」；统一走框架侧助手收口
    path = DAFigureWidget::applyExportFilterSuffix(path, selectedFilter);
    DAFigureExportFormat format = DAFigureWidget::exportFormatFromFilePath(path);
    if (is3D && format != DAFigureExportFormat::Png) {
        // 防御分支：3D 子图仅支持 PNG 位图导出，改存 PNG 并给用户可见提示（SF-14）
        const QFileInfo fi(path);
        path   = fi.path().isEmpty() ? fi.completeBaseName() + QStringLiteral(".png")
                                     : fi.path() + QLatin1Char('/') + fi.completeBaseName() + QStringLiteral(".png");
        format = DAFigureExportFormat::Png;
        qWarning() << "DASignalAnalysisUI::exportCurrentSubChartImage: 3D sub-chart supports PNG export "
                      "only, saved as PNG:"
                   << path;
        mUI->addWarningLogMessage(
            tr("3D sub-chart supports PNG export only; saved as PNG."));  // cn:三维子图仅支持 PNG 导出，已按 PNG 保存。
    }
    if (DASignalChartExport::saveCurrentSubChart(view, path, format)) {
        mUI->addInfoLogMessage(tr("Sub-chart exported: %1").arg(path));  // cn:当前子图已导出：%1
    } else {
        reportError(tr("Failed to export the sub-chart."));  // cn:当前子图导出失败。
    }
}

/**
 * @brief 复制整页（2D + 三维合帧）到剪贴板
 */
void DASignalAnalysisUI::copyWholePageToClipboard()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    if (DASignalChartExport::copyWholePageToClipboard(view)) {
        mUI->addInfoLogMessage(tr("Whole page copied to the clipboard."));  // cn:整页已复制到剪贴板。
    } else {
        reportError(tr("Failed to copy the whole page to the clipboard."));  // cn:复制整页到剪贴板失败。
    }
}

/**
 * @brief 复制当前子图（二维或三维位图）到剪贴板
 */
void DASignalAnalysisUI::copyCurrentSubChartToClipboard()
{
    DASignalAnalysisView* view = currentView();
    if (!view) {
        reportError(tr("No signal is selected."));  // cn:未选中信号。
        return;
    }
    if (DASignalChartExport::copyCurrentSubChartToClipboard(view)) {
        mUI->addInfoLogMessage(tr("Current sub-chart copied to the clipboard."));  // cn:当前子图已复制到剪贴板。
    } else {
        reportError(tr("Failed to copy the current sub-chart to the clipboard."));
        // cn:复制当前子图到剪贴板失败。
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
            // 会话内唯一 id 持久化（BLK-02）：恢复时回填，figures.xml 的 signal-id
            // 存档键按它精确匹配，同名信号不串档；旧工程无该字段走 signal-name 回退
            o[ QStringLiteral("signalId") ] = double(sig->signalId());
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
            // 视图状态（P1）：瀑布模式、色带/色标、网格比例、热图跟随
            if (DASignalAnalysisView* v = mPage ? mPage->viewForSignal(sig.get()) : nullptr) {
                view[ QStringLiteral("waterfallMode") ] = v->waterfallMode();
                view[ QStringLiteral("colormap") ]      = v->colormapName();
                view[ QStringLiteral("dbFloor") ]       = v->dbFloor();
                view[ QStringLiteral("autoScroll") ]    = v->isHeatmapAutoScroll();
                // 显示类开关（D6）：当前帧标记 / 联动缩放 / 双光标 / 峰值保持 / 时间格式 / 色标
                view[ QStringLiteral("timeCursor") ]     = v->isTimeCursorVisible();
                view[ QStringLiteral("linkedZoom") ]     = v->isLinkedZoom();
                view[ QStringLiteral("dualCursor") ]     = v->isDualCursorEnabled();
                view[ QStringLiteral("peakHold") ]       = v->isPeakHoldEnabled();
                view[ QStringLiteral("timeFormat") ]     = v->timeFormat();
                view[ QStringLiteral("colorBarVisible") ] = v->isColorBarVisible();
                // 三维瀑布：色带 / dB 色域 / 当前迹线高亮 / 图元样式（样式从图元读回）
                view[ QStringLiteral("waterfallColormap") ]  = v->waterfallColormapName();
                view[ QStringLiteral("waterfallDbFloor") ]   = v->waterfallDbFloor();
                view[ QStringLiteral("waterfallHighlight") ] = v->isWaterfallHighlighted();
                view[ QStringLiteral("waterfall") ]          = v->waterfallStyle();
                // 云图绘制方式：显示模式与插值（从谱图图元/栅格读回，谱图 transient 不随 figures.xml 保存）
                view[ QStringLiteral("heatmapDisplayMode") ] = v->heatmapDisplayMode();
                view[ QStringLiteral("heatmapResample") ]    = v->heatmapResampleMode();
                // 峰值标记开关（视图无 getter：以 UI 记录的开关状态存，恢复后新视图据此应用）
                view[ QStringLiteral("peakMarkersVisible") ] = mPeakMarkersVisible;
                // T7 起布局是保留式网格：存归一化行/列比例（旧 splitter 像素比例在加载时归一化兼容）
                QJsonArray rowsArr;
                const QList< qreal > gridRows = v->gridRowSizes();
                for (qreal size : gridRows) {
                    rowsArr.append(size);
                }
                QJsonArray colsArr;
                const QList< qreal > gridCols = v->gridColumnSizes();
                for (qreal size : gridCols) {
                    colsArr.append(size);
                }
                view[ QStringLiteral("gridRows") ]    = rowsArr;
                view[ QStringLiteral("gridColumns") ] = colsArr;
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
 *
 * T9：工程存档里同时带 figures.xml（figure 文档）与 chart-data（非 transient 图元）。
 * 先按 signals.json 重建信号与视图（旧工程兼容路径），再把 figures.xml 的布局/样式/
 * 峰值标记/用户图元应用到对应视图；分析结果（transient 图元）由视图按参数重新计算。
 * @param json signals.json 字节
 * @param figuresXml figures.xml 字节（旧工程为空，此时只走 signals.json view{} 兼容路径）
 * @param chartItems 存档里读出的非 transient 图元（可为空）
 */
void DASignalAnalysisUI::restoreState(const QByteArray& json,
                                      const QByteArray& figuresXml,
                                      const DAChartItemsManager* chartItems)
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
        // 回填持久化的信号唯一 id（BLK-02）：figures.xml 的 signal-id 存档键按它匹配；
        // 旧工程没有该字段时保持构造时分配的会话内 id，恢复端走 signal-name 回退
        sig->restoreSignalId(qint64(o.value(QStringLiteral("signalId")).toDouble()));
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
        // D7 兼容：旧档可能存了「dBFS + 相对 1.0」的矛盾组合，加载后统一按幅值口径重新解释
        mDbReference = DASignalAnalysisParams::dbReferenceForAmplitudeMode(mAmplitudeMode);
        // 峰值标记开关：在 addSignalAndActivate 之前恢复，connectSignalView 会把它应用到新视图
        if (view.contains(QStringLiteral("peakMarkersVisible"))) {
            mPeakMarkersVisible = view.value(QStringLiteral("peakMarkersVisible")).toBool(mPeakMarkersVisible);
        }
        addSignalAndActivate(sig);
        // 恢复视图状态（P1）
        if (DASignalAnalysisView* v = mPage ? mPage->viewForSignal(sig.get()) : nullptr) {
            v->setWaterfallMode(view.value(QStringLiteral("waterfallMode")).toInt(0));
            const QString cmap = view.value(QStringLiteral("colormap")).toString();
            if (!cmap.isEmpty()) {
                v->setColormapName(cmap);
            }
            if (view.contains(QStringLiteral("dbFloor"))) {
                v->setDbFloor(view.value(QStringLiteral("dbFloor")).toDouble(v->dbFloor()));
            }
            v->setHeatmapAutoScroll(view.value(QStringLiteral("autoScroll")).toBool(false));
            // 显示类开关（D6）：缺键取默认（旧档兼容）
            v->setTimeCursorVisible(view.value(QStringLiteral("timeCursor")).toBool(true));
            v->setLinkedZoom(view.value(QStringLiteral("linkedZoom")).toBool(false));
            v->setDualCursorEnabled(view.value(QStringLiteral("dualCursor")).toBool(false));
            v->setPeakHoldEnabled(view.value(QStringLiteral("peakHold")).toBool(false));
            v->setTimeFormat(view.value(QStringLiteral("timeFormat")).toInt(0));
            // 三维瀑布显示状态（D12）：色带 / dB 色域 / 当前迹线高亮 / 图元样式
            const QString waterfallColormap = view.value(QStringLiteral("waterfallColormap")).toString();
            if (!waterfallColormap.isEmpty()) {
                v->setWaterfallColormapName(waterfallColormap);
            }
            if (view.contains(QStringLiteral("waterfallDbFloor"))) {
                v->setWaterfallDbFloor(view.value(QStringLiteral("waterfallDbFloor")).toDouble(v->waterfallDbFloor()));
            }
            v->setWaterfallHighlight(view.value(QStringLiteral("waterfallHighlight")).toBool(true));
            // 瀑布样式：STFT 异步，图元未建时视图先记为待应用，首建后套用
            v->applyWaterfallStyle(view.value(QStringLiteral("waterfall")).toObject());
            // 云图绘制方式：显示模式与插值（谱图 transient，恢复时先记为待应用）
            v->setHeatmapDisplayMode(view.value(QStringLiteral("heatmapDisplayMode")).toInt(0));
            v->setHeatmapResampleMode(view.value(QStringLiteral("heatmapResample")).toInt(0));
            if (view.contains(QStringLiteral("colorBarVisible"))) {
                const bool colorBar = view.value(QStringLiteral("colorBarVisible")).toBool(mColorBarVisible);
                v->setColorBarVisible(colorBar);
                mColorBarVisible = colorBar;  // 偏好跟随最后一个恢复的信号
            }
            // 网格比例：优先新键 gridRows/gridColumns；旧工程只有 splitterVertical/splitterHorizontal
            // （像素值），交给 setGridSizes 内部归一化，保证旧存档仍能还原比例
            QList< qreal > gridRows;
            QList< qreal > gridCols;
            const QJsonArray rowsArr = view.value(QStringLiteral("gridRows")).toArray();
            if (!rowsArr.isEmpty()) {
                for (const QJsonValue& s : rowsArr) {
                    gridRows.append(s.toDouble());
                }
            } else {
                for (const QJsonValue& s : view.value(QStringLiteral("splitterVertical")).toArray()) {
                    gridRows.append(s.toDouble());
                }
            }
            const QJsonArray colsArr = view.value(QStringLiteral("gridColumns")).toArray();
            if (!colsArr.isEmpty()) {
                for (const QJsonValue& s : colsArr) {
                    gridCols.append(s.toDouble());
                }
            } else {
                for (const QJsonValue& s : view.value(QStringLiteral("splitterHorizontal")).toArray()) {
                    gridCols.append(s.toDouble());
                }
            }
            v->setGridSizes(gridRows, gridCols);
            v->setAmplitudeDisplay(mAmplitudeMode, mDbReference);
        }
    }
    // T9：把随信号存档的 figure 文档（网格比例/子图样式/峰值标记/用户图元）应用到恢复后的视图；
    // 旧工程没有 figures.xml 时跳过，沿用 signals.json view{} 的兼容恢复结果
    applyFiguresXml(figuresXml, chartItems);
    // 丝带上下文标签与底部工具栏跟随恢复后的当前视图（含瀑布模式/通道/色带/工具开关）
    syncSignalContextControls();
    syncSignalToolbar();
    // 恢复后当前信号可能是「来源缺失」占位：刷新重新指定入口可用性（评审 U16）
    updateRelinkActionState();
    updatePlaybackActionsState();
}

/**
 * @brief 创建工程存档任务（isSave 时收集状态；加载时在回调里重建信号）
 *
 * T9：保存时在主线程生成 signals.json + figures.xml，并把非 transient 图元登记到
 * DAChartItemsManager；BLK-01：登记后立刻在 GUI 线程 makeSnapshots 预序列化成字节
 * 快照，交给后台任务的是**快照列表**（不是 manager）——保存期间 GUI 线程（嵌套
 * 事件循环继续派发定时器）可能删除/重建峰值标记等图元，后台线程只写字节，与活指针
 * 完全解耦。加载时任务先读 chart-data 与 figures.xml，回调里再恢复信号并应用 figure 文档。
 */
std::shared_ptr< DAAbstractArchiveTask > DASignalAnalysisUI::createArchiveTask(bool isSave)
{
    if (isSave) {
        DAChartItemsManager chartItems;
        const QByteArray figuresXml = buildFiguresXml(&chartItems);
        // BLK-01：GUI 线程先把图元序列化成字节快照，后台任务只写字节（不携带 QwtPlotItem*）
        const QList< DAChartItemSnapshot > itemSnapshots = DAChartItemArchive::makeSnapshots(&chartItems);
        auto task = std::make_shared< DASignalArchiveTask >(serializeState(), figuresXml, itemSnapshots);
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
        if (sigTask->archiveEntryFound() && !sigTask->figuresEntryFound()) {
            // 旧工程（T9 之前保存）没有 figures.xml：走 signals.json view{} 的兼容恢复路径
            qInfo() << "DASignalAnalysis: project has no signal-analysis/figures.xml; restore legacy view state";
        }
        // chartItems 指向任务内的图元管理者，任务在回调期间仍存活（shared_ptr 由调用方持有）
        restoreState(sigTask->loadedData(), sigTask->loadedFiguresXml(), &sigTask->chartItemsManager());
    });
    return task;
}

/**
 * @brief 构建随工程存档的 figures.xml（逐信号 figure XML + 非 transient 图元）
 *
 * 只在主线程调用：figure XML 生成需要读取控件状态，并把非 transient 图元
 * （峰值标记/用户添加的标注等）登记到 itemsMgr；后台存档任务只负责写字节。
 * @param chartItems 输出：图元 key 映射（保存任务据此写 chart-data），不可为空
 * @return figures.xml 字节；没有页面/信号时返回空（不写 figures.xml）
 */
QByteArray DASignalAnalysisUI::buildFiguresXml(DAChartItemsManager* chartItems) const
{
    QList< DASignalAnalysisView* > views;
    if (mPage) {
        const QList< DASignalPtr > signalList = mPage->allSignals();
        for (const DASignalPtr& sig : signalList) {
            if (!sig) {
                continue;
            }
            if (DASignalAnalysisView* view = mPage->viewForSignal(sig.get())) {
                views.append(view);
            }
        }
    }
    return DASignalFigureArchive::saveFigures(views, chartItems);
}

/**
 * @brief 把 figures.xml 的 figure 状态应用到恢复后的信号视图
 *
 * 匹配按 signal-id 属性（同名信号不串档，BLK-02）；旧存档（无 signal-id）回退按
 * signal-name。恢复端防争用：figure 元素从候选集合里逐个「取走」（同一元素只被一个
 * 视图消费），chart-data 图元 key 跨视图共享登记（一个 QwtPlotItem 只 attach 到一个
 * 子图，重复命中跳过并 qWarning）。
 * 恢复失败只记录诊断（英文）并保持默认布局，不影响信号本体恢复。
 * 旧工程没有 figures.xml 时直接返回（兼容路径由 restoreState 的 view{} 分支负责）。
 */
void DASignalAnalysisUI::applyFiguresXml(const QByteArray& figuresXml, const DAChartItemsManager* chartItems)
{
    if (figuresXml.isEmpty() || nullptr == mPage) {
        return;
    }
    QDomDocument figureDoc;
    QString parseError;
    if (!figureDoc.setContent(figuresXml, &parseError)) {
        qWarning() << "DASignalAnalysisUI::applyFiguresXml: cannot parse figures.xml:" << parseError;
        return;
    }
    // 候选 figure 元素逐个消费：同一元素不会被两个视图命中（旧存档同名信号按顺序错开）
    QList< QDomElement > candidates = DASignalFigureArchive::figureElements(figureDoc);
    // 已 attach 的 chart-data 图元 key：跨视图共享，防止同一 QwtPlotItem 被两个子图争用
    QSet< QString > consumedItemKeys;
    const QList< DASignalPtr > signalList = mPage->allSignals();
    for (const DASignalPtr& sig : signalList) {
        if (!sig) {
            continue;
        }
        DASignalAnalysisView* view = mPage->viewForSignal(sig.get());
        if (!view) {
            continue;
        }
        const QDomElement figureEle =
            DASignalFigureArchive::takeFigureElement(candidates, sig->signalId(), sig->name());
        if (figureEle.isNull()) {
            qWarning() << "DASignalAnalysisUI::applyFiguresXml: no figure element for signal" << sig->name()
                       << "id" << sig->signalId();
            continue;
        }
        if (!DASignalFigureArchive::restoreFigure(view, figureEle, chartItems, &consumedItemKeys)) {
            qWarning() << "DASignalAnalysisUI::applyFiguresXml: restore figure failed for signal" << sig->name();
        }
        // 存档里的峰值标记非 transient：视图接管后把 UI 开关同步为「可见」，
        // 后续分析结果回填不再重建标记（以存档为准，用户在设置面板里调整过）
        if (!view->peakMarkers().isEmpty() && view == currentView()) {
            mPeakMarkersVisible = true;
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
        QJsonObject cfg      = DASignalImporter::loadPluginConfig();
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
    mTimeFormatMenu         = nullptr;
    mLayoutPresetMenu       = nullptr;
    mTimeFormatGroup        = nullptr;
    actionDisplayReset      = nullptr;
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
    actionLinkedZoom = nullptr;
    actionDualCursor = nullptr;
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
            SARibbonPanel* createPanel = ribbonArea->getPanelByObjectName(
                QString::fromUtf8(UiNames::Ribbon::MainCreatePanel));
            removeActionFromPanel(createPanel, actionOpenSignalPage);
        }
    }
    // 经宿主接口销毁全部动作（含上下文标签新增动作）
    if (mActions) {
        const QList< QAction* > allActions = {
            actionOpenSignalPage, actionImport,          actionImportAudio,     actionImportDataFrame,
            actionImportVideo,    actionPlay,            actionStop,
            actionAnalysisSetting, actionCancelAnalysis, actionPeakMarker,     actionPeakTopN,
            actionPeakMarkerDisplay, actionPeakWindow,   actionWaterfall2D,     actionWaterfall3D,
            actionWaterfallHide,  actionExportPeaks,
            actionExportImage,    actionRelinkSource,
            actionTimeCursor,     actionViewOptions,     actionColormap,
            actionColorBar,       actionWaterfallColormap, actionWaterfallHighlight,
            actionWaterfallSettings, actionHeatmapDisplay,
            actionMeasure
        };
        for (QAction* act : allActions) {
            if (act) {
                mActions->removeAction(act);
            }
        }
    }
    // 全部置空，防止悬空访问
    actionOpenSignalPage    = nullptr;
    actionImport            = nullptr;
    actionImportAudio       = nullptr;
    actionImportDataFrame   = nullptr;
    actionImportVideo       = nullptr;
    actionPlay              = nullptr;
    actionStop              = nullptr;
    actionAnalysisSetting   = nullptr;
    actionCancelAnalysis    = nullptr;
    actionPeakMarker        = nullptr;
    actionPeakTopN          = nullptr;
    actionPeakMarkerDisplay = nullptr;
    actionPeakWindow        = nullptr;
    actionPeakTop10         = nullptr;
    actionPeakTop20         = nullptr;
    actionPeakTop50         = nullptr;
    actionWaterfall2D       = nullptr;
    actionWaterfall3D       = nullptr;
    actionWaterfallHide    = nullptr;
    actionTimeCursor        = nullptr;
    actionViewOptions       = nullptr;
    actionDisplayReset      = nullptr;
    actionExportPeaks       = nullptr;
    actionExportImage       = nullptr;
    actionRelinkSource      = nullptr;
    actionColormap          = nullptr;
    actionColorBar          = nullptr;
    actionWaterfallColormap = nullptr;
    actionWaterfallHighlight = nullptr;
    actionWaterfallSettings = nullptr;
    actionHeatmapDisplay    = nullptr;
    actionMeasure           = nullptr;
    actionLinkedZoom        = nullptr;
    mPanelImportExport      = nullptr;
    mPanelPreprocess        = nullptr;
    mPanelSpectrum          = nullptr;
    mPanelTimeFreq          = nullptr;
    mPanelPeaks             = nullptr;
    mPanelMore              = nullptr;
    // 峰值 panel 控件随所属面板/category 一并销毁，这里只置空指针
    mPeakModeCombo          = nullptr;
    mPeakRelDbSpin          = nullptr;
    mPeakModeLabel          = nullptr;
    mPeakRelDbLabel         = nullptr;
    mHeatmapDbFloorSpin     = nullptr;
    mHeatmapDbFloorLabel    = nullptr;
    mWaterfallDbFloorSpin   = nullptr;
    mWaterfallDbFloorLabel  = nullptr;
    mPeakMarkersVisible     = false;
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
    // 对话框里可能改了任意参数（含幅值口径）：全量回填丝带控件与全局状态，避免口径分叉（评审 N-U3）
    syncParamsRibbonControls();
    // 参数变了：大信号内存预算重新询问一次（用户可能是有意用更大段长重试，评审 N-C1c）
    mMemoryBudgetVerdict.remove(sig.get());
    recomputeCurrentSignal();
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
        // 通道切换后上下文标签/峰值标记状态跟随（视图内部已重发分析请求）
        syncSignalContextControls();
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
            const qint64 value = qBound< qint64 >(0,
                                                  qRound64(view->currentTime() * 1000.0),
                                                  mToolbarTimeSlider->maximum());
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
        actionPlay->setIcon(QIcon(QString::fromUtf8(cIconPrefix)
                                  + QLatin1String(playing ? "pause.svg" : "play.svg")));
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
 * @brief 峰值标记显隐（峰值 panel 第一个动作）
 *
 * 视图没有该状态的 getter：UI 记录 mPeakMarkersVisible，新建/切换视图时由
 * connectSignalView 应用，保证各信号标签页口径一致。
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
    applyParamsPatch(
        sig, [topN](DASignalAnalysisParams& params) { params.peakTopN = topN; }, ParamScope::Peaks);
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
    applyParamsPatch(
        sig, [mode](DASignalAnalysisParams& params) { params.peakMode = mode; }, ParamScope::Peaks);
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
    applyParamsPatch(
        sig, [value](DASignalAnalysisParams& params) { params.peakRelDb = value; }, ParamScope::Peaks);
    syncParamsRibbonControls();
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
    applyParamsPatch(
        sig, [on](DASignalAnalysisParams& params) { params.removeDc = on; }, ParamScope::Preprocess);
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
    applyParamsPatch(
        sig, [detrend](DASignalAnalysisParams& params) { params.detrend = detrend; }, ParamScope::Preprocess);
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
    applyParamsPatch(
        sig, [window](DASignalAnalysisParams& params) { params.window = window; }, ParamScope::Spectrum);
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
        [mode](DASignalAnalysisParams& params) {
            params.amplitudeMode = mode;
            // D7：不再有独立「dB 参考」入口，参考口径由幅值口径派生（字段保留兼容旧档）
            params.dbReference = (mode == QLatin1String("dbRef1")) ? QStringLiteral("one")
                                                                  : QStringLiteral("fullScale");
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
        [value, windowMs](DASignalAnalysisParams& params) {
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
        [value, resolution](DASignalAnalysisParams& params) {
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
    applyParamsPatch(
        sig, [value](DASignalAnalysisParams& params) { params.stftOverlap = value; }, ParamScope::TimeFreq);
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
    QJsonObject cfg                          = DASignalImporter::loadPluginConfig();
    cfg[ QStringLiteral("peakMarkerText") ] = mPeakMarkerText;
    DASignalImporter::savePluginConfig(cfg);
    applyPeakMarkerTextToViews();
}

/**
 * @brief 分析设置页参数控件回填当前信号参数（预处理/频谱/时频/峰值）
 *
 * 切信号、参数变化、页面未创建都要能安全调用：控件写值一律用 QSignalBlocker，
 * 避免回填触发各 on*Changed 造成反向参数覆盖；无当前信号时全部置灰。
 */
void DASignalAnalysisUI::syncParamsRibbonControls()
{
    const DASignalPtr sig = (mPage ? mPage->currentSignal() : DASignalPtr());
    const bool hasSignal  = static_cast< bool >(sig);
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
 * @brief 设置依赖信号的动作是否可用（无信号时播放/分析/峰值/显示/工具/导出全部置灰）
 * @param on 是否可用
 */
void DASignalAnalysisUI::setSignalDependentActionsEnabled(bool on)
{
    const QList< QAction* > acts = {
        actionPlay,         actionStop,
        actionAnalysisSetting, actionPeakMarker, actionPeakTopN,
        actionPeakMarkerDisplay, actionPeakWindow, actionWaterfall2D, actionWaterfall3D,
        actionWaterfallHide, actionExportPeaks, actionTimeCursor, actionViewOptions,
        actionDisplayReset, actionWaterfallColormap, actionWaterfallHighlight, actionWaterfallSettings,
        actionExportImage,  actionRelinkSource,    actionHeatmapDisplay,  actionMeasure,
        actionLinkedZoom,   actionDualCursor,
        actionColormap,     actionColorBar
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
    // 工具类动作还取决于当前 2D 子图是否存在：按当前视图状态再精化一次
    syncSignalContextControls();
}

/**
 * @brief 重译界面文本：标签页名、面板名、全部动作文本
 */
void DASignalAnalysisUI::retranslateUi()
{
    // 页面 dock（插件不再有顶级丝带标签页：主页入口 + 上下文两页）
    if (mPageDock) {
        mPageDock->setWindowTitle(tr("Signal Analysis"));  // cn:信号分析
    }
    // 峰值停靠窗口标题必须自己重译：宿主 resetText() 只处理宿主自己的 dock（评审 §七-5）
    if (mPeakDock) {
        mPeakDock->setWindowTitle(tr("Peaks"));  // cn:峰值
    }
    // 上下文标签页 1「分析设置」的面板
    if (mPanelImportExport) {
        mPanelImportExport->setPanelName(tr("Import / Export"));  // cn:导入导出
    }
    if (mPanelPreprocess) {
        mPanelPreprocess->setPanelName(tr("Preprocess"));  // cn:预处理
    }
    if (mPanelSpectrum) {
        mPanelSpectrum->setPanelName(tr("Spectrum"));  // cn:频谱
    }
    if (mPanelTimeFreq) {
        mPanelTimeFreq->setPanelName(tr("Time-Frequency"));  // cn:时频
    }
    if (mPanelPeaks) {
        mPanelPeaks->setPanelName(tr("Peaks"));  // cn:峰值
    }
    if (mPanelMore) {
        mPanelMore->setPanelName(tr("More"));  // cn:更多
    }
    // 插件上下文标签：标题 / 两页 / 显示页的按对象分组面板
    if (mContextCategory) {
        mContextCategory->setContextTitle(tr("Signal Analysis"));  // cn:信号分析
    }
    if (mContextAnalysisPage) {
        mContextAnalysisPage->setCategoryName(tr("Analysis Settings"));  // cn:分析设置
    }
    if (mContextDisplayPage) {
        mContextDisplayPage->setCategoryName(tr("Signal Display"));  // cn:信号显示
    }
    if (mPanelTimeFreqView) {
        mPanelTimeFreqView->setPanelName(tr("Time-Frequency View"));  // cn:时频图设置
    }
    if (mPanelHeatmap) {
        mPanelHeatmap->setPanelName(tr("Heatmap"));  // cn:云图设置
    }
    if (mPanelWaterfall) {
        mPanelWaterfall->setPanelName(tr("Waterfall"));  // cn:瀑布图设置
    }
    // 主页入口
    if (actionOpenSignalPage) {
        actionOpenSignalPage->setText(tr("Signal Analysis"));  // cn:信号分析
        actionOpenSignalPage->setToolTip(tr("Open the signal analysis workspace"));  // cn:打开信号分析界面
    }
    // 信号
    if (actionImport) {
        actionImport->setText(tr("Import"));  // cn:导入
        actionImport->setToolTip(tr("Import audio, a data frame or a video, or relink a missing data frame source"));
        // cn:导入音频/数据帧/视频，或重新指定缺失的数据帧来源
    }
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
    // 播放（▶/⏸ 文案与图标随播放状态在 syncSignalToolbar/onPlayerStateChanged 中动态切换）
    if (actionPlay) {
        if (!(mPlayer && mPlayer->isPlaying())) {
            actionPlay->setText(tr("Play"));  // cn:播放
            actionPlay->setToolTip(tr("Start playback"));  // cn:开始播放
        }
    }
    if (actionStop) {
        actionStop->setText(tr("Stop"));  // cn:停止
        actionStop->setToolTip(tr("Stop playback and return to the beginning"));
        // cn:停止播放并回到开头
    }
    if (mToolbarTimeLabel) {
        mToolbarTimeLabel->setToolTip(tr("Playback position / total duration"));
        // cn:播放位置 / 总时长
    }
    // 分析（对话框：常用参数已上丝带，这里只留高级项）
    if (actionAnalysisSetting) {
        actionAnalysisSetting->setText(tr("Advanced Settings..."));  // cn:高级设置...
        actionAnalysisSetting->setToolTip(tr("Configure zero padding, power spectrum, advanced peak and calibration options"));
        // cn:配置零填充、功率谱、峰值高级项与标定
    }
    if (actionCancelAnalysis) {
        actionCancelAnalysis->setText(tr("Stop Calculation"));  // cn:停止计算
        actionCancelAnalysis->setToolTip(tr("Stop all running signal analysis computations"));
        // cn:停止全部正在进行的信号分析计算
    }
    // 峰值
    if (actionPeakMarker) {
        actionPeakMarker->setText(tr("Peak Markers"));  // cn:峰值标记
        actionPeakMarker->setToolTip(tr("Mark the detected peaks on the spectrum"));  // cn:在频谱图上标出检测到的峰值
    }
    if (actionPeakTopN) {
        actionPeakTopN->setText(tr("Marker Count"));  // cn:标记数量
        actionPeakTopN->setToolTip(tr("Number of peaks to mark (10/20/50)"));  // cn:标记的峰值数量（10/20/50）
    }
    if (actionPeakMarkerDisplay) {
        actionPeakMarkerDisplay->setText(tr("Marker Display"));  // cn:标记显示
        actionPeakMarkerDisplay->setToolTip(tr("Edit the text shown above peak markers"));  // cn:编辑峰值标记上方显示的文字
    }
    if (mPeakModeLabel) {
        mPeakModeLabel->setText(tr("Mode"));  // cn:模式
    }
    if (mPeakRelDbLabel) {
        mPeakRelDbLabel->setText(tr("Threshold"));  // cn:相对门限
    }
    if (mPeakModeCombo) {
        const QSignalBlocker blocker(mPeakModeCombo);
        mPeakModeCombo->setItemText(0, tr("Simple"));    // cn:简单
        mPeakModeCombo->setItemText(1, tr("Advanced"));  // cn:高级
        mPeakModeCombo->setToolTip(tr("Peak search mode: simple or advanced"));  // cn:峰值查找模式：简单/高级
    }
    if (mPeakRelDbSpin) {
        mPeakRelDbSpin->setToolTip(tr("Only keep peaks within this range below the strongest peak"));
        // cn:只保留低于最强峰不超过该门限的峰值
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
    if (actionTimeCursor) {
        actionTimeCursor->setText(tr("Time Cursor"));  // cn:当前帧标记
        actionTimeCursor->setToolTip(
            tr("Mark the current frame: a dashed line on the heatmap and a highlighted trace in the 3D waterfall"));
        // cn:标记当前帧：二维云图上的虚线，三维瀑布里的当前迹线高亮
    }
    if (actionViewOptions) {
        actionViewOptions->setText(tr("View"));  // cn:视图
        actionViewOptions->setToolTip(tr("Time axis format, sub-chart layout and reset display"));
        // cn:时间轴格式、子图布局比例与重置显示
    }
    if (mTimeFormatMenu) {
        mTimeFormatMenu->setTitle(tr("Time format"));  // cn:时间格式
    }
    // 时间格式三态菜单项（顺序与 mTimeFormatActions 一致：秒 / 毫秒 / mm:ss.s）
    if (mTimeFormatActions.size() >= 3) {
        mTimeFormatActions.at(0)->setText(tr("Seconds"));       // cn:秒
        mTimeFormatActions.at(1)->setText(tr("Milliseconds"));  // cn:毫秒
        mTimeFormatActions.at(2)->setText(tr("mm:ss.s"));       // cn:分:秒.十分位
    }
    if (mLayoutPresetMenu) {
        mLayoutPresetMenu->setTitle(tr("Layout"));  // cn:布局比例
    }
    // 布局比例菜单项（顺序与 mLayoutPresetActions 一致：1:1 / 波形优先 / 分析优先 / 恢复默认）
    if (mLayoutPresetActions.size() >= 4) {
        mLayoutPresetActions.at(0)->setText(tr("1:1"));               // cn:1:1
        mLayoutPresetActions.at(1)->setText(tr("Waveform priority"));  // cn:波形优先
        mLayoutPresetActions.at(2)->setText(tr("Analysis priority"));  // cn:分析优先
        mLayoutPresetActions.at(3)->setText(tr("Restore default"));    // cn:恢复默认
    }
    if (actionDisplayReset) {
        actionDisplayReset->setText(tr("Reset display"));  // cn:重置显示
        actionDisplayReset->setToolTip(tr("Restore all display settings to their defaults"));
        // cn:把所有显示类设置恢复为默认值
    }
    if (actionMeasure) {
        actionMeasure->setText(tr("Measure"));  // cn:测量
        actionMeasure->setToolTip(tr("Linked zoom, dual cursors and peak hold"));
        // cn:联动缩放、双光标测量与峰值保持
    }
    if (actionLinkedZoom) {
        actionLinkedZoom->setText(tr("Linked zoom"));  // cn:联动缩放
        actionLinkedZoom->setToolTip(
            tr("Keep the waveform/heatmap time axes and the spectrum/heatmap frequency axes in sync"));
        // cn:波形与云图的时间轴、频谱与云图的频率轴保持同步缩放
    }
    if (actionDualCursor) {
        actionDualCursor->setText(tr("Dual cursors"));  // cn:双光标测量
        actionDualCursor->setToolTip(
            tr("Click a sub-chart to place cursor A, then B: readouts show delta time/frequency and amplitudes"));
        // cn:点一下子图放 A、再点放 B：读数给出 Δt/Δf 与对应幅值
    }
    // 分析设置页参数控件（标签 / combo 项文本；动作文本另见下方「分析」）
    if (mChannelLabel) {
        mChannelLabel->setText(tr("Channel"));  // cn:通道
    }
    if (mChannelCombo) {
        mChannelCombo->setToolTip(tr("Channel analysed by the spectrum, the time-frequency view and the peak table"));
        // cn:频谱、时频图与峰值表分析的通道
    }
    if (mDetrendLabel) {
        mDetrendLabel->setText(tr("Detrend"));  // cn:去趋势
    }
    if (mDetrendCombo) {
        const QSignalBlocker blocker(mDetrendCombo);
        mDetrendCombo->setItemText(0, tr("None"));      // cn:无
        mDetrendCombo->setItemText(1, tr("Constant"));  // cn:常数
        mDetrendCombo->setItemText(2, tr("Linear"));    // cn:线性
        mDetrendCombo->setToolTip(tr("Remove the DC offset or a linear trend before analysis"));
        // cn:分析前去掉直流分量或线性趋势
    }
    if (mPreprocessDcCheck) {
        mPreprocessDcCheck->setText(tr("Remove DC"));  // cn:去直流
        mPreprocessDcCheck->setToolTip(tr("Remove the DC offset before analysis"));
        // cn:分析前去掉直流分量
    }
    if (mWindowLabel) {
        mWindowLabel->setText(tr("Window"));  // cn:窗函数
    }
    if (mWindowCombo) {
        const QSignalBlocker blocker(mWindowCombo);
        mWindowCombo->setItemText(0, tr("Rectangular"));  // cn:矩形
        mWindowCombo->setItemText(1, tr("Hann"));         // cn:汉宁
        mWindowCombo->setItemText(2, tr("Hamming"));      // cn:汉明
        mWindowCombo->setItemText(3, tr("Blackman"));     // cn:布莱克曼
        mWindowCombo->setItemText(4, tr("Flat top"));     // cn:平顶
        mWindowCombo->setToolTip(tr("Window function shared by the spectrum and the time-frequency view"));
        // cn:频谱与时频视图共用的窗函数
    }
    if (mAmplitudeModeLabel) {
        mAmplitudeModeLabel->setText(tr("Amplitude"));  // cn:幅值口径
    }
    if (mAmplitudeModeCombo) {
        const QSignalBlocker blocker(mAmplitudeModeCombo);
        mAmplitudeModeCombo->setItemText(0, tr("Peak amplitude"));  // cn:峰值幅度
        mAmplitudeModeCombo->setItemText(1, tr("RMS"));             // cn:均方根
        mAmplitudeModeCombo->setItemText(2, tr("dB (FS)"));         // cn:dB（满量程）
        mAmplitudeModeCombo->setItemText(3, tr("dB (re 1.0)"));     // cn:dB（相对 1.0）
        mAmplitudeModeCombo->setToolTip(tr("Amplitude scale used by the plots, the peak table and exports"));
        // cn:频谱图、峰值表与导出使用的幅值口径
    }
    if (mResolutionLabel) {
        mResolutionLabel->setText(tr("Resolution"));  // cn:频率分辨率
    }
    if (mResolutionSpin) {
        mResolutionSpin->setToolTip(tr("Frequency resolution of the spectrum (linked to the STFT window length)"));
        // cn:频谱频率分辨率（与 STFT 窗长联动）
    }
    if (mStftWindowLabel) {
        mStftWindowLabel->setText(tr("Window length"));  // cn:窗长
    }
    if (mStftWindowSpin) {
        mStftWindowSpin->setToolTip(tr("Window length of the time-frequency view (linked to the frequency resolution)"));
        // cn:时频视图的窗长（与频率分辨率联动）
    }
    if (mStftOverlapLabel) {
        mStftOverlapLabel->setText(tr("Hop overlap"));  // cn:跳步重叠
    }
    if (mStftOverlapSpin) {
        mStftOverlapSpin->setToolTip(tr("Overlap between consecutive time-frequency frames"));
        // cn:相邻时频帧之间的重叠率
    }
    if (mDeltaFLabel) {
        // 文案随窗长变化，重译时按当前参数重算一次
        const DASignalAnalysisParams params =
            (mPage && mPage->currentSignal()) ? paramsForSignal(mPage->currentSignal()) : DASignalAnalysisParams::defaults();
        updateDeltaFLabel(params);
    }
    // 信号显示页新增动作（云图设置：色带 / 色标）
    if (actionColormap) {
        actionColormap->setText(tr("Color Map"));  // cn:色带
        actionColormap->setToolTip(tr("Color map of the 2D time-frequency heatmap"));  // cn:二维时频热图的色带
    }
    if (actionColorBar) {
        actionColorBar->setText(tr("Color bar"));  // cn:色标
        actionColorBar->setToolTip(tr("Show or hide the time-frequency color bar"));
        // cn:显示或隐藏时频图色标
    }
    if (mHeatmapDbFloorLabel) {
        mHeatmapDbFloorLabel->setText(tr("dB"));  // cn:dB
    }
    if (mHeatmapDbFloorSpin) {
        mHeatmapDbFloorSpin->setToolTip(tr("Heatmap color scale lower limit (dB)"));
        // cn:热图色标下限（dB）
    }
    // 云图显示 ▾（显示模式 / 插值）
    if (actionHeatmapDisplay) {
        actionHeatmapDisplay->setText(tr("Display"));  // cn:显示
        actionHeatmapDisplay->setToolTip(tr("Heatmap display mode and interpolation"));
        // cn:云图的显示模式与插值方式
    }
    if (mHeatmapDisplayModeSection) {
        mHeatmapDisplayModeSection->setText(tr("Display mode"));  // cn:显示模式
    }
    if (mHeatmapDisplayModeActions.size() >= 3) {
        mHeatmapDisplayModeActions.at(0)->setText(tr("Image"));           // cn:图像
        mHeatmapDisplayModeActions.at(1)->setText(tr("Contour"));         // cn:等高线
        mHeatmapDisplayModeActions.at(2)->setText(tr("Image + Contour")); // cn:图像+等高线
    }
    if (mHeatmapResampleSection) {
        mHeatmapResampleSection->setText(tr("Interpolation"));  // cn:插值
    }
    if (mHeatmapResampleActions.size() >= 2) {
        mHeatmapResampleActions.at(0)->setText(tr("Nearest neighbour"));  // cn:最近邻
        mHeatmapResampleActions.at(1)->setText(tr("Bilinear"));           // cn:双线性
    }
    // 瀑布图设置（只影响三维瀑布）
    if (actionWaterfallSettings) {
        actionWaterfallSettings->setText(tr("Waterfall Settings..."));  // cn:瀑布设置...
        actionWaterfallSettings->setToolTip(
            tr("Open the 3D settings panel for the waterfall item (style, color mode, opacity, outline)"));
        // cn:在右侧 3D 设置面板里编辑瀑布图元（几何样式/着色方式/不透明度/描边）
    }
    if (actionWaterfallColormap) {
        actionWaterfallColormap->setText(tr("Color Map"));  // cn:色带
        actionWaterfallColormap->setToolTip(tr("Color map of the 3D waterfall"));  // cn:三维瀑布的色带
    }
    if (mWaterfallDbFloorLabel) {
        mWaterfallDbFloorLabel->setText(tr("dB"));  // cn:dB
    }
    if (mWaterfallDbFloorSpin) {
        mWaterfallDbFloorSpin->setToolTip(tr("3D waterfall color range lower limit (dB)"));
        // cn:三维瀑布色域下限（dB）
    }
    if (actionWaterfallHighlight) {
        actionWaterfallHighlight->setText(tr("Current Trace"));  // cn:当前迹线高亮
        actionWaterfallHighlight->setToolTip(
            tr("Highlight the trace of the current frame (unavailable in solid color mode)"));
        // cn:高亮当前帧对应的迹线（单色着色方式下不可用）
    }
    // 窗口底部工具栏
    if (mToolbarFollowCheck) {
        mToolbarFollowCheck->setText(tr("Follow playback"));  // cn:跟随播放
        mToolbarFollowCheck->setToolTip(tr("Show the current frame while playback advances"));
        // cn:播放推进时显示当前帧
    }
    if (mToolbarTimeSlider) {
        mToolbarTimeSlider->setToolTip(tr("Playback position"));  // cn:播放位置
    }
    if (mToolbarAutoScrollCheck) {
        mToolbarAutoScrollCheck->setText(tr("Auto scroll"));  // cn:自动滚动
        mToolbarAutoScrollCheck->setToolTip(tr("Scroll the time-frequency view while playback advances"));
        // cn:播放推进时滚动时频视图
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
        actionExportImage->setToolTip(tr("Export the chart page or the current sub-chart, or copy it to the clipboard"));
        // cn:导出整页/当前子图或复制到剪贴板
    }
    // 图片导出菜单项（整页/当前子图/复制剪贴板，T9）
    if (mActionExportWholePage) {
        mActionExportWholePage->setText(tr("Whole page (PNG)..."));  // cn:整页（PNG）...
    }
    if (mActionExportCurrentSubChart) {
        mActionExportCurrentSubChart->setText(tr("Current sub-chart (PNG/SVG/PDF)..."));
        // cn:当前子图（PNG/SVG/PDF）...
    }
    if (mActionCopyWholePage) {
        mActionCopyWholePage->setText(tr("Copy whole page to clipboard"));  // cn:复制整页到剪贴板
    }
    if (mActionCopyCurrentSubChart) {
        mActionCopyCurrentSubChart->setText(tr("Copy current sub-chart to clipboard"));
        // cn:复制当前子图到剪贴板
    }
    // 页面
    if (mPage) {
        mPage->retranslateUi();
    }
}
}  // namespace DA
