#include "DAChartAddVectorFieldWidget.h"
#include "ui_DAChartAddVectorFieldWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "DALogCategory.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_samples.h"
#include <algorithm>
#if DA_ENABLE_PYTHON
// DAPyDataFrame/DAPySeries 通过 DAData.h 间接 include
#endif
namespace DA
{

DAChartAddVectorFieldWidget::DAChartAddVectorFieldWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddVectorFieldWidget)
{
	ui->setupUi(this);
	ui->selectWidgetX->setRoleLabel(tr("X"));  // cn:X
	ui->selectWidgetY->setRoleLabel(tr("Y"));  // cn:Y
	ui->selectWidgetU->setRoleLabel(tr("U"));  // cn:U
	ui->selectWidgetV->setRoleLabel(tr("V"));  // cn:V
	connect(this, &DAChartAddVectorFieldWidget::dataManagerChanged, this, &DAChartAddVectorFieldWidget::onDataManagerChanged);
}

DAChartAddVectorFieldWidget::~DAChartAddVectorFieldWidget()
{
	delete ui;
}

void DAChartAddVectorFieldWidget::setDataManager(DADataManager* dmgr)
{
	DAAbstractChartAddItemWidget::setDataManager(dmgr);
	ui->selectWidgetX->setDataManager(dmgr);
	ui->selectWidgetY->setDataManager(dmgr);
	ui->selectWidgetU->setDataManager(dmgr);
	ui->selectWidgetV->setDataManager(dmgr);
}

void DAChartAddVectorFieldWidget::onDataManagerChanged(DADataManager* dmgr)
{
	ui->selectWidgetX->setDataManager(dmgr);
	ui->selectWidgetY->setDataManager(dmgr);
	ui->selectWidgetU->setDataManager(dmgr);
	ui->selectWidgetV->setDataManager(dmgr);
}

QwtPlotItem* DAChartAddVectorFieldWidget::createPlotItem()
{
#if DA_ENABLE_PYTHON
	QPair< DAData, QString > xSel = ui->selectWidgetX->getCurrentSeries();
	QPair< DAData, QString > ySel = ui->selectWidgetY->getCurrentSeries();
	QPair< DAData, QString > uSel = ui->selectWidgetU->getCurrentSeries();
	QPair< DAData, QString > vSel = ui->selectWidgetV->getCurrentSeries();
	if (xSel.first.isNull() || ySel.first.isNull() || uSel.first.isNull() || vSel.first.isNull()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("X, Y, U, V must be series"));  // cn:X、Y、U、V必须是序列
		return nullptr;
	}
	DAPyDataFrame xdf = xSel.first.toDataFrame();
	DAPyDataFrame ydf = ySel.first.toDataFrame();
	DAPyDataFrame udf = uSel.first.toDataFrame();
	DAPyDataFrame vdf = vSel.first.toDataFrame();
	if (xdf.isNone() || ydf.isNone() || udf.isNone() || vdf.isNone()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("The selected data cannot be converted to a series"));  // cn:所选数据无法转换为序列
		return nullptr;
	}
	DAPySeries xs = xdf[ xSel.second ];
	DAPySeries ys = ydf[ ySel.second ];
	DAPySeries us = udf[ uSel.second ];
	DAPySeries vs = vdf[ vSel.second ];
	if (xs.isNone() || ys.isNone() || us.isNone() || vs.isNone()) {
		QMessageBox::warning(this,
			tr("Warning"),  // cn:警告
			tr("The selected data cannot be converted to a series"));  // cn:所选数据无法转换为序列
		return nullptr;
	}
	std::size_t n = std::min({ xs.size(), ys.size(), us.size(), vs.size() });
	if (n == 0) {
		return nullptr;
	}
	try {
		std::vector< double > vx, vy, vu, vv;
		vx.reserve(xs.size());
		vy.reserve(ys.size());
		vu.reserve(us.size());
		vv.reserve(vs.size());
		xs.castTo< double >(std::back_inserter(vx));
		ys.castTo< double >(std::back_inserter(vy));
		us.castTo< double >(std::back_inserter(vu));
		vs.castTo< double >(std::back_inserter(vv));
		QVector< QwtVectorFieldSample > samples;
		samples.reserve(static_cast< int >(n));
		for (std::size_t i = 0; i < n; ++i) {
			samples.append(QwtVectorFieldSample(vx[ i ], vy[ i ], vu[ i ], vv[ i ]));
		}
		QwtPlotVectorField* item = new QwtPlotVectorField();
		item->setSamples(samples);
		item->setTitle(uSel.second);
		return item;
	} catch (const std::exception& e) {
		daCritical << tr("Exception occurred during extracting vector field data:%1").arg(e.what());  // cn:提取向量场数据过程中出现异常:%1
		return nullptr;
	}
#else
	return nullptr;
#endif
}

}  // namespace DA
