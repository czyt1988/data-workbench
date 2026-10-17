#ifndef DADIALOGMERGECOLUMNS_H
#define DADIALOGMERGECOLUMNS_H
#include <QDialog>
#include <QList>
#include "DAGuiAPI.h"
#include "Commands/DACommandsDataFrame.h"
#include "pandas/DAPyDataFrame.h"

class QListWidget;
class QLineEdit;
class QCheckBox;
class QTableWidget;
class QLabel;
class QPushButton;
class QTimer;

namespace DA
{
/**
 * @brief 列合并对话框（004 T1.9）：顺序、分隔符、跳过空值开关、结果列名 + 实时预览
 *
 * 交互（计划 §4.3）：左侧源列顺序列表（上移/下移调整拼接顺序），分隔符（可空），
 * 跳过空值开关（跳过时不留多余分隔符），结果列名（默认各源列名以 "_" 连接）。
 * 实时预览（300ms 去抖）对头部前 N 行副本调用与执行完全相同的算子
 * （da_head_copy + da_merge_columns，红线5），所见即所得。
 *
 * 结果列名校验：非空、不与任何现有列重名（含源列——源列虽被删除，但名字复用
 * 会让按名撤销产生歧义，v1 一并禁止）。接受后由页面 widget 构建命令入栈。
 */
class DAGUI_API DADialogMergeColumns : public QDialog
{
    Q_OBJECT
public:
    DADialogMergeColumns(const DAPyDataFrame& df, const QList< int >& sourceCols, QWidget* parent = nullptr);
    ~DADialogMergeColumns();

    // 接受后的合并参数（cols 顺序 = 拼接顺序）
    DACommandDataFrame_mergeColumns::MergeParams params() const;

private Q_SLOTS:
    void onMoveUp();
    void onMoveDown();
    void onInputChanged();

private:
    void setupUI();
    void runPreview();
    void setStatusError(const QString& msg);
    void setStatusOk(const QString& msg);
    // 当前顺序的源列位置列表
    QList< int > orderedCols() const;

    DAPyDataFrame mDf;
    QList< int > mSourceCols;              ///< 源列位置（初始=选区升序）
    QListWidget* mListColumns { nullptr }; ///< 顺序列表（item data 存列位置）
    QLineEdit* mEditSep { nullptr };
    QCheckBox* mCheckSkipNa { nullptr };
    QLineEdit* mEditName { nullptr };
    QTableWidget* mTablePreview { nullptr };
    QLabel* mLabelStatus { nullptr };
    QPushButton* mButtonOk { nullptr };
    QTimer* mDebounceTimer { nullptr };
    bool mNameEdited { false };  ///< 用户改过结果列名（不再自动覆盖）
    static constexpr int c_previewRows = 10;  ///< 预览行数
};
}  // end of namespace DA
#endif  // DADIALOGMERGECOLUMNS_H
