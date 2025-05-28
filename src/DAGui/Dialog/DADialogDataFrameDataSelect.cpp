#include "DADialogDataFrameDataSelect.h"
#include "ui_DADialogDataFrameDataSelect.h"
#include <QDebug>
#include <QMessageBox>
namespace DA
{

DADialogDataFrameDataSelect::DADialogDataFrameDataSelect(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataFrameDataSelect)
{
	ui->setupUi(this);
}

DADialogDataFrameDataSelect::~DADialogDataFrameDataSelect()
{
    delete ui;
}

void DADialogDataFrameDataSelect::setDataframe(const DAPyDataFrame& df)
{
    QStringList para = df.columns();

    ui->comboBoxData->clear();
    for (int row = 0; row < para.size(); ++row) {
        ui->comboBoxData->addItem(para[ row ]);
    }
}

void DADialogDataFrameDataSelect::setFilterData(const int index)
{
    ui->comboBoxData->setCurrentIndex(index);
}

QString DADialogDataFrameDataSelect::getFilterData() const
{
    return ui->comboBoxData->currentText();
}

void DADialogDataFrameDataSelect::setLowerValue(const double d)
{
	ui->LowerValue->setText(QString::number(d));
}

double DADialogDataFrameDataSelect::getLowerValue() const
{
	bool isok = false;
	double v  = ui->LowerValue->text().toDouble(&isok);
	if (!isok) {
		qCritical() << tr("The current input cannot be converted to a floating-point number.");  // cn:当前输入内容无法转换为浮点数
		return 0.0;
	}
	return v;
}

void DADialogDataFrameDataSelect::setUpperValue(const double d)
{
	ui->UpperValue->setText(QString::number(d));
}

double DADialogDataFrameDataSelect::getUpperValue() const
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
