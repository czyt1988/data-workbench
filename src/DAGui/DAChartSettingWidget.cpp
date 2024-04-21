#include "DAChartSettingWidget.h"
#include "ui_DAChartSettingWidget.h"
#include <QPointer>
#include <QScrollArea>
#include <QDebug>
#include "Models/DAFigureTreeModel.h"
#include "DAChartOperateWidget.h"
#include "DAChartPlotSettingWidget.h"
namespace DA
{
class DAChartSettingWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartSettingWidget)
public:
	PrivateData(DAChartSettingWidget* p);

public:
	QPointer< DAFigureWidget > mFigure;
	QPointer< DAChartWidget > mChart;
	QPointer< DAChartOperateWidget > mChartOpt;
};

DAChartSettingWidget::PrivateData::PrivateData(DAChartSettingWidget* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DAFigureSettingWidget
//----------------------------------------------------

DAChartSettingWidget::DAChartSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartSettingWidget)
{
	ui->setupUi(this);                           // ui中有信号绑定槽
	ui->comboBoxSelectChart->setVisible(false);  // 初始状态不显示
	ui->comboBoxSelectItem->setVisible(false);   // 初始状态不显示
	showFigureSettingWidget();
	ui->buttonGroupType->setId(ui->toolButtonFigure, static_cast< int >(SettingFigure));
	ui->buttonGroupType->setId(ui->toolButtonChart, static_cast< int >(SettingChart));
	ui->buttonGroupType->setId(ui->toolButtonItem, static_cast< int >(SettingItem));
	connect(ui->comboBoxSelectChart,
			QOverload< int >::of(&QComboBox::activated),
			this,
			&DAChartSettingWidget::onComboBoxChartActivated);
	connect(ui->comboBoxSelectItem,
			QOverload< int >::of(&QComboBox::activated),
			this,
			&DAChartSettingWidget::onComboBoxItemActived);
	connect(ui->buttonGroupType,
			QOverload< int >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartSettingWidget::onButtonGroupTypeButtonClicked);
}

DAChartSettingWidget::~DAChartSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置绘图操作窗口
 * @param opt
 */
void DAChartSettingWidget::setChartOprateWidget(DAChartOperateWidget* opt)
{
	if (d_ptr->mChartOpt == opt) {
		return;
	}
	if (d_ptr->mChartOpt) {
		disconnect(d_ptr->mChartOpt, &DAChartOperateWidget::figureCloseing, this, &DAChartSettingWidget::onFigureCloseing);
		disconnect(d_ptr->mChartOpt, &DAChartOperateWidget::figureCreated, this, &DAChartSettingWidget::onFigureCreated);
		disconnect(d_ptr->mChartOpt,
				   &DAChartOperateWidget::currentFigureChanged,
				   this,
				   &DAChartSettingWidget::onCurrentFigureChanged);
	}
	d_ptr->mChartOpt = opt;
	if (opt) {
		connect(opt, &DAChartOperateWidget::figureCloseing, this, &DAChartSettingWidget::onFigureCloseing);
		connect(opt, &DAChartOperateWidget::figureCreated, this, &DAChartSettingWidget::onFigureCreated);
		connect(opt, &DAChartOperateWidget::currentFigureChanged, this, &DAChartSettingWidget::onCurrentFigureChanged);
	}
}

void DAChartSettingWidget::setFigure(DAFigureWidget* fig)
{
	qDebug() << "setFigure1";
	if (fig == d_ptr->mFigure) {
		return;
	}
	DAFigureWidget* oldfig = d_ptr->mFigure.data();
	if (oldfig) {
		unbindFigure(oldfig);
	}
	bindFigure(fig);
	if (nullptr == fig) {
		// 清空
		showFigureSettingWidget();
	}
}

void DAChartSettingWidget::setFigure(DAFigureWidget* fig, DAChartWidget* chart)
{
	qDebug() << "setFigure2";
	setFigure(fig);
	setCurrentChart(chart);
}

void DAChartSettingWidget::setFigure(DAFigureWidget* fig, DAChartWidget* chart, QwtPlotItem* item)
{
	qDebug() << "setFigure3";
	setFigure(fig, chart);
	setCurrentItem(item);
}

DAFigureWidget* DAChartSettingWidget::getFigure() const
{
	return d_ptr->mFigure.data();
}

DAChartWidget* DAChartSettingWidget::getChart() const
{
	return ui->widgetChartSetting->getChartWidget();
}

QwtPlotItem* DAChartSettingWidget::getItem() const
{
	return ui->widgetItemSetting->getPlotItem();
}

void DAChartSettingWidget::bindFigure(DAFigureWidget* fig)
{
	if (d_ptr->mFigure == fig) {
		return;
	}
	d_ptr->mFigure = fig;
	if (fig) {
		connect(fig, &DAFigureWidget::chartAdded, this, &DAChartSettingWidget::onChartAdded);
		connect(fig, &DAFigureWidget::chartWillRemove, this, &DAChartSettingWidget::onChartWillRemove);
		connect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartSettingWidget::onCurrentChartChanged);
		// 更新chart信息
		const auto charts = fig->getCharts();
		for (auto chart : charts) {
			// 绑定chart的信号槽
			bindChart(chart);
		}
		resetChartsComboBox(fig);
	}
}

void DAChartSettingWidget::unbindFigure(DAFigureWidget* fig)
{
	if (fig) {
		disconnect(fig, &DAFigureWidget::chartAdded, this, &DAChartSettingWidget::onChartAdded);
		disconnect(fig, &DAFigureWidget::chartWillRemove, this, &DAChartSettingWidget::onChartWillRemove);
		disconnect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartSettingWidget::onCurrentChartChanged);
		// 更新chart信息
		const auto charts = fig->getCharts();
		for (auto chart : charts) {
			// 解除绑定chart的信号槽
			unbindChart(chart);
		}
	}
}

void DAChartSettingWidget::bindChart(DAChartWidget* chart)
{
	if (chart) {
		connect(chart, &DAChartWidget::itemAttached, this, &DAChartSettingWidget::onItemAttached);
	}
}

void DAChartSettingWidget::unbindChart(DAChartWidget* chart)
{
	if (chart) {
		disconnect(chart, &DAChartWidget::itemAttached, this, &DAChartSettingWidget::onItemAttached);
	}
}

/**
 * @brief 获取chart在combobox的索引
 * @param chart
 * @return
 */
int DAChartSettingWidget::indexOfChart(const DAChartWidget* chart)
{
	int c = ui->comboBoxSelectChart->count();
	for (int i = 0; i < c; ++i) {
		DAChartWidget* savePtr = ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
		if (chart == savePtr) {
			return i;
		}
	}
	return -1;
}

int DAChartSettingWidget::indexOfItem(const QwtPlotItem* item)
{
	int c = ui->comboBoxSelectItem->count();
	for (int i = 0; i < c; ++i) {
		QwtPlotItem* savePtr = ui->comboBoxSelectItem->itemData(i).value< QwtPlotItem* >();
		if (item == savePtr) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief 设置当前的chart
 * @param chart
 */
void DAChartSettingWidget::setCurrentChart(DAChartWidget* chart)
{
	int index = indexOfChart(chart);
	ui->comboBoxSelectChart->setCurrentIndex(index);
	// 这里会触发indexChanged信号
}

/**
 * @brief 设置当前的item
 * @param item
 */
void DAChartSettingWidget::setCurrentItem(QwtPlotItem* item)
{
	int index = indexOfItem(item);
	ui->comboBoxSelectItem->setCurrentIndex(index);
	// 这里会触发indexChanged信号
}

void DAChartSettingWidget::showFigureSettingWidget()
{
	ui->toolButtonFigure->setChecked(true);
	ui->comboBoxSelectChart->setVisible(false);
	ui->comboBoxSelectItem->setVisible(false);
	ui->stackedWidget->setCurrentWidget(ui->pageFigureSet);
}

void DAChartSettingWidget::showPlotSettingWidget()
{
	ui->toolButtonChart->setChecked(true);
	ui->comboBoxSelectChart->setVisible(true);
	ui->comboBoxSelectItem->setVisible(false);
	ui->stackedWidget->setCurrentWidget(ui->widgetChartSetting);
}

void DAChartSettingWidget::showItemSettingWidget()
{
	ui->toolButtonItem->setChecked(true);
	ui->comboBoxSelectChart->setVisible(true);
	ui->comboBoxSelectItem->setVisible(true);
	ui->stackedWidget->setCurrentWidget(ui->widgetItemSetting);
}

/**
 * @brief 把chart从combobox中移除
 * @param chart
 */
void DAChartSettingWidget::removeChartFromComboBox(DAChartWidget* chart)
{
	auto c = ui->comboBoxSelectChart->count();
	for (int i = 0; i < c; ++i) {
		DAChartWidget* saveChartPtr = ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
		if (saveChartPtr == chart) {
			ui->comboBoxSelectChart->removeItem(i);
			return;
		}
	}
}

void DAChartSettingWidget::changeEvent(QEvent* e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void DAChartSettingWidget::resetChartsComboBox(DAFigureWidget* fig)
{
	QSignalBlocker b(ui->comboBoxSelectChart);
	QVariant curData           = ui->comboBoxSelectChart->currentData();
	DAChartWidget* oldselChart = nullptr;
	if (curData.isValid()) {
		oldselChart = curData.value< DAChartWidget* >();
	}
	const auto cs = fig->getCharts();
	ui->comboBoxSelectChart->clear();
	qDebug() << "comboBoxSelectChart clear";
	for (DAChartWidget* c : cs) {
		qDebug() << DAChartWidgetStandardItem::getChartTitle(fig, c);
		ui->comboBoxSelectChart->addItem(DAChartWidgetStandardItem::getChartTitle(fig, c), QVariant::fromValue(c));
	}
	// 默认选中
	if (oldselChart) {
		for (int i = 0; i < ui->comboBoxSelectChart->count(); ++i) {
			auto c = ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
			if (c == oldselChart) {
				// 选择项不变
				ui->comboBoxSelectChart->setCurrentIndex(i);
				return;
			}
		}
	}
	if (ui->comboBoxSelectChart->count() > 0) {
		ui->comboBoxSelectChart->setCurrentIndex(0);
		// 这个一定要设置，否则不会触发actived
		onComboBoxChartActivated(0);
	}
}

void DAChartSettingWidget::setItemsComboBox(const QList< QwtPlotItem* >& its)
{
	QSignalBlocker b(ui->comboBoxSelectItem);
	QVariant curData     = ui->comboBoxSelectItem->currentData();
	QwtPlotItem* oldItem = nullptr;
	if (curData.isValid()) {
		oldItem = curData.value< QwtPlotItem* >();
	}
	ui->comboBoxSelectItem->clear();
	for (QwtPlotItem* item : its) {
		ui->comboBoxSelectItem->addItem(DAChartItemStandardItem::getItemName(item), QVariant::fromValue(item));
	}
	if (ui->comboBoxSelectItem->count() > 0) {
		ui->comboBoxSelectItem->setCurrentIndex(0);
	}
	// 默认选中
	if (oldItem) {
		for (int i = 0; i < ui->comboBoxSelectItem->count(); ++i) {
			auto c = ui->comboBoxSelectItem->itemData(i).value< QwtPlotItem* >();
			if (c == oldItem) {
				// 选择项不变
				ui->comboBoxSelectItem->setCurrentIndex(i);
				return;
			}
		}
	}
	if (ui->comboBoxSelectItem->count() > 0) {
		ui->comboBoxSelectItem->setCurrentIndex(0);
		// 这个一定要设置，否则不会触发actived
		onComboBoxItemActived(0);
	}
}

void DAChartSettingWidget::resetItemsComboBox(DAChartWidget* chart)
{
	const auto items = chart->itemList();
	setItemsComboBox(items);
}

void DAChartSettingWidget::onFigureCloseing(DAFigureWidget* f)
{
	Q_UNUSED(f);
	if (!d_ptr->mChartOpt) {
		return;
	}
	int c = d_ptr->mChartOpt->getFigureCount();
	if (c == 1) {
		setFigure(nullptr);
	}
}

void DAChartSettingWidget::onFigureCreated(DAFigureWidget* f)
{
	if (f) {
		DAChartWidget* c = f->getCurrentChart();
		setFigure(f, c);
	}
}

void DAChartSettingWidget::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
	Q_UNUSED(index);
	if (f) {
		DAChartWidget* c = f->getCurrentChart();
		setFigure(f, c);
	}
}

void DAChartSettingWidget::onChartAdded(DAChartWidget* c)
{
	if (d_ptr->mFigure) {
		// 刷新chart
		qDebug() << "onChartAdded";
		bindChart(c);
		resetChartsComboBox(d_ptr->mFigure);
	}
}

void DAChartSettingWidget::onChartWillRemove(DAChartWidget* c)
{
	if (d_ptr->mFigure) {
		// 刷新chart
		qDebug() << "onChartWillRemove";
		unbindChart(c);
		removeChartFromComboBox(c);
	}
}

void DAChartSettingWidget::onCurrentChartChanged(DAChartWidget* c)
{
	if (d_ptr->mFigure) {
		// 刷新chart
		setCurrentChart(c);
	}
}

void DAChartSettingWidget::onItemAttached(QwtPlotItem* plotItem, bool on)
{
	DAChartWidget* c = qobject_cast< DAChartWidget* >(sender());
	if (c) {
		qDebug() << "onItemAttached,item::rtti=" << plotItem->rtti() << ",on=" << on;
		onChartItemAttached(c, plotItem, on);
	}
}

void DAChartSettingWidget::onChartItemAttached(DAChartWidget* c, QwtPlotItem* plotItem, bool on)
{
	if (c) {
		resetItemsComboBox(c);
	}
}

/**
 * @brief DAFigureSettingWidget::onComboBoxChartIndexChanged
 *
 * @note 注意，这里不要调用setCurrentChart函数
 * @param i
 */
void DAChartSettingWidget::onComboBoxChartActivated(int i)
{
	// 更新items
	qDebug() << "onComboBoxChartActivated(" << i << ")";
	if (!d_ptr->mFigure) {
		ui->widgetChartSetting->setChartWidget(nullptr);
		qDebug() << "DAChartSettingWidget have nullptr figure";
		return;
	}
	auto charts = d_ptr->mFigure->getCharts();
	if (i < 0 || i >= charts.size()) {
		d_ptr->mChart = nullptr;
		ui->widgetChartSetting->setChartWidget(nullptr);
		qDebug() << "figure have " << charts.size() << " chart,but index=" << i;
		return;
	}
	// 更新当前chart
	auto chart    = charts[ i ];
	d_ptr->mChart = chart;
	// 如果当前chart不是选定的chart，则把当前chart设置为选定的chart
	auto cc = d_ptr->mFigure->getCurrentChart();
	if (cc != chart) {
		QSignalBlocker b(d_ptr->mFigure);
		d_ptr->mFigure->setCurrentChart(chart);
	}
	// 获取chart的元素
	resetItemsComboBox(chart);
	//
	ui->widgetChartSetting->setChartWidget(chart);
}

/**
 * @brief
 *
 * @note 注意，这里不要调用setCurrentItem函数
 * @param i
 */
void DAChartSettingWidget::onComboBoxItemActived(int i)
{
	qDebug() << "onComboBoxItemActived(" << i << ")";
	if (!d_ptr->mChart) {
		ui->widgetItemSetting->setPlotItem(nullptr);
		qDebug() << "DAChartSettingWidget have nullptr Chart";
		return;
	}
	auto items = d_ptr->mChart->itemList();
	if (i < 0 || i >= items.size()) {
		ui->widgetItemSetting->setPlotItem(nullptr);
		qDebug() << "chart itemlist count = " << items.size() << ",but index=" << i;
		return;
	}
	auto item = items[ i ];
	// 按照item类型显示
	ui->widgetItemSetting->setPlotItem(item);
}

/**
 * @brief 按钮组点击
 * @param id
 */
void DAChartSettingWidget::onButtonGroupTypeButtonClicked(int id)
{
	switch (id) {
	case SettingFigure:
		showFigureSettingWidget();
		break;
	case SettingChart:
		showPlotSettingWidget();
		break;
	case SettingItem:
		showItemSettingWidget();
		break;
	}
}

}  // end da
