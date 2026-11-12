#include "DAPeakDockWidget.h"
#include <QEvent>
#include <QHeaderView>
#include <QPen>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <cmath>
#include <utility>
#include "qwt_plot_curve.h"
// DA
#include "DAChartWidget.h"
#include "DAColorTheme.h"
#include "Analysis/DAPeakTableModel.h"
#include "DAPeakCurvePlotSamples.h"

namespace DA
{
DAPeakDockWidget::DAPeakDockWidget(QWidget* parent) : QWidget(parent)
{
    buildUI();
    retranslateUi();
}

DAPeakDockWidget::~DAPeakDockWidget()
{
}

/**
 * @brief 构建界面：上表 2/3 + 下曲线 1/3（D23）
 */
void DAPeakDockWidget::buildUI()
{
    QVBoxLayout* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(2, 2, 2, 2);

    mSplitter = new QSplitter(Qt::Vertical, this);
    mSplitter->setChildrenCollapsible(false);

    // 峰值表
    mModel = new DAPeakTableModel(this);
    mTable = new QTableView(mSplitter);
    mTable->setModel(mModel);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->verticalHeader()->setVisible(false);
    mTable->setAlternatingRowColors(true);
    mSplitter->addWidget(mTable);

    // 峰值曲线：顶部说明行（曲线 / 点的含义）+ 图表
    QWidget* curveArea        = new QWidget(mSplitter);
    QVBoxLayout* curveAreaLay = new QVBoxLayout(curveArea);
    curveAreaLay->setContentsMargins(0, 0, 0, 0);
    curveAreaLay->setSpacing(2);
    mCurveHint = new QLabel(curveArea);
    mCurveHint->setStyleSheet(QStringLiteral("color:#727272;"));
    mCurveHint->setWordWrap(true);
    curveAreaLay->addWidget(mCurveHint);
    mCurveChart       = new DAChartWidget(curveArea);
    mCurvePlaceholder = new QLabel(tr("Open this window to compute the per-frame peak curves"), mCurveChart);
    // cn:打开本窗口后计算每帧前 N 峰值曲线
    mCurvePlaceholder->setAlignment(Qt::AlignCenter);
    mCurvePlaceholder->setStyleSheet(QStringLiteral("color:#727272;"));
    QVBoxLayout* curveLay = new QVBoxLayout(mCurveChart);
    curveLay->addWidget(mCurvePlaceholder);
    curveAreaLay->addWidget(mCurveChart, 1);
    mSplitter->addWidget(curveArea);
    mSplitter->setStretchFactor(0, 2);
    mSplitter->setStretchFactor(1, 1);
    rootLay->addWidget(mSplitter, 1);

    // 按钮行
    QHBoxLayout* btnLay = new QHBoxLayout;
    // D5：局部扫描标注放在按钮行左侧（峰值曲线只覆盖可见区段时提示用户）
    mPartialLabel = new QLabel(this);
    mPartialLabel->setStyleSheet(QStringLiteral("color:#b26a00;font-weight:600;"));
    mPartialLabel->setVisible(false);
    btnLay->addWidget(mPartialLabel);
    mSettingsButton     = new QPushButton(this);
    mSendButton         = new QPushButton(this);
    mExportButton       = new QPushButton(this);
    btnLay->addWidget(mSettingsButton);
    btnLay->addStretch();
    btnLay->addWidget(mSendButton);
    btnLay->addWidget(mExportButton);
    rootLay->addLayout(btnLay);
    connect(mSettingsButton, &QPushButton::clicked, this, &DAPeakDockWidget::settingsRequested);
    connect(mSendButton, &QPushButton::clicked, this, &DAPeakDockWidget::sendToTableRequested);
    connect(mExportButton, &QPushButton::clicked, this, &DAPeakDockWidget::exportCsvRequested);

    mCurvePlaceholder->setVisible(true);
}

/**
 * @brief 设置峰值表数据
 */
void DAPeakDockWidget::setPeaks(const DAPeakResult& peaks)
{
    mPeaks = peaks;
    mModel->setPeaks(peaks);
    mModel->setAmplitudeMode(mAmplitudeMode, mDbReference);
    mModel->setCalibration(mCalibration, mUnit);
}

/**
 * @brief 设置每帧前 N 峰值曲线并重绘
 */
void DAPeakDockWidget::setPeakCurves(const DAPeakCurveResult& curves)
{
    mCurves = curves;
    rebuildCurves();
}

/**
 * @brief 幅值显示模式与标定（与频谱图共用口径）
 */
void DAPeakDockWidget::setAmplitudeDisplay(const QString& amplitudeMode,
                                           const QString& dbReference,
                                           double calibration,
                                           const QString& unit)
{
    mAmplitudeMode = amplitudeMode;
    mDbReference   = dbReference;
    mCalibration   = calibration;
    mUnit          = unit;
    mModel->setAmplitudeMode(amplitudeMode, dbReference);
    mModel->setCalibration(calibration, unit);
}

/**
 * @brief D5：标记峰值曲线为局部扫描（超预算降级后），显示/隐藏「局部扫描」标注
 */
void DAPeakDockWidget::setPartialScan(bool on)
{
    mPartialScan = on;
    if (mPartialLabel) {
        mPartialLabel->setVisible(on);
    }
}

/**
 * @brief D5：当前峰值曲线是否为局部扫描
 */
bool DAPeakDockWidget::isPartialScan() const
{
    return mPartialScan;
}

void DAPeakDockWidget::setCurvePlaceholderVisible(bool visible)
{
    mCurvePlaceholder->setVisible(visible);
}

/**
 * @brief 设置曲线时间原点（秒）：数据帧时间列首值非 0 时与波形/热图对齐
 */
void DAPeakDockWidget::setTimeOrigin(double seconds)
{
    if (qFuzzyCompare(mTimeOrigin + 1.0, seconds + 1.0)) {
        return;
    }
    mTimeOrigin = seconds;
    rebuildCurves();
}

/**
 * @brief 设置 dBFS 参考信号峰值（标定后），与频谱图口径一致
 */
void DAPeakDockWidget::setSignalPeak(double calibratedPeak)
{
    if (mModel) {
        mModel->setSignalPeak(calibratedPeak);
    }
}

/**
 * @brief 显示/隐藏峰值表的时间列（STFT/跟随滑块模式）
 */
void DAPeakDockWidget::setShowTimeColumn(bool on)
{
    if (mModel) {
        mModel->setShowTimeColumn(on);
    }
}

/**
 * @brief 重绘峰值曲线：每条曲线 = 某名次的频率随时间（颜色按名次固定）
 *
 * 「每帧独立取前 N」意味着同名次在相邻帧不一定是同一个峰，直接逐帧连线会在噪声/名次互换时
 * 画出横跨频段的伪连线（整个窗口像一张「笼子」）。这里按名次画两层：
 * - 点（Dots）：所有有效帧，跳变/缺峰处也不丢数据；
 * - 折线（Lines）：只连频率连续（跳变 <= 8 × Δf）的相邻帧，跳变处由 NaN 断开。
 * 这样斜扫频、跳频的水平段、稳定的音调都会连成清楚的轨迹，噪声帧只留下散点。
 */
void DAPeakDockWidget::rebuildCurves()
{
    if (!mCurveChart) {
        return;
    }
    // 清理旧曲线
    for (QwtPlotCurve* c : std::as_const(mCurveItems)) {
        c->detach();
        delete c;
    }
    mCurveItems.clear();

    if (!mCurves.isValid()) {
        mCurvePlaceholder->setVisible(true);
        mCurveChart->replot();
        return;
    }
    mCurvePlaceholder->setVisible(false);
    // 系列色取项目图表主题顺序（与绘图区一致，不自造色板）
    const QList< QColor > seriesColors = DAColorTheme(DAColorTheme::Style_Cassatt1).toColorList();
    // 相邻帧跳变阈值：超过即认为不是同一个峰，只画点不连线（避免伪连线）
    const double jumpThresholdHz = daPeakCurveJumpThresholdHz(mCurves);
    for (int rank = 0; rank < mCurves.topN; ++rank) {
        const QColor color = seriesColors.isEmpty() ? QColor(82, 128, 193) : seriesColors.at(rank % seriesColors.size());
        const QString title = tr("Rank %1").arg(rank + 1);  // cn:第 N 名
        const DAPeakCurvePlotSamples samples = daPeakCurvePlotSamples(mCurves, rank, jumpThresholdHz, mTimeOrigin);
        if (samples.hasDots()) {
            auto* dots = new QwtPlotCurve(title);
            dots->setStyle(QwtPlotCurve::Dots);
            QPen dotPen(color);
            dotPen.setWidthF(2.0);
            dotPen.setCapStyle(Qt::RoundCap);
            dots->setPen(dotPen);
            dots->setSamples(samples.dotXs, samples.dotYs);
            dots->attach(mCurveChart);
            mCurveItems.append(dots);
        }
        if (samples.hasLines()) {
            auto* line = new QwtPlotCurve(title);
            line->setPen(QPen(color, 1.4));
            line->setCurveAttribute(QwtPlotCurve::BreakAtNan, true);
            line->setSamples(samples.lineXs, samples.lineYs);
            line->attach(mCurveChart);
            mCurveItems.append(line);
        }
    }
    mCurveChart->setAxisTitle(QwtPlot::xBottom, tr("Time (s)"));      // cn:时间（秒）
    mCurveChart->setAxisTitle(QwtPlot::yLeft, tr("Frequency (Hz)"));  // cn:频率（赫兹）
    mCurveChart->replot();
}

void DAPeakDockWidget::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

/**
 * @brief 首次显示时显式设定「表 2/3 + 曲线 1/3」分割（评审 §七-1）
 *
 * 只设 stretchFactor 时初次布局按 sizeHint 分配，表格行高会把曲线区挤到几乎没有；
 * 停靠窗口真正显示后在这里显式 setSizes（D23 / 方案 §8.3）。
 */
void DAPeakDockWidget::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if (!mDefaultSplitterSizesApplied && mSplitter) {
        mDefaultSplitterSizesApplied = true;
        const int h = qMax(240, mSplitter->height());
        const int tableH = (h * 2) / 3;
        mSplitter->setSizes({ tableH, h - tableH });
    }
}

void DAPeakDockWidget::retranslateUi()
{
    if (mPartialLabel) {
        mPartialLabel->setText(tr("Local scan"));  // cn:局部扫描
        mPartialLabel->setToolTip(tr("Peak curves cover only the visible time range or use a coarser frame step."));
        // cn:峰值曲线只覆盖可见时间范围，或使用了更粗的帧步进。
    }
    if (mCurveHint) {
        mCurveHint->setText(tr("Per-frame top N peaks: frequency over time (color = rank)"));
        // cn:每帧前 N 名峰值：频率随时间变化（颜色 = 名次）
        mCurveHint->setToolTip(
            tr("Each color is one rank. Adjacent frames whose peak frequency jumps far apart are drawn as separate points instead of a connected line, so lines mean continuous peaks."));
        // cn:每种颜色是一个名次；相邻帧频率跳变过大时只画点不连线，因此连线表示连续的峰。
    }
    if (mCurvePlaceholder) {
        mCurvePlaceholder->setText(tr("Open this window to compute the per-frame peak curves"));
        // cn:打开本窗口后计算每帧前 N 峰值曲线
    }
    if (mSendButton) {
        mSendButton->setText(tr("Send to Table"));  // cn:送到表格页
        // v1 口径（§10.2）：送表格页 = 加入数据管理器，不自动打开表格页（评审 I8）
        mSendButton->setToolTip(tr("Add the peak table to the data manager; open it from the data manager"));
        // cn:把峰值表加入数据管理器；可从数据管理器里打开查看
    }
    if (mSettingsButton) {
        mSettingsButton->setText(tr("Settings..."));  // cn:参数设置...
        mSettingsButton->setToolTip(tr("Open the analysis parameter dialog"));  // cn:打开分析参数对话框
    }
    if (mExportButton) {
        mExportButton->setText(tr("Export CSV"));  // cn:导出 CSV
    }
    if (mModel) {
        mModel->retranslateUi();
    }
}
}  // namespace DA
