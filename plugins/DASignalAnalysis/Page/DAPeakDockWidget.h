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
    // 曲线区占位文案（无峰值曲线数据时）
    void setCurvePlaceholderVisible(bool visible);
    // D5：峰值曲线是否为「局部扫描」（只扫可见区段/隔帧降精度），界面标注「局部扫描」
    void setPartialScan(bool on);
    bool isPartialScan() const;
    // 时间原点（秒）：与波形/热图横向对齐（评审 U12）
    void setTimeOrigin(double seconds);
    // dBFS 参考信号峰值（标定后），与频谱图同一参考
    void setSignalPeak(double calibratedPeak);
    // STFT/跟随滑块模式：峰值表显示时间列（评审 U11）
    void setShowTimeColumn(bool on);

Q_SIGNALS:
    // 用户请求把峰值表送到表格页
    void sendToTableRequested();
    // 用户请求导出峰值表 CSV
    void exportCsvRequested();
    // 用户请求打开分析参数对话框（峰值窗口的齿轮，评审 §七-8）
    void settingsRequested();

protected:
    void changeEvent(QEvent* e) override;
    void showEvent(QShowEvent* e) override;

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
    double mTimeOrigin { 0.0 };
    // 首次显示时是否已显式设定 2:1 分割（评审 §七-1）
    bool mDefaultSplitterSizesApplied { false };

    QTableView* mTable { nullptr };
    DAChartWidget* mCurveChart { nullptr };
    QLabel* mCurvePlaceholder { nullptr };
    // D5：局部扫描标注（仅峰值曲线区域，非局部时隐藏）
    QLabel* mPartialLabel { nullptr };
    bool mPartialScan { false };
    QSplitter* mSplitter { nullptr };
    QPushButton* mSendButton { nullptr };
    QPushButton* mExportButton { nullptr };
    QPushButton* mSettingsButton { nullptr };
    QVector< QwtPlotCurve* > mCurveItems;
};
}  // namespace DA

#endif  // DAPEAKDOCKWIDGET_H
