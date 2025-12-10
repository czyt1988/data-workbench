#ifndef DATAFRAMEEXPORTRANGESELECTDIALOG_H
#define DATAFRAMEEXPORTRANGESELECTDIALOG_H

#include <QDialog>

namespace Ui
{
class DataFrameExportRangeSelectDialog;
}

/**
 * @brief 选择导出的数据范围
 */
class DataFrameExportRangeSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataFrameExportRangeSelectDialog(QWidget* parent = nullptr);
    ~DataFrameExportRangeSelectDialog();
    // 是否保存所有，或者只是保存选中
    bool isExportAll() const;

private:
    Ui::DataFrameExportRangeSelectDialog* ui;
};

#endif  // DATAFRAMEEXPORTRANGESELECTDIALOG_H
