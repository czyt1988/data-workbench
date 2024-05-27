#ifndef DAEXPORTTOPNGSETTINGDIALOG_H
#define DAEXPORTTOPNGSETTINGDIALOG_H

#include <QDialog>

namespace Ui {
class DAExportToPngSettingDialog;
}

class DAExportToPngSettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DAExportToPngSettingDialog(QWidget *parent = nullptr);
    ~DAExportToPngSettingDialog();

private:
    Ui::DAExportToPngSettingDialog *ui;
};

#endif // DAEXPORTTOPNGSETTINGDIALOG_H
