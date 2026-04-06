#ifndef DATAFRAMEQUERYDATASDIALOG_H
#define DATAFRAMEQUERYDATASDIALOG_H

#include <QDialog>
#include "DAGuiAPI.h"
namespace Ui
{
class DataFrameQueryDatasDialog;
}
namespace DA
{

/**
 * @brief querydatas参数设置
 */
class DataFrameQueryDatasDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataFrameQueryDatasDialog(QWidget* parent = nullptr);
    ~DataFrameQueryDatasDialog();
    // 获取输入的条件
    QString getExpr() const;

private:
    Ui::DADialogDataFrameQueryDatas* ui;
};
}

#endif  // DATAFRAMEQUERYDATASDIALOG_H
