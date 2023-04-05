#include "DADialogDataframeColumnCastToDatetime.h"
#include "ui_DADialogDataframeColumnCastToDatetime.h"
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DADialogDataframeColumnCastToDatetime
//===================================================
DADialogDataframeColumnCastToDatetime::DADialogDataframeColumnCastToDatetime(QWidget* parent)
    : DADialogPythonArgs(parent), ui(new Ui::DADialogDataframeColumnCastToDatetime)
{
    ui->setupUi(this);
}

DADialogDataframeColumnCastToDatetime::~DADialogDataframeColumnCastToDatetime()
{
    delete ui;
}

pybind11::dict DADialogDataframeColumnCastToDatetime::getArgs() const
{
    pybind11::dict dict;
    dict[ "errors" ]                = pybind11::str(getArgErrorsValue().toStdString());
    dict[ "infer_datetime_format" ] = pybind11::bool_(ui->checkBoxInferDatetimeFormat->isChecked());
    dict[ "exact" ]                 = pybind11::bool_(ui->checkBoxExact->isChecked());
    QString format                  = ui->lineEditFormat->text();
    if (format.isEmpty()) {
        dict[ "format" ] = pybind11::none();
    } else {
        dict[ "format" ] = pybind11::str(format.toStdString());
    }
    dict[ "unit" ]      = pybind11::str(getArgUnitValue().toStdString());
    dict[ "origin" ]    = pybind11::str(getArgOriginValue().toStdString());
    dict[ "dayfirst" ]  = pybind11::bool_(ui->checkBoxDayfirst->isChecked());
    dict[ "yearfirst" ] = pybind11::bool_(ui->checkBoxYearfirst->isChecked());
    if (ui->groupBoxUTC->isChecked()) {
        dict[ "utc" ] = pybind11::bool_(ui->checkBoxUtc->isChecked());
    } else {
        dict[ "utc" ] = pybind11::none();
    }
    dict[ "cache" ] = pybind11::bool_(ui->checkBoxCache->isChecked());
    return dict;
}

QString DADialogDataframeColumnCastToDatetime::getArgErrorsValue() const
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

QString DADialogDataframeColumnCastToDatetime::getArgUnitValue() const
{
    if (ui->radioButtonMs->isChecked()) {
        return "ms";
    } else if (ui->radioButtonS->isChecked()) {
        return "s";
    } else if (ui->radioButtonNs->isChecked()) {
        return "ns";
    } else if (ui->radioButtonUs->isChecked()) {
        return "us";
    } else if (ui->radioButtonD->isChecked()) {
        return "D";
    }
    return "ms";
}

QString DADialogDataframeColumnCastToDatetime::getArgOriginValue() const
{
    if (ui->radioButtonUnix->isChecked()) {
        return "unix";
    } else if (ui->radioButtonJulian->isChecked()) {
        return "julian";
    }
    return "unix";
}

void DADialogDataframeColumnCastToDatetime::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DADialogDataframeColumnCastToDatetime::on_toolButtonHowEditFormat_clicked()
{
}

void DADialogDataframeColumnCastToDatetime::on_radioButtonUnix_clicked(bool checked)
{
    if (checked) {
        if (ui->radioButtonD->isChecked()) {
            ui->radioButtonMs->setChecked(true);
        }
    }
    ui->radioButtonD->setEnabled(!checked);
    ui->radioButtonS->setEnabled(checked);
    ui->radioButtonMs->setEnabled(checked);
    ui->radioButtonNs->setEnabled(checked);
    ui->radioButtonUs->setEnabled(checked);
}

void DADialogDataframeColumnCastToDatetime::on_radioButtonJulian_clicked(bool checked)
{
    ui->radioButtonD->setChecked(checked);
    ui->radioButtonD->setEnabled(checked);
    ui->radioButtonS->setEnabled(!checked);
    ui->radioButtonMs->setEnabled(!checked);
    ui->radioButtonNs->setEnabled(!checked);
    ui->radioButtonUs->setEnabled(!checked);
}
