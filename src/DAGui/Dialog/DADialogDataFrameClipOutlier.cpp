#include "DADialogDataFrameClipOutlier.h"
#include "ui_DADialogDataFrameClipOutlier.h"
#include <QDebug>
#include <QMessageBox>
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

void DADialogDataFrameClipOutlier::setDataframe(const DAPyDataFrame& df)
{
    QStringList para = df.columns();

    ui->comboBoxData->clear();
    for (int row = 0; row < para.size(); ++row) {
        ui->comboBoxData->addItem(para[ row ]);
    }
}

void DADialogDataFrameClipOutlier::setFilterData(const int index)
{
    ui->comboBoxData->setCurrentIndex(index);
}

QString DADialogDataFrameClipOutlier::getFilterData() const
{
    return ui->comboBoxData->currentText();
}

void DADialogDataFrameClipOutlier::setLowerValue(const double d)
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

void DADialogDataFrameClipOutlier::setUpperValue(const double d)
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
