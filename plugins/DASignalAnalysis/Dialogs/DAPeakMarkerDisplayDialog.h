#ifndef DAPEAKMARKERDISPLAYDIALOG_H
#define DAPEAKMARKERDISPLAYDIALOG_H
#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QDialogButtonBox;

namespace DA
{
/**
 * @brief 峰值标记显示设置对话框（丝带「峰值」panel →「标记显示」）
 *
 * 一个简单的模板编辑器：一排占位符按钮（序号 / 频率 / 幅值）把占位符插入编辑框，
 * 编辑框下方实时渲染成示例文本（如 `{index}-{fre} Hz` 渲染为 `1-32.5 Hz`）。
 * 模板只影响频谱图上峰值标记上方的文本，不改变标记位置与峰值数据。
 *
 * 模板是全局显示偏好（signal-analysis-config.json 的 `peakMarkerText`），对所有信号生效。
 */
class DAPeakMarkerDisplayDialog : public QDialog
{
    Q_OBJECT
public:
    // 弹出对话框：textTemplate 作为上次值预填，确定后被新值覆盖；dbMode 决定预览幅值是否带 dB
    static bool edit(QString* textTemplate, bool dbMode, QWidget* parent);

    explicit DAPeakMarkerDisplayDialog(const QString& textTemplate, bool dbMode, QWidget* parent = nullptr);
    ~DAPeakMarkerDisplayDialog() override;

    // 结果模板（确定后取值）
    QString peakMarkerText() const;

protected:
    void changeEvent(QEvent* e) override;

private Q_SLOTS:
    void onInsertIndex();
    void onInsertFrequency();
    void onInsertAmplitude();
    void onRestoreDefault();
    void updatePreview();

private:
    void buildUI();
    void retranslateUi();
    // 在光标处插入占位符（有选区时替换选区）
    void insertPlaceholder(const QString& placeholder);

private:
    bool mDbMode { false };
    QLineEdit* mEdit { nullptr };
    QLabel* mHintLabel { nullptr };
    QLabel* mPreviewLabel { nullptr };
    QPushButton* mInsertIndexButton { nullptr };
    QPushButton* mInsertFrequencyButton { nullptr };
    QPushButton* mInsertAmplitudeButton { nullptr };
    QPushButton* mRestoreButton { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };
};
}  // namespace DA

#endif  // DAPEAKMARKERDISPLAYDIALOG_H