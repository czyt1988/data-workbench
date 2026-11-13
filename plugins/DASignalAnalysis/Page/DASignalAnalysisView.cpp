#include "DASignalAnalysisView.h"
#include <cmath>
#include <limits>
#include <utility>
#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>
// qwt
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_picker_machine.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_picker.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_raster_data.h"
#include "qwt_samples.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_widget.h"
#include "qwt_symbol.h"
// DA
#include "DAChart3DWidget.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "Analysis/DASignalAmplitude.h"
#include "Render/DAStftRasterData.h"
#include "Render/DAWaterfallBuilder.h"
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
    const QVector< double > pos  = src->stopPos();
    const QVector< QColor > cols = src->stopColors();
    const int n                  = qMin(pos.size(), cols.size());
    for (int i = 0; i < n; ++i) {
        dst->addColorStop(pos[ i ], cols[ i ]);
    }
    return dst;
}

/**
 * @brief 清理峰值标记图元
 *
 * 只对仍挂在频谱图上的标记做 detach+delete（BLK-03a）：共享图元树可能已经删除其中
 * 一部分，对已释放指针二次 delete 会崩溃；失效项只从列表移除。频谱图本身已被销毁时，
 * 标记随 QwtPlot 的 autoDelete 释放，这里只清空列表。
 */
void clearPeakMarkers(QVector< QwtPlotMarker* >& markers, const DAChartWidget* chart)
{
    if (!chart) {
        markers.clear();  // chart 已销毁：标记随 QwtPlot autoDelete 释放，不再 delete
        return;
    }
    const QList< QwtPlotItem* > attached = chart->itemList();
    for (int i = markers.size() - 1; i >= 0; --i) {
        QwtPlotMarker* marker = markers.at(i);
        if (!attached.contains(marker)) {
            markers.removeAt(i);  // 已被外部（图元树）删除：只移除登记，避免二次 delete
            continue;
        }
        marker->detach();
        delete marker;
        markers.removeAt(i);
    }
}

constexpr double cEps             = 1e-30;
constexpr int cEnvelopeDebounceMs = 200;

/**
 * @brief 校验缓存图元是否仍挂在所属 chart 上（BLK-03a）
 *
 * 共享图元树（DAChartManageWidget）可对任意 transient 图元执行 detach+delete，
 * 视图缓存的裸指针随之悬空。这里按 chart 的 itemList() 校验：失效时置空缓存并返回
 * false，调用方按「需要时重建」处理（参考 mSpectrogramItem 的重建分支）。
 * 模板参数匹配缓存成员的具体图元类型（QwtPlotIntervalCurve/QwtPlotCurve 等），
 * 置空直接写回原成员。
 * @param item 缓存图元指针（失效时被置空）
 * @param chart 图元所属子图；为空视为失效
 * @return 图元有效（仍挂载）返回 true；原本为空或已失效返回 false（失效时已置空）
 */
template< typename T >
bool ensureItemAttached(T*& item, const DAChartWidget* chart)
{
    if (!item) {
        return false;
    }
    if (chart && chart->itemList().contains(item)) {
        return true;
    }
    qWarning() << "DASignalAnalysisView: cached plot item was removed externally, it will be rebuilt on demand";
    item = nullptr;
    return false;
}
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
    // 参数变化即触发重算：存档接管的峰值标记让位给重算结果（SF-13，存档优先只在
    // 「加载后到第一次重算前」成立），下一次 rebuildPeakMarkers 按当前 mPeaks 重建
    mPeakMarkersRestored = false;
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
    mEnvXs         = xs;
    mEnvMins       = mins;
    mEnvMaxs       = maxs;
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
    const int n    = qMin(mEnvMins.size(), mEnvMaxs.size());
    double rawPeak = 0.0;
    for (int i = 0; i < n; ++i) {
        rawPeak = qMax(rawPeak, qMax(std::fabs(mEnvMins[ i ]), std::fabs(mEnvMaxs[ i ])));
    }
    mSignalPeak = rawPeak;

    const QString detrend     = mParams.detrend.toLower();
    const bool removeLinear   = (detrend == QLatin1String("linear"));
    const bool removeLevel    = mParams.removeDc || removeLinear || (detrend == QLatin1String("constant"));
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
    double peak            = 0.0;
    for (int i = 0; i < n; ++i) {
        const double base = intercept + slope * double(i);
        peak              = qMax(peak, qMax(std::fabs(mEnvMins[ i ] - base), std::fabs(mEnvMaxs[ i ] - base)));
    }
    mDbfsReferencePeak = (peak > 0.0) ? peak : rawPeak;
}

/**
 * @brief 构建视图布局：figure 网格（T8 起控件的入口在 Ribbon 上下文标签与窗口底部工具栏）
 */
void DASignalAnalysisView::buildUI()
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    buildFigure();
    lay->addWidget(mFigure, 1);

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
 * @brief 创建 figure 并登记 2×2 网格子图（波形跨两列；热图与三维瀑布同格显隐切换）
 */
void DASignalAnalysisView::buildFigure()
{
    mFigure = new DAFigureWidget(this);
    mFigure->setFrameShape(QFrame::NoFrame);
    mFigure->setObjectName(QStringLiteral("da-signal-figure"));
    // figure id 由信号唯一 id 派生（会话内单调递增）：同名信号不再共用同一 figure id
    // （BLK-02）；信号名只作可读标识，存档匹配按 figures.xml 的 signal-id 属性
    mFigure->setFigureId(QStringLiteral("sig-%1").arg(mSignal ? mSignal->signalId() : qint64(0)));
    if (!mFigure->enableGridLayout(2, 2)) {
        qWarning() << "DASignalAnalysisView::buildFigure: failed to enable the 2x2 grid layout";
    }
    // 默认比例：按原 QSplitter 上下 1:1 换算后微调（波形需要更多高度），左右 1:1（方案 §8）
    mFigure->setGridSizes({ 0.56, 0.44 }, { 0.50, 0.50 });

    mWaveChart     = mFigure->createChart();
    mSpectrumChart = mFigure->createChart();
    mHeatmapChart  = mFigure->createChart();
    mWaterfallChart = mFigure->create3DChart();
    if (!mWaveChart || !mSpectrumChart || !mHeatmapChart || !mWaterfallChart) {
        qWarning() << "DASignalAnalysisView::buildFigure: failed to create one or more sub charts";
        return;
    }
    mFigure->setGridCell(mWaveChart, 0, 0, 1, 2);
    mFigure->setGridCell(mSpectrumChart, 1, 0);
    mFigure->setGridCell(mHeatmapChart, 1, 1);
    mFigure->setGridCell(mWaterfallChart, 1, 1);

    // 默认二维热图（D12）：三维瀑布隐藏
    mHeatmapChart->setVisible(true);
    mWaterfallChart->setVisible(false);

    connect(mFigure, &DAFigureWidget::currentChartChanged, this, &DASignalAnalysisView::onFigureCurrentChartChanged);
    connect(mFigure, &DAFigureWidget::current3DChartChanged, this, &DASignalAnalysisView::onFigureCurrent3DChartChanged);

    // ---------- 波形：播放头 + STFT 窗口矩形 + 点击定位 ----------
    // 播放头/STFT 矩形的创建与「被外部删除后重建」共用 ensureWaveDecorItems（BLK-03a）
    ensureWaveDecorItems();

    // 点击波形定位（评审 U1：QwtPicker 不设状态机时 transition() 直接返回，点击无效）。
    // picker 装在 canvas 上且不消费事件；缩放/平移/十字线启用时经 setSeekEnabled 停用（SF-12）
    mSeekPicker = new QwtPlotPicker(QwtPlot::xBottom,
                                    QwtPlot::yLeft,
                                    QwtPlotPicker::NoRubberBand,
                                    QwtPicker::AlwaysOff,
                                    mWaveChart->canvas());
    mSeekPicker->setTrackerMode(QwtPicker::AlwaysOff);
    mSeekPicker->setStateMachine(new QwtPickerClickPointMachine());
    mSeekPicker->setEnabled(mSeekEnabled);
    connect(mSeekPicker, QOverload< const QPointF& >::of(&QwtPlotPicker::selected), this, [this](const QPointF& p) {
        setCurrentTime(p.x() - (mSignal ? mSignal->timeOrigin() : 0.0));
    });
    // STFT 窗口矩形整体拖动（D30）：在画布上监听鼠标，命中矩形内才进入拖动；
    // 同时监听三个 2D 画布，点击即切换当前子图（T8 设置面板 target 跟随）。
    mWaveChart->canvas()->installEventFilter(this);
    mSpectrumChart->canvas()->installEventFilter(this);
    mHeatmapChart->canvas()->installEventFilter(this);
}

/**
 * @brief 校验并按需重建波形画布的装饰图元（播放头/STFT 窗口矩形，BLK-03a）
 *
 * 两者都是 transient 交互装饰，可被共享图元树删除；失效时置空缓存并按原样式重建
 * （transient 属性保持），下一次 updatePlayhead/updateStftWindowRect 自动恢复显示。
 */
void DASignalAnalysisView::ensureWaveDecorItems()
{
    if (!mWaveChart) {
        return;
    }
    if (mPlayhead && !mWaveChart->itemList().contains(mPlayhead)) {
        mPlayhead = nullptr;
    }
    if (!mPlayhead) {
        mPlayhead = new QwtPlotMarker();
        mPlayhead->setLineStyle(QwtPlotMarker::VLine);
        mPlayhead->setLinePen(QPen(QColor(206, 96, 67), 1.5, Qt::DashLine));  // 橙红（危险/标记色）
        mPlayhead->setXValue(0.0);
        mPlayhead->attach(mWaveChart);
        mWaveChart->setItemTransient(mPlayhead);  // 交互装饰：不进存档
    }
    if (mStftWindow && !mWaveChart->itemList().contains(mStftWindow)) {
        mStftWindow = nullptr;
    }
    if (!mStftWindow) {
        mStftWindow = new QwtPlotShapeItem();
        QPen rectPen(QColor(82, 128, 193), 1.0, Qt::SolidLine);  // 主蓝
        QBrush rectBrush(QColor(189, 205, 232, 60));             // 浅蓝半透明
        mStftWindow->setPen(rectPen);
        mStftWindow->setBrush(rectBrush);
        mStftWindow->setVisible(false);
        mStftWindow->attach(mWaveChart);
        mWaveChart->setItemTransient(mStftWindow);  // 交互装饰：不进存档
    }
}

/**
 * @brief 重绘波形（包络 min/max 填充区间 + 播放头）
 */
void DASignalAnalysisView::rebuildWavePlot()
{
    if (!mWaveChart) {
        return;
    }
    if (!mEnvelopeValid) {
        mWaveChart->replot();
        return;
    }
    // 包络曲线可能被共享图元树删除：失效时置空并走重建分支（BLK-03a）
    ensureItemAttached(mEnvelopeCurve, mWaveChart);
    if (!mEnvelopeCurve) {
        mEnvelopeCurve = new QwtPlotIntervalCurve(tr("Envelope"));  // cn:包络
        QPen pen(QColor(73, 124, 173), 1.0);                        // 深蓝
        QBrush brush(QColor(189, 205, 232, 200));                   // 浅蓝
        mEnvelopeCurve->setPen(pen);
        mEnvelopeCurve->setBrush(brush);
        mEnvelopeCurve->attach(mWaveChart);
        // 分析结果图元：transient（序列化跳过，加载后重算重建）
        mWaveChart->setItemTransient(mEnvelopeCurve);
    }
    // 采样点下标 → 时间秒（含时间原点）
    const double fs         = mSignal ? mSignal->sampleRate() : 0.0;
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
    mWaveChart->replot();
}

/**
 * @brief 重绘频谱曲线（峰值标记由 rebuildPeakMarkers 独立维护）
 */
void DASignalAnalysisView::rebuildSpectrumPlot()
{
    if (!mSpectrumChart) {
        return;
    }
    if (!mSpectrum.isValid()) {
        rebuildPeakMarkers();
        mSpectrumChart->replot();
        return;
    }
    // 频谱曲线可能被共享图元树删除：失效时置空并走重建分支（BLK-03a）
    ensureItemAttached(mSpectrumCurve, mSpectrumChart);
    if (!mSpectrumCurve) {
        mSpectrumCurve = new QwtPlotCurve(tr("Amplitude spectrum"));  // cn:幅值谱
        QPen pen(QColor(82, 128, 193), 1.2);                          // 主蓝
        mSpectrumCurve->setPen(pen);
        mSpectrumCurve->attach(mSpectrumChart);
        // 分析结果图元：transient（峰值标记则非 transient，见 rebuildPeakMarkers）
        mSpectrumChart->setItemTransient(mSpectrumCurve);
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
    rebuildPeakMarkers();
    updateAxisTitles();
    mSpectrumChart->setAxisAutoScale(QwtPlot::yLeft, true);
    mSpectrumChart->replot();
}

/**
 * @brief 重建频谱峰值标记（峰值标记非 transient：T9 会写入 chart-data）
 *
 * 只在峰值数据或显隐变化时调用；曲线数据更新不重建标记，避免闪烁。
 * 若峰值标记已从存档接管（mPeakMarkersRestored 且列表非空），存档优先：不删除、
 * 不按重算结果重建，只同步显隐（用户可能手动调整过峰位）。该优先只在「加载后到
 * 第一次重算前」成立：setParams/setAmplitudeDisplay/切通道复位该标志（SF-13）。
 * 已被共享图元树删除的标记跳过并移除登记（BLK-03a）。
 */
void DASignalAnalysisView::rebuildPeakMarkers()
{
    if (!mSpectrumChart) {
        return;
    }
    if (mPeakMarkersRestored && !mPeakMarkers.isEmpty()) {
        const QList< QwtPlotItem* > attached = mSpectrumChart->itemList();
        for (int i = mPeakMarkers.size() - 1; i >= 0; --i) {
            QwtPlotMarker* marker = mPeakMarkers.at(i);
            if (attached.contains(marker)) {
                marker->setVisible(mPeakMarkersVisible);
            } else {
                mPeakMarkers.removeAt(i);  // 已被外部（图元树）删除：只移除登记
            }
        }
        mSpectrumChart->replot();
        return;
    }
    clearPeakMarkers(mPeakMarkers, mSpectrumChart);
    if (!mPeakMarkersVisible || !mPeaks.isValid()) {
        return;
    }
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
        marker->attach(mSpectrumChart);
        // 峰值标记不标 transient：T9 会把它写入 chart-data（inline 持久化）
        mPeakMarkers.append(marker);
    }
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
 *
 * 热图与三维瀑布登记在同一网格 (1,1)，这里按模式切显隐（隐藏者不参与布局重算）。
 */
void DASignalAnalysisView::setWaterfallMode(int mode)
{
    mWaterfallMode = qBound(0, mode, 2);
    if (!mHeatmapChart || !mWaterfallChart || !mFigure) {
        return;
    }
    mHeatmapChart->setVisible(mWaterfallMode == 0);
    mWaterfallChart->setVisible(mWaterfallMode == 1);
    if (mWaterfallMode == 1) {
        // 切到三维：当前子图 = 三维（T8 设置面板切 3D 页）
        mCurrentIs3D = true;
        mFigure->setCurrent3DChart(mWaterfallChart);
        rebuildWaterfall();
    } else {
        mCurrentIs3D = false;
        if (mWaterfallMode == 0) {
            mFigure->setCurrentChart(mHeatmapChart);
        }
    }
    Q_EMIT currentSubChartChanged();
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
 * @brief 设置频率轴线性/对数（同步临时工具栏下拉，触发重绘）
 */
void DASignalAnalysisView::setLogFrequency(bool on)
{
    mLogFrequency = on;
    if (mSpectrumChart) {
        if (on) {
            mSpectrumChart->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
        } else {
            mSpectrumChart->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());
        }
    }
    rebuildSpectrumPlot();
}

/**
 * @brief 当前色带名
 */
QString DASignalAnalysisView::colormapName() const
{
    return mColormapName;
}

/**
 * @brief 设置色带（同步临时工具栏下拉）
 */
void DASignalAnalysisView::setColormapName(const QString& name)
{
    if (name.isEmpty() || mColormapName == name) {
        return;
    }
    mColormapName = name;
    applyHeatmapColors();
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
    applyHeatmapColors();
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
    if (mHeatmapChart) {
        if (!on) {
            mHeatmapChart->setAxisAutoScale(QwtPlot::xBottom, true);
        }
        mHeatmapChart->replot();
    }
}

bool DASignalAnalysisView::isHeatmapAutoScroll() const
{
    return mHeatmapAutoScroll;
}

/**
 * @brief 跟随滑块（频谱显示当前帧而不是整段，D13）
 */
void DASignalAnalysisView::setFollowSlider(bool on)
{
    if (mFollowSlider == on) {
        return;
    }
    mFollowSlider = on;
    rebuildSpectrumPlot();
    // 专用信号（不用 currentTimeChanged）：避免被外部当作 seek 而暂停播放（评审 §七-2）
    Q_EMIT followSliderToggled(on);
}

bool DASignalAnalysisView::isFollowSlider() const
{
    return mFollowSlider;
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
    // 点击任意 2D 子图即切换当前子图（T8 设置面板 target 跟随）。
    // 真实点击落在 QwtPlotCanvas 上：qwt figure 的激活事件过滤器原先装在 QwtPlot 上，
    // 收不到 canvas 事件（QwtPicker::eventFilter 恒 return false，并不消费点击）；
    // 因此这里在画布层显式同步当前子图，且不消费事件（交互行为不变）。
    if (e && e->type() == QEvent::MouseButtonPress && mFigure) {
        auto* me = static_cast< QMouseEvent* >(e);
        if (me->button() == Qt::LeftButton) {
            if (mWaveChart && watched == mWaveChart->canvas()) {
                mFigure->setCurrentChart(mWaveChart);
            } else if (mSpectrumChart && watched == mSpectrumChart->canvas()) {
                mFigure->setCurrentChart(mSpectrumChart);
            } else if (mHeatmapChart && watched == mHeatmapChart->canvas()) {
                mFigure->setCurrentChart(mHeatmapChart);
            }
        }
    }
    if (!mWaveChart) {
        return QWidget::eventFilter(watched, e);
    }
    if (watched != mWaveChart->canvas()) {
        return QWidget::eventFilter(watched, e);
    }
    // STFT 矩形可能被共享图元树删除：失效时按原样式重建（BLK-03a）
    ensureWaveDecorItems();
    // 鼠标位置是否落在 STFT 窗口矩形内（X 与 Y 都要命中，评审 N-U5）
    auto hitWindowRect = [this](const QPoint& pos) -> bool {
        if (!mStftWindow || !mStftWindow->isVisible() || !mStft.isValid()) {
            return false;
        }
        if (!mWaveChart->itemList().contains(mStftWindow)) {
            return false;  // 已被外部删除的缓存指针不参与命中判定（BLK-03a）
        }
        const QRectF rect = mStftWindow->boundingRect();
        if (!rect.isValid()) {
            return false;
        }
        const QPointF plotPos(mWaveChart->invTransform(QwtPlot::xBottom, pos.x()),
                              mWaveChart->invTransform(QwtPlot::yLeft, pos.y()));
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
                mWaveChart->canvas()->setCursor(Qt::SizeAllCursor);
            }
        }
        if (mDraggingWindow) {
            const double curTimeAtPixel = mWaveChart->invTransform(QwtPlot::xBottom, me->pos().x());
            const double delta = curTimeAtPixel - mWaveChart->invTransform(QwtPlot::xBottom, mDragPressPos.x());
            setCurrentTime(mDragPressTime + delta);
            return true;
        }
        // 悬停提示：命中矩形内显示移动光标
        if (mStftWindow && mStftWindow->isVisible() && mStft.isValid()) {
            const Qt::CursorShape want = hitWindowRect(me->pos()) ? Qt::SizeAllCursor : Qt::ArrowCursor;
            if (mWaveChart->canvas()->cursor().shape() != want) {
                mWaveChart->canvas()->setCursor(want);
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
            mWaveChart->canvas()->setCursor(Qt::ArrowCursor);
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
    if (!mHeatmapChart) {
        return;
    }
    if (!mStft.isValid()) {
        mHeatmapChart->replot();
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
    // 栅格所有权归 QwtPlotSpectrogram（setData 接管）；若图表被清空/图元树删除导致谱图
    // 图元已销毁，指针必须一起置空，避免继续写已释放的栅格（评审 H3/A5、BLK-03a）
    if (!ensureItemAttached(mSpectrogramItem, mHeatmapChart)) {
        mStftRaster = nullptr;  // 栅格随谱图图元一起被销毁
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
        mSpectrogramItem = mHeatmapChart->addSpectrogram(static_cast< QwtRasterData* >(mStftRaster),
                                                         tr("Spectrogram"));  // cn:时频图
        if (mSpectrogramItem) {
            mSpectrogramItem->setRenderThreadCount(0);
            mSpectrogramItem->setCachePolicy(QwtPlotRasterItem::PaintCache);
            // 分析结果图元：transient（序列化跳过，加载后重算重建）
            mHeatmapChart->setItemTransient(mSpectrogramItem);
        }
    } else {
        mSpectrogramItem->setData(static_cast< QwtRasterData* >(mStftRaster));
        mSpectrogramItem->attach(mHeatmapChart);
    }
    mHeatmapChart->enableAxis(QwtPlot::yRight);
    if (QwtScaleWidget* colorBar = mHeatmapChart->axisWidget(QwtPlot::yRight)) {
        colorBar->setColorBarEnabled(true);
        colorBar->setColorBarWidth(18);
        colorBar->setTitle(tr("dB"));  // cn:分贝
    }
    mHeatmapChart->setAxisTitle(QwtPlot::xBottom, tr("Time (s)"));      // cn:时间（秒）
    mHeatmapChart->setAxisTitle(QwtPlot::yLeft, tr("Frequency (Hz)"));  // cn:频率（赫兹）
    applyHeatmapColors();
    mHeatmapChart->replot();
}

/**
 * @brief 应用色带与色标范围（色带或 dB 下限变化时调用）
 */
void DASignalAnalysisView::applyHeatmapColors()
{
    if (!mHeatmapChart) {
        return;
    }
    // 谱图图元可能被共享图元树删除：失效时置空（连同栅格）并返回，下一次 rebuildHeatmap 重建（BLK-03a）
    if (!ensureItemAttached(mSpectrogramItem, mHeatmapChart)) {
        mStftRaster = nullptr;  // 栅格随谱图图元一起被销毁
        return;
    }
    auto cmap = QwtColorMapPreset::create(mColormapName);
    if (cmap) {
        mSpectrogramItem->setColorMap(cloneLinearColorMap(cmap.get()));
        if (QwtScaleWidget* colorBar = mHeatmapChart->axisWidget(QwtPlot::yRight)) {
            // setColorMap 同时设定色标区间与色带（接管所有权）
            colorBar->setColorMap(QwtInterval(mDbFloor, 0.0), cloneLinearColorMap(cmap.get()));
        }
    }
    if (mStftRaster) {
        mStftRaster->setValueRange(mDbFloor, 0.0);
    }
    mHeatmapChart->replot();
}

/**
 * @brief 请求外部计算时频热图（目标尺寸按绘图区像素）
 */
void DASignalAnalysisView::requestHeatmapRefresh()
{
    if (!mSignal || !mSignal->isValid() || !mHeatmapChart) {
        return;
    }
    const int columns = qMax(64, mHeatmapChart->canvas()->width());
    const int rows    = qMax(64, mHeatmapChart->canvas()->height());
    Q_EMIT heatmapRequested(columns, rows);
}

/**
 * @brief 更新 STFT 窗口矩形（波形上）：宽 = 窗长、中心 = 当前时间（D30）
 */
void DASignalAnalysisView::updateStftWindowRect()
{
    if (!mSignal) {
        return;
    }
    // 播放头/STFT 矩形可能被共享图元树删除：失效时按原样式重建（BLK-03a）
    ensureWaveDecorItems();
    if (!mStftWindow) {
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
    if (mWaveChart) {
        mWaveChart->replot();
    }
}

/**
 * @brief 峰值表数据回填（频谱图上标记峰位）
 */
void DASignalAnalysisView::setPeaks(const DAPeakResult& peaks)
{
    mPeaks = peaks;
    rebuildPeakMarkers();
    if (mSpectrumChart) {
        mSpectrumChart->replot();
    }
}

/**
 * @brief 峰值标记显隐（丝带「峰值标记」快捷开关）
 */
void DASignalAnalysisView::setPeakMarkersVisible(bool on)
{
    if (mPeakMarkersVisible == on) {
        return;
    }
    mPeakMarkersVisible = on;
    rebuildPeakMarkers();
    if (mSpectrumChart) {
        mSpectrumChart->replot();
    }
}

/**
 * @brief 当前峰值标记图元（存档保存用）
 */
QList< QwtPlotMarker* > DASignalAnalysisView::peakMarkers() const
{
    QList< QwtPlotMarker* > markers;
    markers.reserve(mPeakMarkers.size());
    for (QwtPlotMarker* marker : mPeakMarkers) {
        markers.append(marker);
    }
    return markers;
}

/**
 * @brief 接管从存档恢复的峰值标记
 *
 * 标记由 DAXmlHelper 从 chart-data 读回并已挂在频谱图上，这里只登记并同步显隐状态；
 * 后续 rebuildPeakMarkers 不再删除重建（存档优先，用户可能手动调整过峰位）。
 * @param markers 从 chart-data 恢复的峰值标记（所有权交给视图，随视图销毁/通道切换清理）
 */
void DASignalAnalysisView::adoptRestoredPeakMarkers(const QList< QwtPlotMarker* >& markers)
{
    for (QwtPlotMarker* marker : markers) {
        if (marker && !mPeakMarkers.contains(marker)) {
            mPeakMarkers.append(marker);
        }
    }
    if (mPeakMarkers.isEmpty()) {
        return;
    }
    mPeakMarkersRestored = true;
    mPeakMarkersVisible  = mPeakMarkers.first()->isVisible();
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
    if (mWaveChart) {
        mWaveChart->setAxisTitle(QwtPlot::xBottom, tr("Time (s)"));  // cn:时间（秒）
        QString ampTitle = tr("Amplitude");                          // cn:幅值
        if (!mParams.unit.isEmpty()) {
            ampTitle += QStringLiteral(" [%1]").arg(mParams.unit);
        }
        mWaveChart->setAxisTitle(QwtPlot::yLeft, ampTitle);
    }
    if (mSpectrumChart) {
        mSpectrumChart->setAxisTitle(QwtPlot::xBottom, tr("Frequency (Hz)"));  // cn:频率（赫兹）
        QString specY = tr("Amplitude");                                       // cn:幅值
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
        mSpectrumChart->setAxisTitle(QwtPlot::yLeft, specY);
    }
}

/**
 * @brief 幅值显示模式切换（只重绘，不重算）
 *
 * 口径切换后峰值标记必须跟随：这里复位「存档接管」标志（SF-13），下一次
 * rebuildPeakMarkers（随 rebuildSpectrumPlot 触发）按当前 mPeaks 以新口径换算重建
 * （DASignalAmplitude::toDisplay 是唯一换算入口，mParams.amplitudeMode 已更新）。
 */
void DASignalAnalysisView::setAmplitudeDisplay(const QString& amplitudeMode, const QString& dbReference)
{
    mParams.amplitudeMode = amplitudeMode;
    mParams.dbReference   = dbReference;
    mPeakMarkersRestored  = false;
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
    return mCurrentChannel;
}

/**
 * @brief 切换当前通道：清空通道相关缓存并重发分析请求（幂等）
 */
void DASignalAnalysisView::setCurrentChannel(int channel)
{
    if (!mSignal) {
        return;
    }
    const int count = qMax(1, mSignal->channelCount());
    channel         = qBound(0, channel, count - 1);
    if (channel == mCurrentChannel) {
        return;
    }
    mCurrentChannel = channel;
    // 通道切换：清空缓存并重新请求包络/频谱/热图
    mEnvelopeValid = false;
    mEnvXs.clear();
    mEnvMins.clear();
    mEnvMaxs.clear();
    mStft = DAStftResult();
    // 曲线可能已被共享图元树删除：失效时置空，不写已释放对象（BLK-03a）
    ensureItemAttached(mEnvelopeCurve, mWaveChart);
    if (mEnvelopeCurve) {
        mEnvelopeCurve->setSamples(QVector< QwtIntervalSample >());
        mWaveChart->replot();
    }
    ensureItemAttached(mSpectrumCurve, mSpectrumChart);
    if (mSpectrumCurve) {
        mSpectrumCurve->setSamples(QVector< double >(), QVector< double >());
    }
    clearPeakMarkers(mPeakMarkers, mSpectrumChart);
    // 通道切换会重算峰值：存档恢复的标记已随上面的清理删除，复位「存档接管」状态
    mPeakMarkersRestored = false;
    requestEnvelopeRefresh();
    Q_EMIT spectrumRequested(mSignal, mCurrentChannel);
    requestHeatmapRefresh();
    Q_EMIT channelChanged(mCurrentChannel);
}

/**
 * @brief 通道名列表（Ribbon 通道菜单用）
 */
QStringList DASignalAnalysisView::channelNames() const
{
    if (!mSignal) {
        return QStringList();
    }
    return mSignal->channelNamesOrDefaults();
}

/**
 * @brief 当前波形可见时间范围（秒，绝对时间，含 timeOrigin）
 *
 * 波形横轴 = 相对秒 + timeOrigin，因此直接取波形图 X 轴区间即绝对秒；无包络/未布局时
 * 返回 false（调用方按整段处理）。
 */
bool DASignalAnalysisView::visibleTimeRange(double* startSec, double* endSec) const
{
    if (!mWaveChart || !mSignal) {
        return false;
    }
    const QwtInterval interval = mWaveChart->axisScaleDiv(QwtPlot::xBottom).interval();
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
    return mCurrentTimeSeconds;
}

/**
 * @brief 设置当前时间（秒）：同步临时工具栏滑块与播放头
 */
void DASignalAnalysisView::setCurrentTime(double seconds)
{
    if (!mSignal || mSignal->duration() <= 0.0) {
        return;
    }
    const double dur = mSignal->duration();
    seconds          = qBound(0.0, seconds, dur);
    mCurrentTimeSeconds = seconds;
    updatePlayhead();
    Q_EMIT currentTimeChanged(mCurrentTimeSeconds);
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
    mCurrentTimeSeconds = seconds;
    updatePlayhead();
}

/**
 * @brief 「点击波形即定位」开关（SF-12）
 *
 * seek picker（QwtPickerClickPointMachine）在任意左键按下即触发定位，且不消费事件、
 * 不受缩放/平移开关影响；Zoom/Pan 框选拖动时按下瞬间会把播放头拉到按下位置。
 * 交互工具（缩放/平移/十字线）任一启用时由 UI 层关闭本开关，全关时恢复。
 */
void DASignalAnalysisView::setSeekEnabled(bool on)
{
    mSeekEnabled = on;
    if (mSeekPicker) {
        mSeekPicker->setEnabled(on);
    }
}

/**
 * @brief 「点击波形即定位」是否启用
 */
bool DASignalAnalysisView::isSeekEnabled() const
{
    return mSeekEnabled;
}

/**
 * @brief 更新播放头位置（波形图 x = 当前时间）
 */
void DASignalAnalysisView::updatePlayhead()
{
    if (!mSignal) {
        return;
    }
    // 播放头可能被共享图元树删除：失效时按原样式重建（BLK-03a）
    ensureWaveDecorItems();
    const double t = currentTime() + mSignal->timeOrigin();
    if (mPlayhead) {
        mPlayhead->setXValue(t);
    }
    updateStftWindowRect();
    if (mFollowSlider) {
        rebuildSpectrumPlot();  // 跟随模式下频谱随滑块更新
    }
    // 热图随播放滚动（D27）：只平移 X 轴范围，不改数据
    if (mHeatmapAutoScroll && mStft.isValid() && mHeatmapChart) {
        const double duration   = qMax(mSignal->duration(), 1e-9);
        const double origin     = mSignal->timeOrigin();
        const double winSeconds = (mStft.windowSeconds > 0.0) ? mStft.windowSeconds : 1.0;
        // 可视跨度：优先沿用当前轴跨度；若等于整段（自动缩放）则先放大到 1/5 段，让滚动可见
        double span = mHeatmapChart->axisScaleDiv(QwtPlot::xBottom).interval().width();
        if (!(span > 0.0) || span >= 0.9 * duration) {
            span = qMax(5.0 * winSeconds, duration / 5.0);
            span = qMin(span, duration);
        }
        double left          = t - span / 2.0;
        const double minLeft = origin;
        const double maxLeft = origin + duration - span;
        left                 = qBound(minLeft, left, qMax(minLeft, maxLeft));
        mHeatmapChart->setAxisScale(QwtPlot::xBottom, left, left + span);
        mHeatmapChart->replot();
    }
    if (mWaveChart) {
        mWaveChart->replot();
    }
}

/**
 * @brief 请求外部计算包络（区间 = 整段信号，宽度 = 波形区像素宽）
 */
void DASignalAnalysisView::requestEnvelopeRefresh()
{
    if (!mSignal || !mSignal->isValid() || !mWaveChart) {
        return;
    }
    const qint64 total = mSignal->sampleCount();
    const int width    = qMax(100, mWaveChart->canvas()->width());
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
    Q_EMIT spectrumRequested(mSignal, mCurrentChannel);
    requestHeatmapRefresh();
}

// ---------------------------------------------------------------- 图表文档（T6/T8）

DAFigureWidget* DASignalAnalysisView::figure() const
{
    return mFigure;
}

QList< DAFigureWidget* > DASignalAnalysisView::figures() const
{
    QList< DAFigureWidget* > res;
    if (mFigure) {
        res.append(mFigure);
    }
    return res;
}

DAChartWidget* DASignalAnalysisView::waveChart() const
{
    return mWaveChart;
}

DAChartWidget* DASignalAnalysisView::spectrumChart() const
{
    return mSpectrumChart;
}

DAChartWidget* DASignalAnalysisView::heatmapChart() const
{
    return mHeatmapChart;
}

DAChart3DWidget* DASignalAnalysisView::waterfall3DChart() const
{
    return mWaterfallChart;
}

/**
 * @brief 当前 2D 子图；无当前或当前子图被瀑布模式隐藏时回退波形
 *
 * 注意：不能用 QWidget::isVisible() 判定（父窗口未显示时所有子控件都返回 false），
 * 可见性由瀑布模式决定。
 */
DAChartWidget* DASignalAnalysisView::currentChart() const
{
    if (!mFigure) {
        return nullptr;
    }
    DAChartWidget* chart = mFigure->getCurrentChart();
    if (chart == mHeatmapChart && mWaterfallMode != 0) {
        chart = nullptr;
    }
    return chart ? chart : mWaveChart;
}

DAChart3DWidget* DASignalAnalysisView::current3DChart() const
{
    return mFigure ? mFigure->getCurrent3DChart() : nullptr;
}

bool DASignalAnalysisView::is3DChartActive() const
{
    return mCurrentIs3D && mWaterfallMode == 1;
}

void DASignalAnalysisView::setCurrentChart(DAChartWidget* chart)
{
    if (mFigure && chart) {
        mFigure->setCurrentChart(chart);
    }
}

QString DASignalAnalysisView::figureId() const
{
    return mFigure ? mFigure->getFigureId() : QString();
}

QList< qreal > DASignalAnalysisView::gridRowSizes() const
{
    return mFigure ? mFigure->gridRowSizes() : QList< qreal >();
}

QList< qreal > DASignalAnalysisView::gridColumnSizes() const
{
    return mFigure ? mFigure->gridColumnSizes() : QList< qreal >();
}

/**
 * @brief 恢复网格比例（尺寸不符/非法值忽略）
 */
void DASignalAnalysisView::setGridSizes(const QList< qreal >& rows, const QList< qreal >& cols)
{
    if (!mFigure) {
        return;
    }
    mFigure->setGridSizes(rows, cols);
}

/**
 * @brief 按 figure XML 恢复后刷新视图
 *
 * 各子图 replot、更新坐标轴标题、按瀑布模式保持热图/三维瀑布显隐；
 * 峰值标记若来自存档则只同步显隐（rebuildPeakMarkers 内部判断）。
 * 这里不发起任何分析请求：分析结果类 transient 图元由外部按参数重算重建。
 */
void DASignalAnalysisView::refreshAfterFigureRestore()
{
    if (mHeatmapChart && mWaterfallChart) {
        mHeatmapChart->setVisible(mWaterfallMode == 0);
        mWaterfallChart->setVisible(mWaterfallMode == 1);
    }
    if (mWaveChart) {
        mWaveChart->replot();
    }
    if (mSpectrumChart) {
        rebuildPeakMarkers();
        mSpectrumChart->replot();
    }
    if (mHeatmapChart) {
        mHeatmapChart->replot();
    }
    if (mWaterfallChart) {
        // 3D 子图是 QOpenGLWidget（Qwt3DPlot），没有 replot，交给 Qt 重新绘制
        mWaterfallChart->update();
    }
    updateAxisTitles();
}

void DASignalAnalysisView::onFigureCurrentChartChanged(DA::DAChartWidget* chart)
{
    if (!chart) {
        return;
    }
    mCurrentIs3D = false;
    Q_EMIT currentSubChartChanged();
}

void DASignalAnalysisView::onFigureCurrent3DChartChanged(DA::DAChart3DWidget* chart)
{
    if (!chart || chart != mWaterfallChart || mWaterfallMode != 1) {
        return;
    }
    mCurrentIs3D = true;
    Q_EMIT currentSubChartChanged();
}

/**
 * @brief 导出用整页位图：三维瀑布可见时把 OpenGL 帧缓冲合成到页面位图中（评审 I11）
 *
 * QWidget::grab() 拿不到 QOpenGLWidget 的内容，必须用 Qwt3DPlot::renderPixmap()
 * 单独取帧缓冲再按控件位置贴回页面位图。
 */
QPixmap DASignalAnalysisView::grabForExport()
{
    QwtFigure* fig = mFigure ? mFigure->figure() : nullptr;
    QPixmap page   = fig ? fig->grab() : grab();
    // 可见性以瀑布模式为准：父窗口未显示时 isVisible() 恒为 false，不能用于导出判定
    if (mWaterfallMode != 1 || !mWaterfallChart || !fig) {
        return page;
    }
    const QPixmap gl = mWaterfallChart->renderPixmap(mWaterfallChart->width(), mWaterfallChart->height());
    if (gl.isNull() || page.isNull()) {
        return page;
    }
    QPainter painter(&page);
    const QPoint topLeft = mWaterfallChart->mapTo(fig, QPoint(0, 0));
    painter.drawPixmap(topLeft, gl);
    painter.end();
    return page;
}

void DASignalAnalysisView::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (mSignal && mSignal->isValid() && mResizeDebounce) {
        mResizeDebounce->start();  // 防抖：尺寸稳定后再重算包络
    }
}

void DASignalAnalysisView::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
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
    // 缓存图元可能已被共享图元树删除：失效时置空跳过（BLK-03a），下一次回填按需重建
    if (ensureItemAttached(mEnvelopeCurve, mWaveChart)) {
        mEnvelopeCurve->setTitle(tr("Envelope"));  // cn:包络
    }
    if (ensureItemAttached(mSpectrumCurve, mSpectrumChart)) {
        mSpectrumCurve->setTitle(tr("Amplitude spectrum"));  // cn:幅值谱
    }
    if (ensureItemAttached(mSpectrogramItem, mHeatmapChart)) {
        mSpectrogramItem->setTitle(tr("Spectrogram"));  // cn:时频图
    } else {
        mStftRaster = nullptr;  // 栅格随谱图图元一起被销毁
    }
    if (mWaterfallItem && mWaterfallChart && mWaterfallChart->getWaterfalls().contains(mWaterfallItem)) {
        mWaterfallItem->setTitle(tr("Waterfall"));  // cn:三维瀑布
    } else {
        mWaterfallItem = nullptr;  // 已被外部（clearAllData3D 等）销毁：置空待重建
    }
    updateAxisTitles();
}
}  // namespace DA
