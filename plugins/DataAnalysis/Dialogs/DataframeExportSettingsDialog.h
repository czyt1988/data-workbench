#ifndef DATAFRAMEEXPORTSETTINGSDIALOG_H
#define DATAFRAMEEXPORTSETTINGSDIALOG_H

#include <QDialog>

namespace Ui
{
class DataframeExportSettingsDialog;
}

class DataframeExportSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataframeExportSettingsDialog(QWidget* parent = nullptr);
    ~DataframeExportSettingsDialog();
    // 获取选中的后缀
    QString getSelectSuffix() const;
    // 保存的目录
    QString getSavePath() const;
    // 是否保存所有，或者只是保存选中
    bool isExportAll() const;
private Q_SLOTS:
    void onBrowser();
    //
    void onAccept();

private:
    Ui::DataframeExportSettingsDialog* ui;
};

#endif  // DATAFRAMEEXPORTSETTINGSDIALOG_H
