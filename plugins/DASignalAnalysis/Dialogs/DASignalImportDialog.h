#ifndef DASIGNALIMPORTDIALOG_H
#define DASIGNALIMPORTDIALOG_H
#include <QDialog>
#include <QStringList>
#include "DAData.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QListWidget;
class QPushButton;
class QDialogButtonBox;

namespace DA
{
class DADataManagerInterface;

/**
 * @brief 信号导入对话框（方案 §5，三态：音频 / 数据帧 / 视频）
 *
 * 音频与视频模式：选择文件 + 校验（视频需 FFmpeg 可用）；
 * 数据帧模式：源数据帧 / 时间列 / 幅值列（多选）/ 采样率 / 信号名。
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
    void onTimeColumnChanged(int index);
    void onValueSelectionChanged();
    void onSampleRateEdited(double v);
    void updateOkEnabled();

private:
    void buildAudioVideoUI();
    void buildDataFrameUI();
    void retranslateUi();
    void refreshDataFrameList();
    void refreshColumnLists();
    void refreshInference();
    void setStatusMessage(const QString& msg, bool isError);

private:
    Mode mMode;
    DADataManagerInterface* mDataMgr { nullptr };

    // 音频/视频
    QLineEdit* mFilePathEdit { nullptr };
    QPushButton* mBrowseButton { nullptr };

    // 数据帧
    QComboBox* mDataCombo { nullptr };
    QComboBox* mTimeColumnCombo { nullptr };
    QListWidget* mValueColumnList { nullptr };
    QDoubleSpinBox* mSampleRateSpin { nullptr };
    QLineEdit* mNameEdit { nullptr };
    QLabel* mInferenceLabel { nullptr };

    // 公共
    QLabel* mStatusLabel { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };
    QList< DAData > mDataFrames;
    bool mUpdatingUI { false };
};
}  // namespace DA

#endif  // DASIGNALIMPORTDIALOG_H
