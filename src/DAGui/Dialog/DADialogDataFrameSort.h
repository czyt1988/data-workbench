#ifndef DADIALOGDATAFRAMESORT_H
#define DADIALOGDATAFRAMESORT_H

#include <QDialog>
#include <QVariantMap>
#include "DAData.h"

namespace Ui
{
class DADialogDataFrameSort;
}

namespace DA
{
class DADialogDataFrameSort : public QDialog
{
    Q_OBJECT

public:
    explicit DADialogDataFrameSort(QWidget* parent = nullptr);
    ~DADialogDataFrameSort();

    // 获取排序方式
    bool getSortType() const;

private slots:
    void onAccepted();

private:
    Ui::DADialogDataFrameSort* ui;
};
}

#endif  // DADIALOGDATAFRAMESORT_H
