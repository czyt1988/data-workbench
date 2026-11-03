#ifndef DADIALOGRESHAPE_H
#define DADIALOGRESHAPE_H
#include <QDialog>
#include <QStringList>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"

class QRadioButton;
class QListWidget;
class QLineEdit;
class QCheckBox;
class QTableWidget;
class QLabel;
class QPushButton;
class QTimer;
class QWidget;

namespace DA
{
/**
 * @brief 形状变换对话框（004 T1.27）：一个对话框两种模式（转置 / 宽转长 melt）
 *
 * 模式先例照 DADialogInsertNewColumn（radio + 模式查询接口，调用方按模式建不同产物）。
 * 关键设计（R14）：
 * - **默认产出新数据集**（xxx_Transposed / xxx_Melted），原表与其元数据完全不动；
 *   「就地替换当前表」为勾选项，勾选后由页面弹确认框列出将被清除的元数据类别与
 *   条目数（确认框在页面侧，因为它需要元数据束）；
 * - melt 规模守卫：执行前按 行数 × 展开列数 预估输出行数，超过 c_meltRowLimit
 *   （5000 万）默认拒绝——OK 点击时弹确认给出预估行数，允许强制继续（§6.3-4，
 *   预估逻辑在调用侧）；
 * - 预览与执行同一算子（da_head_copy 样本 + da_transpose / da_melt，红线5 同款口径）。
 */
class DAGUI_API DADialogReshape : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogReshape(const DAPyDataFrame& df, bool meltMode, QWidget* parent = nullptr);
    ~DADialogReshape();

    // 模式与选项
    bool isMeltMode() const;
    bool isInplace() const;
    // melt 参数
    QStringList idVars() const;      ///< 保留列（可空）
    QStringList valueVars() const;   ///< 展开列（空 = 除保留列外全部，pandas 语义）
    QString varName() const;
    QString valueName() const;
    // melt 规模守卫：预估输出行数 = 原行数 × 展开列数（valueVars 空按全部非保留列计）
    qint64 estimatedOutputRows() const;
    // 守卫阈值（超过默认拒绝，可强制继续；计划 §6.3-4）
    static constexpr qint64 c_meltRowLimit = 50000000;
    // 预估纯函数（单测直接覆盖）：行数 × 展开列数
    static qint64 estimateMeltOutputRows(qint64 srcRows, int valueVarCount);

private Q_SLOTS:
    void onModeChanged();
    void onInputChanged();
    // OK 点击：melt 规模守卫确认点（超阈值 → 弹确认给预估行数，允许强制继续）
    void onOkClicked();

private:
    void setupUI();
    void runPreview();
    void setStatusError(const QString& msg);
    void setStatusOk(const QString& msg);
    // 有效展开列数（valueVars 空 = 全部列 - 保留列数）
    int effectiveValueVarCount() const;
    QStringList selectedNames(QListWidget* list) const;

    DAPyDataFrame mDf;
    QRadioButton* mRadioTranspose { nullptr };
    QRadioButton* mRadioMelt { nullptr };
    QWidget* mMeltPanel { nullptr };
    QListWidget* mListIdVars { nullptr };
    QListWidget* mListValueVars { nullptr };
    QLineEdit* mEditVarName { nullptr };
    QLineEdit* mEditValueName { nullptr };
    QCheckBox* mCheckInplace { nullptr };
    QTableWidget* mTablePreview { nullptr };
    QLabel* mLabelStatus { nullptr };
    QPushButton* mButtonOk { nullptr };
    QTimer* mDebounceTimer { nullptr };
    qint64 mSrcRows { 0 };
    static constexpr int c_previewRows = 10;   ///< 预览显示行数
    static constexpr int c_sampleRows = 50;    ///< 预览样本行数（转置样本必须限行，否则大表转置出百万列）
};
}  // end of namespace DA
#endif  // DADIALOGRESHAPE_H
