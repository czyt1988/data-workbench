#ifndef DASIGNALANALYSISVIEW_H
#define DASIGNALANALYSISVIEW_H
#include <QWidget>
#include <QVector>
#include "Signal/DASignal.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASpectrumResult.h"

class QSlider;
class QComboBox;
class QSplitter;
class QStackedWidget;
class QTimer;
class QResizeEvent;

class QwtPlotCurve;
class QwtPlotIntervalCurve;
class QwtPlotMarker;
class QwtPlotShapeItem;
class QwtPlotSpectrogram;

namespace DA
{
class DASignalChartPanel;

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
    // 当前时间（秒）
    double currentTime() const;
    // 幅值显示模式（不重算，只重绘）
    void setAmplitudeDisplay(const QString& amplitudeMode, const QString& dbReference);
    // 请求一次包络计算（视图显示/尺寸变化时调用）
    void requestEnvelopeRefresh();

Q_SIGNALS:
    // 请求外部计算频谱（参数变化或首次显示）
    void spectrumRequested(const DA::DASignalPtr& signal);
    // 请求外部计算包络（start/end 为采样点，width 为目标列数）
    void envelopeRequested(qint64 start, qint64 end, int width);
    // 当前时间变化（滑块/点击/播放头）
    void currentTimeChanged(double seconds);

public Q_SLOTS:
    // 设置当前时间（秒），同步滑块与播放头
    void setCurrentTime(double seconds);

protected:
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void changeEvent(QEvent* e) override;

private Q_SLOTS:
    void onSliderMoved(int value);
    void retranslateUi();

private:
    void buildUI();
    void rebuildWavePlot();
    void rebuildSpectrumPlot();
    void updatePlayhead();
    void updateAxisTitles();
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
    QSlider* mTimeSlider { nullptr };
    QTimer* mResizeDebounce { nullptr };

    // 图元
    QwtPlotIntervalCurve* mEnvelopeCurve { nullptr };
    QwtPlotMarker* mPlayhead { nullptr };
    QwtPlotShapeItem* mStftWindow { nullptr };
    QwtPlotCurve* mSpectrumCurve { nullptr };
    QwtPlotSpectrogram* mSpectrogramItem { nullptr };

    // 频率轴模式：false 线性 / true 对数
    bool mLogFrequency { false };
};
}  // namespace DA

#endif  // DASIGNALANALYSISVIEW_H
