#include "DASignalAnalysisView.h"
#include <cmath>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>
// qwt
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_scale_engine.h"
#include "qwt_plot_picker.h"
#include "qwt_samples.h"
#include "qwt_symbol.h"
// DA
#include "DAChartWidget.h"
#include "DASignalChartPanel.h"

namespace DA
{
namespace
{
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
    mParams = params;
    rebuildSpectrumPlot();
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
    // 信号峰值（用于 dB 满量程参考）
    double peak = 0.0;
    const int n = qMin(mEnvMins.size(), mEnvMaxs.size());
    for (int i = 0; i < n; ++i) {
        peak = qMax(peak, qMax(std::fabs(mEnvMins[ i ]), std::fabs(mEnvMaxs[ i ])));
    }
    mSignalPeak = peak;
    rebuildWavePlot();
    if (mSpectrum.isValid()) {
        rebuildSpectrumPlot();
    }
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
    mTimeSlider = new QSlider(Qt::Horizontal, mWavePanel);
    mTimeSlider->setRange(0, 1000);
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

    // 点击波形定位
    auto* wavePicker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                                         QwtPlotPicker::NoRubberBand, QwtPicker::AlwaysOff,
                                         mWavePanel->chart()->canvas());
    wavePicker->setTrackerMode(QwtPicker::AlwaysOff);
    connect(wavePicker, QOverload< const QPointF& >::of(&QwtPlotPicker::selected), this, [this](const QPointF& p) {
        setCurrentTime(p.x());
    });

    // ---------- 下半：频谱 + 右侧叠置 ----------
    mHorizontalSplitter = new QSplitter(Qt::Horizontal, mVerticalSplitter);
    mHorizontalSplitter->setChildrenCollapsible(false);

    mSpectrumPanel = new DASignalChartPanel(QStringLiteral("Spectrum"), mHorizontalSplitter);  // cn:频谱
    // 频率轴线性/对数切换（方案 D16）
    mFreqAxisCombo = new QComboBox(mSpectrumPanel);
    mFreqAxisCombo->addItem(QStringLiteral("Linear"), false);  // cn:线性
    mFreqAxisCombo->addItem(QStringLiteral("Log"), true);      // cn:对数
    mSpectrumPanel->addTitleBarWidget(mFreqAxisCombo);
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
    mRightStack       = new QStackedWidget(mHorizontalSplitter);
    QLabel* rightPlaceholder = new QLabel(mRightStack);
    rightPlaceholder->setAlignment(Qt::AlignCenter);
    rightPlaceholder->setObjectName(QStringLiteral("da-signal-right-placeholder"));
    rightPlaceholder->setText(tr("Time-frequency view"));  // cn:时频视图
    mRightStack->addWidget(rightPlaceholder);

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
    connect(mResizeDebounce, &QTimer::timeout, this, &DASignalAnalysisView::requestEnvelopeRefresh);
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
    const QVector< double > amps = displayAmplitudes();
    // 对数频率轴从第一个正频率点开始（0 赫兹在对数轴下无意义）
    int startIdx = 0;
    if (mLogFrequency) {
        while (startIdx < mSpectrum.freqs.size() && mSpectrum.freqs[ startIdx ] <= 0.0) {
            ++startIdx;
        }
    }
    QVector< double > freqs(mSpectrum.freqs.size() - startIdx);
    QVector< double > values(amps.size() - startIdx);
    for (int i = startIdx; i < mSpectrum.freqs.size(); ++i) {
        freqs[ i - startIdx ]  = mSpectrum.freqs[ i ];
        values[ i - startIdx ] = (i < amps.size()) ? amps[ i ] : 0.0;
    }
    mSpectrumCurve->setSamples(freqs, values);
    updateAxisTitles();
    chart->setAxisAutoScale(QwtPlot::yLeft, true);
    chart->replot();
}

/**
 * @brief 幅值 → 显示口径（乘标定系数；dB 时按参考换算）
 */
QVector< double > DASignalAnalysisView::displayAmplitudes() const
{
    QVector< double > out;
    out.reserve(mSpectrum.amplitudes.size());
    const double calib = mParams.calibration;
    const bool isDb    = (mParams.amplitudeMode == QLatin1String("dbFS")
                          || mParams.amplitudeMode == QLatin1String("dbRef1"));
    if (!isDb) {
        for (double a : mSpectrum.amplitudes) {
            out.append(a * calib);
        }
        return out;
    }
    const double ref = dbReferenceValue();
    for (double a : mSpectrum.amplitudes) {
        const double v = std::fabs(a * calib);
        out.append(20.0 * std::log10(qMax(v, cEps) / qMax(ref, cEps)));
    }
    return out;
}

/**
 * @brief dB 参考值：满量程取信号峰值，1.0 取固定 1.0
 */
double DASignalAnalysisView::dbReferenceValue() const
{
    if (mParams.dbReference == QLatin1String("one")) {
        return 1.0;
    }
    return (mSignalPeak > 0.0) ? mSignalPeak : 1.0;
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

double DASignalAnalysisView::currentTime() const
{
    if (!mSignal || mSignal->duration() <= 0.0) {
        return 0.0;
    }
    return (mTimeSlider->value() / 1000.0) * mSignal->duration();
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
    const int value  = int(qRound(seconds / dur * 1000.0));
    if (mTimeSlider->value() != value) {
        mTimeSlider->blockSignals(true);
        mTimeSlider->setValue(value);
        mTimeSlider->blockSignals(false);
    }
    updatePlayhead();
    Q_EMIT currentTimeChanged(currentTime());
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
    if (!mEnvelopeValid && mSignal && mSignal->isValid()) {
        requestEnvelopeRefresh();
        Q_EMIT spectrumRequested(mSignal);
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
    if (mEnvelopeCurve) {
        mEnvelopeCurve->setTitle(tr("Envelope"));  // cn:包络
    }
    if (mSpectrumCurve) {
        mSpectrumCurve->setTitle(tr("Amplitude spectrum"));  // cn:幅值谱
    }
    updateAxisTitles();
}
}  // namespace DA
