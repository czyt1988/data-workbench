#include <QMessageBox>
#include <QtMath>
#include "DialogFilterSetting.h"
#include "ui_DialogFilterSetting.h"
#include "DADataManager.h"
#include "Models/DAPySeriesTableModel.h"

DialogFilterSetting::DialogFilterSetting(QWidget* parent) : QDialog(parent), ui(new Ui::DialogFilterSetting)
{
	ui->setupUi(this);

	// 滤波器类型相关ui
	initFilterTypeSetting();

	mModuel = new DA::DAPySeriesTableModel(this);
	ui->tableViewPreview->setModel(mModuel);
	connect(ui->comboBoxDataMgr,
			&DA::DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DialogFilterSetting::onCurrentDataframeSeriesChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogFilterSetting::onAccepted);
}

DialogFilterSetting::~DialogFilterSetting()
{
    delete ui;
}

/**
 * @brief 初始化滤波器类型界面设置，低通/高通显示截止频率设置，带通/带阻显示截止频率上下限设置
 */
void DialogFilterSetting::initFilterTypeSetting()
{
	ui->L_Cutoff_freq_Box->setVisible(false);
	ui->L_Cutoff_freq_Label->setVisible(false);
	ui->U_Cutoff_freq_Box->setVisible(false);
	ui->U_Cutoff_freq_Label->setVisible(false);

	connect(ui->Filter_type_Box,
			static_cast< void (QComboBox::*)(const QString&) >(&QComboBox::activated),
			[ = ](const QString& text) {
				if (text == "bandpass" || text == "bandstop") {
					ui->Cutoff_freq_Box->setVisible(false);
					ui->Cutoff_freq_Label->setVisible(false);
					ui->L_Cutoff_freq_Box->setVisible(true);
					ui->L_Cutoff_freq_Label->setVisible(true);
					ui->U_Cutoff_freq_Box->setVisible(true);
					ui->U_Cutoff_freq_Label->setVisible(true);
				} else {
					ui->Cutoff_freq_Box->setVisible(true);
					ui->Cutoff_freq_Label->setVisible(true);
					ui->L_Cutoff_freq_Box->setVisible(false);
					ui->L_Cutoff_freq_Label->setVisible(false);
					ui->U_Cutoff_freq_Box->setVisible(false);
					ui->U_Cutoff_freq_Label->setVisible(false);
				}
			});
}

void DialogFilterSetting::setDataManager(DA::DADataManager* mgr)
{
	ui->comboBoxDataMgr->setDataManager(mgr);
}

void DialogFilterSetting::setCurrentSeries(const DA::DAPySeries& s)
{
	if (s.isNone()) {
		mModuel->clearData();
		return;
	}
	mModuel->setSeries({ s });
}

DA::DAPySeries DialogFilterSetting::getCurrentSeries() const
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		return DA::DAPySeries();
	}
	return seriess.back();
}

QVariantMap DialogFilterSetting::getFilterSetting()
{
	QVariantMap res;
	res[ "filter_type" ] = ui->Filter_type_Box->currentText();
	if (ui->Filter_type_Box->currentText() == "lowpass" | ui->Filter_type_Box->currentText() == "highpass") {
		res[ "cutoff_freq" ] = ui->Cutoff_freq_Box->value();
	} else if (ui->Filter_type_Box->currentText() == "bandpass" | ui->Filter_type_Box->currentText() == "bandstop") {
		res[ "upper_freq" ] = ui->U_Cutoff_freq_Box->value();
		res[ "lower_freq" ] = ui->L_Cutoff_freq_Box->value();
	}
	if (ui->fab_filter->isChecked()) {
		res[ "phases" ] = true;
	}
	return res;
}

double DialogFilterSetting::getSamplingRate() const
{
	return ui->Samp_freq_Box->value();
}

int DialogFilterSetting::getFilterorder() const
{
	return ui->Filter_order_Box->value();
}

void DialogFilterSetting::onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName)
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

void DialogFilterSetting::onAccepted()
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("You need to select a waveform data for filter")  // cn:你需要选择一个波形数据进行滤波
		);
		return;
	}
	auto fs = ui->Samp_freq_Box->value();

	if (qFuzzyIsNull(fs)) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("The sampling rate cannot be 0")  // cn:采样率不能为0
		);
		return;
	}

	auto cf = ui->Cutoff_freq_Box->value();
	auto uf = ui->U_Cutoff_freq_Box->value();
	auto lf = ui->L_Cutoff_freq_Box->value();

	if (ui->Filter_type_Box->currentText() == "lowpass" | ui->Filter_type_Box->currentText() == "highpass") {
		if (qFuzzyIsNull(cf)) {
			QMessageBox::warning(this,
								 tr("warning"),
								 tr("The cut-off frequency cannot be 0")  // cn:截止频率不能为0
			);
			return;
		}
	} else if (ui->Filter_type_Box->currentText() == "bandpass" | ui->Filter_type_Box->currentText() == "bandstop") {
		if (qFuzzyIsNull(lf) | qFuzzyIsNull(uf)) {
			QMessageBox::warning(
				this,
				tr("warning"),
				tr("The lower cut-off frequency or upper cut-off frequency cannot be 0")  // cn:截止频率上限或下限不能为0
			);
			return;
		}
	}

	if (ui->Filter_order_Box->value() == 0) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("The filter order cannot be 0")  // cn:滤波器阶数不能为0
		);
		return;
	}

	accept();
}
