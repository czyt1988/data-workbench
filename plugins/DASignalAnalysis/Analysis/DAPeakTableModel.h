#ifndef DAPEAKTABLEMODEL_H
#define DAPEAKTABLEMODEL_H
#include <QAbstractTableModel>
#include "Analysis/DAPeakResult.h"

namespace DA
{
/**
 * @brief 峰值表模型（方案 §8.3）
 *
 * 列：序号 /（STFT 模式）时间 / 频率（赫兹）/ 幅值 / 相对主峰（dB）/ 占比（%）。
 * 幅值列按显示口径（峰值幅度、均方根或 dB）换算，与频谱图一致。
 */
class DAPeakTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column
    {
        ColumnIndex = 0,
        ColumnTime,
        ColumnFrequency,
        ColumnAmplitude,
        ColumnRelDb,
        ColumnShare,
        ColumnCount
    };

    explicit DAPeakTableModel(QObject* parent = nullptr);
    ~DAPeakTableModel() override;

    // 设置峰值数据
    void setPeaks(const DAPeakResult& peaks);
    DAPeakResult peaks() const;
    void clear();

    // 幅值显示模式（与频谱图共用口径）
    void setAmplitudeMode(const QString& amplitudeMode, const QString& dbReference);
    // 标定（乘标定系数）
    void setCalibration(double calibration, const QString& unit);
    // dBFS 参考的信号峰值（标定后），与频谱图使用同一参考（评审 I9）
    void setSignalPeak(double calibratedPeak);
    // STFT/跟随滑块模式：显示时间列（评审 U11）；切换会重置模型以改变列数
    void setShowTimeColumn(bool on);
    bool showTimeColumn() const;

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 表头文本（重译用）
    void retranslateUi();

Q_SIGNALS:
    void headerTextChanged();

private:
    QString amplitudeText(double magnitude, bool isDc) const;
    // 视图列 → 逻辑列（隐藏时间列时整体左移一列）
    int mapColumn(int viewColumn) const;

private:
    DAPeakResult mPeaks;
    QString mAmplitudeMode { QStringLiteral("peak") };
    QString mDbReference { QStringLiteral("fullScale") };
    double mCalibration { 1.0 };
    double mSignalPeak { 0.0 };
    bool mShowTimeColumn { false };
    QString mUnit;
};
}  // namespace DA

#endif  // DAPEAKTABLEMODEL_H
