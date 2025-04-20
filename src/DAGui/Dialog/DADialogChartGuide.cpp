﻿#include "DADialogChartGuide.h"
#include "ui_DADialogChartGuide.h"
#include <QPen>
// DA
#include "DAGlobalColorTheme.h"
#include "DADataManager.h"
#include "DAAbstractChartAddItemWidget.h"
#include "DAChartAddCurveWidget.h"
#include "DAChartAddBarWidget.h"
#include "DAChartAddXYESeriesWidget.h"
#include "DAChartAddOHLCSeriesWidget.h"
#include "DAChartUtil.h"
// qwt
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_intervalcurve.h"

namespace DA
{
class DADialogChartGuide::PrivateData
{
	DA_DECLARE_PUBLIC(DADialogChartGuide)
public:
	PrivateData(DADialogChartGuide* p);
	DAChartAddCurveWidget* mAddCurve { nullptr };
	DAChartAddBarWidget* mAddBar { nullptr };
	DAChartAddXYESeriesWidget* mXyeSeries { nullptr };
	DAChartAddOHLCSeriesWidget* mOHLCSeries { nullptr };
};

DADialogChartGuide::PrivateData::PrivateData(DADialogChartGuide* p) : q_ptr(p)
{
}
//----------------------------------------------------
// DADialogChartGuide
//----------------------------------------------------

DADialogChartGuide::DADialogChartGuide(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADialogChartGuide)
{
	ui->setupUi(this);
	DA_D(d);
	initListWidget();
	d->mAddCurve   = new DAChartAddCurveWidget();
	d->mAddBar     = new DAChartAddBarWidget();
	d->mXyeSeries  = new DAChartAddXYESeriesWidget();
	d->mOHLCSeries = new DAChartAddOHLCSeriesWidget();
	ui->stackedWidget->addWidget(d->mAddCurve);
	ui->stackedWidget->addWidget(d->mAddBar);
	ui->stackedWidget->addWidget(d->mXyeSeries);
	ui->stackedWidget->addWidget(d->mOHLCSeries);
	connect(ui->listWidgetChartType, &QListWidget::currentItemChanged, this, &DADialogChartGuide::onListWidgetCurrentItemChanged);
}

DADialogChartGuide::~DADialogChartGuide()
{
	delete ui;
}

void DADialogChartGuide::initListWidget()
{
	QListWidgetItem* item = nullptr;
	// curve
	item = new QListWidgetItem(QIcon(":/DAGui/ChartType/icon/chart-type/chart-curve.svg"), tr("curve"));
	item->setData(Qt::UserRole, static_cast< int >(DA::ChartTypes::Curve));
	ui->listWidgetChartType->addItem(item);
	// scatter
	item = new QListWidgetItem(QIcon(":/DAGui/ChartType/icon/chart-type/chart-scatter.svg"), tr("scatter"));
	item->setData(Qt::UserRole, static_cast< int >(DA::ChartTypes::Scatter));
	ui->listWidgetChartType->addItem(item);
	// bar
	item = new QListWidgetItem(QIcon(":/DAGui/ChartType/icon/chart-type/chart-bar.svg"), tr("bar"));
	item->setData(Qt::UserRole, static_cast< int >(DA::ChartTypes::Bar));
	ui->listWidgetChartType->addItem(item);
	// errorbar
	item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-intervalcurve.svg"), tr("error bar"));
	item->setData(Qt::UserRole, static_cast< int >(DA::ChartTypes::ErrorBar));
	ui->listWidgetChartType->addItem(item);
	// boxplot
	item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-OHLC.svg"), tr("box"));
	item->setData(Qt::UserRole, static_cast< int >(DA::ChartTypes::Box));
	ui->listWidgetChartType->addItem(item);
	// 初始化
	ui->listWidgetChartType->setCurrentRow(0);
}
/**
 * @brief 设置datamanager,会把combox填入所有的dataframe
 * @param dmgr
 */
void DADialogChartGuide::setDataManager(DADataManager* dmgr)
{
	int c = ui->stackedWidget->count();
	for (int i = 0; i < c; ++i) {
		if (DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->widget(i))) {
			w->setDataManager(dmgr);
		}
	}
}

/**
 * @brief 设置当前的数据
 * @param d
 */
void DADialogChartGuide::setCurrentData(const DAData& d)
{
	int c = ui->stackedWidget->count();
	for (int i = 0; i < c; ++i) {
		if (DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->widget(i))) {
			w->setCurrentData(d);
		}
	}
}

/**
 * @brief 获取当前的绘图类型
 * @return
 */
DA::ChartTypes DADialogChartGuide::getCurrentChartType() const
{
	QListWidgetItem* item = ui->listWidgetChartType->currentItem();
	if (item == nullptr) {
		return DA::ChartTypes::Unknow;
	}
	return static_cast< DA::ChartTypes >(item->data(Qt::UserRole).toInt());
}

/**
 * @brief 获取绘图item
 * @return  如果没有返回nullptr
 */
QwtPlotItem* DADialogChartGuide::createPlotItem()
{
	DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->currentWidget());
	if (!w) {
		return nullptr;
	}
	QwtPlotItem* item = w->createPlotItem();
	if (nullptr == item) {
		return nullptr;
	}
	// 针对不同的类型设置item属性
	initSetPlotItem(item);
	return item;
}

/**
 * @brief 更新数据
 */
void DADialogChartGuide::updateData()
{
	int c = ui->stackedWidget->count();
	for (int i = 0; i < c; ++i) {
		if (DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->widget(i))) {
			w->updateData();
		}
	}
}

DAAbstractChartAddItemWidget* DADialogChartGuide::getCurrentChartAddItemWidget() const
{
	return qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->currentWidget());
}

/**
 * @brief 根据当前绘图类型设置item属性
 * @param item
 */
void DADialogChartGuide::initSetPlotItem(QwtPlotItem* item)
{
	DA::ChartTypes ct = getCurrentChartType();
	switch (ct) {
	case DA::ChartTypes::Scatter: {
		if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
			QwtPlotCurve* cur = static_cast< QwtPlotCurve* >(item);
			cur->setStyle(QwtPlotCurve::Dots);
		}
	} break;
	default:
		break;
	}
	// 设置颜色
	QColor c = DAGlobalColorTheme::getInstance().color();
	DAChartUtil::setPlotItemColor(item, c);
}

/**
 * @brief 设置当前的绘图类型
 * @param t
 */
void DADialogChartGuide::setCurrentChartType(DA::ChartTypes t)
{
	int c = ui->listWidgetChartType->count();
	for (int i = 0; i < c; ++i) {
		auto item = ui->listWidgetChartType->item(i);
		int v     = item->data(Qt::UserRole).toInt();
		if (v == static_cast< int >(t)) {
			ui->listWidgetChartType->setCurrentItem(item);
		}
	}
}

void DADialogChartGuide::onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	Q_UNUSED(previous);
	DA_D(d);
	DA::ChartTypes ct = static_cast< DA::ChartTypes >(current->data(Qt::UserRole).toInt());
	switch (ct) {
	case DA::ChartTypes::Curve:
		ui->stackedWidget->setCurrentWidget(d->mAddCurve);
		break;
	case DA::ChartTypes::Scatter:
		ui->stackedWidget->setCurrentWidget(d->mAddCurve);
		break;
	case DA::ChartTypes::Bar:
		ui->stackedWidget->setCurrentWidget(d->mAddBar);
		break;
	case DA::ChartTypes::ErrorBar:
		ui->stackedWidget->setCurrentWidget(d->mXyeSeries);
		break;
	case DA::ChartTypes::Box:
		ui->stackedWidget->setCurrentWidget(d->mOHLCSeries);
		break;
	default:
		break;
	}
}

}
