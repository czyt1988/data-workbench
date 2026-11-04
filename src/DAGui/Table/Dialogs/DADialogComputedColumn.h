#ifndef DADIALOGCOMPUTEDCOLUMN_H
#define DADIALOGCOMPUTEDCOLUMN_H
#include <QDialog>
#include <QStringList>
#include "DAGuiAPI.h"
#include "DAComputedColumn.h"
#include "pandas/DAPyDataFrame.h"

class QLineEdit;
class QTableWidget;
class QLabel;
class QPushButton;
class QToolButton;
class QTimer;

namespace DA
{
class DAPythonCodeEdit;

/**
 * @brief 计算列对话框（004 T1.23）：代码段编辑 + 实时预览
 *
 * 交互：
 * - 目标列名：创建模式必须是不存在的新列名（禁止自覆盖，R7）；编辑模式只读；
 * - 代码段：Python 代码块（最后一行表达式为结果，或赋值给 result）；
 *   左侧 2/3 为代码编辑区（语法高亮 + 自动补全），右侧 1/3 为头部预览；
 * - 列名快捷输入：工具栏「插入列名」菜单按钮，按光标位置插入；
 *   含空格/关键字/保留名的列名自动加反引号（R20）；
 * - 实时预览（300ms 去抖）：对头部前 N 行的独立副本调用与执行完全相同的算子
 *   （da_head_copy + da_eval_column），所见即所得；校验失败给出明确提示且不写数据；
 * - 校验链：目标列名非空 → 创建模式目标不存在 → 代码非空 → 代码可解析
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

    // 编辑模式：目标列名固定为既有计算列，代码段预填（目标字段只读）
    void setEditMode(const DAComputedColumn& def);
    bool isEditMode() const;

    // 接受后的结果
    QString targetColumnName() const;
    QString expressionBody() const;
    // 校验通过的依赖列名列表（da_expr_depends 与实际列的交集）
    QStringList dependencies() const;

private Q_SLOTS:
    // 代码/目标名变化 → 去抖触发校验与预览
    void onInputChanged();

private:
    void setupUI();
    // 校验 + 头部预览（同一算子）；更新状态标签与 OK 可用性
    void runValidationAndPreview();
    // 在代码编辑器光标处插入列名（自动反引号与必要空格）
    void insertColumnName(const QString& name);
    void setStatusError(const QString& msg);
    void setStatusOk(const QString& msg);

    DAPyDataFrame mDf;                             ///< 数据（只读引用，预览在 head 副本上做）
    QLineEdit* mEditTarget { nullptr };            ///< 目标列名
    DAPythonCodeEdit* mEditExpr { nullptr };       ///< Python 代码段编辑器
    QToolButton* mButtonInsertColumn { nullptr };  ///< 插入列名菜单按钮
    QTableWidget* mTablePreview { nullptr };       ///< 头部预览表
    QLabel* mLabelStatus { nullptr };              ///< 校验状态
    QPushButton* mButtonOk { nullptr };
    QTimer* mDebounceTimer { nullptr };            ///< 300ms 去抖
    bool mEditMode { false };
    QStringList mDependencies;                     ///< 最近一次校验通过的依赖列
    static constexpr int c_previewRows = 10;       ///< 预览行数
};
}  // namespace DA
#endif  // DADIALOGCOMPUTEDCOLUMN_H
