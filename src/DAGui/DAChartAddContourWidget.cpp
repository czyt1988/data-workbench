#include "DAChartAddContourWidget.h"
#include "ui_DAChartAddContourWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "DALogCategory.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_point_3d.h"
#include "qwt_interval.h"
#include <algorithm>
#include <limits>
#if DA_ENABLE_PYTHON
// DAPyDataFrame/DAPySeries 通过 DAData.h 间接 include
#endif
namespace DA
{

DAChartAddContourWidget::DAChartAddContourWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddContourWidget)
{
	ui->setupUi(this);
	connect(this, &DAChartAddContourWidget::dataManagerChanged, this, &DAChartAddContourWidget::onDataManagerChanged);
	connect(this, &DAChartAddContourWidget::currentDataChanged, this, &DAChartAddContourWidget::onCurrentDataChanged);
}

DAChartAddContourWidget::~DAChartAddContourWidget()
{
	delete ui;
}

void DAChartAddContourWidget::setDataManager(DADataManager* dmgr)
{
	DAAbstractChartAddItemWidget::setDataManager(dmgr);
	ui->comboBoxX->setDataManager(dmgr);
	ui->comboBoxY->setDataManager(dmgr);
	ui->comboBoxValue->setDataManager(dmgr);
}

void DAChartAddContourWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->comboBoxX->setDataManager(dmgr);
	ui->comboBoxY->setDataManager(dmgr);
	ui->comboBoxValue->setDataManager(dmgr);
}

void DAChartAddContourWidget::onCurrentDataChanged(const DAData& d)
{
	ui->comboBoxX->setCurrentDAData(d);
	ui->comboBoxY->setCurrentDAData(d);
	ui->comboBoxValue->setCurrentDAData(d);
}

QwtPlotItem* DAChartAddContourWidget::createPlotItem()
{
#if DA_ENABLE_PYTHON
	DAData xd = ui->comboBoxX->getCurrentDAData();
	DAData yd = ui->comboBoxY->getCurrentDAData();
	DAData vd = ui->comboBoxValue->getCurrentDAData();
	if (!xd.isSeries() || !yd.isSeries() || !vd.isSeries()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("X, Y and Value must be series"));  // cn:X、Y和Value必须是序列
		return nullptr;
	}
	DAPySeries xs = xd.toSeries();
	DAPySeries ys = yd.toSeries();
	DAPySeries vs = vd.toSeries();
	if (xs.isNone() || ys.isNone() || vs.isNone()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("The selected data cannot be converted to a series"));  // cn:所选数据无法转换为序列
		return nullptr;
	}
	std::size_t n = std::min({ xs.size(), ys.size(), vs.size() });
	if (n == 0) {
		return nullptr;
	}
	try {
		std::vector< double > vx, vy, vv;
		vx.reserve(xs.size());
		vy.reserve(ys.size());
		vv.reserve(vs.size());
		xs.castTo< double >(std::back_inserter(vx));
		ys.castTo< double >(std::back_inserter(vy));
		vs.castTo< double >(std::back_inserter(vv));
		QVector< QwtPoint3D > samples;
		samples.reserve(static_cast< int >(n));
		double zMin = std::numeric_limits< double >::max();
		double zMax = std::numeric_limits< double >::lowest();
		for (std::size_t i = 0; i < n; ++i) {
			double z = vv[ i ];
			if (z < zMin) zMin = z;
			if (z > zMax) zMax = z;
			samples.append(QwtPoint3D(vx[ i ], vy[ i ], z));
		}
		QwtPlotSpectroCurve* item = new QwtPlotSpectroCurve();
		item->setSamples(samples);
		// 根据 z 值范围设置颜色区间
		if (zMin < zMax) {
			item->setColorRange(QwtInterval(zMin, zMax));
		} else {
			item->setColorRange(QwtInterval(0.0, 1.0));
		}
		item->setTitle(vd.getName());
		return item;
	} catch (const std::exception& e) {
		daCritical << tr("Exception occurred during extracting contour data:%1").arg(e.what());  // cn:提取等高线数据过程中出现异常:%1
		return nullptr;
	}
#else
	return nullptr;
#endif
}

}  // namespace DA
