#ifndef DATAFRAMECREATEPIVOTTABLEDIALOG_H
#define DATAFRAMECREATEPIVOTTABLEDIALOG_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"
#include "DADataManager.h"

namespace Ui
{
class DataFrameCreatePivotTableDialog;
}


/**
 * @brief PivotTable参数设置
 */
class DataFrameCreatePivotTableDialog : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DataFrameCreatePivotTableDialog)

public:
    explicit DataFrameCreatePivotTableDialog(QWidget* parent = nullptr);
    ~DataFrameCreatePivotTableDialog();

    // 获取选中的dataframe
    DA::DAPyDataFrame getDataFrame() const;
    void setDataframe(const DA::DAPyDataFrame& df);
    // value参数
    QStringList getPivotTableValue() const;
    // index参数
    QStringList getPivotTableIndex() const;
    // column参数
    QStringList getPivotTableColumn() const;
    // Aggfunc参数
    QString getPivotTableAggfunc() const;
    // Margins参数
    bool isEnableMarginsName() const;
    void setEnableMargins(bool on);

    QString getMarginsName() const;
    void setMarginsName(QString& s);
    // Sort参数
    bool isEnableSort() const;
    void setEnableSort(bool on);

private:
    // Aggfunc参数
    void initPivotTableAggfunc();

private slots:
    void onTableItemClicked(const QModelIndex& index);

private:
    Ui::DataFrameCreatePivotTableDialog* ui;
};

#endif  // DATAFRAMECREATEPIVOTTABLEDIALOG_H
