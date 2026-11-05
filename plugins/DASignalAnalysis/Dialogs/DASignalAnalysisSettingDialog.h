#ifndef DASIGNALANALYSISSETTINGDIALOG_H
#define DASIGNALANALYSISSETTINGDIALOG_H
#include <QDialog>
#include "Analysis/DASignalAnalysisParams.h"

class QLabel;
class QPushButton;
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
    /**
     * @brief 弹出参数对话框
     * @param params 输入输出参数（作为上次值预填，确定后被新值覆盖）
     * @param sampleRate 信号采样率（用于反显段长与毫秒数）
     * @param parent 父窗口（必须是插件页面，避免卸载后残留）
     * @return 用户点确定返回 true
     */
    static bool edit(DASignalAnalysisParams* params, double sampleRate, QWidget* parent);

    explicit DASignalAnalysisSettingDialog(const DASignalAnalysisParams& params,
                                           double sampleRate,
                                           QWidget* parent = nullptr);
    ~DASignalAnalysisSettingDialog() override;

    // 结果参数（确定后取值）
    DASignalAnalysisParams params() const;

protected:
    void changeEvent(QEvent* e) override;

private Q_SLOTS:
    void onFieldValueChanged(const QString& fieldName, const QVariant& value);
    void onRestoreDefaults();

private:
    void buildUI();
    void retranslateUi();
    void refreshDerivedInfo();

private:
    double mSampleRate { 0.0 };
    DAPropertyFormWidget* mForm { nullptr };
    QLabel* mDerivedLabel { nullptr };
    QPushButton* mRestoreButton { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };
};
}  // namespace DA

#endif  // DASIGNALANALYSISSETTINGDIALOG_H
