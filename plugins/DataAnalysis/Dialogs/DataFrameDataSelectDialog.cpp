#include "DataFrameDataSelectDialog.h"
#include "ui_DataFrameDataSelectDialog.h"
#include <QDebug>
#include <QMessageBox>
#include "DALogCategory.h"

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DataFrameDataSelectDialog::DataFrameDataSelectDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataFrameDataSelectDialog)
{
    ui->setupUi(this);
}

/**
 * @brief 析构函数
 */
DataFrameDataSelectDialog::~DataFrameDataSelectDialog()
{
    delete ui;
}

/**
 * @brief 设置数据帧
 * @param df 数据帧
 */
void DataFrameDataSelectDialog::setDataframe(const DA::DAPyDataFrame& df)
{
    QStringList para = df.columns();

    ui->comboBoxData->clear();
    for (int row = 0; row < para.size(); ++row) {
        ui->comboBoxData->addItem(para[ row ]);
    }
}

/**
 * @brief 设置过滤列索引
 * @param index 列索引
 */
void DataFrameDataSelectDialog::setFilterData(const int index)
{
    ui->comboBoxData->setCurrentIndex(index);
}

/**
 * @brief 获取过滤列名
 * @return 列名
 */
QString DataFrameDataSelectDialog::getFilterData() const
{
    return ui->comboBoxData->currentText();
}

/**
 * @brief 设置下限值
 * @param d 下限值
 */
void DataFrameDataSelectDialog::setLowerValue(const double d)
{
    ui->LowerValue->setText(QString::number(d));
}

/**
 * @brief 获取下限值
 * @return 下限值
 */
double DataFrameDataSelectDialog::getLowerValue() const
{
    bool isok = false;
    double v  = ui->LowerValue->text().toDouble(&isok);
    if (!isok) {
        daCritical << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
        return 0.0;
    }
    return v;
}

/**
 * @brief 设置上限值
 * @param d 上限值
 */
void DataFrameDataSelectDialog::setUpperValue(const double d)
{
    ui->UpperValue->setText(QString::number(d));
}

/**
 * @brief 获取上限值
 * @return 上限值
 */
double DataFrameDataSelectDialog::getUpperValue() const
{
    bool isok = false;
    double v  = ui->UpperValue->text().toDouble(&isok);
    if (!isok) {
        daCritical << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
        return 0.0;
    }
    return v;
}
