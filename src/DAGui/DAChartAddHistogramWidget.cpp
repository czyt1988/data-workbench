#include "DAChartAddHistogramWidget.h"
#include "ui_DAChartAddHistogramWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "DALogCategory.h"
#include "qwt_plot_histogram.h"
#include "qwt_interval.h"
#include "qwt_samples.h"
#include <algorithm>
#include <limits>
#if DA_ENABLE_PYTHON
// DAPyDataFrame/DAPySeries 通过 DAData.h 间接 include
#endif
namespace DA
{

DAChartAddHistogramWidget::DAChartAddHistogramWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddHistogramWidget)
{
	ui->setupUi(this);
	connect(this, &DAChartAddHistogramWidget::dataManagerChanged, this, &DAChartAddHistogramWidget::onDataManagerChanged);
	connect(this, &DAChartAddHistogramWidget::currentDataChanged, this, &DAChartAddHistogramWidget::onCurrentDataChanged);
}

DAChartAddHistogramWidget::~DAChartAddHistogramWidget()
{
	delete ui;
}

void DAChartAddHistogramWidget::setDataManager(DADataManager* dmgr)
{
	DAAbstractChartAddItemWidget::setDataManager(dmgr);
	ui->comboBoxData->setDataManager(dmgr);
}

void DAChartAddHistogramWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->comboBoxData->setDataManager(dmgr);
}

void DAChartAddHistogramWidget::onCurrentDataChanged(const DAData& d)
{
	ui->comboBoxData->setCurrentDAData(d);
}

/**
 * @brief 获取 Y 轴显示方案
 * @return 当前选中的模式
 */
DAChartAddHistogramWidget::YAxisMode DAChartAddHistogramWidget::getYAxisMode() const
{
	if (ui->radioButtonDensity->isChecked()) {
		return Density;
	}
	return Count;
}

QwtPlotItem* DAChartAddHistogramWidget::createPlotItem()
{
#if DA_ENABLE_PYTHON
	DAData d = ui->comboBoxData->getCurrentDAData();
	if (!d.isSeries()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("Please select a series"));  // cn:请选择一个序列
		return nullptr;
	}
	DAPySeries s = d.toSeries();
	if (s.isNone()) {
		return nullptr;
	}
	std::vector< double > raw;
	raw.reserve(s.size());
	try {
		s.castTo< double >(std::back_inserter(raw));
	} catch (const std::exception& e) {
		daCritical << tr("Exception occurred during extracting series:%1").arg(e.what());  // cn:提取序列过程中出现异常:%1
		return nullptr;
	}
	if (raw.empty()) {
		return nullptr;
	}
	const std::size_t total = raw.size();
	// 计算最小最大值
	double minV = std::numeric_limits< double >::max();
	double maxV = std::numeric_limits< double >::lowest();
	for (double v : raw) {
		if (v < minV) minV = v;
		if (v > maxV) maxV = v;
	}
	const YAxisMode yMode = getYAxisMode();
	if (minV == maxV) {
		// 退化情况：所有值相同，构造一个 bin
		QVector< QwtIntervalSample > samples;
		double value = static_cast< double >(total);
		if (yMode == Density) {
			// 退化时 bin 宽取 1.0，密度 = count/(total*1.0) = 1.0
			value = 1.0;
		}
		samples.append(QwtIntervalSample(value, minV, minV + 1.0));
		QwtPlotHistogram* item = new QwtPlotHistogram();
		item->setSamples(samples);
		item->setTitle(d.getName());
		return item;
	}
	int binCount = ui->spinBoxBins->value();
	if (binCount < 1) binCount = 1;
	double binWidth = (maxV - minV) / binCount;
	// 初始化 bins
	QVector< QwtIntervalSample > samples(binCount);
	for (int i = 0; i < binCount; ++i) {
		double lo = minV + i * binWidth;
		double hi = minV + (i + 1) * binWidth;
		if (i == binCount - 1) hi = maxV;  // 最后一个 bin 包含 max
		samples[ i ] = QwtIntervalSample(0.0, lo, hi);
	}
	// 计数
	for (double v : raw) {
		int bin = static_cast< int >((v - minV) / binWidth);
		if (bin >= binCount) bin = binCount - 1;
		if (bin < 0) bin = 0;
		samples[ bin ].value += 1.0;
	}
	// 根据 Y 轴方案转换
	if (yMode == Density) {
		// 概率密度：count / (total * binWidth)，积分近似为 1
		const double scale = (total > 0 && binWidth > 0) ? 1.0 / (static_cast< double >(total) * binWidth) : 0.0;
		for (int i = 0; i < binCount; ++i) {
			samples[ i ].value *= scale;
		}
	}
	QwtPlotHistogram* item = new QwtPlotHistogram();
	item->setSamples(samples);
	item->setTitle(d.getName());
	return item;
#else
	return nullptr;
#endif
}

}  // namespace DA
