#include "DAPeakTableModel.h"
#include <cmath>
#include <QColor>

namespace DA
{
namespace
{
constexpr double cEps = 1e-30;
}

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
    }
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
    return ColumnCount;
}

/**
 * @brief 幅值按显示口径换算文本
 */
QString DAPeakTableModel::amplitudeText(double magnitude) const
{
    const double v = magnitude * mCalibration;
    if (mAmplitudeMode == QLatin1String("dbFS") || mAmplitudeMode == QLatin1String("dbRef1")) {
        double ref = 1.0;
        if (mAmplitudeMode == QLatin1String("dbFS")) {
            // 满量程参考 = 最强峰
            ref = mPeaks.magnitudes.isEmpty() ? 1.0 : qMax(mPeaks.magnitudes.first() * mCalibration, cEps);
        }
        return QString::number(20.0 * std::log10(qMax(std::fabs(v), cEps) / ref), 'f', 2) + QStringLiteral(" dB");
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
        switch (index.column()) {
        case ColumnIndex:
            return r + 1;
        case ColumnFrequency:
            return QString::number(mPeaks.frequencies.at(r), 'f', 2);
        case ColumnAmplitude:
            return amplitudeText(mPeaks.magnitudes.value(r));
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
        if (index.column() == ColumnIndex && r == 0) {
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
    switch (section) {
    case ColumnIndex:
        return tr("No.");  // cn:序号
    case ColumnFrequency:
        return tr("Frequency (Hz)");  // cn:频率（赫兹）
    case ColumnAmplitude: {
        QString t = tr("Amplitude");  // cn:幅值
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
}
}  // namespace DA
