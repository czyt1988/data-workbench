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
	Qt5Qt6Compat_Connect_ButtonGroupClicked_int(ui->buttonGroupType, DAChartSettingWidget::onButtonGroupTypeButtonClicked);
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
	qDebug() << "bindFigure" << qintptr(fig);
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
	qDebug() << "unbindFigure ptr=" << qintptr(fig);
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
	qDebug() << "bindChart ptr=" << qintptr(chart);
	if (chart) {
		connect(chart, &DAChartWidget::itemAttached, this, &DAChartSettingWidget::onItemAttached);
	}
}

void DAChartSettingWidget::unbindChart(DAChartWidget* chart)
{
	qDebug() << "unbindChart ptr=" << qintptr(chart);
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
	qDebug() << "setCurrentChart ptr=" << qintptr(chart);
	// 这里不会触发combobox的 任何信号
	if (nullptr == chart) {
		setChartUI(nullptr);
		return;
	}
	// 找到索引
	auto fig = chart->getFigure();
	if (fig == nullptr) {
		setChartUI(nullptr);
		return;
	}
	if (fig != d_ptr->mFigure) {
		qDebug() << "|-odd:chart figure inconsistent";
		d_ptr->mFigure = fig;
		setFigure(fig);
	}
	setChartUI(chart);
	// 如果当前chart不是选定的chart，则把当前chart设置为选定的chart
	auto cc = fig->getCurrentChart();
	if (cc != chart) {
		QSignalBlocker b(d_ptr->mFigure);
		d_ptr->mFigure->setCurrentChart(chart);
	}
}

/**
 * @brief 获取当前的chart
 * @return
 */
DAChartWidget* DAChartSettingWidget::getCurrentChart() const
{
	if (ui->comboBoxSelectChart->currentIndex() < 0) {
		return nullptr;
	}

	return ui->comboBoxSelectChart->currentData().value< DAChartWidget* >();
}

/**
 * @brief 设置当前的item
 * @param item
 */
void DAChartSettingWidget::setCurrentItem(QwtPlotItem* item)
{
	qDebug() << "setCurrentItem ptr=" << qintptr(item);
	// 这里不会触发combobox的 任何信号
	setItemUI(item);
}

QwtPlotItem* DAChartSettingWidget::getCurrentItem() const
{
	if (ui->comboBoxSelectItem->currentIndex() < 0) {
		return nullptr;
	}
	return ui->comboBoxSelectItem->currentData().value< QwtPlotItem* >();
}

void DAChartSettingWidget::showFigureSettingWidget()
{
	qDebug() << "showFigureSettingWidget";
	ui->toolButtonFigure->setChecked(true);
	ui->comboBoxSelectChart->setVisible(false);
	ui->comboBoxSelectItem->setVisible(false);
	ui->stackedWidget->setCurrentWidget(ui->pageFigureSet);
}

void DAChartSettingWidget::showPlotSettingWidget()
{
	qDebug() << "showPlotSettingWidget";
	ui->toolButtonChart->setChecked(true);
	ui->comboBoxSelectChart->setVisible(true);
	ui->comboBoxSelectItem->setVisible(false);
	ui->stackedWidget->setCurrentWidget(ui->widgetChartSetting);
}

void DAChartSettingWidget::showItemSettingWidget()
{
	qDebug() << "showItemSettingWidget";
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
	qDebug() << "removeChartFromComboBox";
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
	qDebug() << "resetChartsComboBox ptr=" << qintptr(fig);
	const auto cs = fig->getCharts();
	if (cs.empty()) {
		qDebug() << "|-comboBoxSelectChart clear";
		ui->comboBoxSelectChart->clear();
		return;
	}
	for (int i = 0; (i < ui->comboBoxSelectChart->count()) && (i < cs.size()); ++i) {
		DAChartWidget* chart = cs[ i ];
		// 更新
		ui->comboBoxSelectChart->setItemText(i, DAChartWidgetStandardItem::getChartTitle(fig, chart));
		ui->comboBoxSelectChart->setItemData(i, QVariant::fromValue(chart));
	}
	if (ui->comboBoxSelectChart->count() < cs.size()) {
		// 追加
		for (int i = ui->comboBoxSelectChart->count(); i < cs.size(); ++i) {
			DAChartWidget* chart = cs[ i ];
			ui->comboBoxSelectChart->addItem(DAChartWidgetStandardItem::getChartTitle(fig, chart),
											 QVariant::fromValue(chart));
		}
	} else if (ui->comboBoxSelectChart->count() > cs.size()) {
		// 删除
		for (int i = cs.size(); i < ui->comboBoxSelectChart->count(); ++i) {
			ui->comboBoxSelectChart->removeItem(i);
		}
	}
	updateChartUI();
}

void DAChartSettingWidget::setItemsComboBox(const QList< QwtPlotItem* >& its)
{
	qDebug() << "setItemsComboBox";
	if (ui->comboBoxSelectItem->count() > 0) {
		ui->comboBoxSelectItem->clear();
	}
	if (its.empty()) {
		return;
	}
	QVariant curData     = ui->comboBoxSelectItem->currentData();
	QwtPlotItem* oldItem = nullptr;
	if (curData.isValid()) {
		oldItem = curData.value< QwtPlotItem* >();
	}
	for (QwtPlotItem* item : its) {
		ui->comboBoxSelectItem->addItem(DAChartItemStandardItem::getItemName(item), QVariant::fromValue(item));
	}
	if (ui->comboBoxSelectItem->count() > 0) {
		ui->comboBoxSelectItem->setCurrentIndex(0);
	}
	// 默认选中
	if (oldItem) {
		setCurrentItem(oldItem);
	} else if (its.size() > 0) {
		setCurrentItem(its.first());
	}
}

void DAChartSettingWidget::resetItemsComboBox(DAChartWidget* chart)
{
	qDebug() << "resetItemsComboBox";
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
	qDebug() << "onFigureCreated ptr=" << qintptr(f);
	if (f) {
		DAChartWidget* c = f->getCurrentChart();
		setFigure(f, c);
		showFigureSettingWidget();
	}
}

void DAChartSettingWidget::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
	qDebug() << "onCurrentFigureChanged ptr=" << qintptr(f) << ",index=" << index;
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
		setCurrentChart(c);
		showPlotSettingWidget();
	}
}

void DAChartSettingWidget::onChartWillRemove(DAChartWidget* c)
{
	qDebug() << "onChartWillRemove ptr=" << qintptr(c);
	if (d_ptr->mFigure) {
		// 刷新chart
		unbindChart(c);
		removeChartFromComboBox(c);
	}
}

void DAChartSettingWidget::onCurrentChartChanged(DAChartWidget* c)
{
	qDebug() << "onCurrentChartChanged ptr=" << qintptr(c);
	if (d_ptr->mFigure) {
		// 刷新chart
		setCurrentChart(c);
	}
}

void DAChartSettingWidget::onItemAttached(QwtPlotItem* plotItem, bool on)
{
	qDebug() << "onItemAttached ptr=" << qintptr(plotItem) << ",on=" << on;
	DAChartWidget* c = qobject_cast< DAChartWidget* >(sender());
	if (c) {
		onChartItemAttached(c, plotItem, on);
	}
}

void DAChartSettingWidget::onChartItemAttached(DAChartWidget* c, QwtPlotItem* plotItem, bool on)
{
	if (c) {
		resetItemsComboBox(c);
		if (on) {
			// 有新增的item，把设置显示出来
			//  按照item类型显示
			showItemSettingWidget();
			setCurrentChart(c);
			setCurrentItem(plotItem);
		}
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
	qDebug() << "onComboBoxChartActivated(" << i << ")";
	DAChartWidget* chart = getChartByIndex(i);
	setCurrentChart(chart);
	// 这里不显示chart对应的stacked widget
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
	if (i < 0) {
		setCurrentItem(nullptr);
	} else {
		QwtPlotItem* item = ui->comboBoxSelectItem->itemData(i).value< QwtPlotItem* >();
		setCurrentItem(item);
		showItemSettingWidget();
	}
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

void DAChartSettingWidget::setChartUI(DAChartWidget* chart)
{
	qDebug() << "setChartUI ptr=" << qintptr(chart);
	QSignalBlocker b(ui->comboBoxSelectChart);
	if (chart == nullptr) {
		ui->widgetChartSetting->setChartWidget(nullptr);
		ui->comboBoxSelectChart->clear();
		ui->comboBoxSelectItem->clear();
	} else {
		// 这里会触发indexChanged信号
		int index = indexOfChart(chart);
		if (index == -1) {
			ui->widgetChartSetting->setChartWidget(nullptr);
			ui->comboBoxSelectChart->clear();
			ui->comboBoxSelectItem->clear();
			return;
		}
		ui->comboBoxSelectChart->setCurrentIndex(index);
		// 重置itemcombobox
		resetItemsComboBox(chart);
		ui->widgetChartSetting->setChartWidget(chart);
	}
}

void DAChartSettingWidget::updateChartUI()
{
	qDebug() << "updateChartUI";
	setChartUI(getCurrentChart());
}

void DAChartSettingWidget::setItemUI(QwtPlotItem* item)
{
	qDebug() << "setItemUI ptr=" << qintptr(item);
	QSignalBlocker b(ui->comboBoxSelectItem);
	if (item == nullptr) {
		ui->widgetItemSetting->setPlotItem(nullptr);
		ui->comboBoxSelectItem->setCurrentIndex(-1);
	} else {
		auto index = indexOfItem(item);
		ui->comboBoxSelectItem->setCurrentIndex(index);
		ui->widgetItemSetting->setPlotItem(item);
	}
}

void DAChartSettingWidget::updateItemUI()
{
	qDebug() << "updateItemUI";
	setItemUI(getCurrentItem());
}

DAChartWidget* DAChartSettingWidget::getChartByIndex(int i) const
{
	if (i < ui->comboBoxSelectChart->count() && i >= 0) {
		return ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
	}
	return nullptr;
}

}  // end da
