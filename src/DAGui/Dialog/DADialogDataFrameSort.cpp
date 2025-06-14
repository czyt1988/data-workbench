#include "DADialogDataFrameSort.h"
#include "ui_DADialogDataFrameSort.h"

namespace DA
{

DADialogDataFrameSort::DADialogDataFrameSort(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogDataFrameSort)
{
    ui->setupUi(this);

    ui->comboBoxSortType->addItem(tr("Ascending"), QStringLiteral("Ascending"));    // cn:升序
    ui->comboBoxSortType->addItem(tr("Descending"), QStringLiteral("Descending"));  // cn:降序
}

DADialogDataFrameSort::~DADialogDataFrameSort()
{
    delete ui;
}

void DADialogDataFrameSort::setDataframe(const DAPyDataFrame& df)
{
    QStringList para = df.columns();

    ui->comboBoxColumns->clear();
    for (int row = 0; row < para.size(); ++row) {
        ui->comboBoxColumns->addItem(para[ row ]);
    }
}

void DADialogDataFrameSort::setSortBy(const int index)
{
    ui->comboBoxColumns->setCurrentIndex(index);
}

QString DADialogDataFrameSort::getSortBy() const
{
    return ui->comboBoxColumns->currentText();
}

bool DADialogDataFrameSort::getSortType() const
{
    QString type = ui->comboBoxSortType->itemData(ui->comboBoxSortType->currentIndex()).toString();
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
