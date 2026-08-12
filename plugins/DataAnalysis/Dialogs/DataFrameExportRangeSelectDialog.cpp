#include "DataFrameExportRangeSelectDialog.h"
#include "ui_DataFrameExportRangeSelectDialog.h"

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DataFrameExportRangeSelectDialog::DataFrameExportRangeSelectDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataFrameExportRangeSelectDialog)
{
    ui->setupUi(this);
}

/**
 * @brief 析构函数
 */
DataFrameExportRangeSelectDialog::~DataFrameExportRangeSelectDialog()
{
    delete ui;
}

/**
 * @brief 获取是否导出全部数据
 * @return true表示导出全部，false表示导出选中数据
 */
bool DataFrameExportRangeSelectDialog::isExportAll() const
{
    return ui->radioButtonExportAll->isChecked();
}
