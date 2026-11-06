#ifndef DASIGNALANALYSISSETTINGDIALOG_H
#define DASIGNALANALYSISSETTINGDIALOG_H
#include <QDialog>
#include "Analysis/DASignalAnalysisParams.h"

class QLabel;
class QPushButton;
class QLineEdit;
class QDialogButtonBox;

namespace DA
{
class DAPropertyFormWidget;

/**
 * @brief 信号分析统一参数对话框（方案 §8.6 / D20 / D21）
 *
 * 内容由 DAFormSpec 生成（可折叠分组），点确定后由调用方立即重算一次；
 * 参数不进全局撤销栈；记住上次生效参数，提供「恢复默认」。
 */
class DASignalAnalysisSettingDialog : public QDialog
{
    Q_OBJECT
public:
    // 弹出参数对话框：params 作为上次值预填，确定后被新值覆盖；sampleRate 用于反显段长
    static bool edit(DASignalAnalysisParams* params, double sampleRate, QWidget* parent);

    explicit DASignalAnalysisSettingDialog(const DASignalAnalysisParams& params,
                                           double sampleRate,
                                           QWidget* parent = nullptr);
    ~DASignalAnalysisSettingDialog() override;

    // 结果参数（确定后取值）
    DASignalAnalysisParams params() const;

protected:
    void changeEvent(QEvent* e) override;
    // 确定前校验 FFmpeg 路径（路径指向不存在的文件时不关闭对话框）
    void accept() override;

private Q_SLOTS:
    void onFieldValueChanged(const QString& fieldName, const QVariant& value);
    void onRestoreDefaults();
    void onBrowseFFmpeg();
    void onFFmpegPathEdited();

private:
    void buildUI();
    void retranslateUi();
    void refreshDerivedInfo();
    // 用户未手改最小间距时，跟随分辨率联动（D17）
    void syncPeakSpacingWithResolution();
    // 频率分辨率 ↔ STFT 窗长二选一联动（方案 §6.3 / 评审 C4）
    void syncStftWindowWithResolution();
    void syncResolutionWithStftWindow();
    double currentDerivedResolution() const;
    // FFmpeg 路径行状态（可用/无效的内联提示，不用模态消息框）
    bool applyFFmpegPath();

private:
    double mSampleRate { 0.0 };
    double mAutoSpacing { -1.0 };  ///< 当前自动跟随行距；-1 表示用户已手改
    DAPropertyFormWidget* mForm { nullptr };
    QLabel* mDerivedLabel { nullptr };
    QLabel* mFFmpegLabel { nullptr };
    QLineEdit* mFFmpegEdit { nullptr };
    QPushButton* mFFmpegBrowse { nullptr };
    QLabel* mFFmpegStatus { nullptr };
    QPushButton* mRestoreButton { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };
};
}  // namespace DA

#endif  // DASIGNALANALYSISSETTINGDIALOG_H
