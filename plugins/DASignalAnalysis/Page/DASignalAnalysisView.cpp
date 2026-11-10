#include "DASignalAnalysisView.h"
#include <cmath>
#include <limits>
#include <utility>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>
// qwt
#include "qwt_plot_curve.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_scale_widget.h"
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"
#include "qwt_raster_data.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_scale_engine.h"
#include "qwt_plot_picker.h"
#include "qwt_picker_machine.h"
#include "qwt_samples.h"
#include "qwt_symbol.h"
// DA
#include "DAChartWidget.h"
#include "DASignalChartPanel.h"
#include "Analysis/DASignalAmplitude.h"
#include "Render/DAStftRasterData.h"
#include "Render/DAWaterfallBuilder.h"
#include "DAChart3DWidget.h"
#include "qwt3d_waterfall.h"

namespace DA
{
namespace
{
/**
 * @brief 深拷贝线性色图（QwtLinearColorMap 拷贝构造被删除）
 */
QwtColorMap* cloneLinearColorMap(const QwtLinearColorMap* src)
{
    if (!src) {
        return nullptr;
    }
    auto* dst = new QwtLinearColorMap(src->format());
    dst->setMode(src->mode());
    const QVector< double > pos   = src->stopPos();
    const QVector< QColor > cols  = src->stopColors();
    const int n                   = qMin(pos.size(), cols.size());
    for (int i = 0; i < n; ++i) {
        dst->addColorStop(pos[ i ], cols[ i ]);
    }
    return dst;
}

/**
 * @brief 清理峰值标记图元
 */
void clearPeakMarkers(QVector< QwtPlotMarker* >& markers)
{
    for (QwtPlotMarker* m : std::as_const(markers)) {
        m->detach();
        delete m;
    }
    markers.clear();
}

constexpr double cEps = 1e-30;
constexpr int cEnvelopeDebounceMs = 200;
}  // namespace

DASignalAnalysisView::DASignalAnalysisView(const DASignalPtr& signal, QWidget* parent)
    : QWidget(parent), mSignal(signal)
{
    buildUI();
    retranslateUi();
}

DASignalAnalysisView::~DASignalAnalysisView()
{
}

DASignalPtr DASignalAnalysisView::signal() const
{
    return mSignal;
}

void DASignalAnalysisView::setParams(const DASignalAnalysisParams& params)
{
    mParams     = params;
    mTimeOrigin = mSignal ? mSignal->timeOrigin() : 0.0;
    rebuildSpectrumPlot();
    // 参数变化后热图（窗长/重叠/零填充）与矩形几何都跟着变（评审 U6 / D21）
    rebuildHeatmap();
    updateStftWindowRect();
    updateAxisTitles();
}

DASignalAnalysisParams DASignalAnalysisView::params() const
{
    return mParams;
}

/**
 * @brief 频谱结果回填并重绘
 */
void DASignalAnalysisView::setSpectrum(const DASpectrumResult& result)
{
    mSpectrum = result;
    rebuildSpectrumPlot();
}

/**
 * @brief 包络结果回填并重绘波形
 */
void DASignalAnalysisView::setEnvelope(const QVector< double >& xs,
                                       const QVector< double >& mins,
                                       const QVector< double >& maxs)
{
    mEnvXs    = xs;
    mEnvMins  = mins;
    mEnvMaxs  = maxs;
    mEnvelopeValid = !xs.isEmpty();
    // 信号峰值（波形范围 + dBFS 参考；两者口径不同，见 updateSignalPeaks）
    updateSignalPeaks();
    rebuildWavePlot();
    if (mSpectrum.isValid()) {
        rebuildSpectrumPlot();
    }
}

/**
 * @brief 按包络统计信号峰值
 *
 * 包络每列给出该像素列内的最小/最大值。这里同时产出两套峰值：
 * - mSignalPeak：原始峰值（含直流/趋势），只用于波形上与 STFT 窗口矩形的纵向范围；
 * - mDbfsReferencePeak：按与频谱/峰值相同的预处理口径（params.removeDc / params.detrend）
 *   去掉基线后的峰值，作为 dBFS 参考（评审 N-C2：原实现用原始峰值当参考，
 *   有直流偏置或趋势的信号 dB 值整体偏低，与去直流/去趋势后的频谱口径不一致）。
 * 复杂度 O(列数)，列数 ≈ 画布像素宽，界面线程可接受。
 */
void DASignalAnalysisView::updateSignalPeaks()
{
    const int n = qMin(mEnvMins.size(), mEnvMaxs.size());
    double rawPeak = 0.0;
    for (int i = 0; i < n; ++i) {
        rawPeak = qMax(rawPeak, qMax(std::fabs(mEnvMins[ i ]), std::fabs(mEnvMaxs[ i ])));
    }
    mSignalPeak = rawPeak;

    const QString detrend = mParams.detrend.toLower();
    const bool removeLinear = (detrend == QLatin1String("linear"));
    const bool removeLevel  = mParams.removeDc || removeLinear || (detrend == QLatin1String("constant"));
    if (!removeLevel || n <= 0) {
        mDbfsReferencePeak = rawPeak;
        return;
    }
    // 用每列中点估计基线：常量去趋势取均值，线性去趋势取最小二乘拟合
    double meanCenter = 0.0;
    for (int i = 0; i < n; ++i) {
        meanCenter += 0.5 * (mEnvMins[ i ] + mEnvMaxs[ i ]);
    }
    meanCenter /= double(n);
    double slope = 0.0;
    if (removeLinear && n > 1) {
        double sumX = 0.0, sumY = 0.0, sumXX = 0.0, sumXY = 0.0;
        for (int i = 0; i < n; ++i) {
            const double x = double(i);
            const double y = 0.5 * (mEnvMins[ i ] + mEnvMaxs[ i ]);
            sumX += x;
            sumY += y;
            sumXX += x * x;
            sumXY += x * y;
        }
        const double denom = double(n) * sumXX - sumX * sumX;
        if (std::fabs(denom) > 1e-12) {
            slope = (double(n) * sumXY - sumX * sumY) / denom;
        }
    }
    const double intercept = meanCenter - slope * 0.5 * double(n - 1);
    double peak = 0.0;
    for (int i = 0; i < n; ++i) {
        const double base = intercept + slope * double(i);
        peak = qMax(peak, qMax(std::fabs(mEnvMins[ i ] - base), std::fabs(mEnvMaxs[ i ] - base)));
    }
    mDbfsReferencePeak = (peak > 0.0) ? peak : rawPeak;
}

/**
 * @brief 构建三图布局（方案 §8.2 / D25）
 */
void DASignalAnalysisView::buildUI()
{
    mVerticalSplitter   = new QSplitter(Qt::Vertical, this);
    mVerticalSplitter->setChildrenCollapsible(false);

    // ---------- 上半：波形 ----------
    mWavePanel = new DASignalChartPanel(QStringLiteral("Waveform"), mVerticalSplitter);  // cn:波形
    // 多通道下拉：一次分析一个通道（方案 §2.1）
    mChannelCombo = new QComboBox(mWavePanel);
    mChannelCombo->setObjectName(QStringLiteral("da-signal-channel-combo"));
    const QStringList chNames = (mSignal ? mSignal->channelNamesOrDefaults() : QStringList());
    for (int i = 0; i < chNames.size(); ++i) {
        mChannelCombo->addItem(chNames.at(i), i);
    }
    mChannelCombo->setEnabled(chNames.size() > 1);
    mWavePanel->addTitleBarWidget(mChannelCombo);
    connect(mChannelCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this](int) {
        // 通道切换：清空缓存并重新请求包络/频谱/热图
        mEnvelopeValid = false;
        mEnvXs.clear();
        mEnvMins.clear();
        mEnvMaxs.clear();
        mStft = DAStftResult();
        requestEnvelopeRefresh();
        if (mSignal) {
            Q_EMIT spectrumRequested(mSignal, currentChannel());
        }
        requestHeatmapRefresh();
        Q_EMIT channelChanged(currentChannel());
    });
    mTimeSlider = new QSlider(Qt::Horizontal, mWavePanel);
    // 滑块以毫秒为范围（评审 U13：固定 0–1000 时 1 小时信号单步 3.6 秒，定位精度差）
    const int durationMs = mSignal ? qMax(1, int(qRound64(mSignal->duration() * 1000.0))) : 1000;
    mTimeSlider->setRange(0, durationMs);
    mTimeSlider->setValue(0);
    mWavePanel->setBottomWidget(mTimeSlider);
    connect(mTimeSlider, &QSlider::valueChanged, this, &DASignalAnalysisView::onSliderMoved);

    // 播放头（垂直线）
    mPlayhead = new QwtPlotMarker();
    mPlayhead->setLineStyle(QwtPlotMarker::VLine);
    mPlayhead->setLinePen(QPen(QColor(206, 96, 67), 1.5, Qt::DashLine));  // 橙红（危险/标记色）
    mPlayhead->setXValue(0.0);
    mPlayhead->attach(mWavePanel->chart());

    // STFT 窗口矩形（M6 填充数据）
    mStftWindow = new QwtPlotShapeItem();
    QPen rectPen(QColor(82, 128, 193), 1.0, Qt::SolidLine);  // 主蓝
    QBrush rectBrush(QColor(189, 205, 232, 60));             // 浅蓝半透明
    mStftWindow->setPen(rectPen);
    mStftWindow->setBrush(rectBrush);
    mStftWindow->setVisible(false);
    mStftWindow->attach(mWavePanel->chart());

    // 点击波形定位（评审 U1：QwtPicker 不设状态机时 transition() 直接返回，点击无效）
    auto* wavePicker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                                         QwtPlotPicker::NoRubberBand, QwtPicker::AlwaysOff,
                                         mWavePanel->chart()->canvas());
    wavePicker->setTrackerMode(QwtPicker::AlwaysOff);
    wavePicker->setStateMachine(new QwtPickerClickPointMachine());
    connect(wavePicker, QOverload< const QPointF& >::of(&QwtPlotPicker::selected), this, [this](const QPointF& p) {
        setCurrentTime(p.x() - (mSignal ? mSignal->timeOrigin() : 0.0));
    });
    // STFT 窗口矩形整体拖动（D30）：在画布上监听鼠标，命中矩形内才进入拖动
    mWavePanel->chart()->canvas()->installEventFilter(this);

    // ---------- 下半：频谱 + 右侧叠置 ----------
    mHorizontalSplitter = new QSplitter(Qt::Horizontal, mVerticalSplitter);
    mHorizontalSplitter->setChildrenCollapsible(false);

    mSpectrumPanel = new DASignalChartPanel(QStringLiteral("Spectrum"), mHorizontalSplitter);  // cn:频谱
    // 频率轴线性/对数切换（方案 D16）
    mFreqAxisCombo = new QComboBox(mSpectrumPanel);
    mFreqAxisCombo->addItem(QStringLiteral("Linear"), false);  // cn:线性
    mFreqAxisCombo->addItem(QStringLiteral("Log"), true);      // cn:对数
    mSpectrumPanel->addTitleBarWidget(mFreqAxisCombo);
    // 整段 / 跟随滑块（D13：默认整段）
    mFollowSliderCheck = new QCheckBox(mSpectrumPanel);
    mSpectrumPanel->addTitleBarWidget(mFollowSliderCheck);
    connect(mFollowSliderCheck, &QCheckBox::toggled, this, [this](bool on) {
        mFollowSlider = on;
        rebuildSpectrumPlot();
        // 专用信号（不用 currentTimeChanged）：避免被外部当作 seek 而暂停播放（评审 §七-2）
        Q_EMIT followSliderToggled(on);
    });
    connect(mFreqAxisCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        mLogFrequency = mFreqAxisCombo->itemData(idx).toBool();
        if (mLogFrequency) {
            mSpectrumPanel->chart()->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
        } else {
            mSpectrumPanel->chart()->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());
        }
        rebuildSpectrumPlot();
    });

    // 右侧叠置：二维热图（M6）/ 三维瀑布（M8）/ 隐藏
    mRightStack   = new QStackedWidget(mHorizontalSplitter);
    mHeatmapPanel = new DASignalChartPanel(tr("Time-Frequency"), mRightStack);  // cn:时频
    // 色带选择（默认 Viridis）与 dB 动态范围下限
    mColormapCombo = new QComboBox(mHeatmapPanel);
    const QStringList presets = QwtColorMapPreset::availablePresets();
    for (const QString& names : presets) {
        mColormapCombo->addItem(names, names);
    }
    const int viridisIdx = mColormapCombo->findData(QStringLiteral("viridis"));
    if (viridisIdx >= 0) {
        mColormapCombo->setCurrentIndex(viridisIdx);
    }
    mHeatmapPanel->addTitleBarWidget(mColormapCombo);
    mDbFloorSpin = new QDoubleSpinBox(mHeatmapPanel);
    mDbFloorSpin->setRange(-200.0, 0.0);
    mDbFloorSpin->setDecimals(0);
    mDbFloorSpin->setSingleStep(5.0);
    mDbFloorSpin->setValue(mDbFloor);
    mHeatmapPanel->addTitleBarWidget(mDbFloorSpin);
    connect(mColormapCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this](int) {
        applyHeatmapColors();
    });
    connect(mDbFloorSpin, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        mDbFloor = v;
        applyHeatmapColors();
    });
    // 热图随播放滚动（D27，默认关）
    mAutoScrollCheck = new QCheckBox(mHeatmapPanel);
    mHeatmapPanel->addTitleBarWidget(mAutoScrollCheck);
    connect(mAutoScrollCheck, &QCheckBox::toggled, this, [this](bool on) { setHeatmapAutoScroll(on); });
    mRightStack->addWidget(mHeatmapPanel);
    // 三维曲面瀑布（qwt 现成 Qwt3DWaterfall，叠置第 2 页）
    mWaterfallChart = new DAChart3DWidget(mRightStack);
    mRightStack->addWidget(mWaterfallChart);

    mHorizontalSplitter->addWidget(mSpectrumPanel);
    mHorizontalSplitter->addWidget(mRightStack);
    mHorizontalSplitter->setStretchFactor(0, 1);
    mHorizontalSplitter->setStretchFactor(1, 1);

    mVerticalSplitter->addWidget(mWavePanel);
    mVerticalSplitter->addWidget(mHorizontalSplitter);
    mVerticalSplitter->setStretchFactor(0, 1);
    mVerticalSplitter->setStretchFactor(1, 1);

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(mVerticalSplitter);

    // 尺寸变化防抖后重算包络
    mResizeDebounce = new QTimer(this);
    mResizeDebounce->setSingleShot(true);
    mResizeDebounce->setInterval(cEnvelopeDebounceMs);
    connect(mResizeDebounce, &QTimer::timeout, this, [this]() {
        requestEnvelopeRefresh();
        requestHeatmapRefresh();
    });
}

/**
 * @brief 重绘波形（包络 min/max 填充区间 + 播放头）
 */
void DASignalAnalysisView::rebuildWavePlot()
{
    DAChartWidget* chart = mWavePanel->chart();
    if (!mEnvelopeValid) {
        chart->replot();
        return;
    }
    if (!mEnvelopeCurve) {
        mEnvelopeCurve = new QwtPlotIntervalCurve(tr("Envelope"));  // cn:包络
        QPen pen(QColor(73, 124, 173), 1.0);                        // 深蓝
        QBrush brush(QColor(189, 205, 232, 200));                   // 浅蓝
        mEnvelopeCurve->setPen(pen);
        mEnvelopeCurve->setBrush(brush);
        mEnvelopeCurve->attach(chart);
    }
    // 采样点下标 → 时间秒（含时间原点）
    const double fs        = mSignal ? mSignal->sampleRate() : 0.0;
    const double timeOrigin = mSignal ? mSignal->timeOrigin() : 0.0;
    QVector< QwtIntervalSample > samples;
    samples.reserve(mEnvXs.size());
    const int n = qMin(mEnvXs.size(), qMin(mEnvMins.size(), mEnvMaxs.size()));
    for (int i = 0; i < n; ++i) {
        const double t = (fs > 0.0) ? (mEnvXs[ i ] / fs + timeOrigin) : mEnvXs[ i ];
        samples.append(QwtIntervalSample(t, mEnvMins[ i ], mEnvMaxs[ i ]));
    }
    mEnvelopeCurve->setSamples(samples);
    updateAxisTitles();
    chart->replot();
}

/**
 * @brief 重绘频谱（按当前幅值模式与频率轴模式）
 */
void DASignalAnalysisView::rebuildSpectrumPlot()
{
    DAChartWidget* chart = mSpectrumPanel->chart();
    if (!mSpectrum.isValid()) {
        chart->replot();
        return;
    }
    if (!mSpectrumCurve) {
        mSpectrumCurve = new QwtPlotCurve(tr("Amplitude spectrum"));  // cn:幅值谱
        QPen pen(QColor(82, 128, 193), 1.2);                          // 主蓝
        mSpectrumCurve->setPen(pen);
        mSpectrumCurve->attach(chart);
    }
    // 跟随滑块：用 STFT 当前时间所在列的幅值谱（D13）；未算 STFT 时回退整段频谱
    QVector< double > srcFreqs = mSpectrum.freqs;
    QVector< double > srcAmps  = displayAmplitudes();
    if (mFollowSlider && mStft.isValid() && mStft.columns > 0) {
        const int col = currentStftColumn();
        QVector< double > colAmps(mStft.rows);
        for (int r = 0; r < mStft.rows; ++r) {
            const double v = mStft.values.value(r * mStft.columns + col);
            colAmps[ r ]   = std::isfinite(v) ? v : 0.0;
        }
        srcFreqs = mStft.freqs;
        srcAmps.resize(colAmps.size());
        for (int i = 0; i < colAmps.size(); ++i) {
            srcAmps[ i ] = DASignalAmplitude::toDisplay(colAmps[ i ],
                                                        i == 0,
                                                        mParams.amplitudeMode,
                                                        mParams.dbReference,
                                                        mParams.calibration,
                                                        signalPeak() * mParams.calibration);
        }
    }
    // 对数频率轴从第一个正频率点开始（0 赫兹在对数轴下无意义）
    int startIdx = 0;
    if (mLogFrequency) {
        while (startIdx < srcFreqs.size() && srcFreqs[ startIdx ] <= 0.0) {
            ++startIdx;
        }
    }
    QVector< double > freqs(srcFreqs.size() - startIdx);
    QVector< double > values(qMax(0, srcAmps.size() - startIdx));
    for (int i = startIdx; i < srcFreqs.size(); ++i) {
        freqs[ i - startIdx ]  = srcFreqs[ i ];
        values[ i - startIdx ] = (i < srcAmps.size()) ? srcAmps[ i ] : 0.0;
    }
    mSpectrumCurve->setSamples(freqs, values);
    // 峰值标记（按当前幅值口径定位；第 1 名橙红，其余主蓝）
    clearPeakMarkers(mPeakMarkers);
    if (mPeakMarkersVisible && mPeaks.isValid()) {
        const double signalPeakCal = signalPeak() * mParams.calibration;
        for (int i = 0; i < mPeaks.frequencies.size(); ++i) {
            const double f = mPeaks.frequencies.at(i);
            const double y = DASignalAmplitude::toDisplay(mPeaks.magnitudes.value(i),
                                                          f <= 0.0,
                                                          mParams.amplitudeMode,
                                                          mParams.dbReference,
                                                          mParams.calibration,
                                                          signalPeakCal);
            auto* marker = new QwtPlotMarker();
            marker->setXValue(f);
            marker->setYValue(y);
            marker->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                            QBrush(i == 0 ? QColor(206, 96, 67) : QColor(82, 128, 193)),
                                            QPen(Qt::NoPen),
                                            QSize(7, 7)));
            marker->attach(chart);
            mPeakMarkers.append(marker);
        }
    }
    updateAxisTitles();
    chart->setAxisAutoScale(QwtPlot::yLeft, true);
    chart->replot();
}

/**
 * @brief 时频显示栅格回填并重绘
 */
DAStftResult DASignalAnalysisView::stftResult() const
{
    return mStft;
}

void DASignalAnalysisView::setStft(const DAStftResult& result)
{
    mStft = result;
    rebuildHeatmap();
    if (mWaterfallMode == 1) {
        rebuildWaterfall();
    }
    updateStftWindowRect();
}

/**
 * @brief 瀑布模式切换：0=二维 / 1=三维 / 2=隐藏（D12 / D25）
 */
void DASignalAnalysisView::setWaterfallMode(int mode)
{
    mWaterfallMode = qBound(0, mode, 2);
    if (!mRightStack) {
        return;
    }
    if (mWaterfallMode == 2) {
        mRightStack->setVisible(false);
        return;
    }
    mRightStack->setVisible(true);
    mRightStack->setCurrentIndex(mWaterfallMode);
    if (mWaterfallMode == 1) {
        rebuildWaterfall();
    }
}

/**
 * @brief 当前瀑布模式（0=二维 / 1=三维 / 2=隐藏）
 */
int DASignalAnalysisView::waterfallMode() const
{
    return mWaterfallMode;
}

/**
 * @brief 是否对数频率轴
 */
bool DASignalAnalysisView::isLogFrequency() const
{
    return mLogFrequency;
}

/**
 * @brief 设置频率轴线性/对数（同步下拉，触发重绘）
 */
void DASignalAnalysisView::setLogFrequency(bool on)
{
    if (mFreqAxisCombo) {
        const int idx = mFreqAxisCombo->findData(on);
        if (idx >= 0) {
            mFreqAxisCombo->setCurrentIndex(idx);  // 触发 currentIndexChanged → 引擎切换 + 重绘
            return;
        }
    }
    mLogFrequency = on;
    rebuildSpectrumPlot();
}

/**
 * @brief 当前色带名
 */
QString DASignalAnalysisView::colormapName() const
{
    return mColormapCombo ? mColormapCombo->currentData().toString() : QStringLiteral("viridis");
}

/**
 * @brief 设置色带（同步下拉）
 */
void DASignalAnalysisView::setColormapName(const QString& name)
{
    if (!mColormapCombo) {
        return;
    }
    const int idx = mColormapCombo->findData(name);
    if (idx >= 0) {
        mColormapCombo->setCurrentIndex(idx);
    }
}

/**
 * @brief 热图色标下限（dB）
 */
double DASignalAnalysisView::dbFloor() const
{
    return mDbFloor;
}

/**
 * @brief 设置热图色标下限（dB）
 */
void DASignalAnalysisView::setDbFloor(double dbFloor)
{
    mDbFloor = dbFloor;
    if (mDbFloorSpin) {
        mDbFloorSpin->setValue(dbFloor);
    }
    applyHeatmapColors();
}

/**
 * @brief 上下分割比例（波形 / 下半区）
 */
QList< int > DASignalAnalysisView::verticalSplitterSizes() const
{
    return mVerticalSplitter ? mVerticalSplitter->sizes() : QList< int >();
}

/**
 * @brief 左右分割比例（频谱 / 热图）
 */
QList< int > DASignalAnalysisView::horizontalSplitterSizes() const
{
    return mHorizontalSplitter ? mHorizontalSplitter->sizes() : QList< int >();
}

/**
 * @brief 恢复分割比例（空列表/尺寸不符时忽略）
 */
void DASignalAnalysisView::setSplitterSizes(const QList< int >& vertical, const QList< int >& horizontal)
{
    if (mVerticalSplitter && vertical.size() == mVerticalSplitter->count()) {
        mVerticalSplitter->setSizes(vertical);
        mDefaultSplitterSizesApplied = true;  // 存档值优先，首次显示的默认比例不再覆盖
    }
    if (mHorizontalSplitter && horizontal.size() == mHorizontalSplitter->count()) {
        mHorizontalSplitter->setSizes(horizontal);
        mDefaultSplitterSizesApplied = true;
    }
}

/**
 * @brief 首次显示时显式设定分割比例（上下、左右各 1:1，方案 §8.2）
 *
 * 只设 stretchFactor 时初次布局按子控件 sizeHint 分配，波形区会被频谱图的
 * 标题栏/坐标轴挤得比例失真；这里在网络布局有尺寸后显式 setSizes（评审 §七-1）。
 */
void DASignalAnalysisView::applyDefaultSplitterSizes()
{
    // 页面隐藏/尚未布局时尺寸可能为 0，用保底高度避免 setSizes 无效；
    // 真正显示后 QSplitter 会按比例缩放，用户拖动后不再覆盖
    if (mVerticalSplitter) {
        const int h = qMax(240, mVerticalSplitter->height());
        mVerticalSplitter->setSizes({ h / 2, h - h / 2 });
    }
    if (mHorizontalSplitter) {
        const int w = qMax(320, mHorizontalSplitter->width());
        mHorizontalSplitter->setSizes({ w / 2, w - w / 2 });
    }
}

/**
 * @brief 设置显示用时间原点（秒）：数据帧时间列首值非 0 时三张图横向对齐（评审 U12）
 */
void DASignalAnalysisView::setTimeOrigin(double seconds)
{
    mTimeOrigin = seconds;
    if (mStft.isValid()) {
        rebuildHeatmap();
    }
    updatePlayhead();
}

/**
 * @brief 信号峰值幅度（未乘标定；dBFS 参考口径，即去直流/去趋势后的峰值）
 */
double DASignalAnalysisView::signalPeak() const
{
    return mDbfsReferencePeak;
}

/**
 * @brief 热图随播放滚动开关（D27，默认关）
 */
void DASignalAnalysisView::setHeatmapAutoScroll(bool on)
{
    mHeatmapAutoScroll = on;
    if (mAutoScrollCheck && mAutoScrollCheck->isChecked() != on) {
        const bool blocked = mAutoScrollCheck->blockSignals(true);
        mAutoScrollCheck->setChecked(on);
        mAutoScrollCheck->blockSignals(blocked);
    }
    if (mHeatmapPanel) {
        DAChartWidget* heatChart = mHeatmapPanel->chart();
        if (!on) {
            heatChart->setAxisAutoScale(QwtPlot::xBottom, true);
        }
        heatChart->replot();
    }
}

bool DASignalAnalysisView::isHeatmapAutoScroll() const
{
    return mHeatmapAutoScroll;
}

/**
 * @brief 画布事件过滤：STFT 窗口矩形整体拖动（D30，不可拖边）
 *
 * 命中判定用矩形包围盒（含 Y 范围），按下时只记「拖动候选」，鼠标位移超过
 * QApplication::startDragDistance() 才真正进入拖动——否则矩形内单击会被拖动逻辑吞掉，
 * 无法定位播放头（评审 N-U5）；未进入拖动的左键释放交回 QwtPicker 的点击定位。
 */
bool DASignalAnalysisView::eventFilter(QObject* watched, QEvent* e)
{
    if (!mWavePanel || !mWavePanel->chart()) {
        return QWidget::eventFilter(watched, e);
    }
    DAChartWidget* chart = mWavePanel->chart();
    if (watched != chart->canvas()) {
        return QWidget::eventFilter(watched, e);
    }
    // 鼠标位置是否落在 STFT 窗口矩形内（X 与 Y 都要命中，评审 N-U5）
    auto hitWindowRect = [this, chart](const QPoint& pos) -> bool {
        if (!mStftWindow || !mStftWindow->isVisible() || !mStft.isValid()) {
            return false;
        }
        const QRectF rect = mStftWindow->boundingRect();
        if (!rect.isValid()) {
            return false;
        }
        const QPointF plotPos(chart->invTransform(QwtPlot::xBottom, pos.x()),
                              chart->invTransform(QwtPlot::yLeft, pos.y()));
        return rect.contains(plotPos);
    };
    switch (e->type()) {
    case QEvent::MouseButtonPress: {
        auto* me = static_cast< QMouseEvent* >(e);
        if (me->button() == Qt::LeftButton && hitWindowRect(me->pos())) {
            // 只记候选：是否拖动等移动超过阈值再定，不吞 press（点击定位要能用）
            mPressCandidate = true;
            mDragPressPos   = me->pos();
            mDragPressTime  = currentTime();
        }
        break;
    }
    case QEvent::MouseMove: {
        auto* me = static_cast< QMouseEvent* >(e);
        if (mPressCandidate && !mDraggingWindow) {
            const int moveDist = (me->pos() - mDragPressPos).manhattanLength();
            if (moveDist >= QApplication::startDragDistance()) {
                mDraggingWindow = true;
                chart->canvas()->setCursor(Qt::SizeAllCursor);
            }
        }
        if (mDraggingWindow) {
            const double curTimeAtPixel = chart->invTransform(QwtPlot::xBottom, me->pos().x());
            const double delta          = curTimeAtPixel - chart->invTransform(QwtPlot::xBottom, mDragPressPos.x());
            setCurrentTime(mDragPressTime + delta);
            return true;
        }
        // 悬停提示：命中矩形内显示移动光标
        if (mStftWindow && mStftWindow->isVisible() && mStft.isValid()) {
            const Qt::CursorShape want = hitWindowRect(me->pos()) ? Qt::SizeAllCursor : Qt::ArrowCursor;
            if (chart->canvas()->cursor().shape() != want) {
                chart->canvas()->setCursor(want);
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto* me = static_cast< QMouseEvent* >(e);
        if (me->button() == Qt::LeftButton) {
            const bool wasDragging = mDraggingWindow;
            mPressCandidate        = false;
            mDraggingWindow        = false;
            chart->canvas()->setCursor(Qt::ArrowCursor);
            if (wasDragging) {
                return true;  // 拖动结束：不再当作点击定位
            }
            // 未拖动：交回 QwtPicker 处理本次点击（矩形内单击仍然定位）
        }
        break;
    }
    default:
        break;
    }
    return QWidget::eventFilter(watched, e);
}

/**
 * @brief 重建三维瀑布（抽帧抽频点后逐帧 pushTrace）
 */
void DASignalAnalysisView::rebuildWaterfall()
{
    if (!mWaterfallChart) {
        return;
    }
    QVector< QVector< double > > traces;
    QVector< double > binValues;
    // 时间方向 200 条迹线、频率方向最多 512 频点（方案 §7.4）
    if (!DAWaterfallBuilder::build(mStft, 200, 512, traces, binValues)) {
        return;
    }
    if (mWaterfallItem) {
        // 瀑布图元已纳入 DAChart3DWidget::data3DRttis()（评审 H4），外部 clearAllData3D()
        // 可能已经销毁它：先按当前挂载列表校验，失效则只置空，避免二次 delete（评审 N-H4）
        const bool stillAttached = mWaterfallChart->getWaterfalls().contains(mWaterfallItem);
        Qwt3DWaterfall* stale   = mWaterfallItem;
        mWaterfallItem          = nullptr;
        if (stillAttached) {
            stale->detach();
            delete stale;
        }
    }
    mWaterfallItem = mWaterfallChart->addWaterfall(traces, binValues, tr("Waterfall"));  // cn:三维瀑布
    if (mWaterfallItem) {
        mWaterfallItem->setColorMode(Qwt3DWaterfall::ByAmplitude);
        mWaterfallItem->setPlotStyle(Qwt3DWaterfall::RibbonMesh);
    }
}

/**
 * @brief 重绘时频热图（线性幅值 → dB，色标范围 = [下限, 0]）
 */
void DASignalAnalysisView::rebuildHeatmap()
{
    if (!mHeatmapPanel) {
        return;
    }
    DAChartWidget* chart = mHeatmapPanel->chart();
    if (!mStft.isValid()) {
        chart->replot();
        return;
    }
    double peak = 0.0;
    for (double v : std::as_const(mStft.values)) {
        if (std::isfinite(v)) {
            peak = qMax(peak, std::fabs(v));
        }
    }
    if (peak <= 0.0) {
        peak = 1.0;
    }
    QVector< double > dbValues(mStft.values.size());
    for (int i = 0; i < mStft.values.size(); ++i) {
        const double v = mStft.values[ i ];
        dbValues[ i ]  = std::isfinite(v) ? 20.0 * std::log10(qMax(v / peak, 1e-30))
                                         : std::numeric_limits< double >::quiet_NaN();
    }
    // 栅格所有权归 QwtPlotSpectrogram（setData 接管）；若图表被清空导致谱图图元已销毁，
    // 指针必须一起置空，避免继续写已释放的栅格（评审 H3/A5）
    if (mSpectrogramItem) {
        bool itemStillAttached = false;
        const QList< QwtPlotItem* > items = chart->itemList();
        for (QwtPlotItem* item : items) {
            if (item == mSpectrogramItem) {
                itemStillAttached = true;
                break;
            }
        }
        if (!itemStillAttached) {
            mSpectrogramItem = nullptr;
            mStftRaster      = nullptr;
        }
    }
    if (!mStftRaster) {
        mStftRaster = new DAStftRasterData();
    }
    mStftRaster->setGrid(dbValues,
                         mStft.rows,
                         mStft.columns,
                         mStft.times.first() + mTimeOrigin,
                         mStft.times.last() + mTimeOrigin,
                         mStft.freqs.first(),
                         mStft.freqs.last());
    mStftRaster->setValueRange(mDbFloor, 0.0);

    if (!mSpectrogramItem) {
        mSpectrogramItem = chart->addSpectrogram(static_cast< QwtRasterData* >(mStftRaster),
                                                 tr("Spectrogram"));  // cn:时频图
        if (mSpectrogramItem) {
            mSpectrogramItem->setRenderThreadCount(0);
            mSpectrogramItem->setCachePolicy(QwtPlotRasterItem::PaintCache);
        }
    } else {
        mSpectrogramItem->setData(static_cast< QwtRasterData* >(mStftRaster));
        mSpectrogramItem->attach(chart);
    }
    chart->enableAxis(QwtPlot::yRight);
    if (QwtScaleWidget* colorBar = chart->axisWidget(QwtPlot::yRight)) {
        colorBar->setColorBarEnabled(true);
        colorBar->setColorBarWidth(18);
        colorBar->setTitle(tr("dB"));  // cn:分贝
    }
    chart->setAxisTitle(QwtPlot::xBottom, tr("Time (s)"));      // cn:时间（秒）
    chart->setAxisTitle(QwtPlot::yLeft, tr("Frequency (Hz)"));  // cn:频率（赫兹）
    applyHeatmapColors();
    chart->replot();
}

/**
 * @brief 应用色带与色标范围（色带或 dB 下限变化时调用）
 */
void DASignalAnalysisView::applyHeatmapColors()
{
    if (!mHeatmapPanel || !mSpectrogramItem) {
        return;
    }
    DAChartWidget* chart  = mHeatmapPanel->chart();
    const QString cmapName = mColormapCombo ? mColormapCombo->currentData().toString() : QStringLiteral("viridis");
    auto cmap              = QwtColorMapPreset::create(cmapName);
    if (cmap) {
        mSpectrogramItem->setColorMap(cloneLinearColorMap(cmap.get()));
        if (QwtScaleWidget* colorBar = chart->axisWidget(QwtPlot::yRight)) {
            // setColorMap 同时设定色标区间与色带（接管所有权）
            colorBar->setColorMap(QwtInterval(mDbFloor, 0.0), cloneLinearColorMap(cmap.get()));
        }
    }
    if (mStftRaster) {
        mStftRaster->setValueRange(mDbFloor, 0.0);
    }
    chart->replot();
}

/**
 * @brief 请求外部计算时频热图（目标尺寸按绘图区像素）
 */
void DASignalAnalysisView::requestHeatmapRefresh()
{
    if (!mSignal || !mSignal->isValid() || !mHeatmapPanel) {
        return;
    }
    const int columns = qMax(64, mHeatmapPanel->chart()->canvas()->width());
    const int rows    = qMax(64, mHeatmapPanel->chart()->canvas()->height());
    Q_EMIT heatmapRequested(columns, rows);
}

/**
 * @brief 更新 STFT 窗口矩形（波形上）：宽 = 窗长、中心 = 当前时间（D30）
 */
void DASignalAnalysisView::updateStftWindowRect()
{
    if (!mStftWindow || !mSignal) {
        return;
    }
    if (!mStft.isValid() || mStft.windowSeconds <= 0.0) {
        mStftWindow->setVisible(false);
        return;
    }
    const double center = currentTime() + mSignal->timeOrigin();
    const double half   = mStft.windowSeconds / 2.0;
    const double yTop   = qMax(std::fabs(mSignalPeak), 1e-3);
    const QRectF rect(center - half, -yTop, mStft.windowSeconds, 2.0 * yTop);
    mStftWindow->setRect(rect);
    mStftWindow->setVisible(true);
    mWavePanel->chart()->replot();
}

/**
 * @brief 峰值表数据回填（频谱图上标记峰位）
 */
void DASignalAnalysisView::setPeaks(const DAPeakResult& peaks)
{
    mPeaks = peaks;
    rebuildSpectrumPlot();
}

/**
 * @brief 峰值标记显隐（丝带「峰值标记」快捷开关）
 */
void DASignalAnalysisView::setPeakMarkersVisible(bool on)
{
    mPeakMarkersVisible = on;
    rebuildSpectrumPlot();
}

/**
 * @brief 幅值 → 显示口径（乘标定系数；dB 时按参考换算）
 */
QVector< double > DASignalAnalysisView::displayAmplitudes() const
{
    QVector< double > out;
    out.reserve(mSpectrum.amplitudes.size());
    const double signalPeakCal = signalPeak() * mParams.calibration;
    for (int i = 0; i < mSpectrum.amplitudes.size(); ++i) {
        out.append(DASignalAmplitude::toDisplay(mSpectrum.amplitudes.at(i),
                                               i == 0,
                                               mParams.amplitudeMode,
                                               mParams.dbReference,
                                               mParams.calibration,
                                               signalPeakCal));
    }
    return out;
}

/**
 * @brief dB 满量程参考值：满量程取信号峰值（标定后），1.0 取固定 1.0
 */
double DASignalAnalysisView::dbReferenceValue() const
{
    if (mParams.dbReference == QLatin1String("one")) {
        return 1.0;
    }
    const double ref = signalPeak() * mParams.calibration;
    return (ref > 0.0) ? ref : 1.0;
}

/**
 * @brief 更新坐标轴标题（含标定单位）
 */
void DASignalAnalysisView::updateAxisTitles()
{
    DAChartWidget* waveChart = mWavePanel->chart();
    waveChart->setAxisTitle(QwtPlot::xBottom, tr("Time (s)"));  // cn:时间（秒）
    QString ampTitle = tr("Amplitude");                          // cn:幅值
    if (!mParams.unit.isEmpty()) {
        ampTitle += QStringLiteral(" [%1]").arg(mParams.unit);
    }
    waveChart->setAxisTitle(QwtPlot::yLeft, ampTitle);

    DAChartWidget* specChart = mSpectrumPanel->chart();
    specChart->setAxisTitle(QwtPlot::xBottom, tr("Frequency (Hz)"));  // cn:频率（赫兹）
    QString specY = tr("Amplitude");                                   // cn:幅值
    if (mParams.amplitudeMode == QLatin1String("dbFS")) {
        specY = tr("dB (FS)");  // cn:dB（满量程）
    } else if (mParams.amplitudeMode == QLatin1String("dbRef1")) {
        specY = tr("dB (re 1.0)");  // cn:dB（相对 1.0）
    } else if (mParams.amplitudeMode == QLatin1String("rms")) {
        specY = tr("RMS");  // cn:均方根
    }
    if (!mParams.unit.isEmpty()) {
        specY += QStringLiteral(" [%1]").arg(mParams.unit);
    }
    specChart->setAxisTitle(QwtPlot::yLeft, specY);
}

/**
 * @brief 幅值显示模式切换（只重绘，不重算）
 */
void DASignalAnalysisView::setAmplitudeDisplay(const QString& amplitudeMode, const QString& dbReference)
{
    mParams.amplitudeMode = amplitudeMode;
    mParams.dbReference   = dbReference;
    rebuildSpectrumPlot();
}

/**
 * @brief 当前时间对应的 STFT 列索引（取最近的列中心）
 */
int DASignalAnalysisView::currentStftColumn() const
{
    if (!mStft.isValid() || mStft.columns <= 0) {
        return 0;
    }
    const double t = currentTime();
    int best       = 0;
    double bestD   = std::numeric_limits< double >::infinity();
    for (int c = 0; c < mStft.times.size(); ++c) {
        const double d = std::fabs(mStft.times.at(c) - t);
        if (d < bestD) {
            bestD = d;
            best  = c;
        }
    }
    return best;
}

int DASignalAnalysisView::currentChannel() const
{
    if (!mChannelCombo) {
        return 0;
    }
    return qMax(0, mChannelCombo->currentData().toInt());
}

void DASignalAnalysisView::setCurrentChannel(int channel)
{
    if (mChannelCombo && channel >= 0 && channel < mChannelCombo->count()) {
        mChannelCombo->setCurrentIndex(channel);
    }
}

bool DASignalAnalysisView::isFollowSlider() const
{
    return mFollowSlider;
}

/**
 * @brief 当前波形可见时间范围（秒，绝对时间，含 timeOrigin）
 *
 * 波形横轴 = 相对秒 + timeOrigin，因此直接取波形图 X 轴区间即绝对秒；无包络/未布局时
 * 返回 false（调用方按整段处理）。
 */
bool DASignalAnalysisView::visibleTimeRange(double* startSec, double* endSec) const
{
    if (!mWavePanel || !mWavePanel->chart() || !mSignal) {
        return false;
    }
    const QwtInterval interval = mWavePanel->chart()->axisScaleDiv(QwtPlot::xBottom).interval();
    if (!interval.isValid() || interval.width() <= 0.0) {
        return false;
    }
    if (startSec) {
        *startSec = interval.minValue();
    }
    if (endSec) {
        *endSec = interval.maxValue();
    }
    return true;
}

double DASignalAnalysisView::currentTime() const
{
    if (!mSignal || mSignal->duration() <= 0.0) {
        return 0.0;
    }
    // 滑块以毫秒为单位（评审 U13）
    return mTimeSlider->value() / 1000.0;
}

/**
 * @brief 设置当前时间（秒）：同步滑块与播放头
 */
void DASignalAnalysisView::setCurrentTime(double seconds)
{
    if (!mSignal || mSignal->duration() <= 0.0) {
        return;
    }
    const double dur = mSignal->duration();
    seconds          = qBound(0.0, seconds, dur);
    const int value  = int(qRound(seconds * 1000.0));
    if (mTimeSlider->value() != value) {
        mTimeSlider->blockSignals(true);
        mTimeSlider->setValue(value);
        mTimeSlider->blockSignals(false);
    }
    updatePlayhead();
    Q_EMIT currentTimeChanged(currentTime());
}

/**
 * @brief 播放头位置回写（播放器驱动）：只更新显示，不回发 currentTimeChanged，避免 seek 循环
 */
void DASignalAnalysisView::setPlayheadFromPlayback(double seconds)
{
    if (!mSignal || mSignal->duration() <= 0.0) {
        return;
    }
    const double dur = mSignal->duration();
    seconds          = qBound(0.0, seconds, dur);
    const int value  = int(qRound(seconds * 1000.0));
    if (mTimeSlider->value() != value) {
        mTimeSlider->blockSignals(true);
        mTimeSlider->setValue(value);
        mTimeSlider->blockSignals(false);
    }
    updatePlayhead();
}

void DASignalAnalysisView::onSliderMoved(int value)
{
    Q_UNUSED(value)
    updatePlayhead();
    Q_EMIT currentTimeChanged(currentTime());
}

/**
 * @brief 更新播放头位置（波形图 x = 当前时间）
 */
void DASignalAnalysisView::updatePlayhead()
{
    if (!mSignal) {
        return;
    }
    const double t = currentTime() + mSignal->timeOrigin();
    mPlayhead->setXValue(t);
    updateStftWindowRect();
    if (mFollowSlider) {
        rebuildSpectrumPlot();  // 跟随模式下频谱随滑块更新
    }
    // 热图随播放滚动（D27）：只平移 X 轴范围，不改数据
    if (mHeatmapAutoScroll && mStft.isValid() && mHeatmapPanel) {
        DAChartWidget* heatChart = mHeatmapPanel->chart();
        const double duration    = qMax(mSignal->duration(), 1e-9);
        const double origin      = mSignal->timeOrigin();
        const double winSeconds  = (mStft.windowSeconds > 0.0) ? mStft.windowSeconds : 1.0;
        // 可视跨度：优先沿用当前轴跨度；若等于整段（自动缩放）则先放大到 1/5 段，让滚动可见
        double span = heatChart->axisScaleDiv(QwtPlot::xBottom).interval().width();
        if (!(span > 0.0) || span >= 0.9 * duration) {
            span = qMax(5.0 * winSeconds, duration / 5.0);
            span = qMin(span, duration);
        }
        double left          = t - span / 2.0;
        const double minLeft = origin;
        const double maxLeft = origin + duration - span;
        left                 = qBound(minLeft, left, qMax(minLeft, maxLeft));
        heatChart->setAxisScale(QwtPlot::xBottom, left, left + span);
        heatChart->replot();
    }
    mWavePanel->chart()->replot();
}

/**
 * @brief 请求外部计算包络（区间 = 整段信号，宽度 = 波形区像素宽）
 */
void DASignalAnalysisView::requestEnvelopeRefresh()
{
    if (!mSignal || !mSignal->isValid()) {
        return;
    }
    const qint64 total = mSignal->sampleCount();
    const int width    = qMax(100, mWavePanel->chart()->canvas()->width());
    Q_EMIT envelopeRequested(0, total, width);
}

/**
 * @brief 首次分析：请求包络、整段频谱与时频热图
 *
 * 视图加入页面时 showEvent 早于外部 connect，此时发出的请求会丢失，
 * 因此由外部在连接完成后显式调用一次。
 */
void DASignalAnalysisView::requestInitialAnalysis()
{
    if (!mSignal || !mSignal->isValid()) {
        return;
    }
    requestEnvelopeRefresh();
    Q_EMIT spectrumRequested(mSignal, currentChannel());
    requestHeatmapRefresh();
}

/**
 * @brief 导出用整页位图：三维瀑布可见时把 OpenGL 帧缓冲合成到页面位图中（评审 I11）
 *
 * QWidget::grab() 拿不到 QOpenGLWidget 的内容，必须用 Qwt3DPlot::renderPixmap()
 * 单独取帧缓冲再按控件位置贴回页面位图。
 */
QPixmap DASignalAnalysisView::grabForExport()
{
    QPixmap page = grab();
    if (mWaterfallMode != 1 || !mWaterfallChart || !mWaterfallChart->isVisible()) {
        return page;
    }
    const QPixmap gl = mWaterfallChart->renderPixmap(mWaterfallChart->width(), mWaterfallChart->height());
    if (gl.isNull() || page.isNull()) {
        return page;
    }
    QPainter painter(&page);
    const QPoint topLeft = mWaterfallChart->mapTo(this, QPoint(0, 0));
    painter.drawPixmap(topLeft, gl);
    painter.end();
    return page;
}

void DASignalAnalysisView::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (mSignal && mSignal->isValid()) {
        mResizeDebounce->start();  // 防抖：尺寸稳定后再重算包络
    }
}

void DASignalAnalysisView::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    // 首次显示显式设定默认分割比例（恢复存档时由 setSplitterSizes 覆盖，评审 §七-1）
    if (!mDefaultSplitterSizesApplied) {
        mDefaultSplitterSizesApplied = true;
        applyDefaultSplitterSizes();
    }
    if (!mEnvelopeValid) {
        requestInitialAnalysis();
    }
}

void DASignalAnalysisView::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

void DASignalAnalysisView::retranslateUi()
{
    if (mWavePanel) {
        mWavePanel->setTitle(tr("Waveform"));  // cn:波形
    }
    if (mSpectrumPanel) {
        mSpectrumPanel->setTitle(tr("Spectrum"));  // cn:频谱
    }
    if (mFreqAxisCombo) {
        mFreqAxisCombo->setItemText(0, tr("Linear"));  // cn:线性
        mFreqAxisCombo->setItemText(1, tr("Log"));     // cn:对数
    }
    if (mFollowSliderCheck) {
        mFollowSliderCheck->setText(tr("Follow slider"));  // cn:跟随滑块
        mFollowSliderCheck->setToolTip(tr("Show the current frame instead of the whole signal"));
        // cn:显示当前帧而不是整段信号
    }
    if (mAutoScrollCheck) {
        mAutoScrollCheck->setText(tr("Follow playback"));  // cn:随播放滚动
        mAutoScrollCheck->setToolTip(tr("Scroll the time-frequency view while playback advances"));
        // cn:播放推进时平移时频图的时间轴
    }
    if (mHeatmapPanel) {
        mHeatmapPanel->setTitle(tr("Time-Frequency"));  // cn:时频
    }
    if (mEnvelopeCurve) {
        mEnvelopeCurve->setTitle(tr("Envelope"));  // cn:包络
    }
    if (mSpectrumCurve) {
        mSpectrumCurve->setTitle(tr("Amplitude spectrum"));  // cn:幅值谱
    }
    updateAxisTitles();
}
}  // namespace DA
