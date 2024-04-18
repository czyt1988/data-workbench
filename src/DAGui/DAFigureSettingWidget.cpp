#include "DAFigureSettingWidget.h"
#include "ui_DAFigureSettingWidget.h"
#include <QPointer>
#include <QScrollArea>
#include "Models/DAFigureTreeModel.h"
#include "DAChartOperateWidget.h"
#include "DAChartSettingWidget.h"
namespace DA
{
class DAFigureSettingWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAFigureSettingWidget)
public:
	PrivateData(DAFigureSettingWidget* p);

public:
	QPointer< DAFigureWidget > mFigure;
	QPointer< DAChartWidget > mChart;
    QPointer< DAChartOperateWidget > mChartOpt;
	QwtPlotItem* mItem { nullptr };
};

DAFigureSettingWidget::PrivateData::PrivateData(DAFigureSettingWidget* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DAFigureSettingWidget
//----------------------------------------------------

DAFigureSettingWidget::DAFigureSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAFigureSettingWidget)
{
	ui->setupUi(this);                           // ui中有信号绑定槽
	ui->comboBoxSelectChart->setVisible(false);  // 初始状态不显示
	ui->comboBoxSelectItem->setVisible(false);   // 初始状态不显示
    ui->toolButtonFigure->setChecked(true);
    ui->buttonGroupType->setId(ui->toolButtonFigure, static_cast< int >(SettingFigure));
    ui->buttonGroupType->setId(ui->toolButtonChart, static_cast< int >(SettingChart));
    ui->buttonGroupType->setId(ui->toolButtonItem, static_cast< int >(SettingItem));
    connect(ui->comboBoxSelectChart,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAFigureSettingWidget::onComboBoxChartIndexChanged);
    connect(ui->comboBoxSelectItem,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAFigureSettingWidget::onComboBoxItemIndexChanged);
    connect(ui->buttonGroupType,
            QOverload< int >::of(&QButtonGroup::buttonClicked),
            this,
            &DAFigureSettingWidget::onButtonGroupTypeButtonClicked);
}

DAFigureSettingWidget::~DAFigureSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置绘图操作窗口
 * @param opt
 */
void DAFigureSettingWidget::setChartOprateWidget(DAChartOperateWidget* opt)
{
    if (d_ptr->mChartOpt == opt) {
        return;
    }
    if (d_ptr->mChartOpt) {
        disconnect(d_ptr->mChartOpt, &DAChartOperateWidget::figureCloseing, this, &DAFigureSettingWidget::onFigureCloseing);
        disconnect(d_ptr->mChartOpt, &DAChartOperateWidget::figureCreated, this, &DAFigureSettingWidget::onFigureCreated);
        disconnect(d_ptr->mChartOpt,
                   &DAChartOperateWidget::currentFigureChanged,
                   this,
                   &DAFigureSettingWidget::onCurrentFigureChanged);
    }
    d_ptr->mChartOpt = opt;
    if (opt) {
        connect(opt, &DAChartOperateWidget::figureCloseing, this, &DAFigureSettingWidget::onFigureCloseing);
        connect(opt, &DAChartOperateWidget::figureCreated, this, &DAFigureSettingWidget::onFigureCreated);
        connect(opt, &DAChartOperateWidget::currentFigureChanged, this, &DAFigureSettingWidget::onCurrentFigureChanged);
    }
}

void DAFigureSettingWidget::setFigure(DAFigureWidget* fig)
{
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
        ui->toolButtonFigure->setChecked(true);
    }
}

void DAFigureSettingWidget::setFigure(DAFigureWidget* fig, DAChartWidget* chart)
{
	setFigure(fig);
	if (!ui->toolButtonChart->isChecked()) {
		ui->toolButtonChart->setChecked(true);
	}
	setCurrentChart(chart);
}

void DAFigureSettingWidget::setFigure(DAFigureWidget* fig, DAChartWidget* chart, QwtPlotItem* item)
{
    setFigure(fig, chart);
    if (!ui->toolButtonItem->isChecked()) {
        ui->toolButtonItem->setChecked(true);
    }
}

DAFigureWidget* DAFigureSettingWidget::getFigure() const
{
	return d_ptr->mFigure.data();
}

DAChartWidget* DAFigureSettingWidget::getChart() const
{
	return d_ptr->mChart.data();
}

QwtPlotItem* DAFigureSettingWidget::getItem() const
{
	return d_ptr->mItem;
}

void DAFigureSettingWidget::bindFigure(DAFigureWidget* fig)
{
	if (d_ptr->mFigure == fig) {
		return;
	}
	d_ptr->mFigure = fig;
	ui->comboBoxSelectChart->clear();
	if (fig) {
		connect(fig, &DAFigureWidget::chartAdded, this, &DAFigureSettingWidget::onChartAdded);
		connect(fig, &DAFigureWidget::chartWillRemove, this, &DAFigureSettingWidget::onChartWillRemove);
		connect(fig, &DAFigureWidget::currentChartChanged, this, &DAFigureSettingWidget::onCurrentChartChanged);
		// 更新chart信息
		const auto charts = fig->getCharts();
		for (auto chart : charts) {
			// 绑定chart的信号槽
			connect(chart, &DAChartWidget::itemAttached, this, &DAFigureSettingWidget::onItemAttached);
		}
        setChartsComboBox(fig);
	}
}

void DAFigureSettingWidget::unbindFigure(DAFigureWidget* fig)
{
	if (fig) {
		disconnect(fig, &DAFigureWidget::chartAdded, this, &DAFigureSettingWidget::onChartAdded);
		disconnect(fig, &DAFigureWidget::chartWillRemove, this, &DAFigureSettingWidget::onChartWillRemove);
		disconnect(fig, &DAFigureWidget::currentChartChanged, this, &DAFigureSettingWidget::onCurrentChartChanged);
		// 更新chart信息
		const auto charts = fig->getCharts();
		for (auto chart : charts) {
			// 解除绑定chart的信号槽
			disconnect(chart, &DAChartWidget::itemAttached, this, &DAFigureSettingWidget::onItemAttached);
		}
	}
}

/**
 * @brief 获取chart在combobox的索引
 * @param chart
 * @return
 */
int DAFigureSettingWidget::indexOfChart(const DAChartWidget* chart)
{
	int c = ui->comboBoxSelectItem->count();
	for (int i = 0; i < c; ++i) {
		DAChartWidget* savePtr = ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
		if (chart == savePtr) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief 设置当前的chart
 * @param chart
 */
void DAFigureSettingWidget::setCurrentChart(DAChartWidget* chart)
{
	int index = indexOfChart(chart);
    ui->comboBoxSelectChart->setCurrentIndex(index);
}

/**
 * @brief 设置当前的item
 * @param item
 */
void DAFigureSettingWidget::setCurrentItem(QwtPlotItem* item)
{
}

void DAFigureSettingWidget::changeEvent(QEvent* e)
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

void DAFigureSettingWidget::setChartsComboBox(DAFigureWidget* fig)
{
    QVariant curData           = ui->comboBoxSelectChart->currentData();
    DAChartWidget* oldselChart = nullptr;
    if (curData.isValid()) {
        oldselChart = curData.value< DAChartWidget* >();
    }
    const auto cs = fig->getCharts();
    ui->comboBoxSelectChart->clear();
    for (DAChartWidget* c : cs) {
        ui->comboBoxSelectChart->addItem(DAChartWidgetStandardItem::getChartTitle(fig, c), QVariant::fromValue(c));
    }
    // 默认选中
    if (oldselChart) {
        for (int i = 0; i < ui->comboBoxSelectChart->count(); ++i) {
            auto c = ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
            if (c == oldselChart) {
                // 选择项不变
                QSignalBlocker b(ui->comboBoxSelectChart);
                ui->comboBoxSelectChart->setCurrentIndex(i);
                return;
            }
        }
    }
    if (ui->comboBoxSelectChart->count() > 0) {
        ui->comboBoxSelectChart->setCurrentIndex(0);
    }
}

void DAFigureSettingWidget::setItemsComboBox(const QList< QwtPlotItem* >& its)
{
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
                QSignalBlocker b(ui->comboBoxSelectItem);
                ui->comboBoxSelectItem->setCurrentIndex(i);
                return;
            }
        }
    }
    if (ui->comboBoxSelectItem->count() > 0) {
        ui->comboBoxSelectItem->setCurrentIndex(0);
    }
}

void DAFigureSettingWidget::setItemsComboBox(DAChartWidget* chart)
{
    const auto items = chart->itemList();
    setItemsComboBox(items);
}

void DAFigureSettingWidget::onFigureCloseing(DAFigureWidget* f)
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

void DAFigureSettingWidget::onFigureCreated(DAFigureWidget* f)
{
    if (f) {
        DAChartWidget* c = f->getCurrentChart();
        setFigure(f, c);
    }
}

void DAFigureSettingWidget::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
    Q_UNUSED(index);
    if (f) {
        DAChartWidget* c = f->getCurrentChart();
        setFigure(f, c);
    }
}

void DAFigureSettingWidget::onChartAdded(DAChartWidget* c)
{
    if (d_ptr->mFigure) {
        setChartsComboBox(d_ptr->mFigure);
    }
}

void DAFigureSettingWidget::onChartWillRemove(DAChartWidget* c)
{
}

void DAFigureSettingWidget::onCurrentChartChanged(DAChartWidget* c)
{
    if (d_ptr->mFigure) {
        setChartsComboBox(d_ptr->mFigure);
    }
}

void DAFigureSettingWidget::onItemAttached(QwtPlotItem* plotItem, bool on)
{
	DAChartWidget* c = qobject_cast< DAChartWidget* >(sender());
	if (c) {
		onChartItemAttached(c, plotItem, on);
	}
}

void DAFigureSettingWidget::onChartItemAttached(DAChartWidget* c, QwtPlotItem* plotItem, bool on)
{
    if (c) {
        setItemsComboBox(c);
    }
}

/**
 * @brief DAFigureSettingWidget::onComboBoxChartIndexChanged
 * @param i
 */
void DAFigureSettingWidget::onComboBoxChartIndexChanged(int i)
{
    // 更新items
    if (!d_ptr->mFigure) {
        return;
    }
    auto charts = d_ptr->mFigure->getCharts();
    if (i < 0 || i >= charts.size()) {
        return;
    }
    // 更新当前chart
    d_ptr->mChart = charts[ i ];
    // 如果当前chart不是选定的chart，则把当前chart设置为选定的chart
    auto cc = d_ptr->mFigure->getCurrentChart();
    if (cc != d_ptr->mChart) {
        d_ptr->mFigure->setCurrentChart(d_ptr->mChart);
    }
    // 获取chart的元素
    setItemsComboBox(d_ptr->mChart);
    //
    ui->widgetChartSetting->setChartWidget(d_ptr->mChart);
}

void DAFigureSettingWidget::onComboBoxItemIndexChanged(int i)
{
    if (!d_ptr->mChart) {
        return;
    }
    auto items = d_ptr->mChart->itemList();
    if (i < 0 || i >= items.size()) {
        return;
    }
    d_ptr->mItem = items[ i ];
    // 按照item类型显示
}

/**
 * @brief 按钮组点击
 * @param id
 */
void DAFigureSettingWidget::onButtonGroupTypeButtonClicked(int id)
{
    switch (id) {
    case SettingFigure:
        ui->stackedWidget->setCurrentWidget(ui->pageFigureSet);
        break;
    case SettingChart:
        ui->stackedWidget->setCurrentWidget(ui->widgetChartSetting);
        break;
    case SettingItem:
        ui->stackedWidget->setCurrentWidget(ui->widgetItemSetting);
        break;
    }
}

}  // end da
