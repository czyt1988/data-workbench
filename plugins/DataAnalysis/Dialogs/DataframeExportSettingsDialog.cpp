#include "DataframeExportSettingsDialog.h"
#include "ui_DataframeExportSettingsDialog.h"
#include <QMessageBox>
#include "DAFilePathEditWidget.h"

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DataframeExportSettingsDialog::DataframeExportSettingsDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataframeExportSettingsDialog)
{
    ui->setupUi(this);
    ui->filePathEditFolder->setMode(DA::DAFilePathEditWidget::Mode::ExistingDirectory);
    ui->filePathEditFolder->setPlaceholderText(tr("Select Folder"));  // cn:选择文件夹
    connect(this, &QDialog::accepted, this, &DataframeExportSettingsDialog::onAccept);
}

/**
 * @brief 析构函数
 */
DataframeExportSettingsDialog::~DataframeExportSettingsDialog()
{
    delete ui;
}

/**
 * @brief 获取导出文件后缀
 * @return 文件后缀名
 */
QString DataframeExportSettingsDialog::getSelectSuffix() const
{
    if (ui->radioButtonCsv->isChecked()) {
        return "csv";
    } else if (ui->radioButtonXlsx->isChecked()) {
        return "xlsx";
    } else if (ui->radioButtonParquet->isChecked()) {
        return "parquet";
    } else if (ui->radioButtonPickle->isChecked()) {
        return "pickle";
    } else if (ui->radioButtonFeather->isChecked()) {
        return "feather";
    } else if (ui->radioButtonJson->isChecked()) {
        return "json";
    } else if (ui->radioButtonHtml->isChecked()) {
        return "html";
    }
    return "csv";
}

/**
 * @brief 获取保存路径
 * @return 保存路径
 */
QString DataframeExportSettingsDialog::getSavePath() const
{
    return ui->filePathEditFolder->getFilePath();
}

/**
 * @brief 获取是否导出全部数据
 * @return true表示导出全部，false表示导出选中数据
 */
bool DataframeExportSettingsDialog::isExportAll() const
{
    return ui->radioButtonExportAll->isChecked();
}

/**
 * @brief 确认按钮的槽函数
 */
void DataframeExportSettingsDialog::onAccept()
{
    QString savepath = getSavePath();
    if (savepath.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Warning"),                                     // cn:警告
                             tr("Please select the folder for exporting data")  // cn:请选择需要导出数据的文件夹
        );
        return;
    }
    QDialog::accept();
}
