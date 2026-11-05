#ifndef DASIGNALANALYSISUI_H
#define DASIGNALANALYSISUI_H
#include <QObject>
#include <QAction>
#include <QHash>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"
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

/**
 * @brief 信号分析插件的界面层：中央页面 + 丝带标签页 + 动作管理
 *
 * 页面经 DADockingAreaInterface::createDockWidgetTabAtCenterDockArea 加入中央停靠区
 * （与绘图/表格/工作流并列，方案 D11）；丝带用 SARibbonBar::addCategoryPage 新建顶级标签页
 * （方案 §8.1，注意不是文档里的 addCategory）。
 *
 * 所有动作经宿主 DAActionsInterface 创建与销毁（动作 parent 是宿主接口，不随插件卸载销毁）。
 */
class DASignalAnalysisUI : public QObject
{
    Q_OBJECT
public:
    explicit DASignalAnalysisUI(QObject* par = nullptr);
    ~DASignalAnalysisUI() override;

    // 初始化：创建中央页面与丝带标签页
    bool initialize(DACoreInterface* core);
    // 清理：移除丝带标签页、动作与页面停靠窗口（插件热卸载前调用）
    void finalize();
    // 重译界面文本
    void retranslateUi();

    DASignalAnalysisPage* page() const;

    // 无信号时置灰全部依赖信号的动作（导入动作始终保持可用）
    void setSignalDependentActionsEnabled(bool on);

    // 当前信号的参数（无记录时返回默认参数）
    DA::DASignalAnalysisParams paramsForSignal(const DA::DASignalPtr& sig) const;
    // 最近一次频谱结果（无结果返回无效对象）
    DA::DASpectrumResult spectrumForSignal(const DA::DASignalPtr& sig) const;

Q_SIGNALS:
    // 频谱结果更新（显示层订阅）
    void spectrumUpdated();

public Q_SLOTS:
    // 导入三入口（丝带按钮与空状态按钮共用）
    void importAudioFile();
    void importDataFrame();
    void importVideoFile();
    // 打开统一参数对话框（D20），确定后立即重算一次（D21）
    void openAnalysisSettings();
    // 幅值显示模式切换（只重绘，不重算）
    void onAmplitudeModeChanged();
    // 重新计算当前信号的频谱（参数变化后调用）
    void recomputeCurrentSignal();

private Q_SLOTS:
    void onWorkerProgress(double percent, const QString& stage);
    void onWorkerSpectrumReady(const DA::DASpectrumResult& result);
    void onWorkerEnvelopeReady(const QVector< double >& xs,
                               const QVector< double >& mins,
                               const QVector< double >& maxs);
    void onWorkerFailed(const QString& error);
    void onViewRequestSpectrum(const DA::DASignalPtr& signal);
    void onViewRequestEnvelope(qint64 start, qint64 end, int width);
    void onViewRequestHeatmap(int columns, int rows);
    void onWorkerHeatmapReady(const DA::DAStftResult& result);
    void onWorkerPeaksReady(const DA::DAPeakResult& result);
    void onWorkerPeakCurvesReady(const DA::DAPeakCurveResult& result);
    void onPeakWindowToggled(bool on);
    void onPeakMarkerToggled(bool on);
    void onPeakTopNChanged();

private:
    void buildCenterPage();
    void buildSignalCategory();
    void buildPeakTopNMenu();
    void buildAmplitudeModeMenu();
    void buildPeakDock();
    // 触发当前信号的峰值表与峰值曲线计算
    void requestPeaks(bool withCurves);
    // 导入成功后加入页面并激活依赖动作
    void addSignalAndActivate(const DA::DASignalPtr& sig);

private:
    DACoreInterface* mCore { nullptr };
    DAUIInterface* mUI { nullptr };
    DAActionsInterface* mActions { nullptr };
    SARibbonCategory* mCategory { nullptr };
    ads::CDockWidget* mPageDock { nullptr };
    DASignalAnalysisPage* mPage { nullptr };
    QMenu* mPeakTopNMenu { nullptr };
    QMenu* mAmplitudeModeMenu { nullptr };
    QActionGroup* mWaterfallModeGroup { nullptr };
    QActionGroup* mPeakTopNGroup { nullptr };
    QActionGroup* mAmplitudeModeGroup { nullptr };

public:
    //===================================================
    // 丝带面板
    //===================================================
    SARibbonPanel* mPanelSignal { nullptr };    ///< 信号（导入）
    SARibbonPanel* mPanelPlayback { nullptr };  ///< 播放
    SARibbonPanel* mPanelAnalysis { nullptr };  ///< 分析
    SARibbonPanel* mPanelPeaks { nullptr };     ///< 峰值
    SARibbonPanel* mPanelDisplay { nullptr };   ///< 显示
    SARibbonPanel* mPanelExport { nullptr };    ///< 导出

    //===================================================
    // 信号（Signal）
    //===================================================
    QAction* actionImportAudio { nullptr };      ///< 导入音频
    QAction* actionImportDataFrame { nullptr };  ///< 导入数据帧
    QAction* actionImportVideo { nullptr };      ///< 导入视频

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
    QAction* actionPeakTable { nullptr };      ///< 频谱/峰值表页面切换（占位）
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
    // 每个信号标签页记住上次生效参数（D21）
    QHash< const DA::DASignal*, DA::DASignalAnalysisParams > mParamsPerSignal;
    // 频谱结果缓存（随信号对象存活，参数变化才重算）
    QHash< const DA::DASignal*, DA::DASpectrumResult > mSpectrumCache;
    const DA::DASignal* mPendingSignal { nullptr };
    // 最近一次请求包络的视图（包络结果回填目标）
    DA::DASignalAnalysisView* mPendingEnvelopeView { nullptr };
    // 最近一次请求时频热图的视图
    DA::DASignalAnalysisView* mPendingHeatmapView { nullptr };
    // 当前幅值显示模式（D14）
    QString mAmplitudeMode { QStringLiteral("peak") };
    QString mDbReference { QStringLiteral("fullScale") };
};
}  // namespace DA

#endif  // DASIGNALANALYSISUI_H
