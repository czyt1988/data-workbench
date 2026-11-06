#ifndef DASIGNALANALYSISVIEW_H
#define DASIGNALANALYSISVIEW_H
#include <QWidget>
#include <QVector>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"
#include "Analysis/DAStftResult.h"
#include "Analysis/DAPeakResult.h"

class QSlider;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QSplitter;
class QStackedWidget;
class QTimer;
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
class DAChart3DWidget;
class DASignalChartPanel;
class DAStftRasterData;

/**
 * @brief 单个信号的分析视图（方案 §8.2 / D25）
 *
 * 布局：
 * - 上半：波形图（包络降采样 + 播放头 + 时间滑块 + STFT 窗口矩形）
 * - 下半：左右并排——频谱图（线性/对数频率轴切换）+ 右侧叠置（二维热图 / 三维瀑布 / 隐藏）
 *
 * 视图不自己算频谱与包络：长任务由外部（UI 层的 worker）在 Python 守护线程算完回填。
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
    // 分割比例（视图状态持久化用；空列表表示不恢复）
    QList< int > verticalSplitterSizes() const;
    QList< int > horizontalSplitterSizes() const;
    void setSplitterSizes(const QList< int >& vertical, const QList< int >& horizontal);
    // 时间轴原点（秒）：数据帧时间列首值非 0 时三张图横向对齐用（评审 U12）
    void setTimeOrigin(double seconds);
    // 信号峰值幅度（包络回填时统计；dBFS 参考与峰值表共用）
    double signalPeak() const;
    // 热图随播放自动滚动开关（D27，默认关）
    void setHeatmapAutoScroll(bool on);
    bool isHeatmapAutoScroll() const;
    // 播放头跟随（外部按播放位置调用，避免与用户 seek 混淆）
    void setPlayheadFromPlayback(double seconds);
    // 当前时间（秒）
    double currentTime() const;
    // 当前通道（多通道信号一次分析一个通道）
    int currentChannel() const;
    void setCurrentChannel(int channel);
    // 是否「跟随滑块」显示当前帧（D13：默认整段）
    bool isFollowSlider() const;
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

public Q_SLOTS:
    // 设置当前时间（秒），同步滑块与播放头
    void setCurrentTime(double seconds);

protected:
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void changeEvent(QEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* e) override;

private Q_SLOTS:
    void onSliderMoved(int value);
    void retranslateUi();

private:
    void buildUI();
    void rebuildWaterfall();
    void rebuildWavePlot();
    void rebuildSpectrumPlot();
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

private:
    DASignalPtr mSignal;
    DASignalAnalysisParams mParams;
    DASpectrumResult mSpectrum;

    // 包络缓存
    QVector< double > mEnvXs;
    QVector< double > mEnvMins;
    QVector< double > mEnvMaxs;
    double mSignalPeak { 0.0 };
    bool mEnvelopeValid { false };

    // 布局
    QSplitter* mVerticalSplitter { nullptr };
    QSplitter* mHorizontalSplitter { nullptr };
    DASignalChartPanel* mWavePanel { nullptr };
    DASignalChartPanel* mSpectrumPanel { nullptr };
    QStackedWidget* mRightStack { nullptr };
    QComboBox* mFreqAxisCombo { nullptr };
    QComboBox* mChannelCombo { nullptr };
    QSlider* mTimeSlider { nullptr };
    QTimer* mResizeDebounce { nullptr };

    // 图元
    QwtPlotIntervalCurve* mEnvelopeCurve { nullptr };
    QwtPlotMarker* mPlayhead { nullptr };
    QwtPlotShapeItem* mStftWindow { nullptr };
    QwtPlotCurve* mSpectrumCurve { nullptr };
    QwtPlotSpectrogram* mSpectrogramItem { nullptr };
    DAStftRasterData* mStftRaster { nullptr };

    // 时频热图结果与显示设置
    DAStftResult mStft;
    bool mFollowSlider { false };
    double mDbFloor { -80.0 };
    // 显示用时间原点（秒）：数据帧时间列首值非 0 时三图横向对齐
    double mTimeOrigin { 0.0 };
    DASignalChartPanel* mHeatmapPanel { nullptr };
    // 峰值标记
    DAPeakResult mPeaks;
    bool mPeakMarkersVisible { false };
    QVector< QwtPlotMarker* > mPeakMarkers;
    QComboBox* mColormapCombo { nullptr };
    QDoubleSpinBox* mDbFloorSpin { nullptr };
    QCheckBox* mFollowSliderCheck { nullptr };
    // 热图随播放滚动（D27，默认关）
    QCheckBox* mAutoScrollCheck { nullptr };
    bool mHeatmapAutoScroll { false };
    // STFT 窗口矩形拖动（D30：整体可拖、不可拖边）
    bool mDraggingWindow { false };
    QPoint mDragPressPos;
    double mDragPressTime { 0.0 };
    // 三维瀑布
    DA::DAChart3DWidget* mWaterfallChart { nullptr };
    Qwt3DWaterfall* mWaterfallItem { nullptr };
    int mWaterfallMode { 0 };  ///< 0=二维 / 1=三维 / 2=隐藏

    // 频率轴模式：false 线性 / true 对数
    bool mLogFrequency { false };
};
}  // namespace DA

#endif  // DASIGNALANALYSISVIEW_H
