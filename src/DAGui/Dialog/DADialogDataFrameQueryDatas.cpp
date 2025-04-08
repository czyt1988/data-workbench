#include "DADialogDataFrameQueryDatas.h"
#include "ui_DADialogDataFrameQueryDatas.h"
#include <QLineEdit>

namespace DA
{
DADialogDataFrameQueryDatas::DADialogDataFrameQueryDatas(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataFrameQueryDatas)
{
	ui->setupUi(this);
    ui->textBrowser->setMarkdown(
        tr("Using the **Query Data** feature, you can filter data using expressions:"
           "\n"
           "\n1  **Comparison Operators**: Supports `==`, `>`, `<`, `>=`, `<=`, `!=` for direct comparison of column "
           "names and values.  "
           "\n   **Example**:  "
           "\n   `A > 2 & B < 8` filters rows where the value in column **A** is greater than **2** and the value in "
           "column **B** is less than **8**."
           "\n"
           "\n2  **Inter-Column Comparisons**: Directly compare values between columns.  "
           "\n   **Example**:  "
           "\n   `A > B` filters rows where the value in column **A** is greater than the value in column **B**."
           "\n"
           "\n3  **Logical Operators**: Supports `and`, `or`, `not`, `in`, and `not in` for simplified multi-condition "
           "filtering."
           "\n   **Examples**:  "
           "\n   - `A > 2 and B < 8` filters rows where **A** > 2 and **B** < 8.  "
           "\n   - `A in (\" S \", \" C \")` filters rows where **A** is either \" S \" or \" C \"."
           "\n"
           "\n4  **Arithmetic and Complex Logic**: Allows arithmetic operations and complex logical expressions.  "
           "\n   **Example**:  "
           "\n   `(A * 3 > 1) | ((B + 12.5) < 5)`."
           "\n"
           "\n5  **Range Filtering with `between`**: Use `between` to filter numeric ranges.  "
           "\n   **Example**:  "
           "\n   `A.between(2, 8)` filters values in column **A** between **2** and **8**."
           "\n"
           "\n6  **String Operations with `str` Methods**: Supports string column processing (e.g., length, prefix "
           "matching).  "
           "\n   **Example**:  "
           "\n   `Ticket.str.startswith(\" A \")` filters rows where the **Ticket** column starts with \" A \"."
           "\n"
           "\n**Note**:  "
           "\nIf a column name contains spaces or special characters, enclose it in backticks (`` ` ``), e.g., `` "
           "`Embarked On` ``."));  // cn:
    // 通过 **Query Data** 功能，你可以通过表达式进行数据筛选：
    //
    // 1 **支持比较运算符**：如 `==`、`>`、`<`、`>=`、`<=`、`!=`，可直接对列名和值进行比较。
    //    **例如**：
    //    `A > 2 & B < 8`，可以筛选出列名 **A** 数值大于 **2** 且列名 **B** 数值小于 **8** 的行。
    //
    // 2 **支持列间比较**：直接比较不同列的值。
    //    **例如**：
    //    `A > B`，筛选出列名 **A** 数值大于列名 **B** 的行。
    //
    // 3 **支持逻辑运算符**：如 `and`、`or`、`not`，以及 `in` 和 `not in` 简化多条件筛选。
    //    **例如**：
    //    - `A > 2 and B < 8`，筛选出列名 **A** 数值大于 **2** 且列名 **B** 数值小于 **8** 的行。
    //    - `A in ("S", "C")`，筛选出列名 **A** 数值为 **"S"** 或 **"C"** 的行。
    //
    // 4 **支持算术运算和复杂逻辑**：
    //    **例如**：
    //    `(A * 3 > 1) | ((B + 12.5) < 5)`。
    //
    // 5 **支持数值范围筛选**：结合 `between` 方法筛选数值范围。
    //    **例如**：
    //    `A.between(2, 8)`。
    //
    // 6 **支持字符串操作**：使用 `str` 方法处理字符串列（如长度、前缀匹配等）。
    //    **例如**：
    //    `Ticket.str.startswith("A")`，筛选 **Ticket** 列以 **A** 开头的内容。
    //
    // **注**：
    // 列名含空格或特殊字符时，需用反引号 `` ` `` 包裹，例如：`Embarked On`。
}

DADialogDataFrameQueryDatas::~DADialogDataFrameQueryDatas()
{
    delete ui;
}

QString DADialogDataFrameQueryDatas::getExpr() const
{
    return ui->textEdit->toPlainText();
}

}  // end DA
