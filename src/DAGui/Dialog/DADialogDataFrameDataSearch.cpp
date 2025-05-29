#include "DADialogDataFrameDataSearch.h"
#include "ui_DADialogDataFrameDataSearch.h"

namespace DA
{
DADialogDataFrameDataSearch::DADialogDataFrameDataSearch(QWidget* parent)
	: QDialog(parent), ui(new Ui::DADialogDataFrameDataSearch)
{
	ui->setupUi(this);
}

DADialogDataFrameDataSearch::~DADialogDataFrameDataSearch()
{
	delete ui;
}

QString DADialogDataFrameDataSearch::getExpr() const
{
	return ui->textEdit->toPlainText();
}
}  // end DA
