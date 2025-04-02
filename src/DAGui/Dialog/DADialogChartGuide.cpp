#include "DADialogChartGuide.h"
#include "ui_DADialogChartGuide.h"
#include "DADataManager.h"
#include "DAAbstractChartAddItemWidget.h"
#include "DAChartAddCurveWidget.h"
#include "DAChartAddBarWidget.h"
#include "DAChartAddErrorBarWidget.h"
#include <iterator>
#include <vector>
#define STR_DADIALOGCHARTGUIDE_FINISHE tr("Finish")
#define STR_DADIALOGCHARTGUIDE_NEXT tr("Next")
// qwt
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_intervalcurve.h"
namespace DA
{

DADialogChartGuide::DADialogChartGuide(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogChartGuide)
{
	ui->setupUi(this);
	init();
	connect(ui->pushButtonPrevious, &QPushButton::clicked, this, &DADialogChartGuide::onPushButtonPreviousClicked);
	connect(ui->pushButtonNext, &QPushButton::clicked, this, &DADialogChartGuide::onPushButtonNextClicked);
	connect(ui->pushButtonCancel, &QPushButton::clicked, this, &DADialogChartGuide::onPushButtonCancelClicked);
	connect(ui->listWidgetChartType, &QListWidget::currentItemChanged, this, &DADialogChartGuide::onListWidgetCurrentItemChanged);
	updateButtonTextAndState();
}

DADialogChartGuide::~DADialogChartGuide()
{
	delete ui;
}

void DADialogChartGuide::init()
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
	// 初始化
	ui->stackedWidget->setCurrentWidget(ui->pageCurve);
	ui->listWidgetChartType->setCurrentRow(0);
}
/**
 * @brief 设置datamanager,会把combox填入所有的dataframe
 * @param dmgr
 */
void DADialogChartGuide::setDataManager(DADataManager* dmgr)
{
	ui->pageCurve->setDataManager(dmgr);
	ui->pageBar->setDataManager(dmgr);
	ui->pageErrorBar->setDataManager(dmgr);
}

/**
 * @brief 设置当前的数据
 * @param d
 */
void DADialogChartGuide::setCurrentData(const DAData& d)
{
	QWidget* w = ui->stackedWidget->currentWidget();
	if (DAChartAddCurveWidget* c = qobject_cast< DAChartAddCurveWidget* >(w)) {
		c->setCurrentData(d);
		// 重新设置数据的话，步骤回到第一步
		c->toFirst();
	} else if (DAChartAddBarWidget* b = qobject_cast< DAChartAddBarWidget* >(w)) {
		b->setCurrentData(d);
		b->toFirst();
	} else if (DAChartAddErrorBarWidget* e = qobject_cast< DAChartAddErrorBarWidget* >(w)) {
		e->setCurrentData(d);
		e->toFirst();
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
	return item;
}

/**
 * @brief 更新数据
 */
void DADialogChartGuide::updateData()
{
	DAAbstractChartAddItemWidget* w = getCurrentChartAddItemWidget();
	if (!w) {
		return;
	}
	w->updateData();
}

void DADialogChartGuide::updateButtonTextAndState()
{
	DAAbstractChartAddItemWidget* w = getCurrentChartAddItemWidget();
	if (!w) {
		ui->pushButtonNext->setText(STR_DADIALOGCHARTGUIDE_FINISHE);  // cn:完成
		ui->pushButtonPrevious->setEnabled(false);
		return;
	}
	if (hasNext(w)) {
		ui->pushButtonNext->setText(STR_DADIALOGCHARTGUIDE_NEXT);
	} else {
		ui->pushButtonNext->setText(STR_DADIALOGCHARTGUIDE_FINISHE);
	}
	ui->pushButtonPrevious->setEnabled(hasPrevious(w));
}

DAAbstractChartAddItemWidget* DADialogChartGuide::getCurrentChartAddItemWidget() const
{
	return qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->currentWidget());
}

bool DADialogChartGuide::hasNext(DAAbstractChartAddItemWidget* w)
{
	int sc = w->getStepCount();
	int cc = w->getCurrentStep();
	if (sc <= 1) {
		return false;
	}
	if (cc < sc - 1) {
		return true;
	}
	return false;
}

bool DADialogChartGuide::hasPrevious(DAAbstractChartAddItemWidget* w)
{
	int sc = w->getStepCount();
	int cc = w->getCurrentStep();
	if (sc <= 1) {
		return false;
	}
	if (cc != 0) {
		return true;
	}
	return false;
}

/**
 * @brief 设置到第一步
 */
void DADialogChartGuide::allToFirst()
{
	int c = ui->stackedWidget->count();
	for (int i = 0; i < c; ++i) {
		if (DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->widget(i))) {
			w->toFirst();
		}
	}
	updateButtonTextAndState();
}

/**
 * @brief 设置到第最后一步
 */
void DADialogChartGuide::allToLast()
{
	int c = ui->stackedWidget->count();
	for (int i = 0; i < c; ++i) {
		if (DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->widget(i))) {
			w->toLast();
		}
	}
	updateButtonTextAndState();
}

/**
 * @brief 显示的时候，把窗口设置为第一步
 * @param event
 */
void DADialogChartGuide::showEvent(QShowEvent* event)
{
	allToFirst();
	QDialog::showEvent(event);
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
	DA::ChartTypes ct = static_cast< DA::ChartTypes >(current->data(Qt::UserRole).toInt());
	switch (ct) {
	case DA::ChartTypes::Curve:
		ui->stackedWidget->setCurrentWidget(ui->pageCurve);
		ui->pageCurve->setScatterMode(false);
		break;
	case DA::ChartTypes::Scatter:
		ui->stackedWidget->setCurrentWidget(ui->pageCurve);
		ui->pageCurve->setScatterMode(true);
		break;
	case DA::ChartTypes::Bar:
		ui->stackedWidget->setCurrentWidget(ui->pageBar);
	case DA::ChartTypes::ErrorBar:
		ui->stackedWidget->setCurrentWidget(ui->pageErrorBar);
	default:
		break;
	}
	updateButtonTextAndState();
}

void DADialogChartGuide::onPushButtonPreviousClicked()
{
	DAAbstractChartAddItemWidget* w = getCurrentChartAddItemWidget();
	if (!w) {
		return;
	}
	if (hasPrevious(w)) {
		w->previous();
	}
	updateButtonTextAndState();
}

void DADialogChartGuide::onPushButtonNextClicked()
{
	DAAbstractChartAddItemWidget* w = getCurrentChartAddItemWidget();
	if (!w) {
		return;
	}
	if (hasNext(w)) {
		w->next();
	} else {
		// 如果不是next，就是确认
		accept();
	}
	updateButtonTextAndState();
}

void DADialogChartGuide::onPushButtonCancelClicked()
{
	reject();
}
}
