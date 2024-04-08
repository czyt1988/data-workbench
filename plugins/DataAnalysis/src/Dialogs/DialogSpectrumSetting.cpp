#include "DialogSpectrumSetting.h"
#include <QMessageBox>
#include <QtMath>
#include "ui_DialogSpectrumSetting.h"
#include "DADataManager.h"
#include "Models/DAPySeriesTableModule.h"

DialogSpectrumSetting::DialogSpectrumSetting(QWidget* parent) : QDialog(parent), ui(new Ui::DialogSpectrumSetting)
{
	ui->setupUi(this);
	mModuel = new DA::DAPySeriesTableModule(this);
	ui->tableViewPreview->setModel(mModuel);
	connect(ui->comboBoxDataMgr,
			&DA::DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DialogSpectrumSetting::onCurrentDataframeSeriesChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogSpectrumSetting::onAccepted);
}

DialogSpectrumSetting::~DialogSpectrumSetting()
{
    delete ui;
}

/**
 * @brief DialogSpectrumSetting::setDataManager
 * @param mgr
 */
void DialogSpectrumSetting::setDataManager(DA::DADataManager* mgr)
{
    ui->comboBoxDataMgr->setDataManager(mgr);
}

/**
 * @brief 设置当前的序列
 * @param s
 */
void DialogSpectrumSetting::setCurrentSeries(const DA::DAPySeries& s)
{
	if (s.isNone()) {
		mModuel->clearData();
		return;
	}
	mModuel->setSeries({ s });
}

DA::DAPySeries DialogSpectrumSetting::getCurrentSeries() const
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		return DA::DAPySeries();
	}
	return seriess.back();
}

QVariantMap DialogSpectrumSetting::getSpectrumSetting()
{
	QVariantMap res;
	if (ui->radioButtonFFTSizeCusmize->isChecked()) {
		res[ "fftsize" ] = ui->doubleSpinBoxFFTSize->value();
	} else if (ui->radioButtonFFTSizePower2->isChecked()) {
		res[ "nextpower2" ] = true;
	}
	if (ui->radioButtonDB->isChecked()) {
		res[ "db" ] = true;
	}
	return res;
}

double DialogSpectrumSetting::getSamplingRate() const
{
	return ui->doubleSpinBoxFs->value();
}

void DialogSpectrumSetting::onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName)
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

void DialogSpectrumSetting::onAccepted()
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
	accept();
}
