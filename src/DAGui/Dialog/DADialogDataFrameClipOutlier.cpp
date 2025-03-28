#include "DADialogDataFrameClipOutlier.h"
#include "ui_DADialogDataFrameClipOutlier.h"
#include <QDebug>
namespace DA
{

DADialogDataFrameClipOutlier::DADialogDataFrameClipOutlier(QWidget* parent)
	: QDialog(parent), ui(new Ui::DADialogDataFrameClipOutlier)
{
	ui->setupUi(this);
}

DADialogDataFrameClipOutlier::~DADialogDataFrameClipOutlier()
{
	delete ui;
}

void DADialogDataFrameClipOutlier::setLowerValue(double d)
{
	ui->LowerValue->setText(QString::number(d));
}

double DADialogDataFrameClipOutlier::getLowerValue() const
{
	bool isok = false;
	double v  = ui->LowerValue->text().toDouble(&isok);
	if (!isok) {
		qCritical() << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
		return 0.0;
	}
	return v;
}

void DADialogDataFrameClipOutlier::setUpperValue(double d)
{
	ui->UpperValue->setText(QString::number(d));
}

double DADialogDataFrameClipOutlier::getUpperValue() const
{
	bool isok = false;
	double v  = ui->UpperValue->text().toDouble(&isok);
	if (!isok) {
		qCritical() << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
		return 0.0;
	}
	return v;
}

}  // end DA
