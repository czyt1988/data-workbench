#include "DADialogDataframeColumnCastToNumeric.h"
#include "ui_DADialogDataframeColumnCastToNumeric.h"
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DADialogDataframeColumnCastToNumeric
//===================================================
DADialogDataframeColumnCastToNumeric::DADialogDataframeColumnCastToNumeric(QWidget* parent)
    : DADialogPythonArgs(parent), ui(new Ui::DADialogDataframeColumnCastToNumeric)
{
    ui->setupUi(this);
}

DADialogDataframeColumnCastToNumeric::~DADialogDataframeColumnCastToNumeric()
{
    delete ui;
}

pybind11::dict DADialogDataframeColumnCastToNumeric::getArgs() const
{
    pybind11::dict dict;
    dict[ "errors" ] = pybind11::str(getArgErrorsValue().toStdString());
    QString dc       = getArgDowncastValue();
    if (dc.isEmpty()) {
        dict[ "downcast" ] = pybind11::none();
    } else {
        dict[ "downcast" ] = pybind11::str(dc.toStdString());
    }
    return dict;
}

QString DADialogDataframeColumnCastToNumeric::getArgErrorsValue() const
{
    if (ui->radioButtonCoerce->isChecked()) {
        return "coerce";
    } else if (ui->radioButtonIgnore->isChecked()) {
        return "ignore";
    } else if (ui->radioButtonRaise->isChecked()) {
        return "raise";
    }
    return "coerce";
}

QString DADialogDataframeColumnCastToNumeric::getArgDowncastValue() const
{
    if (ui->radioButtonFloat->isChecked()) {
        return "float";
    } else if (ui->radioButtonInteger->isChecked()) {
        return "integer";
    } else if (ui->radioButtonSigned->isChecked()) {
        return "signed";
    } else if (ui->radioButtonUnsigned->isChecked()) {
        return "unsigned";
    }
    return QString();
}
