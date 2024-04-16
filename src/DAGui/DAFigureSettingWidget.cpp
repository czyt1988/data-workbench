#include "DAFigureSettingWidget.h"
#include "ui_DAFigureSettingWidget.h"
#include <QPointer>
#include "Models/DAFigureTreeModel.h"
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
}

DAFigureSettingWidget::~DAFigureSettingWidget()
{
	delete ui;
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
			ui->comboBoxSelectChart->addItem(DAChartWidgetStandardItem::getChartTitle(fig, chart),
											 QVariant::fromValue(chart));
			// 绑定chart的信号槽
			connect(chart, &DAChartWidget::itemAttached, this, &DAFigureSettingWidget::onItemAttached);
		}
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

void DAFigureSettingWidget::onChartAdded(DAChartWidget* c)
{
}

void DAFigureSettingWidget::onChartWillRemove(DAChartWidget* c)
{
}

void DAFigureSettingWidget::onCurrentChartChanged(DAChartWidget* c)
{
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
}

}  // end da
