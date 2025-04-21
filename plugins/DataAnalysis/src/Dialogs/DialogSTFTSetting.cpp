#include "DialogSTFTSetting.h"
#include "ui_DialogSTFTSetting.h"
#include <QMessageBox>
#include <QtMath>
#include "DADataManager.h"
#include "Models/DAPySeriesTableModule.h"

DialogSTFTSetting::DialogSTFTSetting(QWidget* parent) : QDialog(parent), ui(new Ui::DialogSTFTSetting)
{
	ui->setupUi(this);
	mModuel = new DA::DAPySeriesTableModule(this);
	ui->tableViewPreview->setModel(mModuel);
	connect(ui->comboBoxDataMgr,
			&DA::DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DialogSTFTSetting::onCurrentDataframeSeriesChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogSTFTSetting::onAccepted);
}

DialogSTFTSetting::~DialogSTFTSetting()
{
    delete ui;
}

/**
 * @brief DialogSTFTSetting::setDataManager
 * @param mgr
 */
void DialogSTFTSetting::setDataManager(DA::DADataManager* mgr)
{
    ui->comboBoxDataMgr->setDataManager(mgr);
}

/**
 * @brief 设置当前的序列
 * @param s
 */
void DialogSTFTSetting::setCurrentSeries(const DA::DAPySeries& s)
{
	if (s.isNone()) {
		mModuel->clearData();
		return;
	}
	mModuel->setSeries({ s });
}

DA::DAPySeries DialogSTFTSetting::getCurrentSeries() const
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		return DA::DAPySeries();
	}
	return seriess.back();
}

QVariantMap DialogSTFTSetting::getSTFTSetting()
{
	QVariantMap res;
	res[ "window" ]  = ui->comboBoxWindow->currentText();
	res[ "nperseg" ] = ui->spinBoxNperseg->value();
	if (ui->checkBox->isChecked()) {
		res[ "noverlap" ] = ui->doubleSpinBoxNoverlap->value();
	} else {
		res[ "noverlap" ] = (ui->spinBoxNperseg->value()) / 2;
	}

	if (ui->checkBox_2->isChecked()) {
		res[ "nfft" ] = ui->spinBoxNfft->value();
	} else {
		res[ "nfft" ] = ui->spinBoxNperseg->value();
	}
	if (ui->checkBoxDetrend->isChecked()) {
		if (ui->radioButtonConstant->isChecked()) {
			res[ "detrend" ] = "constant";
		} else if (ui->radioButtonLinear->isChecked()) {
			res[ "detrend" ] = "linear";
		}
	}
	res[ "scaling" ] = ui->comboBoxScaling->currentText();
	if (ui->radioButtonDB->isChecked()) {
		res[ "db" ] = true;
	}

	return res;
}

double DialogSTFTSetting::getSamplingRate() const
{
	return ui->doubleSpinBoxFs->value();
}

void DialogSTFTSetting::onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName)
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

void DialogSTFTSetting::onAccepted()
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("You need to select a waveform data for spectrum analysis")  // cn:你需要选择一个波形数据进行频谱分析
		);
		return;
	}
	auto fs = ui->doubleSpinBoxFs->value();
	if (qFuzzyIsNull(fs)) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("The sampling rate cannot be 0")  // cn:采样率不能为0
		);
		return;
	}
	if (ui->checkBox_2->isChecked()) {
		if (ui->spinBoxNfft->value() < ui->spinBoxNperseg->value()) {
			QMessageBox::warning(this,
								 tr("warning"),
								 tr("The nfft cannot lowerr than nperseg")  // cn:FFT点数不能小于样本数
			);
			return;
		}
	}
	accept();
}
