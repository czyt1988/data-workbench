#ifndef DADIALOGDATAFRAMESORT_H
#define DADIALOGDATAFRAMESORT_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"

namespace Ui
{
class DADialogDataFrameSort;
}

namespace DA
{
class DAGUI_API DADialogDataFrameSort : public QDialog
{
    Q_OBJECT

public:
    explicit DADialogDataFrameSort(QWidget* parent = nullptr);
    ~DADialogDataFrameSort();

    // 获取选中的dataframe
    void setDataframe(const DAPyDataFrame& df);

    //获取排序依据，列名
    QString getSortBy() const;
    // 获取排序方式，升序or降序
    bool getSortType() const;

private slots:
    void onAccepted();

private:
    Ui::DADialogDataFrameSort* ui;
};
}

#endif  // DADIALOGDATAFRAMESORT_H
