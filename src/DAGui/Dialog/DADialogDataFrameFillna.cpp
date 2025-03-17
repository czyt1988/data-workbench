#include "DADialogDataFrameFillna.h"
#include "ui_DADialogDataFrameFillna.h"
#include <QDebug>
namespace DA
{

DADialogDataFrameFillna::DADialogDataFrameFillna(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogDataFrameFillna)
{
	ui->setupUi(this);
}

DADialogDataFrameFillna::~DADialogDataFrameFillna()
{
	delete ui;
}

void DADialogDataFrameFillna::setFillNanValue(double d)
{
	ui->lineEditValue->setText(QString::number(d));
}

double DADialogDataFrameFillna::getFillNanValue() const
{
	bool isok = false;
	double v  = ui->lineEditValue->text().toDouble(&isok);
	if (!isok) {
		qCritical() << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
		return 0.0;
	}
	return v;
}

bool DADialogDataFrameFillna::isEnableLimitCount() const
{
	return ui->checkBoxLimit->isChecked();
}

void DADialogDataFrameFillna::setEnableLimit(bool on)
{
	ui->checkBoxLimit->setChecked(on);
}

int DADialogDataFrameFillna::getLimitCount() const
{
	return ui->spinBoxLimit->value();
}

void DADialogDataFrameFillna::setLimitCount(int d)
{
	ui->spinBoxLimit->setValue(d);
}

}  // end DA
