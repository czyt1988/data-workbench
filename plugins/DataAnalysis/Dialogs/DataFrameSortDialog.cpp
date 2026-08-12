#include "DataFrameSortDialog.h"
#include "ui_DataFrameSortDialog.h"


/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DataFrameSortDialog::DataFrameSortDialog(QWidget* parent) : QDialog(parent), ui(new Ui::DataFrameSortDialog)
{
    ui->setupUi(this);

    ui->comboBoxSortType->addItem(tr("Ascending"), QStringLiteral("Ascending"));    // cn:升序
    ui->comboBoxSortType->addItem(tr("Descending"), QStringLiteral("Descending"));  // cn:降序
}

/**
 * @brief 析构函数
 */
DataFrameSortDialog::~DataFrameSortDialog()
{
    delete ui;
}

/**
 * @brief 设置数据帧
 * @param df 数据帧
 */
void DataFrameSortDialog::setDataframe(const DA::DAPyDataFrame& df)
{
    QStringList para = df.columns();

    ui->comboBoxColumns->clear();
    for (int row = 0; row < para.size(); ++row) {
        ui->comboBoxColumns->addItem(para[ row ]);
    }
}

/**
 * @brief 设置排序列索引
 * @param index 列索引
 */
void DataFrameSortDialog::setSortBy(const int index)
{
    ui->comboBoxColumns->setCurrentIndex(index);
}

/**
 * @brief 获取排序列名
 * @return 列名
 */
QString DataFrameSortDialog::getSortBy() const
{
    return ui->comboBoxColumns->currentText();
}

/**
 * @brief 获取排序方式
 * @return true表示升序，false表示降序
 */
bool DataFrameSortDialog::getSortType() const
{
    QString type = ui->comboBoxSortType->itemData(ui->comboBoxSortType->currentIndex()).toString();
    if (type == "Ascending")
        return true;
    else
        return false;
}

/**
 * @brief 确认按钮的槽函数
 */
void DataFrameSortDialog::onAccepted()
{
    accept();
}
