#ifndef DATAFRAMEEVALDATASDIALOG_H
#define DATAFRAMEEVALDATASDIALOG_H

#include <QDialog>
#include "DAGuiAPI.h"
namespace Ui
{
class DataFrameEvalDatasDialog;
}


/**
 * @brief evaldatas参数设置
 */
class DataFrameEvalDatasDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataFrameEvalDatasDialog(QWidget* parent = nullptr);
    ~DataFrameEvalDatasDialog();
    // 获取输入的条件
    QString getExpr() const;

private:
    Ui::DataFrameEvalDatasDialog* ui;
};


#endif  // DATAFRAMEEVALDATASDIALOG_H
