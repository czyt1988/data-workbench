#include "DAExportToPngSettingDialog.h"
#include "ui_DAExportToPngSettingDialog.h"

DAExportToPngSettingDialog::DAExportToPngSettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DAExportToPngSettingDialog)
{
    ui->setupUi(this);
}

DAExportToPngSettingDialog::~DAExportToPngSettingDialog()
{
    delete ui;
}
