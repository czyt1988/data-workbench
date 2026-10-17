#ifndef DADIALOGSPLITCOLUMN_H
#define DADIALOGSPLITCOLUMN_H
#include <QDialog>
#include <QStringList>
#include "DAGuiAPI.h"
#include "Commands/DACommandsDataFrame.h"
#include "pandas/DAPyDataFrame.h"

class QLineEdit;
class QRadioButton;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QTableWidget;
class QLabel;
class QPushButton;
class QTimer;

namespace DA
{
/**
 * @brief 文本分列对话框（004 T1.9）：三种分隔模式 + 溢出策略 + 类型推断 + 实时预览
 *
 * 交互（计划 §4.3）：
 * - 分隔模式：单字符分隔符 / 正则（捕获组）/ 固定宽度（逗号分隔的宽度列表）；
 * - 溢出策略：结果列数超上限时 丢弃超出 / 保留原文（剩余并入最后一列）/ 报错；
 * - 类型推断开关：结果列尝试数值解析（整列非空值全可解析才转）；
 * - 结果列名自动推断（"{源列名}_{i}"）且可改（逗号分隔，个数须与结果列数一致、
 *   非空、互不重复、不与现有列重名——保证按名撤销安全）；
 * - 实时预览（300ms 去抖）：对头部前 N 行副本调用与执行完全相同的算子
 *   （da_head_copy + da_split_column，红线5：预览与执行同一算子），所见即所得。
 *
 * 本对话框只做取参/校验/预览，不改原表——接受后由页面 widget 构建命令入栈。
 */
class DAGUI_API DADialogSplitColumn : public QDialog
{
    Q_OBJECT
public:
    DADialogSplitColumn(const DAPyDataFrame& df, int sourceCol, QWidget* parent = nullptr);
    ~DADialogSplitColumn();

    // 接受后的分列参数（与命令共用同一结构）
    DACommandDataFrame_splitColumn::SplitParams params() const;

private Q_SLOTS:
    void onModeChanged();
    void onInputChanged();
    void onNamesEdited();

private:
    void setupUI();
    // 校验 + 头部预览（同一算子）；更新状态标签/预览表/OK 可用性
    void runPreview();
    void setStatusError(const QString& msg);
    void setStatusOk(const QString& msg);
    // 当前模式的参数有效性（不含结果列名校验）
    bool collectModeParams(DACommandDataFrame_splitColumn::SplitParams& p, QString* err) const;

    DAPyDataFrame mDf;
    int mSourceCol { -1 };
    QLineEdit* mEditDelimiter { nullptr };
    QLineEdit* mEditPattern { nullptr };
    QLineEdit* mEditWidths { nullptr };
    QRadioButton* mRadioChar { nullptr };
    QRadioButton* mRadioRegex { nullptr };
    QRadioButton* mRadioWidth { nullptr };
    QSpinBox* mSpinMaxColumns { nullptr };
    QComboBox* mComboOverflow { nullptr };
    QCheckBox* mCheckInferNumeric { nullptr };
    QLineEdit* mEditNames { nullptr };   ///< 结果列名（逗号分隔）
    QTableWidget* mTablePreview { nullptr };
    QLabel* mLabelStatus { nullptr };
    QLabel* mLabelSource { nullptr };
    QPushButton* mButtonOk { nullptr };
    QTimer* mDebounceTimer { nullptr };
    bool mNamesEdited { false };         ///< 用户改过结果列名（不再自动覆盖）
    int mPreviewColumnCount { 0 };       ///< 最近一次预览的结果列数
    static constexpr int c_previewRows = 10;  ///< 预览行数
};
}  // end of namespace DA
#endif  // DADIALOGSPLITCOLUMN_H
