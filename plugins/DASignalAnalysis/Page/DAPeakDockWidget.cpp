#include "DAPeakDockWidget.h"
#include <QEvent>
#include <QHeaderView>
#include <QPen>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <cmath>
#include "qwt_plot_curve.h"
// DA
#include "DAChartWidget.h"
#include "DAColorTheme.h"
#include "Analysis/DAPeakTableModel.h"

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

    // 峰值曲线
    mCurveChart = new DAChartWidget(mSplitter);
    mCurvePlaceholder = new QLabel(tr("Run the short-time Fourier transform first"), mCurveChart);
    // cn:需先计算短时傅里叶变换
    mCurvePlaceholder->setAlignment(Qt::AlignCenter);
    mCurvePlaceholder->setStyleSheet(QStringLiteral("color:#727272;"));
    QVBoxLayout* curveLay = new QVBoxLayout(mCurveChart);
    curveLay->addWidget(mCurvePlaceholder);
    mSplitter->addWidget(mCurveChart);
    mSplitter->setStretchFactor(0, 2);
    mSplitter->setStretchFactor(1, 1);
    rootLay->addWidget(mSplitter, 1);

    // 按钮行
    QHBoxLayout* btnLay = new QHBoxLayout;
    mSendButton        = new QPushButton(this);
    mExportButton      = new QPushButton(this);
    btnLay->addStretch();
    btnLay->addWidget(mSendButton);
    btnLay->addWidget(mExportButton);
    rootLay->addLayout(btnLay);
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

void DAPeakDockWidget::setCurvePlaceholderVisible(bool visible)
{
    mCurvePlaceholder->setVisible(visible);
}

/**
 * @brief 重绘峰值曲线：每条曲线 = 某名次的频率随时间（颜色按名次固定）
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
    for (int rank = 0; rank < mCurves.topN; ++rank) {
        QVector< double > xs;
        QVector< double > ys;
        xs.reserve(mCurves.nFrames);
        ys.reserve(mCurves.nFrames);
        for (int i = 0; i < mCurves.nFrames; ++i) {
            const double f = mCurves.at(rank, i);
            xs.append(mCurves.times.at(i));
            // 缺失帧写 NaN，让曲线断开（不用 0 补点）
            ys.append(std::isfinite(f) ? f : std::nan(""));
        }
        auto* curve = new QwtPlotCurve(tr("Rank %1").arg(rank + 1));  // cn:第 N 名
        const QColor color = seriesColors.isEmpty() ? QColor(82, 128, 193) : seriesColors.at(rank % seriesColors.size());
        curve->setPen(QPen(color, 1.4));
        curve->setSamples(xs, ys);
        curve->attach(mCurveChart);
        mCurveItems.append(curve);
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

void DAPeakDockWidget::retranslateUi()
{
    if (mCurvePlaceholder) {
        mCurvePlaceholder->setText(tr("Run the short-time Fourier transform first"));
        // cn:需先计算短时傅里叶变换
    }
    if (mSendButton) {
        mSendButton->setText(tr("Send to Table"));  // cn:送到表格页
    }
    if (mExportButton) {
        mExportButton->setText(tr("Export CSV"));  // cn:导出 CSV
    }
    if (mModel) {
        mModel->retranslateUi();
    }
}
}  // namespace DA
