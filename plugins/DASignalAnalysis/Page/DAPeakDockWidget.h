#ifndef DAPEAKDOCKWIDGET_H
#define DAPEAKDOCKWIDGET_H
#include <QWidget>
#include <QVector>
#include "Analysis/DAPeakResult.h"

class QTableView;
class QSplitter;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;

class QwtPlotCurve;

namespace DA
{
class DAChartWidget;
class DAPeakTableModel;

/**
 * @brief 峰值停靠窗口内容（方案 §8.3 / D23）
 *
 * 垂直分割：峰值表占上 2/3、峰值曲线占下 1/3，由丝带「峰值窗口」按钮控制显隐。
 * 没算过 STFT 时曲线区显示占位文案，不置灰整个窗口。
 */
class DAPeakDockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DAPeakDockWidget(QWidget* parent = nullptr);
    ~DAPeakDockWidget() override;

    // 峰值表数据
    void setPeaks(const DAPeakResult& peaks);
    // 每帧前 N 峰值曲线数据
    void setPeakCurves(const DAPeakCurveResult& curves);
    // 幅值显示模式与标定（与频谱图共用）
    void setAmplitudeDisplay(const QString& amplitudeMode, const QString& dbReference, double calibration,
                             const QString& unit);
    // 曲线区占位文案（无 STFT 数据时）
    void setCurvePlaceholderVisible(bool visible);

Q_SIGNALS:
    // 用户请求把峰值表送到表格页
    void sendToTableRequested();
    // 用户请求导出峰值表 CSV
    void exportCsvRequested();

protected:
    void changeEvent(QEvent* e) override;

private:
    void buildUI();
    void rebuildCurves();
    void retranslateUi();

private:
    DAPeakTableModel* mModel { nullptr };
    DAPeakResult mPeaks;
    DAPeakCurveResult mCurves;
    QString mAmplitudeMode { QStringLiteral("peak") };
    QString mDbReference { QStringLiteral("fullScale") };
    double mCalibration { 1.0 };
    QString mUnit;

    QTableView* mTable { nullptr };
    DAChartWidget* mCurveChart { nullptr };
    QLabel* mCurvePlaceholder { nullptr };
    QSplitter* mSplitter { nullptr };
    QPushButton* mSendButton { nullptr };
    QPushButton* mExportButton { nullptr };
    QVector< QwtPlotCurve* > mCurveItems;
};
}  // namespace DA

#endif  // DAPEAKDOCKWIDGET_H
