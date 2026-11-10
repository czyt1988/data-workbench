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
    // 回显用参数副本：<=0 的自动哨兵替换为 5 倍实际分辨率（表单 min=0 会把 -1 夹成 0，评审 N-U6）
    DASignalAnalysisParams makeEchoParams(const DASignalAnalysisParams& params, double derivedResolution) const;
    // 语言切换：重建表单规格以重译字段标签，保留当前值（评审 I7）
    void rebuildFormForLanguage();
    void refreshDerivedInfo();
    // 当前生效的实际分辨率（由分辨率参数反推段长后回算）
    double derivedResolutionFor(const DASignalAnalysisParams& params) const;
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
    bool mPeakSpacingAuto { false };  ///< 最小间距是否自动跟随分辨率（参数里写回 -1 哨兵，评审 N-C5）
    bool mFormReady { false };        ///< 表单规格是否已就绪（retranslateUi 在构造早期也会被调用）
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
