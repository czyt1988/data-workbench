#include "DADialogDataFrameDataSearch.h"
#include "ui_DADialogDataFrameDataSearch.h"

namespace DA
{
DADialogDataFrameDataSearch::DADialogDataFrameDataSearch(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataFrameDataSearch)
{
	ui->setupUi(this);

    connect(ui->pushButtonNext, &QPushButton::clicked, this, &DADialogDataFrameDataSearch::on_pushButtonNext_clicked);
}

DADialogDataFrameDataSearch::~DADialogDataFrameDataSearch()
{
    delete ui;
}

QString DADialogDataFrameDataSearch::getFindItem() const
{
    return ui->lineEditFindItem->text();
}

void DADialogDataFrameDataSearch::setItemCoor(const QList< QPair< int, int > >& matches)
{
    mMatches = matches;
    mIndex   = 0;
}

QList< QPair< int, int > > DADialogDataFrameDataSearch::getItemCoor() const
{
    return mMatches;
}

void DADialogDataFrameDataSearch::setCoorIndex(const int idx)
{
    mIndex = idx;
}

int DADialogDataFrameDataSearch::getCoorIndex() const
{
    return mIndex;
}

void DADialogDataFrameDataSearch::on_pushButtonNext_clicked()
{
    if (mIndex < 0 || mIndex > mMatches.size())
        return;

    emit accept();
}

}  // end DA
