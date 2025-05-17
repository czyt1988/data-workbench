#include "DialogWaveletDWTSetting.h"
#include "ui_DialogWaveletDWTSetting.h"
#include <QMessageBox>
#include <QtMath>
#include "DADataManager.h"
#include "Models/DAPySeriesTableModel.h"

DialogWaveletDWTSetting::DialogWaveletDWTSetting(QWidget* parent) : QDialog(parent), ui(new Ui::DialogWaveletDWTSetting)
{
	ui->setupUi(this);
	mModuel = new DA::DAPySeriesTableModel(this);
	ui->tableViewPreview->setModel(mModuel);
	connect(ui->comboBoxDataMgr,
			&DA::DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DialogWaveletDWTSetting::onCurrentDataframeSeriesChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogWaveletDWTSetting::onAccepted);
}

DialogWaveletDWTSetting::~DialogWaveletDWTSetting()
{
	delete ui;
}

void DialogWaveletDWTSetting::setDataManager(DA::DADataManager* mgr)
{
	ui->comboBoxDataMgr->setDataManager(mgr);
}

void DialogWaveletDWTSetting::setCurrentSeries(const DA::DAPySeries& s)
{
	if (s.isNone()) {
		mModuel->clearData();
		return;
	}
	mModuel->setSeries({ s });
}

DA::DAPySeries DialogWaveletDWTSetting::getCurrentSeries() const
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		return DA::DAPySeries();
	}
	return seriess.back();
}

QVariantMap DialogWaveletDWTSetting::getWaveletDWTSetting()
{
	QVariantMap res;
	res[ "wavelet" ] = ui->comboBoxWavelet->currentText();
	res[ "mode" ]    = ui->comboBoxMode->currentText();
	res[ "level" ]   = ui->spinBoxLevel->value();
	res[ "axis" ]    = ui->spinBoxAxis->value();
	return res;
}

double DialogWaveletDWTSetting::getSamplingRate() const
{
	return ui->doubleSpinBoxSamplingRate->value();
}

void DialogWaveletDWTSetting::onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName)
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

void DialogWaveletDWTSetting::onAccepted()
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("You need to select a waveform data for dwt")  // cn:你需要选择一个波形数据进行离散小波变换
		);
		return;
	}

	auto fs = ui->doubleSpinBoxSamplingRate->value();
	if (qFuzzyIsNull(fs)) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("The sampling rate cannot be 0")  // cn:采样率不能为0
		);
		return;
	}

	auto level = ui->spinBoxLevel->value();
	if (level <= 0) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("The level cannot be 0")  // cn:分解次数不能为0
		);
		return;
	}

	accept();
}
