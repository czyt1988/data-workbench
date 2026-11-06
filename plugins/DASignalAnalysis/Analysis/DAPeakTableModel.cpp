#include "DAPeakTableModel.h"
#include <cmath>
#include <QColor>
#include "Analysis/DASignalAmplitude.h"

namespace DA
{
DAPeakTableModel::DAPeakTableModel(QObject* parent) : QAbstractTableModel(parent)
{
}

DAPeakTableModel::~DAPeakTableModel()
{
}

/**
 * @brief 设置峰值数据并刷新
 */
void DAPeakTableModel::setPeaks(const DAPeakResult& peaks)
{
    beginResetModel();
    mPeaks = peaks;
    endResetModel();
}

DAPeakResult DAPeakTableModel::peaks() const
{
    return mPeaks;
}

void DAPeakTableModel::clear()
{
    setPeaks(DAPeakResult());
}

/**
 * @brief 设置幅值显示模式（与频谱图共用）
 */
void DAPeakTableModel::setAmplitudeMode(const QString& amplitudeMode, const QString& dbReference)
{
    mAmplitudeMode = amplitudeMode;
    mDbReference   = dbReference;
    if (rowCount() > 0) {
        Q_EMIT dataChanged(index(0, ColumnAmplitude), index(rowCount() - 1, ColumnAmplitude));
    }
}

void DAPeakTableModel::setCalibration(double calibration, const QString& unit)
{
    mCalibration = calibration;
    mUnit        = unit;
    if (rowCount() > 0) {
        Q_EMIT dataChanged(index(0, ColumnAmplitude), index(rowCount() - 1, ColumnAmplitude));
        Q_EMIT headerTextChanged();
        // 表头文字变化必须通知视图（评审 §七-6）
        Q_EMIT headerDataChanged(Qt::Horizontal, ColumnAmplitude, ColumnAmplitude);
    }
}

/**
 * @brief 设置 dBFS 参考的信号峰值（标定后），与频谱图口径一致
 */
void DAPeakTableModel::setSignalPeak(double calibratedPeak)
{
    mSignalPeak = calibratedPeak;
    if (rowCount() > 0) {
        Q_EMIT dataChanged(index(0, ColumnAmplitude), index(rowCount() - 1, ColumnAmplitude));
    }
}

/**
 * @brief 切换时间列（STFT/跟随滑块模式下显示）
 */
void DAPeakTableModel::setShowTimeColumn(bool on)
{
    if (mShowTimeColumn == on) {
        return;
    }
    beginResetModel();
    mShowTimeColumn = on;
    endResetModel();
    Q_EMIT headerTextChanged();
}

bool DAPeakTableModel::showTimeColumn() const
{
    return mShowTimeColumn;
}

/**
 * @brief 视图列 → 逻辑列（隐藏时间列时整体左移一列）
 */
int DAPeakTableModel::mapColumn(int viewColumn) const
{
    if (mShowTimeColumn) {
        return viewColumn;
    }
    // 隐藏时间列：序号列保持第 0 列，其余视图列整体左移一列
    return (viewColumn <= ColumnIndex) ? viewColumn : (viewColumn + 1);
}

int DAPeakTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return mPeaks.frequencies.size();
}

int DAPeakTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return mShowTimeColumn ? ColumnCount : (ColumnCount - 1);
}

/**
 * @brief 幅值按显示口径换算文本（与频谱图同一换算函数）
 */
QString DAPeakTableModel::amplitudeText(double magnitude, bool isDc) const
{
    const double refPeak = (mSignalPeak > 0.0) ? mSignalPeak
                                               : (mPeaks.magnitudes.isEmpty()
                                                      ? 1.0
                                                      : qMax(mPeaks.magnitudes.first() * mCalibration,
                                                             DASignalAmplitude::cEps));
    const double v = DASignalAmplitude::toDisplay(magnitude,
                                                  isDc,
                                                  mAmplitudeMode,
                                                  mDbReference,
                                                  mCalibration,
                                                  refPeak);
    if (DASignalAmplitude::isDbMode(mAmplitudeMode)) {
        return QString::number(v, 'f', 2) + QStringLiteral(" dB");
    }
    return QString::number(v, 'g', 6);
}

QVariant DAPeakTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= mPeaks.frequencies.size()) {
        return QVariant();
    }
    const int r = index.row();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole: {
        switch (mapColumn(index.column())) {
        case ColumnIndex:
            return r + 1;
        case ColumnTime:
            return QString::number(mPeaks.frameTime, 'f', 3);  // 已含时间原点（由 UI 组装）
        case ColumnFrequency:
            return QString::number(mPeaks.frequencies.at(r), 'f', 2);
        case ColumnAmplitude:
            return amplitudeText(mPeaks.magnitudes.value(r), mPeaks.frequencies.value(r) <= 0.0);
        case ColumnRelDb:
            return QString::number(mPeaks.relDb.value(r), 'f', 2);
        case ColumnShare:
            return QString::number(mPeaks.share.value(r), 'f', 2);
        default:
            break;
        }
        break;
    }
    case Qt::TextAlignmentRole:
        return int(Qt::AlignRight | Qt::AlignVCenter);
    case Qt::ForegroundRole:
        // 排名颜色区分：第 1 名橙红（与峰值曲线第 1 名一致），其余正文色
        if (index.column() == 0 && r == 0) {
            return QColor(206, 96, 67);
        }
        return QVariant();
    default:
        break;
    }
    return QVariant();
}

QVariant DAPeakTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    if (orientation == Qt::Vertical) {
        return QVariant();
    }
    switch (mapColumn(section)) {
    case ColumnIndex:
        return tr("No.");  // cn:序号
    case ColumnTime:
        return tr("Time (s)");  // cn:时间（秒）
    case ColumnFrequency:
        return tr("Frequency (Hz)");  // cn:频率（赫兹）
    case ColumnAmplitude: {
        QString t;
        if (DASignalAmplitude::isRmsMode(mAmplitudeMode)) {
            t = tr("RMS");  // cn:均方根
        } else if (DASignalAmplitude::isDbMode(mAmplitudeMode)) {
            t = tr("Amplitude (dB)");  // cn:幅值（dB）
        } else {
            t = tr("Amplitude");  // cn:幅值
        }
        if (!mUnit.isEmpty()) {
            t += QStringLiteral(" [%1]").arg(mUnit);
        }
        return t;
    }
    case ColumnRelDb:
        return tr("Rel. peak (dB)");  // cn:相对主峰（dB）
    case ColumnShare:
        return tr("Share (%)");  // cn:占比（%）
    default:
        break;
    }
    return QVariant();
}

/**
 * @brief 重译表头（模型不是 QWidget，语言切换由持有者调用）
 */
void DAPeakTableModel::retranslateUi()
{
    Q_EMIT headerTextChanged();
    Q_EMIT headerDataChanged(Qt::Horizontal, 0, ColumnCount - 1);
}
}  // namespace DA
