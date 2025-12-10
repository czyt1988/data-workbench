#include "DataFrameExportRangeSelectDialog.h"
#include "ui_DataFrameExportRangeSelectDialog.h"

DataFrameExportRangeSelectDialog::DataFrameExportRangeSelectDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataFrameExportRangeSelectDialog)
{
    ui->setupUi(this);
}

DataFrameExportRangeSelectDialog::~DataFrameExportRangeSelectDialog()
{
    delete ui;
}

bool DataFrameExportRangeSelectDialog::isExportAll() const
{
    return ui->radioButtonExportAll->isChecked();
}
