#ifndef DASIGNALANALYSISVIEW_H
#define DASIGNALANALYSISVIEW_H
#include <QWidget>
#include <QPointer>
#include <QPointF>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QJsonObject>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"
#include "Analysis/DAStftResult.h"
#include "Analysis/DAPeakResult.h"
// 联动缩放槽需要 DAChartWidget::ChartPropertyChangeFlags（嵌套类型不能用前向声明）
#include "DAChartWidget.h"

class QTimer;
class QResizeEvent;
class QPixmap;

class QwtPlotItem;
class QwtPlotCurve;
class QwtPlotIntervalCurve;
class QwtPlotMarker;
class QwtPlotPicker;
class QwtPlotShapeItem;
class QwtPlotSpectrogram;
class Qwt3DWaterfall;

namespace DA
{
class DAFigureWidget;
class DAChartWidget;
class DAChart3DWidget;
class DAStftRasterData;
class DAWaterfallColor;

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
 * 注：视图内不留 inline 控件（除播放头/STFT 矩形等画布元素）：显示类控件在 Ribbon
 * 上下文标签、播放类在窗口底部工具栏、外观类在右侧设置面板（T8）；本类的 setter/getter
 * 是这些入口共同操作的状态源。
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
    // 峰值标记当前显隐（与 setter 成对：丝带勾选状态以此为准，不再依赖 UI 全局值）
    bool peakMarkersVisible() const;
    // 峰值标记文本模板（{index}/{fre}/{amp}，空串表示不显示文本）
    void setPeakMarkerText(const QString& textTemplate);
    QString peakMarkerText() const;
    // 当前峰值标记图元（存档保存用）
    QList< QwtPlotMarker* > peakMarkers() const;
    // 接管从存档恢复的峰值标记（追加、同步显隐状态，不重建/replot）
    void adoptRestoredPeakMarkers(const QList< QwtPlotMarker* >& markers);
    // 瀑布模式：0=二维热图 / 1=三维曲面 / 2=隐藏（D12 / D25）
    void setWaterfallMode(int mode);
    int waterfallMode() const;
    // 色带与 dB 动态范围下限（视图状态持久化用）
    QString colormapName() const;
    void setColormapName(const QString& name);
    // 从谱图图元当前色带回读预设名（唯一真相 = item 的 colorMap；不匹配返回 "custom"）
    QString colormapPreset() const;
    double dbFloor() const;
    void setDbFloor(double dbFloor);
    // 云图显示模式：0=图像 / 1=等高线 / 2=图像+等高线（状态真相 = 谱图图元，供宿主面板双向使用）
    int heatmapDisplayMode() const;
    void setHeatmapDisplayMode(int mode);
    // 云图采样插值：0=最近邻 / 1=双线性（状态真相 = 栅格数据）
    int heatmapResampleMode() const;
    void setHeatmapResampleMode(int mode);
    // 当前帧标记开关（二维云图竖线 + 三维当前迹线高亮的时间游标；默认开）
    bool isTimeCursorVisible() const;
    void setTimeCursorVisible(bool on);
    // 联动缩放（波形 x↔云图 x、频谱 x↔云图 y；默认关）
    bool isLinkedZoom() const;
    void setLinkedZoom(bool on);
    // 双光标测量（默认关）
    bool isDualCursorEnabled() const;
    void setDualCursorEnabled(bool on);
    // 峰值保持（Max Hold，默认关；只在跟随播放时有效）
    bool isPeakHoldEnabled() const;
    void setPeakHoldEnabled(bool on);
    // 清空峰值保持缓冲（开启时/切通道/改参数/关闭跟随时由内部调用，丝带另有手动重置）
    void resetPeakHold();
    // 时间轴标签格式：0=秒 / 1=毫秒 / 2=mm:ss.s（作用到波形 x 与云图 x）
    int timeFormat() const;
    void setTimeFormat(int format);
    // 三维瀑布专属显示状态：dB 色域下限 / 色带预设 / 当前迹线高亮（与二维云图相互独立，D12）
    double waterfallDbFloor() const;
    void setWaterfallDbFloor(double dbFloor);
    QString waterfallColormapName() const;
    void setWaterfallColormapName(const QString& name);
    bool isWaterfallHighlighted() const;
    void setWaterfallHighlight(bool on);
    // 三维瀑布当前着色方式（0=按深度 / 1=按幅值 / 2=单色；无图元返回 -1）
    int waterfallColorMode() const;
    // 三维瀑布图元（瀑布设置快捷入口定位当前图元用）
    Qwt3DWaterfall* waterfall3DItem() const;
    // 三维瀑布样式（保存时从图元读回：style/colorMode/opacity/depthOrder/outlineColor/outlineWidth/solidColor）
    QJsonObject waterfallStyle() const;
    // 应用三维瀑布样式（图元未建时先记为待应用，首建后套用）
    void applyWaterfallStyle(const QJsonObject& style);
    // 三维瀑布样式恢复默认（RibbonMesh / ByAmplitude / 1.0 / NewestInFront）
    void resetWaterfallStyle();
    // 时频图色标（colorbar）显隐：隐藏时不占横向空间（默认显示）
    void setColorBarVisible(bool on);
    bool isColorBarVisible() const;
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
    // 跟随播放的当前帧幅值谱回填（外部同步计算完成后调用；与峰值曲线同口径）
    void setFollowFrameSpectrum(const QVector< double >& freqs, const QVector< double >& amplitudes);
    // 播放头跟随（外部按播放位置调用，避免与用户 seek 混淆）
    void setPlayheadFromPlayback(double seconds);
    // 「点击波形即定位」开关：缩放/平移/十字线等交互工具启用时关闭（SF-12）
    void setSeekEnabled(bool on);
    bool isSeekEnabled() const;
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
    // 清空全部实时分析结果显示（频谱曲线/热图栅格/三维瀑布/峰值标记）；
    // 切通道时调用：只清一次，播放路径不动（避免闪烁）
    void clearRealtimePlots();
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
    // figure id（"sig-<信号唯一id>"；存档恢复时沿用存档里的持久 id）
    QString figureId() const;
    // 网格行/列比例（归一化，和≈1）
    QList< qreal > gridRowSizes() const;
    QList< qreal > gridColumnSizes() const;
    // 恢复网格比例（尺寸不符/非法值忽略）
    void setGridSizes(const QList< qreal >& rows, const QList< qreal >& cols);
    // 按 figure XML 恢复后刷新（replot/轴标题/瀑布模式显隐，不触发新的分析请求）
    void refreshAfterFigureRestore();

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
    // 请求当前跟随帧的幅值谱（帧号按 STFT 帧栅格换算；外部同步计算后回填）
    void followFrameSpectrumRequested(qint64 frameIndex);
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
    void rebuildWaterfall();
    void rebuildWavePlot();
    void rebuildSpectrumPlot();
    void rebuildPeakMarkers();
    // 按当前峰值数据 + 已绘制曲线快照算出可画标记位置（曲线范围外的峰被跳过）
    QVector< QPointF > visiblePeakMarkerPositions() const;
    // 就地更新已有峰值标记坐标（重绘/播放路径：不增删图元；数量不匹配时回退完整重建）
    void updatePeakMarkerPositions();
    // 按当前模板重写全部峰值标记的文本（原点位置不变：同时适用于重算生成的与存档接管的标记）
    void applyPeakMarkerLabels();
    void rebuildHeatmap();
    void updatePlayhead();
    void updateStftWindowRect();
    void updateAxisTitles();
    void applyHeatmapColors();
    // 校验并按需重建波形画布的装饰图元（播放头/STFT 窗口矩形；被外部删除后按原样重建，BLK-03a）
    void ensureWaveDecorItems();
    // 校验并按需重建云图画布的装饰图元（当前帧竖线/窗宽带；瞬时图元，不进存档）
    void ensureHeatmapDecorItems();
    // 当前帧标记：把云图竖线与 STFT 窗宽带移到给定时间（含像素节流）
    void updateHeatmapCursor(double absoluteTime);
    // 整份 STFT 栅格的幅值峰值（云图与瀑布共用同一 dB 归一化口径）
    double stftGridPeak() const;
    // 重建三维瀑布的色带 functor（子类，含当前迹线高亮）
    void applyWaterfallColorFunctor();
    // 三维瀑布图例刻度/标题对齐 dB 色域（qwt 不自动跟数据）
    void updateWaterfallLegend();
    // 三维瀑布当前迹线高亮：把当前时间映射到最近槽位，槽位变化才重建顶点色
    void updateWaterfallHighlight();
    // dB 色域下限的有效值（[floor, 0] 需要 floor < 0）
    double waterfallDbFloorLower() const;
    // 把待应用的云图显示模式 / 插值写到谱图图元与栅格（图元不存在时空操作）
    void applyHeatmapDisplayMode(int mode);
    void applyHeatmapResampleMode();
    // 联动缩放：按需挂接各 2D 子图的 zoomer/panner（工具是懒创建的，不能替用户开工具）
    void refreshLinkedZoomConnections();
    // 2D 子图属性变化（缩放/平移工具开关）→ 重新尝试挂接联动
    void onLinkedChartPropertiesChanged(DAChartWidget* chart, DAChartWidget::ChartPropertyChangeFlags flag);
    // 某个子图的缩放/平移发生后，把相关坐标轴同步到其它子图
    void syncLinkedAxesFrom(DAChartWidget* source);
    // 断开全部联动缩放连接
    void disconnectLinkedZoom();
    // 按要求重建峰值保持曲线（灰虚线，transient；不参与导出）
    void updatePeakHoldCurve();
    // 把当前跟随帧谱并入峰值保持缓冲（网格变化时重建）
    void accumulatePeakHold();
    // 双光标测量：在当前 2D 子图上放置 A/B，刷新读数文本
    void placeDualCursor(DAChartWidget* chart, const QPoint& canvasPos);
    void updateDualCursorReadout();
    // 清理双光标图元（关闭开关 / 切子图 / 切信号 / 参数变化）
    void clearDualCursor();
    // 当前时间对应的 STFT 列索引（跟随滑块用）
    int currentStftColumn() const;
    // 时间轴标签格式应用到波形 x 与云图 x（换新 ScaleDraw 实例，plot 接管所有权）
    void applyTimeAxisFormat();
    // 时间轴标题（随格式变：Time (s) / Time (ms) / Time (mm:ss)）
    QString timeAxisTitle() const;
    // 按当前时间请求「当前帧幅值谱」（同一帧不重复请求）；跟随关闭/STFT 未就绪时不做
    void requestFollowFrameSpectrum();
    // 作废跟随帧快照（切通道/参数变化/STFT 栅格变化时调用）
    void clearFollowFrameSpectrum();
    // 幅值 → 显示口径（乘标定系数；dB 时换算对数）
    QVector< double > displayAmplitudes() const;
    // dB 参考值（满量程 = 信号峰值，1.0 = 固定 1.0）
    double dbReferenceValue() const;
    // 按包络统计信号峰值：原始峰值与 dBFS 参考峰值（去直流/去趋势口径，评审 N-C2）
    void updateSignalPeaks();

private:
    DASignalPtr mSignal;
    DASignalAnalysisParams mParams;
    DASpectrumResult mSpectrum;
    // 当前真正画到频谱图上的曲线快照（峰值标记吸附到曲线峰顶用；跟随播放时为当前帧谱）
    QVector< double > mDisplayedFreqs;
    QVector< double > mDisplayedValues;

    // 包络缓存
    QVector< double > mEnvXs;
    QVector< double > mEnvMins;
    QVector< double > mEnvMaxs;
    // 原始峰值（波形范围用）与 dBFS 参考峰值（预处理口径，signalPeak() 返回）
    double mSignalPeak { 0.0 };
    double mDbfsReferencePeak { 0.0 };
    bool mEnvelopeValid { false };

    // 图表文档
    // figure 归视图所有（父对象），子图可被共享图元树删除：缓存用 QPointer 防悬空（BLK-03a）
    DAFigureWidget* mFigure { nullptr };
    QPointer< DAChartWidget > mWaveChart;
    QPointer< DAChartWidget > mSpectrumChart;
    QPointer< DAChartWidget > mHeatmapChart;
    QPointer< DAChart3DWidget > mWaterfallChart;
    // 当前激活子图是否为 3D（点击 3D 置 true，点击任一 2D 子图置 false）
    bool mCurrentIs3D { false };

    // 视图状态（T8 起由 Ribbon/底部工具栏驱动；临时工具栏只是视图）
    int mCurrentChannel { 0 };
    double mCurrentTimeSeconds { 0.0 };
    bool mFollowSlider { false };
    QString mColormapName { QStringLiteral("viridis") };
    double mDbFloor { -80.0 };
    // 时频图色标（yRight colorbar）显隐
    bool mColorBarVisible { true };
    bool mPeakMarkersVisible { false };
    bool mHeatmapAutoScroll { false };
    int mWaterfallMode { 0 };  ///< 0=二维 / 1=三维 / 2=隐藏
    // 显示用时间原点（秒）：数据帧时间列首值非 0 时三图横向对齐
    double mTimeOrigin { 0.0 };
    // 当前帧标记开关（默认开）：二维云图竖线 / STFT 窗宽带 / 三维当前迹线高亮
    bool mTimeCursorVisible { true };
    // 联动缩放 / 双光标测量 / 峰值保持（默认关；由丝带「测量 ▾」与「时频图设置」驱动）
    bool mLinkedZoom { false };
    bool mDualCursorEnabled { false };
    bool mPeakHoldEnabled { false };
    // 联动缩放：同步期间的防重入标志与已建立的连接（关闭/切图时逐条断开）
    bool mSyncingAxes { false };
    QList< QMetaObject::Connection > mLinkedZoomConnections;
    // 已挂接联动信号的子图与工具（zoomer/panner 懒创建且复用，按对象去重）
    QSet< const QObject* > mLinkedZoomCharts;
    QSet< const QObject* > mLinkedZoomTools;
    // 双光标测量：当前子图、A/B 与 Δ 文本图元（全部 transient）、下一次点击放 B
    QPointer< DAChartWidget > mDualCursorChart;
    QwtPlotMarker* mDualCursorA { nullptr };
    QwtPlotMarker* mDualCursorB { nullptr };
    QwtPlotMarker* mDualCursorDelta { nullptr };
    bool mDualCursorNextIsB { false };
    // 峰值保持（Max Hold）：跟随帧谱按频率网格取最大值，一条灰虚线（transient，不参与导出）
    QVector< double > mHoldFreqs;
    QVector< double > mHoldValues;  ///< 原始线性幅值的最大值（显示口径在绘制时换算）
    QwtPlotCurve* mHoldCurve { nullptr };
    // 时间轴格式：0=秒 / 1=毫秒 / 2=mm:ss.s
    int mTimeFormat { 0 };

    // T7 临时工具栏已删除（T8）：控件迁到 Ribbon 上下文标签与窗口底部工具栏
    QTimer* mResizeDebounce { nullptr };

    // 图元
    // 缓存图元可能被共享图元树删除（detach+delete）：使用前经 ensureItemAttached 校验（BLK-03a）
    QwtPlotIntervalCurve* mEnvelopeCurve { nullptr };
    QwtPlotMarker* mPlayhead { nullptr };
    QwtPlotShapeItem* mStftWindow { nullptr };
    QwtPlotCurve* mSpectrumCurve { nullptr };
    QwtPlotSpectrogram* mSpectrogramItem { nullptr };
    // 云图当前帧标记：半透明虚线竖线 + STFT 窗宽带（都 transient，不写入工程）
    QwtPlotMarker* mHeatmapCursor { nullptr };
    QwtPlotShapeItem* mHeatmapWindowBand { nullptr };
    // 节流：上次绘制时的像素列与被显式作废标志（数据/几何变化后置 false）
    double mHeatmapCursorPixel { 0.0 };
    bool mHeatmapCursorApplied { false };
    DAStftRasterData* mStftRaster { nullptr };
    QVector< QwtPlotMarker* > mPeakMarkers;
    // 峰值标记文本模板（{index}/{fre}/{amp}），由丝带「标记显示」对话框写入
    QString mPeakMarkerText;
    // 峰值标记已从存档接管：rebuildPeakMarkers 不再删除重建（存档优先，用户可能手动调过）。
    // 该优先只在「加载后到第一次重算前」成立：setParams/setAmplitudeDisplay/切通道复位（SF-13）
    bool mPeakMarkersRestored { false };
    // 「点击波形即定位」picker（SF-12：缩放/平移/十字线启用时停用）
    QwtPlotPicker* mSeekPicker { nullptr };
    bool mSeekEnabled { true };

    // 时频热图结果与显示设置
    DAStftResult mStft;
    // 跟随播放的当前帧幅值谱快照（与峰值曲线同口径）：有效时优先于 STFT 栅格列
    QVector< double > mFollowFreqs;
    QVector< double > mFollowAmplitudes;
    bool mFollowFrameValid { false };
    qint64 mFollowFrameIndex { -1 };
    DAPeakResult mPeaks;
    // STFT 窗口矩形拖动（D30：整体可拖、不可拖边）
    bool mDraggingWindow { false };
    // 左键按下但还没超过拖动阈值（评审 N-U5：矩形内单击仍要能定位）
    bool mPressCandidate { false };
    QPoint mDragPressPos;
    double mDragPressTime { 0.0 };
    // 三维瀑布
    Qwt3DWaterfall* mWaterfallItem { nullptr };
    // 色带 functor 由图元持有（setDataColor 接管所有权）：这里是非 owning 指针，图元重建时置空
    DAWaterfallColor* mWaterfallColor { nullptr };
    // 迹线时间（与瀑布迹线一一对应）与当前高亮槽位
    QVector< double > mWaterfallTraceTimes;
    int mWaterfallHighlightSlot { -1 };
    // 三维瀑布专属显示状态（与二维云图独立，D12）
    double mWaterfallDbFloor { -80.0 };
    QString mWaterfallColormapName { QStringLiteral("viridis") };
    bool mWaterfallHighlight { true };
    // 待应用的瀑布样式（工程恢复时 STFT 异步，图元尚不存在，首建后套用）
    QJsonObject mPendingWaterfallStyle;
    // 待应用的云图显示状态（工程恢复时谱图图元尚未重建；-1 表示无待应用值）
    int mPendingHeatmapDisplayMode { -1 };
    int mPendingHeatmapResampleMode { -1 };
};
}  // namespace DA

#endif  // DASIGNALANALYSISVIEW_H
