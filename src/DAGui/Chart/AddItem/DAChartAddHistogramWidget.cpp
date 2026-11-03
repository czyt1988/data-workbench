#include "Chart/AddItem/DAChartAddHistogramWidget.h"
#include "ui_DAChartAddHistogramWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "DALogCategory.h"
#include "qwt_plot_histogram.h"
#include "qwt_interval.h"
#include "qwt_samples.h"
#include <algorithm>
#include <limits>
namespace DA
{

DAChartAddHistogramWidget::DAChartAddHistogramWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddHistogramWidget)
{
	ui->setupUi(this);
	ui->selectWidgetData->setRoleLabel(tr("Data"));  // cn:数据
	connect(this, &DAChartAddHistogramWidget::dataManagerChanged, this, &DAChartAddHistogramWidget::onDataManagerChanged);
}

DAChartAddHistogramWidget::~DAChartAddHistogramWidget()
{
	delete ui;
}

void DAChartAddHistogramWidget::setDataManager(DADataManager* dmgr)
{
	DAAbstractChartAddItemWidget::setDataManager(dmgr);
	ui->selectWidgetData->setDataManager(dmgr);
}

void DAChartAddHistogramWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->selectWidgetData->setDataManager(dmgr);
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
	if (mHasProgrammatic) {
		std::vector< double > raw;
		raw.reserve(static_cast< std::size_t >(mProgrammaticValues.size()));
		for (double v : std::as_const(mProgrammaticValues)) {
			raw.push_back(v);
		}
		return buildHistogramItem(raw, mProgrammaticTitle, mProgrammaticBins);
	}

	QPair< DAData, QString > sel = ui->selectWidgetData->getCurrentSeries();
	DAData d = sel.first;
	QString seriesName = sel.second;
	if (!d.isDataFrame()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("Please select a series"));  // cn:请选择一个序列
		return nullptr;
	}
	DAPyDataFrame df = d.toDataFrame();
	if (df.isNone()) {
		return nullptr;
	}
	DAPySeries s = df[ seriesName ];
	if (s.isNone()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("Please select a series"));  // cn:请选择一个序列
		return nullptr;
	}
	QString title = seriesName;
	std::vector< double > raw;
	raw.reserve(s.size());
	try {
		s.castTo< double >(std::back_inserter(raw));
	} catch (const std::exception& e) {
		daCritical << tr("Exception occurred during extracting series:%1").arg(e.what());  // cn:提取序列过程中出现异常:%1
		return nullptr;
	}
	return buildHistogramItem(raw, title, ui->spinBoxBins->value());
}

/**
 * @brief 把数值序列分箱成直方图图元
 *
 * 界面取数与编程式取数共用这一份分箱实现，保证两条路径的图元完全一致。
 * @param raw 数值序列（调用方已剔除 NaN）
 * @param title 图元标题
 * @param binCount 分箱数，小于 1 时按 1 处理
 * @return 未 attach 的 QwtPlotHistogram；数据为空时返回 nullptr
 */
QwtPlotItem* DAChartAddHistogramWidget::buildHistogramItem(const std::vector< double >& raw,
														   const QString& title,
														   int binCount) const
{
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
		item->setTitle(title);
		return item;
	}
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
	item->setTitle(title);
	return item;
}

/**
 * @brief 编程式数据入口
 * @param values 数值序列
 * @param binCount 分箱数
 * @param title 图元标题（为空时使用 "histogram"）
 */
void DAChartAddHistogramWidget::setProgrammaticValues(const QVector< double >& values, int binCount, const QString& title)
{
	mProgrammaticValues = values;
	mProgrammaticBins   = binCount;
	mProgrammaticTitle  = title;
	mHasProgrammatic    = true;
}

/**
 * @brief 清除编程式数据，回到界面取数路径
 */
void DAChartAddHistogramWidget::clearProgrammaticValues()
{
	mProgrammaticValues.clear();
	mProgrammaticTitle.clear();
	mProgrammaticBins = 10;
	mHasProgrammatic  = false;
}

/**
 * @brief 是否处于编程式数据模式
 */
bool DAChartAddHistogramWidget::hasProgrammaticValues() const
{
	return mHasProgrammatic;
}

}  // namespace DA
