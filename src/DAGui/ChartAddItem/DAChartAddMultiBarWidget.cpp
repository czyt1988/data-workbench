#include "DAChartAddMultiBarWidget.h"
#include "ui_DAChartAddMultiBarWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "DALogCategory.h"
#include "DAPySeriesListView.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_samples.h"
#include "qwt_text.h"
#include <algorithm>
namespace DA
{

DAChartAddMultiBarWidget::DAChartAddMultiBarWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddMultiBarWidget)
{
	ui->setupUi(this);
	ui->listViewX->setAcceptMode(DAPySeriesListView::AcceptOneSeries);
	ui->listViewY->setAcceptMode(DAPySeriesListView::AcceptMultDataframeMultSeries);
	connect(ui->listViewX, &DAPySeriesListView::seriesChanged, this, &DAChartAddMultiBarWidget::onXSeriesChanged);
	connect(ui->listViewY, &DAPySeriesListView::seriesChanged, this, &DAChartAddMultiBarWidget::onYSeriesChanged);
	connect(ui->groupBoxXAutoincrement, &QGroupBox::clicked, this, &DAChartAddMultiBarWidget::onGroupBoxXAutoincrementClicked);
	connect(ui->toolButtonRemoveFromX, &QToolButton::clicked, this, &DAChartAddMultiBarWidget::onButtonXRemoveClicked);
	connect(ui->toolButtonRemoveFromY, &QToolButton::clicked, this, &DAChartAddMultiBarWidget::onButtonYRemoveClicked);
}

DAChartAddMultiBarWidget::~DAChartAddMultiBarWidget()
{
	delete ui;
}

void DAChartAddMultiBarWidget::setDataManager(DADataManager* dmgr)
{
	DAAbstractChartAddItemWidget::setDataManager(dmgr);
	ui->listViewX->setDataManager(dmgr);
	ui->listViewY->setDataManager(dmgr);
}

void DAChartAddMultiBarWidget::onXSeriesChanged()
{
}

void DAChartAddMultiBarWidget::onYSeriesChanged()
{
}

void DAChartAddMultiBarWidget::onGroupBoxXAutoincrementClicked(bool on)
{
	Q_UNUSED(on);
}

void DAChartAddMultiBarWidget::onButtonXRemoveClicked()
{
	ui->listViewX->removeCurrentSelect();
}

void DAChartAddMultiBarWidget::onButtonYRemoveClicked()
{
	ui->listViewY->removeCurrentSelect();
}

bool DAChartAddMultiBarWidget::tryGetXSelfInc(double& base, double& step)
{
	bool isOK = false;
	double a  = ui->lineEditXInitValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	double b = ui->lineEditXStepValue->text().toDouble(&isOK);
	if (!isOK) {
		return false;
	}
	base = a;
	step = b;
	return true;
}

bool DAChartAddMultiBarWidget::extractXSeries(std::vector< double >& res)
{
	if (ui->groupBoxXAutoincrement->isChecked()) {
		double base, step;
		if (!tryGetXSelfInc(base, step)) {
			QMessageBox::warning(this,
				tr("Warning"),  // cn:警告
				tr("The initial value and step of x auto increment must be floating-point numbers"));  // cn:x自增序列的初始值和步长必须为浮点数
			return false;
		}
		// 先留空，后面根据 y 的长度填充
		return true;
	}
	QList< QPair< DAData, QStringList > > xData = ui->listViewX->getSeries();
	if (xData.isEmpty() || xData.first().second.isEmpty()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please drag a series into the X list"));  // cn:警告 / 请把一个序列拖入X列表
		return false;
	}
	DAData d = xData.first().first;
	QString seriesName = xData.first().second.first();
	DAPyDataFrame df = d.toDataFrame();
	if (df.isNone()) {
		DAPySeries s = d.toSeries();
		if (s.isNone()) {
			return false;
		}
		s.castTo< double >(std::back_inserter(res));
	} else {
		DAPySeries s = df[ seriesName ];
		if (s.isNone()) {
			return false;
		}
		s.castTo< double >(std::back_inserter(res));
	}
	return true;
}

bool DAChartAddMultiBarWidget::extractYSeriesList(QVector< std::vector< double > >& res, QStringList& names)
{
	QList< QPair< DAData, QStringList > > yData = ui->listViewY->getSeries();
	if (yData.isEmpty()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please drag at least one series into the Y list"));  // cn:警告 / 请至少把一个序列拖入Y列表
		return false;
	}
	try {
		for (const auto& pair : yData) {
			DAData d = pair.first;
			DAPyDataFrame df = d.toDataFrame();
			for (const QString& name : pair.second) {
				DAPySeries s;
				if (df.isNone()) {
					s = d.toSeries();
				} else {
					s = df[ name ];
				}
				if (s.isNone()) {
					continue;
				}
				std::vector< double > v;
				v.reserve(s.size());
				s.castTo< double >(std::back_inserter(v));
				res.append(v);
				names << name;
			}
		}
	} catch (const std::exception& e) {
		daCritical << tr("Exception occurred during extracting y series:%1").arg(e.what());  // cn:提取y序列过程中出现异常:%1
		return false;
	}
	return !res.isEmpty();
}

QwtPlotItem* DAChartAddMultiBarWidget::createPlotItem()
{
	std::vector< double > xValues;
	if (!extractXSeries(xValues)) {
		return nullptr;
	}
	QVector< std::vector< double > > yValues;
	QStringList yNames;
	if (!extractYSeriesList(yValues, yNames)) {
		return nullptr;
	}
	// 确定 x 长度
	std::size_t xLen = 0;
	if (ui->groupBoxXAutoincrement->isChecked()) {
		// 用最长的 y 作为长度
		for (const auto& y : yValues) {
			xLen = std::max(xLen, y.size());
		}
	} else {
		xLen = xValues.size();
	}
	if (xLen == 0) {
		return nullptr;
	}
	// 构造 QwtSetSample
	QVector< QwtSetSample > samples;
	samples.reserve(static_cast< int >(xLen));
	double xBase = 1.0, xStep = 1.0;
	if (ui->groupBoxXAutoincrement->isChecked()) {
		tryGetXSelfInc(xBase, xStep);
	}
	try {
		for (std::size_t i = 0; i < xLen; ++i) {
			double xv = ui->groupBoxXAutoincrement->isChecked() ? (xBase + i * xStep) : (i < xValues.size() ? xValues[ i ] : 0.0);
			QVector< double > set;
			set.reserve(yValues.size());
			for (const auto& y : yValues) {
				set.append(i < y.size() ? y[ i ] : 0.0);
			}
			samples.append(QwtSetSample(xv, set));
		}
	} catch (const std::exception& e) {
		daCritical << tr("Exception occurred during building multi-bar samples:%1").arg(e.what());  // cn:构建多重柱状图样本过程中出现异常:%1
		return nullptr;
	}
	QwtPlotMultiBarChart* item = new QwtPlotMultiBarChart();
	item->setSamples(samples);
	// 设置 bar titles 为列名
	QList< QwtText > barTitles;
	for (const QString& n : yNames) {
		barTitles.append(QwtText(n));
	}
	item->setBarTitles(barTitles);
	item->setTitle(yNames.join("|"));
	return item;
}

}  // namespace DA
