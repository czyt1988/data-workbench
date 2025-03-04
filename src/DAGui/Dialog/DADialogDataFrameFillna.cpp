#include "DADialogDataFrameFillna.h"
#include "ui_DADialogDataFrameFillna.h"
#include <QButtonGroup>

DADialogDataFrameFillna::DADialogDataFrameFillna(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogDataFrameFillna)
{
	ui->setupUi(this);
	this->initCheckBoxGroup();
}

DADialogDataFrameFillna::~DADialogDataFrameFillna()
{
	delete ui;
}

void DADialogDataFrameFillna::initCheckBoxGroup()
{
	QButtonGroup* pButtonGroup = new QButtonGroup(this);
	pButtonGroup->addButton(ui->valueFilledCheckBox, 1);
	pButtonGroup->addButton(ui->padCheckBox, 2);
	pButtonGroup->addButton(ui->ffillCheckBox, 3);
	pButtonGroup->addButton(ui->backfillCheckBox, 4);
	pButtonGroup->addButton(ui->bfillCheckBox, 5);
}

int DADialogDataFrameFillna::getCheckBoxStatus()
{
	if (ui->valueFilledCheckBox->isChecked())
		return 1;
	if (ui->padCheckBox->isChecked())
		return 2;
	if (ui->ffillCheckBox->isChecked())
		return 3;
	if (ui->backfillCheckBox->isChecked())
		return 4;
	if (ui->bfillCheckBox->isChecked())
		return 5;
	return -1;
}

float DADialogDataFrameFillna::getFilledValue()
{
	return ui->valueSpinBox->value();
}

QString DADialogDataFrameFillna::getFillMethod(int filltype)
{
	QString method = "";
	if (filltype == 2)
		method = ui->padCheckBox->text();
	if (filltype == 3)
		method = ui->ffillCheckBox->text();
	if (filltype == 4)
		method = ui->backfillCheckBox->text();
	if (filltype == 5)
		method = ui->bfillCheckBox->text();
	return method;
}
