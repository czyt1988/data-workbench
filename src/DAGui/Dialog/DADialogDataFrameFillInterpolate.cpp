#include "DADialogDataFrameFillInterpolate.h"
#include "ui_DADialogDataFrameFillInterpolate.h"
#include <QDebug>

namespace DA
{
DADialogDataFrameFillInterpolate::DADialogDataFrameFillInterpolate(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataFrameFillInterpolate)
{
	ui->setupUi(this);
	this->initDialogDataFrameInterpolate();
}

DADialogDataFrameFillInterpolate::~DADialogDataFrameFillInterpolate()
{
    delete ui;
}

/**
 * @brief 初始化插值法填充界面，多项式插值显示次数设置
 */
void DADialogDataFrameFillInterpolate::initDialogDataFrameInterpolate()
{
	ui->labelOrder->setVisible(false);
	ui->lineEditOrder->setVisible(false);

    ui->comboBoxMethod->addItem(tr("spline"), QStringLiteral("spline"));          // cn:线性插值
    ui->comboBoxMethod->addItem(tr("polynomial"), QStringLiteral("polynomial"));  // cn:多项式插值

	connect(ui->comboBoxMethod,
            static_cast< void (QComboBox::*)(const QString&) >(&QComboBox::activated),
            [=](const QString& text) {
                if (text == "spline") {
                    ui->labelOrder->setVisible(false);
                    ui->lineEditOrder->setVisible(false);
                } else {
                    ui->labelOrder->setVisible(true);
                    ui->lineEditOrder->setVisible(true);
                }
            });
}

QString DADialogDataFrameFillInterpolate::getInterpolateMethod() const
{
	return ui->comboBoxMethod->currentText();
}

void DADialogDataFrameFillInterpolate::setInterpolateOrder(double d)
{
	ui->lineEditOrder->setText(QString::number(d));
}

double DADialogDataFrameFillInterpolate::getInterpolateOrder() const
{
	bool isok = false;
	double v  = ui->lineEditOrder->text().toInt(&isok);
	if (!isok) {
		qCritical() << tr("The current input cannot be converted to a integer number.");  // cn:当前输入内容无法转换为浮点数
		return 1;
	}
	return v;
}

bool DADialogDataFrameFillInterpolate::getInterPolateAxis() const
{
	if (ui->radioButtonAxis0->isChecked())
		return 0;
	return 1;
}

bool DADialogDataFrameFillInterpolate::isEnableLimitCount() const
{
	return ui->checkBoxLimit->isChecked();
}

void DADialogDataFrameFillInterpolate::setEnableLimit(bool on)
{
	ui->checkBoxLimit->setChecked(on);
}

int DADialogDataFrameFillInterpolate::getLimitCount() const
{
	return ui->spinBoxLimit->value();
}

void DADialogDataFrameFillInterpolate::setLimitCount(int d)
{
	ui->spinBoxLimit->setValue(d);
}

}  // end of DA
