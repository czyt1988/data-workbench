#include "DialogWaveletCWTSetting.h"
#include "ui_DialogWaveletCWTSetting.h"
#include <QMessageBox>
#include <QtMath>
#include "DADataManager.h"
#include "Models/DAPySeriesTableModule.h"

DialogWaveletCWTSetting::DialogWaveletCWTSetting(QWidget* parent) : QDialog(parent), ui(new Ui::DialogWaveletCWTSetting)
{
	ui->setupUi(this);
	mModuel = new DA::DAPySeriesTableModule(this);
	ui->tableViewPreview->setModel(mModuel);
	connect(ui->comboBoxDataMgr,
			&DA::DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DialogWaveletCWTSetting::onCurrentDataframeSeriesChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogWaveletCWTSetting::onAccepted);
}

DialogWaveletCWTSetting::~DialogWaveletCWTSetting()
{
	delete ui;
}

void DialogWaveletCWTSetting::setDataManager(DA::DADataManager* mgr)
{
	ui->comboBoxDataMgr->setDataManager(mgr);
	ui->comboBoxScales->setDataManager(mgr);
}

void DialogWaveletCWTSetting::setCurrentSeries(const DA::DAPySeries& s)
{
	if (s.isNone()) {
		mModuel->clearData();
		return;
	}
	mModuel->setSeries({ s });
}

DA::DAPySeries DialogWaveletCWTSetting::getCurrentSeries() const
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		return DA::DAPySeries();
	}
	return seriess.back();
}

DA::DAPySeries DialogWaveletCWTSetting::getScalesSeries() const
{
	return ui->comboBoxScales->getCurrentDAData().toSeries();
}

QVariantMap DialogWaveletCWTSetting::getWaveletCWTSetting()
{
	QVariantMap res;
	res[ "wavelet" ]         = ui->comboBoxWavelet->currentText();
	res[ "sampling_period" ] = ui->doubleSpinBoxSamplingPeriod->value();
	res[ "method" ]          = ui->comboBoxMethod->currentText();
	res[ "axis" ]            = ui->spinBoxAxis->value();
	return res;
}

double DialogWaveletCWTSetting::getSamplingRate() const
{
	return ui->doubleSpinBoxSamplingRate->value();
}

void DialogWaveletCWTSetting::onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName)
{
	if (!data.isDataFrame()) {
		mModuel->clearData();
		return;
	}
	if (seriesName.isEmpty()) {
		return;
	}
	auto df     = data.toDataFrame();
	auto series = df[ seriesName ];
	setCurrentSeries(series);
}

void DialogWaveletCWTSetting::onAccepted()
{
	auto seriess = mModuel->getSeries();

	accept();
}
