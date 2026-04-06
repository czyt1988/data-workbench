#include "DataFrameDataSelectDialog.h"
#include "ui_DataFrameDataSelectDialog.h"
#include <QDebug>
#include <QMessageBox>
namespace DA
{

DataFrameDataSelectDialog::DataFrameDataSelectDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataFrameDataSelectDialog)
{
    ui->setupUi(this);
}

DataFrameDataSelectDialog::~DataFrameDataSelectDialog()
{
    delete ui;
}

void DataFrameDataSelectDialog::setDataframe(const DA::DAPyDataFrame& df)
{
    QStringList para = df.columns();

    ui->comboBoxData->clear();
    for (int row = 0; row < para.size(); ++row) {
        ui->comboBoxData->addItem(para[ row ]);
    }
}

void DataFrameDataSelectDialog::setFilterData(const int index)
{
    ui->comboBoxData->setCurrentIndex(index);
}

QString DataFrameDataSelectDialog::getFilterData() const
{
    return ui->comboBoxData->currentText();
}

void DataFrameDataSelectDialog::setLowerValue(const double d)
{
    ui->LowerValue->setText(QString::number(d));
}

double DataFrameDataSelectDialog::getLowerValue() const
{
    bool isok = false;
    double v  = ui->LowerValue->text().toDouble(&isok);
    if (!isok) {
        qCritical() << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
        return 0.0;
    }
    return v;
}

void DataFrameDataSelectDialog::setUpperValue(const double d)
{
    ui->UpperValue->setText(QString::number(d));
}

double DataFrameDataSelectDialog::getUpperValue() const
{
    bool isok = false;
    double v  = ui->UpperValue->text().toDouble(&isok);
    if (!isok) {
        qCritical() << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
        return 0.0;
    }
    return v;
}

}  // end DA
