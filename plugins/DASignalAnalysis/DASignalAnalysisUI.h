#ifndef DASIGNALANALYSISUI_H
#define DASIGNALANALYSISUI_H
#include <QObject>
#include <QAction>
#include <QHash>
#include <QList>
#include <QPointer>
#include <functional>
#include <memory>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"
#include "Analysis/DASignalTaskContext.h"
#include "Analysis/DASignalAnalysisWorker.h"
#include "Analysis/DAPeakResult.h"
// QPointer 成员要求完整类型（Qt6 的 QPointer 内部用 std::is_base_of 校验）
#include "DockWidget.h"
#include "DAChartWidget.h"
#include "Page/DASignalAnalysisPage.h"

class QMenu;
class QActionGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QSlider;
class QToolBar;
class QToolButton;
class QTimer;
class SARibbonCategory;
class SARibbonContextCategory;
class SARibbonPanel;

namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DAActionsInterface;
class DADockingAreaInterface;
class DAFigureWidget;
class DASignalAnalysisView;
class DASignalAnalysisSettingContext;
class DAPeakDockWidget;
class DASignalPlayer;
class DAAbstractArchiveTask;
class DAChartItemsManager;

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
    // 当前信号视图（页面未创建/无信号时返回 nullptr；provider 与上下文标签共用）
    DASignalAnalysisView* currentView() const;
    // 全部信号的 figure（设置上下文图元树用；无页面返回空）
    QList< DAFigureWidget* > signalFigures() const;
    // 显示/隐藏插件 Ribbon 上下文标签（provider 激活/失活时调用）
    void setSignalContextCategoryVisible(bool on);
    // 显示/隐藏窗口底部播放工具栏（provider 激活/失活时调用）
    void setSignalToolbarVisible(bool on);
    // 停止播放并把播放头归零（页面失活时 provider 调用：D6）
    void stopPlayback();
    // 宿主停靠区接口（provider 注册设置上下文/图元树/设置面板用）
    DADockingAreaInterface* getDockingArea() const;

    // 无信号时置灰全部依赖信号的动作（导入动作始终保持可用）
    void setSignalDependentActionsEnabled(bool on);

    // 当前信号的参数（无记录时返回默认参数）
    DA::DASignalAnalysisParams paramsForSignal(const DA::DASignalPtr& sig) const;
    // 指定信号/通道最近一次频谱结果（无结果返回无效对象）
    DA::DASpectrumResult spectrumForSignal(const DA::DASignalPtr& sig, int channel) const;

    // 工程保存/加载（经插件 createArchiveTask，任务在后台线程只做文件读写）
    std::shared_ptr< DA::DAAbstractArchiveTask > createArchiveTask(bool isSave);
    // 序列化当前状态为 JSON（保存用，signals.json）
    QByteArray serializeState() const;
    // 从 signals.json 重建信号并应用随信号存档的 figures.xml 图表状态（主线程）
    void restoreState(const QByteArray& json,
                      const QByteArray& figuresXml = QByteArray(),
                      const DAChartItemsManager* chartItems = nullptr);


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
    // 全局设置（设置 → 信号分析）应用后：刷新 FFmpeg 相关入口可用性
    void onGlobalSettingsApplied();
    // 取消全部后台计算（评审 N-C7：界面可见的取消入口）
    void cancelAllComputations();
    // 重新计算当前信号（全量：对话框确定后调用）：频谱、热图、峰值表与曲线统一提交
    void recomputeCurrentSignal();

private Q_SLOTS:
    // 分析设置页参数控件（改后按 §5 的粒度分级重算，不走全量）
    void onPreprocessDcToggled(bool on);
    void onDetrendChanged(int index);
    void onWindowChanged(int index);
    void onAmplitudeModeComboChanged(int index);
    void onResolutionChanged(double value);
    void onStftWindowChanged(double value);
    void onStftOverlapChanged(double value);
    void onWorkerProgress(double percent, const QString& stage, int runningTaskCount);
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
    // 后台导入准备完成（I2）：成功则加入页面；失败显示可翻译提示；取消时 signal/error 都为空
    void onWorkerImportFinished(const DA::DASignalPtr& signal, const QString& error);
    // 任务被用户取消：只清进度，不弹失败（评审 N-C7）
    void onWorkerCanceled(const QString& taskId);
    void onPeakWindowToggled(bool on);
    void onPeakMarkerToggled(bool on);
    void onPeakTopNChanged();
    // 峰值 panel「模式」下拉：写当前信号参数并重算峰值表/曲线
    void onPeakModeChanged(int index);
    // 峰值 panel「相对门限」：写当前信号参数并重算峰值表/曲线
    void onPeakRelDbChanged(double value);
    // 峰值 panel「标记显示」：弹出峰值标记文本模板编辑对话框
    void openPeakMarkerDisplaySettings();
    void onPlayerPositionChanged(qint64 ms);
    // 播放器播放/暂停状态变化：同步 ▶/⏸ 图标、勾选与时间文本
    void onPlayerStateChanged(bool playing);
    // 「播放/暂停」动作切换（按钮勾选 = 正在播放）
    void onPlayPauseToggled(bool on);
    // 信号标签页关闭：清理该信号的参数/结果缓存
    void onSignalRemoved(const DA::DASignalPtr& signal);
    // 当前信号标签页切换：播放源、峰值表与曲线随当前信号重置（评审 U4）
    void onCurrentSignalChanged();
    // 跟随滑块时按当前帧刷新峰值表与峰值标记（D13）
    void updatePeaksForCurrentFrame();
    void onViewTimeChanged(double seconds);
    // 通道切换：峰值与曲线按新通道重算
    void onViewChannelChanged(int channel);
    // 导出：峰值表 CSV / 频谱 CSV / 热图矩阵 CSV / 整页位图 / 当前子图 / 剪贴板 / 峰值表送表格页
    void exportPeaksCsv();
    void exportSpectrumCsv();
    void exportPsdCsv();
    void exportHeatmapCsv();
    // 导出整页（2D 网格 + 3D 帧缓冲合帧）为 PNG
    void exportWholePageImage();
    // 导出当前子图（2D 支持 PNG/SVG/PDF，3D 仅 PNG）
    void exportCurrentSubChartImage();
    // 复制整页 / 当前子图到剪贴板
    void copyWholePageToClipboard();
    void copyCurrentSubChartToClipboard();
    void sendPeaksToTable();
    // 重新指定数据帧来源（当前信号来源缺失时可用，评审 U16）
    void relinkCurrentSignalSource();
    // 宿主清空工程（新建/加载前/快照回滚）：清理本插件信号、缓存与临时文件
    void onProjectCleaned();
    // 上下文标签：通道切换 / 色带菜单 / 色标显隐 / 当前帧标记
    void onChannelComboChanged(int index);
    void onContextColormapTriggered(QAction* action);
    // 时频图色标显隐开关：写当前视图并立即持久化偏好
    void onColorBarToggled(bool on);
    // 当前帧标记开关（二维云图竖线 + 三维当前迹线高亮）
    void onTimeCursorToggled(bool on);
    // 云图设置 dB 范围：写当前视图的二维色域下限（[floor, 0]）
    void onHeatmapDbFloorChanged(double value);
    // 当前视图「跟随滑块」变化（回勾底部工具栏）
    void onViewFollowSliderToggled(bool on);
    // 当前视图请求跟随帧幅值谱（同步短调用：成功回填、失败静默，不弹消息）
    void onViewFollowFrameSpectrumRequested(qint64 frameIndex);
    // 当前 2D/3D 子图切换：同步上下文标签动作状态
    void onViewSubChartChanged();

private:
    void buildCenterPage();
    // 主页「创建」panel 注入「信号分析」入口（插件不再占用顶级丝带标签页）
    void installHomeEntry();
    // 构建「分析设置」上下文页（导入/分析/峰值等常用参数）
    void buildAnalysisSettingsPage(SARibbonContextCategory* contextCategory);
    // 构建插件 Ribbon 上下文标签（分析设置 / 信号显示 两页）
    void buildSignalContextCategory();
    // 给「信号显示」页重新下发面板标题开关（SARibbonBar 样式切换会统一改写，D9）
    void applyDisplayPagePanelTitles();
    // 构建窗口底部播放工具栏（默认隐藏）
    void buildSignalToolbar();
    // 当前视图状态 → 上下文标签控件（QSignalBlocker 防回环）
    void syncSignalContextControls();
    // 绑定当前 2D 子图的属性变化（内置「绘图操作」切换缩放/平移/十字线后同步 SF-12）
    void bindCurrentChartSignals();
    // 解绑当前 2D 子图的属性变化并清空绑定指针（先断后连，QPointer 防悬空）
    void unbindCurrentChartSignals();
    // 当前视图状态 → 底部工具栏控件（QSignalBlocker 防回环）
    void syncSignalToolbar();
    // 刷新底部工具栏时间文本（mm:ss.s / mm:ss.s；无源显示占位）
    void updateToolbarTimeText();
    void buildPeakTopNMenu();
    void buildPeakDock();
    // 经宿主动作管理器创建带图标的动作（丝带面板共用）
    QAction* createRibbonIconAction(const char* objectName, const QString& iconPath, bool checkable = false);
    // 丝带参数控件 → 当前信号参数回填（预处理/频谱/时频/峰值；QSignalBlocker 防回环，无信号置灰）
    void syncParamsRibbonControls();
    // 参数重算范围：由「哪个 panel 改了参数」决定，避免所有改动都走全量重算（方案 §5）
    enum class ParamScope
    {
        Preprocess,   ///< 去直流/去趋势：频谱 + 热图 + 峰值
        Spectrum,     ///< 窗函数/频率分辨率：频谱 + 热图 + 峰值（resolutionHz 联动 STFT 窗长）
        TimeFreq,     ///< STFT 窗长/跳步：热图 + 峰值曲线（窗长联动分辨率时含频谱）
        Peaks,        ///< 模式/相对门限/标记数量：只重算峰值表与曲线
        DisplayOnly   ///< 幅值口径：只重绘，不重算
    };
    // 参数统一写回入口：拷贝参数 → 改字段 → 存表 → 版本 +1 → 按 scope 分级重算
    void applyParamsPatch(const DA::DASignalPtr& sig,
                          const std::function< void(DA::DASignalAnalysisParams&) >& patch,
                          ParamScope scope);
    // 按参数重算范围提交后台任务（不改变参数版本，版本由 applyParamsPatch 递增）
    void recomputeScoped(ParamScope scope);
    // 刷新时频 panel 的 Δf 只读提示（按当前采样率与实际段长反算）
    void updateDeltaFLabel(const DA::DASignalAnalysisParams& params);
    // 把全局标记文本模板应用到已创建的全部信号视图
    void applyPeakMarkerTextToViews();
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
    // 状态栏计算进度（防抖）：短任务不显示进度条与停止按钮，超过延迟仍在算才显示
    void applyProgressIndicator();
    // 清掉本插件显示的状态栏进度与停止按钮（不抢其它调用方的进度条）
    void hideProgressIndicator();
    // 进度文本：单任务显示阶段名，多任务显示任务数
    QString progressStageText() const;
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
    // D5：内存超预算时的降级选择（降低精度 / 只扫可见区段 / 取消）
    enum BudgetChoice
    {
        BudgetProceed,
        BudgetReducePrecision,
        BudgetVisibleRangeOnly,
        BudgetCancel
    };
    BudgetChoice promptBudgetChoice(const DA::DASignalPtr& sig,
                                    const QString& stage,
                                    qint64 estimatedBytes,
                                    bool allowVisibleRange);
    // 按当前信号是否为「来源缺失」占位刷新「重新指定来源」动作（评审 U16）
    void updateRelinkActionState();
    // 按在跑任务数刷新「取消计算」动作可用性（评审 N-C7）
    void updateCancelActionState();
    // 导入在途时禁用三个导入入口（I2：视频/压缩音频转码在后台跑，避免并发导入互相覆盖）
    void updateImportActionsState();
    // 按播放源/导出中状态刷新播放动作可用性（评审 U9：导出期间禁用播放）
    void updatePlaybackActionsState();
    // 构建随工程存档的 figures.xml：逐信号 figure XML + 非 transient 图元（主线程）
    QByteArray buildFiguresXml(DA::DAChartItemsManager* chartItems) const;
    // 把 figures.xml 的 figure 状态应用到恢复后的信号视图（网格比例/子图样式/峰值/用户图元）
    void applyFiguresXml(const QByteArray& figuresXml, const DA::DAChartItemsManager* chartItems);

private:
    DACoreInterface* mCore { nullptr };
    DAUIInterface* mUI { nullptr };
    DAActionsInterface* mActions { nullptr };
    // 插件自建的 Ribbon 上下文标签（objectName 固定，供 show/hide 查找）
    // 两页：「分析设置」（常用参数，排在前保证默认页）与「信号显示」
    SARibbonContextCategory* mContextCategory { nullptr };
    SARibbonCategory* mContextAnalysisPage { nullptr };
    SARibbonCategory* mContextDisplayPage { nullptr };
    // 「信号显示」页按对象分组的面板：时频图设置 / 云图设置 / 瀑布图设置
    SARibbonPanel* mPanelTimeFreqView { nullptr };
    SARibbonPanel* mPanelHeatmap { nullptr };
    SARibbonPanel* mPanelWaterfall { nullptr };
    // 上下文标签控件与色带菜单（mColormapMenu 归二维云图，mWaterfallColormapMenu 归三维瀑布）
    QComboBox* mChannelCombo { nullptr };
    QMenu* mColormapMenu { nullptr };
    QActionGroup* mColormapGroup { nullptr };
    QMenu* mWaterfallColormapMenu { nullptr };
    QActionGroup* mWaterfallColormapGroup { nullptr };
    // Ribbon 样式切换 / 页切换的连接（面板标题只给本插件显示页开启，样式切换后要重新下发）
    QMetaObject::Connection mRibbonStyleConnection;
    QMetaObject::Connection mRibbonTabChangedConnection;
    // 窗口底部播放工具栏（默认隐藏，provider 激活时显示）
    QToolBar* mSignalToolbar { nullptr };
    QCheckBox* mToolbarFollowCheck { nullptr };
    QToolButton* mToolbarPlayButton { nullptr };
    QToolButton* mToolbarStopButton { nullptr };
    QSlider* mToolbarTimeSlider { nullptr };
    QLabel* mToolbarTimeLabel { nullptr };
    QCheckBox* mToolbarAutoScrollCheck { nullptr };
    // 云图设置 dB 范围（二维色域下限；从底部播放器控制条迁入，D5）
    QDoubleSpinBox* mHeatmapDbFloorSpin { nullptr };
    QLabel* mHeatmapDbFloorLabel { nullptr };
    // 峰值标记开关（视图无 getter：UI 记录状态并在新视图创建时应用）
    bool mPeakMarkersVisible { false };
    // 全局偏好：峰值标记文本模板（{index}/{fre}/{amp}，signal-analysis-config.json 的 peakMarkerText）
    QString mPeakMarkerText;
    // 峰值 panel 的峰值搜索控件（参数网格）：模式 / 相对门限
    QComboBox* mPeakModeCombo { nullptr };
    QDoubleSpinBox* mPeakRelDbSpin { nullptr };
    QLabel* mPeakModeLabel { nullptr };
    QLabel* mPeakRelDbLabel { nullptr };
    // 分析设置页「预处理 / 频谱 / 时频」参数控件（参数网格；标签随语言切换重译）
    QCheckBox* mPreprocessDcCheck { nullptr };
    QComboBox* mDetrendCombo { nullptr };
    QLabel* mDetrendLabel { nullptr };
    QLabel* mChannelLabel { nullptr };
    QComboBox* mWindowCombo { nullptr };
    QLabel* mWindowLabel { nullptr };
    QComboBox* mAmplitudeModeCombo { nullptr };
    QLabel* mAmplitudeModeLabel { nullptr };
    QDoubleSpinBox* mResolutionSpin { nullptr };
    QLabel* mResolutionLabel { nullptr };
    QDoubleSpinBox* mStftWindowSpin { nullptr };
    QLabel* mStftWindowLabel { nullptr };
    QDoubleSpinBox* mStftOverlapSpin { nullptr };
    QLabel* mStftOverlapLabel { nullptr };
    // 时频 panel 第三行：Δf 只读提示（与频率分辨率联动，不参与输入）
    QLabel* mDeltaFLabel { nullptr };
    // 设置上下文提供者（注册到宿主停靠区，finalize 前注销并销毁）
    DASignalAnalysisSettingContext* mSettingContext { nullptr };
    // 当前绑定 chartPropertiesChanged 的 2D 子图：用户经内置「绘图操作」标签切换
    // 缩放/平移/十字线后，本插件据此重新计算「点击定位」互斥（SF-12）
    QPointer< DAChartWidget > mBoundChart;
    QMetaObject::Connection mChartPropertiesConnection;
    // 停靠窗口由 dockManager 拥有：宿主销毁 dockManager 时会被连带销毁，
    // 用 QPointer 防止 finalize()/后台回调在窗口已销毁后继续访问（关闭程序时崩溃）
    QPointer< ads::CDockWidget > mPageDock;
    QPointer< DASignalAnalysisPage > mPage;
    QMenu* mPeakTopNMenu { nullptr };
    // 导入菜单（父动作 actionImport；菜单不是 QAction 的父子，finalize() 先 setMenu(nullptr) 再删）
    QMenu* mImportMenu { nullptr };
    QMenu* mExportMenu { nullptr };
    // 图片导出菜单（整页 / 当前子图 / 复制剪贴板，T9）
    QMenu* mExportImageMenu { nullptr };
    QAction* mActionExportWholePage { nullptr };
    QAction* mActionExportCurrentSubChart { nullptr };
    QAction* mActionCopyWholePage { nullptr };
    QAction* mActionCopyCurrentSubChart { nullptr };
    // 导出菜单项（retranslateUi 需要逐条刷新文本，评审 §七-5）
    QAction* mActionExportPeaksCsv { nullptr };
    QAction* mActionExportSpectrumCsv { nullptr };
    QAction* mActionExportPsdCsv { nullptr };
    QAction* mActionExportHeatmapCsv { nullptr };
    QActionGroup* mWaterfallModeGroup { nullptr };
    QActionGroup* mPeakTopNGroup { nullptr };

public:
    //===================================================
    // 丝带面板（全部位于插件自建的上下文标签页）
    //===================================================
    SARibbonPanel* mPanelImportExport { nullptr };  ///< 分析设置页：导入导出（导入▾/导出数据▾/导出图片▾）
    SARibbonPanel* mPanelPreprocess { nullptr };    ///< 分析设置页：预处理（去直流/去趋势）
    SARibbonPanel* mPanelSpectrum { nullptr };      ///< 分析设置页：频谱（通道/窗函数/幅值口径/频率分辨率）
    SARibbonPanel* mPanelTimeFreq { nullptr };      ///< 分析设置页：时频（窗长/跳步重叠/Δf）
    SARibbonPanel* mPanelPeaks { nullptr };         ///< 分析设置页：峰值（全项目唯一峰值入口）
    SARibbonPanel* mPanelMore { nullptr };          ///< 分析设置页：更多（高级设置…）

    //===================================================
    // 窗口（Window）
    //===================================================
    QAction* actionOpenSignalPage { nullptr };  ///< 打开信号分析窗口（页面惰性创建，关闭后可恢复）

    //===================================================
    // 信号（Signal）
    //===================================================
    QAction* actionImport { nullptr };           ///< 导入（大按钮 + 菜单：音频/数据帧/视频/重新指定来源）
    QAction* actionImportAudio { nullptr };      ///< 导入音频
    QAction* actionImportDataFrame { nullptr };  ///< 导入数据帧
    QAction* actionImportVideo { nullptr };      ///< 导入视频
    QAction* actionRelinkSource { nullptr };     ///< 重新指定数据帧来源（来源缺失时可用，评审 U16）

    //===================================================
    // 播放（Playback，仅底部播放器控制条使用）
    //===================================================
    QAction* actionPlay { nullptr };   ///< 播放/暂停（可勾选：勾选=正在播放）
    QAction* actionStop { nullptr };   ///< 停止

    //===================================================
    // 分析（Analysis）
    //===================================================
    QAction* actionAnalysisSetting { nullptr };  ///< 分析设置
    QAction* actionCancelAnalysis { nullptr };   ///< 取消全部后台计算（评审 N-C7）

    //===================================================
    // 峰值（Peaks）
    //===================================================
    QAction* actionPeakMarker { nullptr };        ///< 峰值标记（峰值 panel 第一个动作）
    QAction* actionPeakTopN { nullptr };          ///< 标记数量（10/20/50，带菜单）
    QAction* actionPeakMarkerDisplay { nullptr }; ///< 标记显示（弹出峰值标记文本模板对话框）
    QAction* actionPeakWindow { nullptr };        ///< 峰值窗口显隐
    // 标记数量菜单项
    QAction* actionPeakTop10 { nullptr };
    QAction* actionPeakTop20 { nullptr };
    QAction* actionPeakTop50 { nullptr };

    //===================================================
    // 显示（Display，位于 Ribbon 上下文标签；按对象分组）
    //===================================================
    QAction* actionWaterfall2D { nullptr };       ///< 时频图设置：二维云图
    QAction* actionWaterfall3D { nullptr };       ///< 时频图设置：三维瀑布
    QAction* actionWaterfallHide { nullptr };     ///< 时频图设置：隐藏
    QAction* actionTimeCursor { nullptr };        ///< 时频图设置：当前帧标记（勾选）
    QAction* actionColormap { nullptr };          ///< 云图设置：色带（带菜单，只影响二维谱图与色标）
    QAction* actionColorBar { nullptr };          ///< 云图设置：色标显隐（勾选）

    //===================================================
    // 导出（Export）
    //===================================================
    QAction* actionExportPeaks { nullptr };  ///< 导出峰值表
    QAction* actionExportImage { nullptr };  ///< 导出图片（带菜单：整页/当前子图/复制剪贴板）

private:
    DASignalAnalysisWorker* mWorker { nullptr };
    // 峰值停靠窗口（右侧停靠区第三个标签页，D24）
    DAPeakDockWidget* mPeakPanel { nullptr };
    // 峰值停靠窗口由 dockManager 拥有，宿主销毁 dockManager 时会被连带销毁；
    // QPointer 保证窗口先被销毁时这里自动置空，finalize()/回调不会访问悬空对象
    QPointer< ads::CDockWidget > mPeakDock;
    bool mFirstSignalActivated { false };
    bool mPeakCurvesRequested { false };
    // D5：本轮峰值曲线是否为局部扫描（只扫可见区段/隔帧降精度），峰值窗口据此显示「局部扫描」
    bool mPeakCurvesLocal { false };
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
    // 插件偏好：时频图右侧色标是否显示（默认显示；signal-analysis-config.json 的 colorBarVisible）
    bool mColorBarVisible { true };
    // 插件偏好：峰值窗口显隐（-1=从未设置 / 0=用户关闭 / 1=用户打开，评审 N-U1/P2）
    int mPeakDockVisiblePref { -1 };
    // 峰值停靠窗口创建期间屏蔽 visibilityChanged 对偏好的写入（评审 N-U1）
    bool mCreatingPeakDock { false };
    // 数据帧播放 WAV 后台导出的在途键（信号:通道；非空表示导出中，导出期间禁用播放，评审 U9）
    QString mPlaybackWavPendingKey;
    // 导入在途状态（I2）：true 表示后台正在准备导入，期间三个导入入口置灰
    bool mImportTaskActive { false };
    // 状态栏计算进度（防抖）：延迟窗口内结束的小计算不闪进度条；进度/任务数供延迟到点后回填
    QTimer* mProgressDelayTimer { nullptr };
    bool mProgressVisible { false };
    double mProgressPercent { 0.0 };
    QString mProgressStage;
    int mProgressTaskCount { 0 };
    // 在途导入是否为视频（失败提示用「导入视频失败」还是「导入音频失败」）
    bool mPendingImportIsVideo { false };
};
}  // namespace DA

#endif  // DASIGNALANALYSISUI_H
