#ifndef DATAFRAMESORTDIALOG_H
#define DATAFRAMESORTDIALOG_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"

namespace Ui
{
class DataFrameSortDialog;
}


class DataFrameSortDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataFrameSortDialog(QWidget* parent = nullptr);
    ~DataFrameSortDialog();

    // 获取选中的dataframe
    void setDataframe(const DA::DAPyDataFrame& df);

    // 获取排序依据，列名
    void setSortBy(const int index);
    QString getSortBy() const;
    // 获取排序方式，升序or降序
    bool getSortType() const;

private slots:
    void onAccepted();

private:
    Ui::DataFrameSortDialog* ui;
};


#endif  // DataFrameSortDialog_H
