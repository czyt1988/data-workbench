#include "DialogPeakAnalysisSetting.h"
#include "ui_DialogPeakAnalysisSetting.h"
#include "DADataManager.h"
#include "Models/DAPySeriesTableModel.h"
#include <QMessageBox>
#include <QtMath>

DialogPeakAnalysisSetting::DialogPeakAnalysisSetting(QWidget* parent)
	: QDialog(parent), ui(new Ui::DialogPeakAnalysisSetting)
{
	ui->setupUi(this);

	mModuel = new DA::DAPySeriesTableModel(this);
	ui->tableViewPreview->setModel(mModuel);
	connect(ui->comboBoxDataMgr,
			&DA::DADataManagerComboBox::currentDataframeSeriesChanged,
			this,
			&DialogPeakAnalysisSetting::onCurrentDataframeSeriesChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogPeakAnalysisSetting::onAccepted);

	// 自定义基线，获取当前数据的最大值、最小值、均值、中位数
	connect(ui->comboBoxHeight,
			static_cast< void (QComboBox::*)(const QString&) >(&QComboBox::activated),
			[ = ](const QString& text) {
				ui->comboBoxBaseLineDirection->setEnabled(true);
				if (text == "Customize")
					ui->doubleSpinBoxHeight->setEnabled(true);
				else {
					ui->doubleSpinBoxHeight->setEnabled(false);
					if (mCount.empty())
						return;
					if (text == "Min")  // 最小值
						ui->doubleSpinBoxHeight->setValue(mCount[ 0 ]);
					if (text == "Max")  // 最大值
						ui->doubleSpinBoxHeight->setValue(mCount[ 1 ]);
					if (text == "Mean")  // 平均值
						ui->doubleSpinBoxHeight->setValue(mCount[ 2 ]);
					if (text == "Median")  // 中位数
						ui->doubleSpinBoxHeight->setValue(mCount[ 3 ]);
				}
			});
}

DialogPeakAnalysisSetting::~DialogPeakAnalysisSetting()
{
	delete ui;
}

void DialogPeakAnalysisSetting::setDataManager(DA::DADataManager* mgr)
{
	ui->comboBoxDataMgr->setDataManager(mgr);
}

void DialogPeakAnalysisSetting::setCurrentSeries(const DA::DAPySeries& s)
{
	if (s.isNone()) {
		mModuel->clearData();
		return;
	}
	mModuel->setSeries({ s });
}

double DialogPeakAnalysisSetting::getSamplingRate() const
{
	return ui->doubleSpinBoxSamplingRate->value();
}

DA::DAPySeries DialogPeakAnalysisSetting::getCurrentSeries() const
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		return DA::DAPySeries();
	}
	return seriess.back();
}

QVariantMap DialogPeakAnalysisSetting::getPeakAnalysisSetting()
{
	if (ui->checkBoxAuto->isChecked())
		return QVariantMap();
	QVariantMap res;
	res[ "height" ] = ui->doubleSpinBoxHeight->value();
	// 方向，0为正，1为负，2为两者
	int direction = 0;
	if (ui->comboBoxBaseLineDirection->currentText() == "Positive")
		direction = 0;
	if (ui->comboBoxBaseLineDirection->currentText() == "Negative")
		direction = 1;
	if (ui->comboBoxBaseLineDirection->currentText() == "Both")
		direction = 2;
	res[ "direction" ]  = direction;
	res[ "threshold" ]  = ui->doubleSpinBoxSeekThreshold->value();
	res[ "distance" ]   = ui->spinBoxSeekDistance->value();
	res[ "prominence" ] = ui->doubleSpinBoxSeekProminence->value();
	res[ "width" ]      = ui->doubleSpinBoxSeekWidth->value();
	res[ "wlen" ]       = ui->spinBoxSeekWlen->value();
	res[ "rel_height" ] = ui->doubleSpinBoxSeekRelHeight->value();

	return res;
}

void DialogPeakAnalysisSetting::onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName)
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

	if (series.size() < 1)
		return;
	// 把series转换为一个容器数组
	QList< double > seriesList;
	for (size_t i = 0; i < series.size(); ++i)
		seriesList.append(series[ i ].toDouble());
	// series.castTo< double >(seriesList.begin());

	// 排序
	std::sort(seriesList.begin(), seriesList.end());
	mCount.clear();
	// 最小值
	mCount.append(seriesList.first());
	// 最大值
	mCount.append(seriesList.last());
	// 平均值
	mCount.append(std::accumulate(seriesList.begin(), seriesList.end(), 0.0) / seriesList.size());
	// 中位数
	int n = seriesList.size();
	if (n % 2 == 1)
		mCount.append(seriesList[ std::floor(n / 2) ]);
	else
		mCount.append((seriesList[ n / 2 - 1 ] + seriesList[ n / 2 ]) / 2.0);

	if (mCount.empty())
		return;
	if (ui->comboBoxHeight->currentText() == "Min")
		ui->doubleSpinBoxHeight->setValue(mCount[ 0 ]);
	if (ui->comboBoxHeight->currentText() == "Max")
		ui->doubleSpinBoxHeight->setValue(mCount[ 1 ]);
	if (ui->comboBoxHeight->currentText() == "Mean")
		ui->doubleSpinBoxHeight->setValue(mCount[ 2 ]);
	if (ui->comboBoxHeight->currentText() == "Median")
		ui->doubleSpinBoxHeight->setValue(mCount[ 3 ]);
}

void DialogPeakAnalysisSetting::onAccepted()
{
	auto seriess = mModuel->getSeries();
	if (seriess.size() != 1) {
		QMessageBox::warning(this,
							 tr("warning"),
							 tr("You need to select a data for peak analysis")  // cn:你需要选择一组数据进行峰值分析
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

	if (!ui->checkBoxAuto->isChecked()) {
		if (ui->spinBoxSeekDistance->value() < 1) {
			QMessageBox::warning(this,
								 tr("warning"),
								 tr("The wlen cannot be Less than 1")  // cn:相邻峰距离不能小于1
			);
			return;
		}
		if (ui->spinBoxSeekWlen->value() <= 1) {
			QMessageBox::warning(this,
								 tr("warning"),
								 tr("The wlen cannot be Less than 1")  // cn:窗口长度不能小于1
			);
			return;
		}
	}

	accept();
}

void DialogPeakAnalysisSetting::on_checkBoxAuto_toggled(bool checked)
{
	if (checked) {
		ui->comboBoxHeight->setEnabled(false);
		ui->comboBoxBaseLineDirection->setEnabled(false);
		ui->doubleSpinBoxSeekThreshold->setEnabled(false);
		ui->spinBoxSeekDistance->setEnabled(false);
		ui->doubleSpinBoxSeekProminence->setEnabled(false);
		ui->doubleSpinBoxSeekWidth->setEnabled(false);
		ui->spinBoxSeekWlen->setEnabled(false);
		ui->doubleSpinBoxSeekRelHeight->setEnabled(false);
	} else {
		ui->comboBoxHeight->setEnabled(true);
		ui->comboBoxBaseLineDirection->setEnabled(true);
		ui->doubleSpinBoxSeekThreshold->setEnabled(true);
		ui->spinBoxSeekDistance->setEnabled(true);
		ui->doubleSpinBoxSeekProminence->setEnabled(true);
		ui->doubleSpinBoxSeekWidth->setEnabled(true);
		ui->spinBoxSeekWlen->setEnabled(true);
		ui->doubleSpinBoxSeekRelHeight->setEnabled(true);
	}
}
