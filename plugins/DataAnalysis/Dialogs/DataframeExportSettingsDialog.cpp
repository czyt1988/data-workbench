#include "DataframeExportSettingsDialog.h"
#include "ui_DataframeExportSettingsDialog.h"
#include <QFileDialog>
DataframeExportSettingsDialog::DataframeExportSettingsDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataframeExportSettingsDialog)
{
    ui->setupUi(this);
    connect(ui->toolButtonBrowser, &QToolButton::clicked, this, &DataframeExportSettingsDialog::onBrowser);
}

DataframeExportSettingsDialog::~DataframeExportSettingsDialog()
{
    delete ui;
}

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

QString DataframeExportSettingsDialog::getSavePath() const
{
    return ui->lineEditFolderPath->text();
}

bool DataframeExportSettingsDialog::isExportAll() const
{
    return ui->radioButtonExportAll->isChecked();
}

void DataframeExportSettingsDialog::onBrowser()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,                 // 父窗口
                                                           tr("Select Folder"),  // cn:选择文件夹
                                                           ui->lineEditFolderPath->text(),  // 初始目录（若输入框已有内容）
                                                           QFileDialog::ShowDirsOnly  // 仅显示目录
    );
    if (folderPath.isEmpty()) {
        return;
    }
    ui->lineEditFolderPath->setText(folderPath);
}
