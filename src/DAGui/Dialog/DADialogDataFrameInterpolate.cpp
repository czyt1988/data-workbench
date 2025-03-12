#include "DADialogDataFrameInterpolate.h"
#include "ui_DADialogDataFrameInterpolate.h"
#include <QDebug>

namespace DA
{
DADialogDataFrameInterpolate::DADialogDataFrameInterpolate(QWidget* parent)
	: QDialog(parent), ui(new Ui::DADialogDataFrameInterpolate)
{
	ui->setupUi(this);
	this->initDialogDataFrameInterpolate();
}

DADialogDataFrameInterpolate::~DADialogDataFrameInterpolate()
{
    delete ui;
}

/**
 * @brief 初始化插值法填充界面，多项式插值显示次数设置
 */
void DADialogDataFrameInterpolate::initDialogDataFrameInterpolate()
{
	ui->labelOrder->setVisible(false);
	ui->lineEditOrder->setVisible(false);

	connect(ui->comboBoxMethod,
			static_cast< void (QComboBox::*)(const QString&) >(&QComboBox::activated),
			[ = ](const QString& text) {
				if (text == "spline") {
					ui->labelOrder->setVisible(false);
					ui->lineEditOrder->setVisible(false);
				} else {
					ui->labelOrder->setVisible(true);
					ui->lineEditOrder->setVisible(true);
				}
			});
}

QString DADialogDataFrameInterpolate::getInterpolateMethod() const
{
	return ui->comboBoxMethod->currentText();
}

void DADialogDataFrameInterpolate::setInterpolateOrder(double d)
{
	ui->lineEditOrder->setText(QString::number(d));
}

double DADialogDataFrameInterpolate::getInterpolateOrder() const
{
	bool isok = false;
	double v  = ui->lineEditOrder->text().toInt(&isok);
	if (!isok) {
		qCritical() << tr("The current input cannot be converted to a integer number.");  // cn:当前输入内容无法转换为浮点数
		return 1;
	}
	return v;
}

bool DADialogDataFrameInterpolate::isEnableLimitCount() const
{
	return ui->checkBoxLimit->isChecked();
}

void DADialogDataFrameInterpolate::setEnableLimit(bool on)
{
	ui->checkBoxLimit->setChecked(on);
}

int DADialogDataFrameInterpolate::getLimitCount() const
{
	return ui->spinBoxLimit->value();
}

void DADialogDataFrameInterpolate::setLimitCount(int d)
{
	ui->spinBoxLimit->setValue(d);
}
}
