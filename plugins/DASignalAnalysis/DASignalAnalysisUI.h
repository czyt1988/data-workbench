#ifndef DASIGNALANALYSISUI_H
#define DASIGNALANALYSISUI_H
#include <QObject>
#include <QAction>
#include <QHash>
#include <memory>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"
#include "Analysis/DASignalTaskContext.h"
#include "Analysis/DASignalAnalysisWorker.h"
#include "Analysis/DAPeakResult.h"

class QMenu;
class QActionGroup;
class SARibbonCategory;
class SARibbonPanel;
namespace ads
{
class CDockWidget;
}

namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DAActionsInterface;
class DASignalAnalysisPage;
class DASignalAnalysisView;
class DAPeakDockWidget;
class DASignalPlayer;
class DAAbstractArchiveTask;

/**
 * @brief 信号分析插件的界面层：中央页面 + 丝带标签页 + 动作管理
 *
 * 页面经 DADockingAreaInterface::createDockWidgetTabAtCenterDockArea 加入中央停靠区
 * （与绘图/表格/工作流并列，方案 D11）；丝带用 SARibbonBar::addCategoryPage 新建顶级标签页
 * （方案 §8.1，注意不是文档里的 addCategory）。
 *
 * 页面惰性创建：大部分用户不会用到信号分析，插件初始化只建丝带标签与动作，
 * 首次打开（丝带「打开信号分析窗口」）、导入信号或工程恢复时才创建中央页面与峰值窗口，
 * 避免程序一启动就多出一个空的停靠标签页。
 *
 * 所有动作经宿主 DAActionsInterface 创建与销毁（动作 parent 是宿主接口，不随插件卸载销毁）。
 */
class DASignalAnalysisUI : public QObject
{
    Q_OBJECT
public:
    explicit DASignalAnalysisUI(QObject* par = nullptr);
    ~DASignalAnalysisUI() override;

    // 初始化：创建丝带标签页与动作（中央页面与峰值窗口惰性创建）
    bool initialize(DACoreInterface* core);
    // 清理：移除丝带标签页、动作与页面/峰值停靠窗口（插件热卸载前调用）
    void finalize();
    // 重译界面文本
    void retranslateUi();

    DASignalAnalysisPage* page() const;

    // 无信号时置灰全部依赖信号的动作（导入动作始终保持可用）
    void setSignalDependentActionsEnabled(bool on);

    // 当前信号的参数（无记录时返回默认参数）
    DA::DASignalAnalysisParams paramsForSignal(const DA::DASignalPtr& sig) const;
    // 指定信号/通道最近一次频谱结果（无结果返回无效对象）
    DA::DASpectrumResult spectrumForSignal(const DA::DASignalPtr& sig, int channel) const;

    // 工程保存/加载（经插件 createArchiveTask，任务在后台线程只做文件读写）
    std::shared_ptr< DA::DAAbstractArchiveTask > createArchiveTask(bool isSave);
    // 序列化当前状态为 JSON（保存用）
    QByteArray serializeState() const;
    // 从 JSON 重建信号（加载用，主线程；先清空当前工程残留的信号与缓存）
    void restoreState(const QByteArray& json);


Q_SIGNALS:
    // 频谱结果更新（显示层订阅）
    void spectrumUpdated();

public Q_SLOTS:
    // 打开（首次创建）信号分析页面并唤起到前台；页面关闭后再点可恢复显示
    void openSignalAnalysisPage();
    // 导入三入口（丝带按钮与空状态按钮共用）
    void importAudioFile();
    void importDataFrame();
    void importVideoFile();
    // 打开统一参数对话框（D20），确定后立即重算一次（D21）
    void openAnalysisSettings();
    // 取消全部后台计算（评审 N-C7：界面可见的取消入口）
    void cancelAllComputations();
    // 幅值显示模式切换（只重绘，不重算）
    void onAmplitudeModeChanged();
    // 重新计算当前信号（参数变化后调用）：频谱、热图、峰值表与曲线统一提交
    void recomputeCurrentSignal();

private Q_SLOTS:
    void onWorkerProgress(double percent, const QString& stage);
    void onWorkerSpectrumReady(const DA::DASignalTaskContext& ctx, const DA::DASpectrumResult& result);
    void onWorkerEnvelopeReady(const DA::DASignalTaskContext& ctx,
                               const QVector< double >& xs,
                               const QVector< double >& mins,
                               const QVector< double >& maxs);
    void onWorkerFailed(const QString& error);
    void onViewRequestSpectrum(const DA::DASignalPtr& signal, int channel);
    void onViewRequestEnvelope(qint64 start, qint64 end, int width);
    void onViewRequestHeatmap(int columns, int rows);
    void onWorkerHeatmapReady(const DA::DASignalTaskContext& ctx, const DA::DAStftResult& result);
    void onWorkerPeaksReady(const DA::DASignalTaskContext& ctx, const DA::DAPeakResult& result);
    void onWorkerPeakCurvesReady(const DA::DASignalTaskContext& ctx, const DA::DAPeakCurveResult& result);
    // 数据帧播放 WAV 后台导出完成（评审 U9）：校验上下文后交给播放器
    void onWorkerPlaybackWavReady(const DA::DASignalTaskContext& ctx, const QString& wavPath);
    // 任务被用户取消：只清进度，不弹失败（评审 N-C7）
    void onWorkerCanceled(const QString& taskId);
    void onPeakWindowToggled(bool on);
    void onPeakMarkerToggled(bool on);
    void onPeakTopNChanged();
    void onPlayerPositionChanged(qint64 ms);
    // 信号标签页关闭：清理该信号的参数/结果缓存
    void onSignalRemoved(const DA::DASignalPtr& signal);
    // 当前信号标签页切换：播放源、峰值表与曲线随当前信号重置（评审 U4）
    void onCurrentSignalChanged();
    // 跟随滑块时按当前帧刷新峰值表与峰值标记（D13）
    void updatePeaksForCurrentFrame();
    void onViewTimeChanged(double seconds);
    // 通道切换：峰值与曲线按新通道重算
    void onViewChannelChanged(int channel);
    // 导出：峰值表 CSV / 频谱 CSV / 热图矩阵 CSV / 整页位图 / 峰值表送表格页
    void exportPeaksCsv();
    void exportSpectrumCsv();
    void exportPsdCsv();
    void exportHeatmapCsv();
    void exportImage();
    void sendPeaksToTable();
    // 重新指定数据帧来源（当前信号来源缺失时可用，评审 U16）
    void relinkCurrentSignalSource();
    // 宿主清空工程（新建/加载前/快照回滚）：清理本插件信号、缓存与临时文件
    void onProjectCleaned();

private:
    void buildCenterPage();
    void buildSignalCategory();
    void buildPeakTopNMenu();
    void buildAmplitudeModeMenu();
    void buildPeakDock();
    // 惰性创建页面（首次打开/导入/工程恢复时调用），已创建返回 true
    bool ensurePageCreated();
    // 触发当前信号的峰值表与峰值曲线计算
    void requestPeaks(bool withCurves);
    // 构造后台任务上下文（信号 + 当前视图通道 + 参数版本号）
    DA::DASignalTaskContext makeTaskContext(const DA::DASignalPtr& sig, int channel) const;
    // 参数版本 +1（参数变化后旧结果失效）
    void bumpParamsVersion(const DA::DASignalPtr& sig);
    // 校验后台结果是否仍属于当前信号/通道/参数版本
    bool isContextCurrent(const DA::DASignalTaskContext& ctx, bool checkChannel) const;
    // 清空页面上全部信号与相关缓存（工程加载前调用）
    void clearAllSignals();
    // 清空峰值表/曲线缓存
    void clearPeakState();
    // 信号的当前视图通道（无视图时 0）
    int channelOfSignal(const DA::DASignalPtr& sig) const;
    // 后台任务全部结束时清空状态栏进度（评审 §七-4）
    void clearProgressIfIdle();
    // 统一错误上报：进界面消息队列（da.user）+ 唤起界面日志窗口，避免功能静默缺失
    void reportError(const QString& text);
    // 统一失败上报：界面只显示翻译后的提示，英文诊断原文只写日志（评审 §七-9）
    void reportOperationFailure(const QString& userText, const QString& detail);
    // 按 FFmpeg 可用性刷新入口状态（未检测到时视频导入置灰并说明原因）
    void refreshFFmpegAvailability();
    // 当前信号的播放源切换（信号标签切换时调用）
    void syncPlayerSource();
    // 导入成功后加入页面并激活依赖动作
    void addSignalAndActivate(const DA::DASignalPtr& sig);
    // 连接单个信号视图的请求/事件信号并发起首次计算（导入与重新指定来源共用）
    void connectSignalView(const DA::DASignalPtr& sig);
    // 首次导入信号后按偏好决定是否显示峰值窗口（D24 / 评审 N-U1）
    void showPeakDockForFirstSignal();
    // 参数对话框改幅值口径后同步丝带勾选与全局状态（评审 N-U3）
    void syncAmplitudeUiFromParams(const DA::DASignalAnalysisParams& params);
    // 大信号内存预算门：返回 false 表示用户取消本次计算（评审 N-C1c）
    bool checkMemoryBudget(const DA::DASignalPtr& sig, const QString& stage, qint64 estimatedBytes);
    // 按当前信号是否为「来源缺失」占位刷新「重新指定来源」动作（评审 U16）
    void updateRelinkActionState();
    // 按在跑任务数刷新「取消计算」动作可用性（评审 N-C7）
    void updateCancelActionState();
    // 按播放源/导出中状态刷新播放动作可用性（评审 U9：导出期间禁用播放）
    void updatePlaybackActionsState();

private:
    DACoreInterface* mCore { nullptr };
    DAUIInterface* mUI { nullptr };
    DAActionsInterface* mActions { nullptr };
    SARibbonCategory* mCategory { nullptr };
    ads::CDockWidget* mPageDock { nullptr };
    DASignalAnalysisPage* mPage { nullptr };
    QMenu* mPeakTopNMenu { nullptr };
    QMenu* mExportMenu { nullptr };
    QMenu* mAmplitudeModeMenu { nullptr };
    // 导出菜单项（retranslateUi 需要逐条刷新文本，评审 §七-5）
    QAction* mActionExportPeaksCsv { nullptr };
    QAction* mActionExportSpectrumCsv { nullptr };
    QAction* mActionExportPsdCsv { nullptr };
    QAction* mActionExportHeatmapCsv { nullptr };
    QActionGroup* mWaterfallModeGroup { nullptr };
    QActionGroup* mPeakTopNGroup { nullptr };
    QActionGroup* mAmplitudeModeGroup { nullptr };

public:
    //===================================================
    // 丝带面板
    //===================================================
    SARibbonPanel* mPanelOpen { nullptr };      ///< 窗口（打开信号分析窗口）
    SARibbonPanel* mPanelSignal { nullptr };    ///< 信号（导入）
    SARibbonPanel* mPanelPlayback { nullptr };  ///< 播放
    SARibbonPanel* mPanelAnalysis { nullptr };  ///< 分析
    SARibbonPanel* mPanelPeaks { nullptr };     ///< 峰值
    SARibbonPanel* mPanelDisplay { nullptr };   ///< 显示
    SARibbonPanel* mPanelExport { nullptr };    ///< 导出

    //===================================================
    // 窗口（Window）
    //===================================================
    QAction* actionOpenSignalPage { nullptr };  ///< 打开信号分析窗口（页面惰性创建，关闭后可恢复）

    //===================================================
    // 信号（Signal）
    //===================================================
    QAction* actionImportAudio { nullptr };      ///< 导入音频
    QAction* actionImportDataFrame { nullptr };  ///< 导入数据帧
    QAction* actionImportVideo { nullptr };      ///< 导入视频
    QAction* actionRelinkSource { nullptr };     ///< 重新指定数据帧来源（来源缺失时可用，评审 U16）

    //===================================================
    // 播放（Playback）
    //===================================================
    QAction* actionPlay { nullptr };   ///< 播放
    QAction* actionPause { nullptr };  ///< 暂停
    QAction* actionStop { nullptr };   ///< 停止

    //===================================================
    // 分析（Analysis）
    //===================================================
    QAction* actionAnalysisSetting { nullptr };  ///< 分析设置
    QAction* actionCancelAnalysis { nullptr };   ///< 取消全部后台计算（评审 N-C7）

    //===================================================
    // 峰值（Peaks）
    //===================================================
    QAction* actionPeakMarker { nullptr };  ///< 峰值标记（快捷开关）
    QAction* actionPeakTopN { nullptr };    ///< 前 N 个峰值（10/20/50，带菜单）
    QAction* actionPeakWindow { nullptr };  ///< 峰值窗口显隐
    // 前 N 菜单项
    QAction* actionPeakTop10 { nullptr };
    QAction* actionPeakTop20 { nullptr };
    QAction* actionPeakTop50 { nullptr };

    //===================================================
    // 显示（Display）
    //===================================================
    QAction* actionWaterfall2D { nullptr };    ///< 二维时频热图
    QAction* actionWaterfall3D { nullptr };    ///< 三维曲面瀑布
    QAction* actionWaterfallHide { nullptr };  ///< 隐藏瀑布
    QAction* actionAmplitudeMode { nullptr };  ///< 幅值模式（带菜单）
    QAction* actionPeakTable { nullptr };      ///< 峰值表页面切换（占位）
    // 幅值模式菜单项
    QAction* actionAmpPeak { nullptr };     ///< 峰值幅度
    QAction* actionAmpRms { nullptr };      ///< 均方根
    QAction* actionAmpDbFs { nullptr };     ///< dB（相对满量程）
    QAction* actionAmpDbRef1 { nullptr };   ///< dB（相对 1.0）

    //===================================================
    // 导出（Export）
    //===================================================
    QAction* actionExportPeaks { nullptr };  ///< 导出峰值表
    QAction* actionExportImage { nullptr };  ///< 导出图片

private:
    DASignalAnalysisWorker* mWorker { nullptr };
    // 峰值停靠窗口（右侧停靠区第三个标签页，D24）
    DAPeakDockWidget* mPeakPanel { nullptr };
    ads::CDockWidget* mPeakDock { nullptr };
    bool mFirstSignalActivated { false };
    bool mPeakCurvesRequested { false };
    DA::DAPeakResult mCurrentPeaks;
    DA::DAPeakCurveResult mCurrentCurves;
    // 当前峰值数据所属的信号与通道（切信号/通道后旧数据不再显示，评审 U4）
    const DA::DASignal* mCurrentPeaksSignal { nullptr };
    int mCurrentPeaksChannel { 0 };
    // 播放控制器（方案 D26）
    DASignalPlayer* mPlayer { nullptr };
    // 大信号内存预算判定：信号 → 0=未询问 / 1=用户已确认继续 / 2=用户拒绝（评审 N-C1c/D19）
    QHash< const DA::DASignal*, int > mMemoryBudgetVerdict;
    bool mSyncingFromPlayer { false };
    // 每个信号标签页记住上次生效参数（D21）
    QHash< const DA::DASignal*, DA::DASignalAnalysisParams > mParamsPerSignal;
    // 参数版本号：参数变化即 +1，迟到的后台结果按版本丢弃（评审 U3 / v2 方案 B4）
    quint64 mParamsVersionCounter { 0 };
    QHash< const DA::DASignal*, quint64 > mParamsVersion;
    // 频谱结果缓存（随信号对象存活，参数或通道变化才重算）
    QHash< const DA::DASignal*, DA::DASpectrumResult > mSpectrumCache;
    QHash< const DA::DASignal*, int > mSpectrumCacheChannel;
    // 当前幅值显示模式（D14）
    QString mAmplitudeMode { QStringLiteral("peak") };
    QString mDbReference { QStringLiteral("fullScale") };
    // 插件偏好：上次使用的色带（P2）
    QString mLastColormap;
    // 插件偏好：峰值窗口显隐（-1=从未设置 / 0=用户关闭 / 1=用户打开，评审 N-U1/P2）
    int mPeakDockVisiblePref { -1 };
    // 峰值停靠窗口创建期间屏蔽 visibilityChanged 对偏好的写入（评审 N-U1）
    bool mCreatingPeakDock { false };
    // 数据帧播放 WAV 后台导出的在途键（信号:通道；非空表示导出中，导出期间禁用播放，评审 U9）
    QString mPlaybackWavPendingKey;
};
}  // namespace DA

#endif  // DASIGNALANALYSISUI_H
