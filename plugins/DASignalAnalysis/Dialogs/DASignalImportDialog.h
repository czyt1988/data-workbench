#ifndef DASIGNALIMPORTDIALOG_H
#define DASIGNALIMPORTDIALOG_H
#include <QDialog>
#include <QStringList>
#include "DAData.h"

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QDialogButtonBox;

namespace DA
{
class DADataManagerInterface;
class DAPySeriesListView;

/**
 * @brief 信号导入对话框（方案 §5，三态：音频 / 数据帧 / 视频）
 *
 * 音频与视频模式：选择文件 + 校验（视频需 FFmpeg 可用）；
 * 数据帧模式：源数据帧 / 时间列 / 幅值列（多选）/ 采样率 / 信号名。
 *
 * 选列控件按 Q5 决定 ① 使用 DAPySeriesListView（评审 I6）：时间列与幅值列各一个列表，
 * 打开时用 addSeries() 预填候选列，列表项带 dtype 图标/描述（列类型显示）；
 * 时间列单选、幅值列多选（作多通道）。
 *
 * 采样率规则（方案 §4.4 / D3）：
 * - 有时间列：fs = 1 / median(diff(t))，预填输入框；
 * - 无时间列：输入框为空，确定按钮置灰并显示内联提示（不用模态消息框做字段校验）。
 */
class DASignalImportDialog : public QDialog
{
    Q_OBJECT
public:
    enum Mode
    {
        AudioMode,      ///< 导入音频文件
        DataFrameMode,  ///< 从数据帧导入
        VideoMode       ///< 导入视频（抽取音频轨）
    };

    DASignalImportDialog(Mode mode, DADataManagerInterface* dataMgr, QWidget* parent = nullptr);
    ~DASignalImportDialog() override;

    Mode mode() const;

    // ---------- 音频 / 视频模式结果 ----------
    QString filePath() const;

    // ---------- 数据帧模式结果 ----------
    DAData selectedData() const;
    QString selectedTimeColumn() const;
    QStringList selectedValueColumns() const;
    double sampleRate() const;
    QString signalName() const;

protected:
    void changeEvent(QEvent* e) override;

private Q_SLOTS:
    void onBrowseFile();
    void onSourceDataChanged(int index);
    void onTimeColumnChanged();
    void onValueSelectionChanged();
    void onNoTimeColumnToggled(bool checked);
    void onSampleRateEdited(double v);
    void updateOkEnabled();

private:
    // 内联状态提示的种类（语言切换时按种类重新取 tr 文本，评审 I7）
    enum StatusKind
    {
        StatusNone,
        StatusNoDataFrame,
        StatusNeedPositiveSampleRate,
        StatusNoFFmpeg,
        StatusDetail
    };
    // 推断采样率的当前状态（重译时按状态重新渲染提示，评审 I7）
    enum InferenceState
    {
        InferenceNone,
        InferenceNoTimeColumn,
        InferenceOk,
        InferenceFailed
    };

    void buildAudioVideoUI();
    void buildDataFrameUI();
    void retranslateUi();
    void refreshDataFrameList();
    void refreshColumnLists();
    void refreshInference();
    void updateInferenceText();
    void updateFileInfo();
    void setStatusMessage(StatusKind kind, const QString& detail = QString());
    void updateStatusText();
    void clearSeriesList(DAPySeriesListView* list);

private:
    Mode mMode;
    DADataManagerInterface* mDataMgr { nullptr };

    // 音频/视频
    QLineEdit* mFilePathEdit { nullptr };
    QPushButton* mBrowseButton { nullptr };
    QLabel* mFileInfoLabel { nullptr };

    // 数据帧
    QComboBox* mDataCombo { nullptr };
    DAPySeriesListView* mTimeColumnList { nullptr };
    QCheckBox* mNoTimeColumnCheck { nullptr };
    DAPySeriesListView* mValueColumnList { nullptr };
    QDoubleSpinBox* mSampleRateSpin { nullptr };
    QLineEdit* mNameEdit { nullptr };
    QLabel* mInferenceLabel { nullptr };
    // 表单行标签（语言切换时需要重译）
    QLabel* mFileLabel { nullptr };
    QLabel* mDataLabel { nullptr };
    QLabel* mTimeLabel { nullptr };
    QLabel* mValueLabel { nullptr };
    QLabel* mSampleRateLabel { nullptr };
    QLabel* mNameLabel { nullptr };

    // 公共
    QLabel* mStatusLabel { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };
    StatusKind mStatusKind { StatusNone };
    QString mStatusDetail;
    InferenceState mInferenceState { InferenceNone };
    double mInferredRate { 0.0 };
    double mInferredDuration { 0.0 };
    QString mInferenceError;
    QList< DAData > mDataFrames;
    bool mUpdatingUI { false };
};
}  // namespace DA

#endif  // DASIGNALIMPORTDIALOG_H
