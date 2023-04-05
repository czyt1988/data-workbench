#include "DAPathLineEdit.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPathLineEdit
//===================================================
DAPathLineEdit::DAPathLineEdit(QWidget* p) : ctkPathLineEdit(p)
{
}

DAPathLineEdit::DAPathLineEdit(const QString& label, const QStringList& nameFilters, ctkPathLineEdit::Filters filters, QWidget* p)
    : ctkPathLineEdit(label, nameFilters, filters, p)
{
}
