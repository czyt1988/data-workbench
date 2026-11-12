#ifndef DASIGNALANALYSISVIEW_H
#define DASIGNALANALYSISVIEW_H
#include <QWidget>
#include <QVector>
#include <QStringList>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"
#include "Analysis/DAStftResult.h"
#include "Analysis/DAPeakResult.h"

class QSlider;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QTimer;
class QToolBar;
class QResizeEvent;
class QPixmap;

class QwtPlotCurve;
class QwtPlotIntervalCurve;
class QwtPlotMarker;
class QwtPlotShapeItem;
class QwtPlotSpectrogram;
class Qwt3DWaterfall;

namespace DA
{
class DAFigureWidget;
class DAChartWidget;
class DAChart3DWidget;
class DAStftRasterData;

/**
 * @brief 单个信号的分析视图（方案 §8.2 / D25 / T7）
 *
 * 布局：DAFigureWidget 2×2 保留式网格（可拖分隔条调比例）
 * - 波形 (0,0,1,2)：包络曲线 + 播放头 + STFT 窗口矩形
 * - 频谱 (1,0)：幅值谱曲线 + 峰值标记（非 transient）
 * - 热图 (1,1)：时频栅格（瀑布模式 1/2 时与三维瀑布显隐切换）
 * - 三维瀑布 (1,1)
 *
 * 图元策略：分析结果类图元标 transient（加载后重算重建）；峰值标记非 transient（T9 写入工程）。
 * 视图不自己算频谱与包络：长任务由外部（UI 层的 worker）在 Python 守护线程算完回填。
 *
 * 注：T7 阶段临时用视图底部 QToolBar 承载原内联控件（TODO(T8)：迁移到 Ribbon 上下文标签与
 * 窗口底部工具栏）。业务状态（当前通道/时间/跟随/色带/dB 下限等）全部保存在成员变量中，
 * 临时工具栏只是它们的视图，T8 删除工具栏后 setter/getter 语义不变。
 */
class DASignalAnalysisView : public QWidget
{
    Q_OBJECT
public:
    explicit DASignalAnalysisView(const DASignalPtr& signal, QWidget* parent = nullptr);
    ~DASignalAnalysisView() override;

    DASignalPtr signal() const;
    // 参数（显示层只用到幅值模式/标定；计算参数由 UI 层持有）
    void setParams(const DASignalAnalysisParams& params);
    DASignalAnalysisParams params() const;
    // 频谱结果回填（外部计算完成后调用）
    void setSpectrum(const DASpectrumResult& result);
    // 包络结果回填（外部计算完成后调用）
    void setEnvelope(const QVector< double >& xs, const QVector< double >& mins, const QVector< double >& maxs);
    // 时频显示栅格回填（外部计算完成后调用）
    void setStft(const DAStftResult& result);
    // 当前时频显示栅格（导出热图矩阵用）
    DAStftResult stftResult() const;
    // 峰值表数据回填（在频谱图上标出峰位）
    void setPeaks(const DAPeakResult& peaks);
    // 峰值标记显隐（丝带「峰值标记」开关）
    void setPeakMarkersVisible(bool on);
    // 瀑布模式：0=二维热图 / 1=三维曲面 / 2=隐藏（D12 / D25）
    void setWaterfallMode(int mode);
    int waterfallMode() const;
    // 频率轴线性/对数（D16）
    bool isLogFrequency() const;
    void setLogFrequency(bool on);
    // 色带与 dB 动态范围下限（视图状态持久化用）
    QString colormapName() const;
    void setColormapName(const QString& name);
    double dbFloor() const;
    void setDbFloor(double dbFloor);
    // 时间轴原点（秒）：数据帧时间列首值非 0 时三张图横向对齐用（评审 U12）
    void setTimeOrigin(double seconds);
    // 信号峰值幅度（dBFS 参考；按去直流/去趋势后的预处理口径统计，评审 N-C2）
    double signalPeak() const;
    // 热图随播放自动滚动开关（D27，默认关）
    void setHeatmapAutoScroll(bool on);
    bool isHeatmapAutoScroll() const;
    // 跟随滑块（显示当前帧而不是整段）
    void setFollowSlider(bool on);
    bool isFollowSlider() const;
    // 播放头跟随（外部按播放位置调用，避免与用户 seek 混淆）
    void setPlayheadFromPlayback(double seconds);
    // 当前时间（秒）
    double currentTime() const;
    // 当前波形可见时间范围（秒，绝对时间，含 timeOrigin）；无数据/未布局时返回 false
    // D5：内存超限时「只扫可见区段」用它把范围换算成采样点
    bool visibleTimeRange(double* startSec, double* endSec) const;
    // 当前通道（多通道信号一次分析一个通道）
    int currentChannel() const;
    void setCurrentChannel(int channel);
    // 通道名列表（Ribbon 通道菜单用）
    QStringList channelNames() const;
    // 幅值显示模式（不重算，只重绘）
    void setAmplitudeDisplay(const QString& amplitudeMode, const QString& dbReference);
    // 请求一次包络计算（视图显示/尺寸变化时调用）
    void requestEnvelopeRefresh();
    // 请求一次时频热图计算（外部参数变化后调用）
    void requestHeatmapRefresh();
    // 首次分析：发起包络、整段频谱与时频热图三类计算（外部信号连接完成后调用）
    void requestInitialAnalysis();
    // 导出用整页位图：三维瀑布可见时把 OpenGL 帧缓冲合成到页面位图中（评审 I11）
    QPixmap grabForExport();

    // ==================== 图表文档（T6/T8 复用） ====================

    // 视图持有的 figure（插件设置上下文/图元树/undo 用）
    DAFigureWidget* figure() const;
    // 全部 figure（目前一个信号一个 figure）
    QList< DAFigureWidget* > figures() const;
    // 波形子图（0,0,1,2）
    DAChartWidget* waveChart() const;
    // 频谱子图（1,0）
    DAChartWidget* spectrumChart() const;
    // 热图子图（1,1）
    DAChartWidget* heatmapChart() const;
    // 三维瀑布子图（1,1）
    DAChart3DWidget* waterfall3DChart() const;
    // 当前 2D 子图；无当前或当前不可见时回退波形
    DAChartWidget* currentChart() const;
    // 当前 3D 子图（DAFigureWidget::getCurrent3DChart）
    DAChart3DWidget* current3DChart() const;
    // 当前激活的是否为 3D 子图（瀑布模式为三维且存在当前 3D 子图）
    bool is3DChartActive() const;
    // 设置当前 2D 子图
    void setCurrentChart(DAChartWidget* chart);
    // figure id（"sig-<信号名>"；T9 存档细化）
    QString figureId() const;
    // 网格行/列比例（归一化，和≈1）
    QList< qreal > gridRowSizes() const;
    QList< qreal > gridColumnSizes() const;
    // 恢复网格比例（尺寸不符/非法值忽略）
    void setGridSizes(const QList< qreal >& rows, const QList< qreal >& cols);
    // 兼容旧存档接口（T7 临时保留供旧 UI.cpp 编译，T8 改为 gridRowSizes/gridColumnSizes 后删除）
    QList< int > verticalSplitterSizes() const;
    QList< int > horizontalSplitterSizes() const;
    void setSplitterSizes(const QList< int >& vertical, const QList< int >& horizontal);

Q_SIGNALS:
    // 请求外部计算频谱（参数变化或首次显示）
    void spectrumRequested(const DA::DASignalPtr& signal, int channel);
    // 通道切换（外部重新发起全部分析）
    void channelChanged(int channel);
    // 请求外部计算包络（start/end 为采样点，width 为目标列数）
    void envelopeRequested(qint64 start, qint64 end, int width);
    // 请求外部计算时频热图（columns/rows 为显示栅格目标尺寸）
    void heatmapRequested(int columns, int rows);
    // 当前时间变化（滑块/点击/播放头）
    void currentTimeChanged(double seconds);
    // 「跟随滑块」开关变化（只影响显示与峰值表刷新，不触发 seek）
    void followSliderToggled(bool on);
    // 当前 2D/3D 子图切换（网格点击 / 瀑布模式切换 / 切信号），T8 设置面板 target 用
    void currentSubChartChanged();

public Q_SLOTS:
    // 设置当前时间（秒），同步播放头
    void setCurrentTime(double seconds);

protected:
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void changeEvent(QEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* e) override;

private Q_SLOTS:
    void retranslateUi();
    // 当前 2D 子图变化（DAFigureWidget::currentChartChanged）
    void onFigureCurrentChartChanged(DA::DAChartWidget* chart);
    // 当前 3D 子图变化（DAFigureWidget::current3DChartChanged）
    void onFigureCurrent3DChartChanged(DA::DAChart3DWidget* chart);

private:
    void buildUI();
    void buildFigure();
    // T7 临时底部工具栏（TODO(T8)：删除，控件迁 Ribbon/窗口底部工具栏）
    void buildTemporaryToolbar();
    void rebuildWaterfall();
    void rebuildWavePlot();
    void rebuildSpectrumPlot();
    void rebuildPeakMarkers();
    void rebuildHeatmap();
    void updatePlayhead();
    void updateStftWindowRect();
    void updateAxisTitles();
    void applyHeatmapColors();
    // 当前时间对应的 STFT 列索引（跟随滑块用）
    int currentStftColumn() const;
    // 幅值 → 显示口径（乘标定系数；dB 时换算对数）
    QVector< double > displayAmplitudes() const;
    // dB 参考值（满量程 = 信号峰值，1.0 = 固定 1.0）
    double dbReferenceValue() const;
    // 按包络统计信号峰值：原始峰值与 dBFS 参考峰值（去直流/去趋势口径，评审 N-C2）
    void updateSignalPeaks();
    // 让临时工具栏控件反映当前成员状态（防止程序化 setter 与控件不同步）
    void syncTemporaryToolbar();

private:
    DASignalPtr mSignal;
    DASignalAnalysisParams mParams;
    DASpectrumResult mSpectrum;

    // 包络缓存
    QVector< double > mEnvXs;
    QVector< double > mEnvMins;
    QVector< double > mEnvMaxs;
    // 原始峰值（波形范围用）与 dBFS 参考峰值（预处理口径，signalPeak() 返回）
    double mSignalPeak { 0.0 };
    double mDbfsReferencePeak { 0.0 };
    bool mEnvelopeValid { false };

    // 图表文档
    DAFigureWidget* mFigure { nullptr };
    DAChartWidget* mWaveChart { nullptr };
    DAChartWidget* mSpectrumChart { nullptr };
    DAChartWidget* mHeatmapChart { nullptr };
    DAChart3DWidget* mWaterfallChart { nullptr };
    // 网格比例是否已被外部（存档恢复）显式设置，未设置时首次显示用默认比例
    bool mGridSizesApplied { false };
    // 当前激活子图是否为 3D（点击 3D 置 true，点击任一 2D 子图置 false）
    bool mCurrentIs3D { false };

    // 视图状态（T8 起由 Ribbon/底部工具栏驱动；临时工具栏只是视图）
    int mCurrentChannel { 0 };
    double mCurrentTimeSeconds { 0.0 };
    bool mFollowSlider { false };
    QString mColormapName { QStringLiteral("viridis") };
    double mDbFloor { -80.0 };
    bool mPeakMarkersVisible { false };
    bool mHeatmapAutoScroll { false };
    int mWaterfallMode { 0 };  ///< 0=二维 / 1=三维 / 2=隐藏
    // 显示用时间原点（秒）：数据帧时间列首值非 0 时三图横向对齐
    double mTimeOrigin { 0.0 };
    // 频率轴模式：false 线性 / true 对数
    bool mLogFrequency { false };

    // T7 临时工具栏（TODO(T8)：删除成员与 buildTemporaryToolbar/syncTemporaryToolbar）
    QToolBar* mTempToolbar { nullptr };
    QComboBox* mChannelCombo { nullptr };
    QComboBox* mFreqAxisCombo { nullptr };
    QComboBox* mColormapCombo { nullptr };
    QDoubleSpinBox* mDbFloorSpin { nullptr };
    QCheckBox* mFollowSliderCheck { nullptr };
    QCheckBox* mAutoScrollCheck { nullptr };
    QSlider* mTimeSlider { nullptr };
    QTimer* mResizeDebounce { nullptr };

    // 图元
    QwtPlotIntervalCurve* mEnvelopeCurve { nullptr };
    QwtPlotMarker* mPlayhead { nullptr };
    QwtPlotShapeItem* mStftWindow { nullptr };
    QwtPlotCurve* mSpectrumCurve { nullptr };
    QwtPlotSpectrogram* mSpectrogramItem { nullptr };
    DAStftRasterData* mStftRaster { nullptr };
    QVector< QwtPlotMarker* > mPeakMarkers;

    // 时频热图结果与显示设置
    DAStftResult mStft;
    DAPeakResult mPeaks;
    // STFT 窗口矩形拖动（D30：整体可拖、不可拖边）
    bool mDraggingWindow { false };
    // 左键按下但还没超过拖动阈值（评审 N-U5：矩形内单击仍要能定位）
    bool mPressCandidate { false };
    QPoint mDragPressPos;
    double mDragPressTime { 0.0 };
    // 三维瀑布
    Qwt3DWaterfall* mWaterfallItem { nullptr };
};
}  // namespace DA

#endif  // DASIGNALANALYSISVIEW_H
