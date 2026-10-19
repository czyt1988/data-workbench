#ifndef DADIALOGCOMPUTEDCOLUMN_H
#define DADIALOGCOMPUTEDCOLUMN_H
#include <QDialog>
#include <QStringList>
#include "DAGuiAPI.h"
#include "DAComputedColumn.h"
#include "pandas/DAPyDataFrame.h"

class QLineEdit;
class QListWidget;
class QTableWidget;
class QLabel;
class QPushButton;
class QTimer;

namespace DA
{
/**
 * @brief 计算列对话框（004 T1.23）：表达式编辑 + 列名点选插入 + 头部数据实时预览
 *
 * 交互（计划 §3.3-10）：
 * - 目标列名：创建模式必须是不存在的新列名（禁止自覆盖，R7）；编辑模式只读；
 * - 表达式：赋值右侧表达式体（v1 = pandas df.eval 单表达式），左侧列名列表点选
 *   插入——含空格/特殊字符的列名自动加反引号（R20）；
 * - 实时预览（300ms 去抖）：对头部前 N 行的独立副本调用与执行完全相同的算子
 *   （da_head_copy + da_eval_column），所见即所得；校验失败给出明确提示且不写数据；
 * - 校验链：目标列名非空 → 创建模式目标不存在 → 表达式非空 → 表达式可解析
 *   （da_expr_depends ast 解析）→ 不自覆盖（依赖不含目标列）→ 预览求值成功。
 *
 * 本对话框只做取参与校验，不求值原表——接受后由页面 widget 执行求值并构建命令入栈。
 */
class DAGUI_API DADialogComputedColumn : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogComputedColumn(const DAPyDataFrame& df, QWidget* parent = nullptr);
    ~DADialogComputedColumn();

    // 编辑模式：目标列名固定为既有计算列，表达式预填（目标字段只读）
    void setEditMode(const DAComputedColumn& def);
    bool isEditMode() const;

    // 接受后的结果
    QString targetColumnName() const;
    QString expressionBody() const;
    // 校验通过的依赖列名列表（da_expr_depends 与实际列的交集）
    QStringList dependencies() const;

private Q_SLOTS:
    // 列名点选插入（自动反引号）
    void onColumnItemClicked(int row);
    // 表达式/目标名变化 → 去抖触发校验与预览
    void onInputChanged();

private:
    void setupUI();
    // 校验 + 头部预览（同一算子）；更新状态标签与 OK 可用性
    void runValidationAndPreview();
    void setStatusError(const QString& msg);
    void setStatusOk(const QString& msg);

    DAPyDataFrame mDf;                       ///< 数据（只读引用，预览在 head 副本上做）
    QLineEdit* mEditTarget { nullptr };      ///< 目标列名
    QLineEdit* mEditExpr { nullptr };        ///< 表达式体（RHS）
    QListWidget* mListColumns { nullptr };   ///< 列名列表（点选插入）
    QTableWidget* mTablePreview { nullptr }; ///< 头部预览表
    QLabel* mLabelStatus { nullptr };        ///< 校验状态
    QPushButton* mButtonOk { nullptr };
    QTimer* mDebounceTimer { nullptr };      ///< 300ms 去抖
    bool mEditMode { false };
    QStringList mDependencies;               ///< 最近一次校验通过的依赖列
    static constexpr int c_previewRows = 10;  ///< 预览行数
};
}  // end of namespace DA
#endif  // DADIALOGCOMPUTEDCOLUMN_H
