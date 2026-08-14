#include "DAChartAddBoxChartWidget.h"
#include "ui_DAChartAddBoxChartWidget.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include "DADataManager.h"
#include "DALogCategory.h"
#include "qwt_plot_boxchart.h"
#include "qwt_box_statistics.h"
#include "qwt_samples.h"
namespace DA
{

DAChartAddBoxChartWidget::DAChartAddBoxChartWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddBoxChartWidget)
{
	ui->setupUi(this);
	ui->comboBoxDataFrame->setShowSeriesUnderDataframe(false);
	connect(this, &DAChartAddBoxChartWidget::dataManagerChanged, this, &DAChartAddBoxChartWidget::onDataManagerChanged);
	connect(this, &DAChartAddBoxChartWidget::currentDataChanged, this, &DAChartAddBoxChartWidget::onCurrentDataChanged);
	connect(ui->comboBoxDataFrame,
		&DADataManagerComboBox::currentDataChanged,
		this,
		&DAChartAddBoxChartWidget::onComboBoxCurrentDataChanged);
}

DAChartAddBoxChartWidget::~DAChartAddBoxChartWidget()
{
	delete ui;
}

void DAChartAddBoxChartWidget::setDataManager(DADataManager* dmgr)
{
	DAAbstractChartAddItemWidget::setDataManager(dmgr);
	ui->comboBoxDataFrame->setDataManager(dmgr);
}

void DAChartAddBoxChartWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->comboBoxDataFrame->setDataManager(dmgr);
}

void DAChartAddBoxChartWidget::onCurrentDataChanged(const DAData& d)
{
	ui->comboBoxDataFrame->setCurrentDAData(d);
	refreshColumns();
}

void DAChartAddBoxChartWidget::onComboBoxCurrentDataChanged(const DAData& d)
{
	Q_UNUSED(d);
	refreshColumns();
}

void DAChartAddBoxChartWidget::refreshColumns()
{
	ui->listWidgetColumns->clear();
	DAData d = ui->comboBoxDataFrame->getCurrentDAData();
	if (!d.isDataFrame()) {
		return;
	}
	DAPyDataFrame df = d.toDataFrame();
	if (df.isNone()) {
		return;
	}
	const QList< QString > cols = df.columns();
	for (const QString& name : cols) {
		QListWidgetItem* item = new QListWidgetItem(name, ui->listWidgetColumns);
		item->setCheckState(Qt::Unchecked);
	}
}

QwtPlotItem* DAChartAddBoxChartWidget::createPlotItem()
{
	DAData d = ui->comboBoxDataFrame->getCurrentDAData();
	if (!d.isDataFrame()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("Please select a dataframe"));  // cn:请选择一个数据框
		return nullptr;
	}
	DAPyDataFrame df = d.toDataFrame();
	if (df.isNone()) {
		return nullptr;
	}
	// 收集选中的列
	QStringList selectedCols;
	for (int i = 0; i < ui->listWidgetColumns->count(); ++i) {
		QListWidgetItem* item = ui->listWidgetColumns->item(i);
		if (item->checkState() == Qt::Checked) {
			selectedCols << item->text();
		}
	}
	if (selectedCols.isEmpty()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("Please select at least one column"));  // cn:请至少选择一列
		return nullptr;
	}
	// 对每列计算箱体统计
	QVector< QwtBoxSample > samples;
	samples.reserve(selectedCols.size());
	try {
		for (int i = 0; i < selectedCols.size(); ++i) {
			DAPySeries s = df[ selectedCols[ i ] ];
			if (s.isNone()) {
				continue;
			}
			std::vector< double > raw;
			raw.reserve(s.size());
			s.castTo< double >(std::back_inserter(raw));
			QVector< double > qraw;
			qraw.reserve(static_cast< int >(raw.size()));
			for (double v : raw) {
				qraw.append(v);
			}
			QwtBoxSample sample =
				QwtBoxStatisticsCalculator::calculateFromRaw(static_cast< double >(i), qraw);
			samples.append(sample);
		}
	} catch (const std::exception& e) {
		daCritical << tr("Exception occurred during box chart data extraction:%1").arg(e.what());  // cn:箱线图数据提取过程中出现异常:%1
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("Failed to extract data"));  // cn:数据提取失败
		return nullptr;
	}
	if (samples.isEmpty()) {
		return nullptr;
	}
	QwtPlotBoxChart* item = new QwtPlotBoxChart();
	item->setSamples(samples);
	item->setTitle(selectedCols.join("|"));
	return item;
}

}  // namespace DA
