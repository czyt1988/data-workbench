#include "DADialogDataFrameEvalDatas.h"
#include "ui_DADialogDataFrameEvalDatas.h"
#include <QLineEdit>

namespace DA
{
DADialogDataFrameEvalDatas::DADialogDataFrameEvalDatas(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataFrameEvalDatas)
{
	ui->setupUi(this);
    ui->textBrowser->setMarkdown(
        tr("# I. Basic Syntax"
           "\n"
           "\nYou can write expressions using the following elements:"
           "\n"
           "\n- **Column names**: Use column names directly in calculations (e.g., `age`, `salary`)"
           "\n- **Constants**: Numbers, strings, and boolean values (e.g., `10`, `\" male \"`, `True`)"
           "\n- **Operators**:"
           "\n  - Mathematical operations: `+`, `-`, `*`, `/`, `**` (power), `%` (modulus)"
           "\n  - Comparison operations: `==`, `!=`, `>`, `<`, `>=`, `<=`"
           "\n  - Logical operations: `and`, `or`, `not`"
           "\n- **Function calls** (partially supported):"
           "\n  - Common math functions: `abs()`, `sin()`, `cos()`, `log()`, `exp()`, etc."
           "\n  - Conditional logic: `where(condition, x, y)`"
           "\n  - String operations: `str.contains()`, `str.startswith()`, etc. (to be used with columns)"
           "\n"
           "\n| Goal | Example Expression |"
           "\n|------|--------------------|"
           "\n| Add a new column | `new_col = col1 + col2` |"
           "\n| Modify an existing column | `col = col * 2` |"
           "\n| Conditional assignment | `col = where(col > 10, 1, 0)` |"
           "\n| Filter rows (returns boolean) | `col1 > 5 and col2 < 10` |"
           "\n"
           "\n---"
           "\n"
           "\n## Example 1: Add or Modify a Column"
           "\n"
           "\n```python"
           "\nage + 10"
           "\n```"
           "\n"
           "\nThis adds 10 to each value in the `age` column and either updates the original column or writes to a new "
           "column."
           "\n"
           "\n---"
           "\n"
           "\n## Example 2: Create a New Column and Assign Values"
           "\n"
           "\n```python"
           "\nnew_column = salary * 1.1"
           "\n```"
           "\n"
           "\nThis creates a new column named `new_column`, whose values are 1.1 times those of the `salary` column."
           "\n"
           "\n---"
           "\n"
           "\n## Example 3: Conditional Filtering and Assignment"
           "\n"
           "\n```python"
           "\nbonus = where(age > 30, salary * 0.2, salary * 0.1)"
           "\n```"
           "\n"
           "\nThis means: if age is greater than 30, the bonus is 20% of the salary; otherwise, it's 10%."
           "\n"
           "\n---"
           "\n"
           "\n## Example 4: String Matching (for filtering)"
           "\n"
           "\n```python"
           "\nname.str.contains(\"John\")"
           "\n```"
           "\n"
           "\nThis can be used to filter rows where the name contains \" John \"."));  // cn:
    // # 一、基本语法
    //
    // 您可以使用以下内容编写表达式：
    //
    // - **列名**：直接使用数据表中的列名参与运算（如 `age`, `salary`）
    // - **常量**：数字、字符串、布尔值（如 `10`, `"male"`, `True`）
    // - **运算符**：
    //   - 数学运算：`+`, `-`, `*`, `/`, `**`（幂）、`%`（取余）
    //   - 比较运算：`==`, `!=`, `>`, `<`, `>=`, `<=`
    //   - 逻辑运算：`and`, `or`, `not`
    // - **函数调用**（部分支持）：
    //   - 常见数学函数：`abs()`, `sin()`, `cos()`, `log()`, `exp()` 等
    //   - 条件判断：`where(condition, x, y)`
    //   - 字符串操作：`str.contains()`, `str.startswith()` 等（需配合列使用）
    //
    // | 目标 | 示例表达式 |
    // |------|------------|
    // | 新增一列 | `new_col = col1 + col2` |
    // | 修改已有列 | `col = col * 2` |
    // | 条件赋值 | `col = where(col > 10, 1, 0)` |
    // | 过滤行（返回布尔值） | `col1 > 5 and col2 < 10` |
    //
    // ---
    //
    // ## 示例 1：新增一列或修改现有列的值
    //
    // ```python
    // age + 10
    // ```
    //
    // 表示将 `age` 列的每个值加 10，并更新回原列或写入新列。
    //
    // ---
    //
    // ## 示例 2：创建新列并赋值
    //
    // ```python
    // new_column = salary * 1.1
    // ```
    //
    // 表示新建一列 `new_column`，其值为 `salary` 列的 1.1 倍。
    //
    // ---
    //
    // ## 示例 3：条件筛选与赋值
    //
    // ```python
    // bonus = where(age > 30, salary * 0.2, salary * 0.1)
    // ```
    //
    // 表示如果年龄大于 30，则奖金为工资的 20%，否则为 10%。
    //
    // ---
    //
    // ## 示例 4：字符串匹配（用于过滤）
    //
    // ```python
    // name.str.contains("John")
    // ```
    //
    // 可用于筛选名字中包含 "John" 的行。
}

DADialogDataFrameEvalDatas::~DADialogDataFrameEvalDatas()
{
    delete ui;
}

QString DADialogDataFrameEvalDatas::getExpr() const
{
    return ui->textEdit->toPlainText();
}

}  // end DA
