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
 * 只放按信号生效的分析参数（信号预处理/频谱/功率谱/短时傅里叶变换/峰值查找高级项/标定）：
 * 峰值查找的常见项（模式/标记数量/相对门限）在丝带「峰值」panel，本对话框只保留
 * 「模式 = 高级」时生效的三个高级参数；内容由 DAFormSpec 生成（可折叠分组），
 * 点确定后由调用方立即重算一次；参数不进全局撤销栈；记住上次生效参数，提供「恢复默认」。
 *
 * 与数据无关的全局设置（FFmpeg 路径等）不在本对话框里，而在 主页 → 设置 → 信号分析
 * （DASignalAnalysisSettingPage），这样没有导入数据时也能配置。
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

private Q_SLOTS:
    void onFieldValueChanged(const QString& fieldName, const QVariant& value);
    void onRestoreDefaults();

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

private:
    double mSampleRate { 0.0 };
    // 打开对话框时的参数：峰值查找的常见项（模式/标记数量/相对门限）已迁到丝带，表单里没有它们，
    // 关闭时从这份基线回填，避免「确定」把 ribbon 上的设置重置成默认值
    DASignalAnalysisParams mBaseParams;
    bool mPeakSpacingAuto { false };  ///< 最小间距是否自动跟随分辨率（参数里写回 -1 哨兵，评审 N-C5）
    bool mFormReady { false };        ///< 表单规格是否已就绪（retranslateUi 在构造早期也会被调用）
    DAPropertyFormWidget* mForm { nullptr };
    QLabel* mDerivedLabel { nullptr };
    QPushButton* mRestoreButton { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };
};
}  // namespace DA

#endif  // DASIGNALANALYSISSETTINGDIALOG_H
