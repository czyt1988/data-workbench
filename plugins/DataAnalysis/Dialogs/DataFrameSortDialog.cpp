#include "DataFrameSortDialog.h"
#include "ui_DataFrameSortDialog.h"


DataFrameSortDialog::DataFrameSortDialog(QWidget* parent) : QDialog(parent), ui(new Ui::DataFrameSortDialog)
{
    ui->setupUi(this);

    ui->comboBoxSortType->addItem(tr("Ascending"), QStringLiteral("Ascending"));    // cn:升序
    ui->comboBoxSortType->addItem(tr("Descending"), QStringLiteral("Descending"));  // cn:降序
}

DataFrameSortDialog::~DataFrameSortDialog()
{
    delete ui;
}

void DataFrameSortDialog::setDataframe(const DA::DAPyDataFrame& df)
{
    QStringList para = df.columns();

    ui->comboBoxColumns->clear();
    for (int row = 0; row < para.size(); ++row) {
        ui->comboBoxColumns->addItem(para[ row ]);
    }
}

void DataFrameSortDialog::setSortBy(const int index)
{
    ui->comboBoxColumns->setCurrentIndex(index);
}

QString DataFrameSortDialog::getSortBy() const
{
    return ui->comboBoxColumns->currentText();
}

bool DataFrameSortDialog::getSortType() const
{
    QString type = ui->comboBoxSortType->itemData(ui->comboBoxSortType->currentIndex()).toString();
    if (type == "Ascending")
        return true;
    else
        return false;
}

void DataFrameSortDialog::onAccepted()
{
    accept();
}
