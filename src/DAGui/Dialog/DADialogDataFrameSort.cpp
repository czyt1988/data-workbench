#include "DADialogDataFrameSort.h"
#include "ui_DADialogDataFrameSort.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "Models/DAPySeriesTableModule.h"

namespace DA
{

DADialogDataFrameSort::DADialogDataFrameSort(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogDataFrameSort)
{
    ui->setupUi(this);
}

DADialogDataFrameSort::~DADialogDataFrameSort()
{
    delete ui;
}

bool DADialogDataFrameSort::getSortType() const
{
    QString type = ui->comboBoxSortType->currentText();
    if (type == "Ascending")
        return true;
    else
        return false;
}

void DADialogDataFrameSort::onAccepted()
{
    accept();
}
}
