#include "DAExportToPngSettingDialog.h"
#include "ui_DAExportToPngSettingDialog.h"
#include <QApplication>
#include <QScreen>
#include <QFileDialog>
namespace DA
{
DAExportToPngSettingDialog::DAExportToPngSettingDialog(QWidget* parent)
	: QDialog(parent), ui(new Ui::DAExportToPngSettingDialog)
{
	ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	connect(ui->buttonGroupDPIType,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAExportToPngSettingDialog::onButtonGroupDPITypeButtonClicked);
#else
	connect(ui->buttonGroupDPIType,
			&QButtonGroup::buttonClicked,
			this,
			&DAExportToPngSettingDialog::onButtonGroupDPITypeButtonClicked);
#endif
	connect(ui->pushButtonExport, &QPushButton::clicked, this, &DAExportToPngSettingDialog::onPushButtonExportClicked);
}

DAExportToPngSettingDialog::~DAExportToPngSettingDialog()
{
	delete ui;
}

QString DAExportToPngSettingDialog::getSelectSaveFilePath() const
{
	return mSaveFilePath;
}

int DAExportToPngSettingDialog::getDPI() const
{
	return ui->spinBoxDPI->value();
}

void DAExportToPngSettingDialog::onButtonGroupDPITypeButtonClicked(QAbstractButton* button)
{
	if (button == ui->radioButtonPrint) {
		ui->spinBoxDPI->setValue(600);
	} else if (button == ui->radioButtonScreen) {
		qreal dpi = 96;
		if (QScreen* sc = QApplication::primaryScreen()) {
			dpi = sc->physicalDotsPerInch();
		}
		ui->spinBoxDPI->setValue(dpi);
	}
}

void DAExportToPngSettingDialog::onPushButtonExportClicked()
{
	mSaveFilePath = QFileDialog::getSaveFileName(this, tr(""), QString(), tr("Images (*.png)"));
	if (mSaveFilePath.isEmpty()) {
		return;
	}
	accept();
}

}  // end DA
