#include "DAChartPlotItemSettingWidget.h"
#include "ui_DAChartPlotItemSettingWidget.h"
#include <QDebug>
#include "qwt_plot_item.h"
#include "qwt_text.h"
#include "qwt_plot.h"

namespace DA
{

DAChartPlotItemSettingWidget::DAChartPlotItemSettingWidget(QWidget* parent)
	: DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartPlotItemSettingWidget)
{
	ui->setupUi(this);
	connect(ui->buttonGroupAxis,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartPlotItemSettingWidget::onButtonGroupAxisClicked);
	connect(ui->lineEditTitle, &QLineEdit::editingFinished, this, &DAChartPlotItemSettingWidget::onItemTitleEditingFinished);
	connect(ui->doubleSpinBoxZ,
			QOverload< double >::of(&QDoubleSpinBox::valueChanged),
			this,
			&DAChartPlotItemSettingWidget::onItemZValueChanged);
}

DAChartPlotItemSettingWidget::~DAChartPlotItemSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置item
 * @param item
 */
void DAChartPlotItemSettingWidget::setPlotItem(QwtPlotItem* item)
{
	DAAbstractChartItemSettingWidget::setPlotItem(item);
	updateUI(item);
}

/**
 * @brief 清除所有信息和内容
 */
void DAChartPlotItemSettingWidget::clear()
{
	setPlotItem(nullptr);

	QSignalBlocker b1(ui->lineEditTitle), b2(ui->doubleSpinBoxZ);
	ui->lineEditTitle->clear();
	ui->doubleSpinBoxZ->setValue(0);
}

/**
 * @brief 刷新ui
 * 此函数不会触发信号
 */
void DAChartPlotItemSettingWidget::updateUI(const QwtPlotItem* item)
{
	QSignalBlocker b1(ui->lineEditTitle), b2(ui->doubleSpinBoxZ);
	ui->lineEditTitle->setText(item->title().text());
	ui->doubleSpinBoxZ->setValue(item->z());
	updateAxis(item);
}

/**
 * @brief 更新坐标轴的设置
 */
void DAChartPlotItemSettingWidget::updateAxis(const QwtPlotItem* item)
{
	QSignalBlocker b(ui->buttonGroupAxis);
	Q_UNUSED(b);
	if (QwtPlot::yLeft == item->yAxis() && QwtPlot::xBottom == item->xAxis()) {
		ui->toolButtonLeftBottom->setChecked(true);
	} else if (QwtPlot::yLeft == item->yAxis() && QwtPlot::xTop == item->xAxis()) {
		ui->toolButtonLeftTop->setChecked(true);
	} else if (QwtPlot::yRight == item->yAxis() && QwtPlot::xBottom == item->xAxis()) {
		ui->toolButtonRightBottom->setChecked(true);
	} else if (QwtPlot::yRight == item->yAxis() && QwtPlot::xTop == item->xAxis()) {
		ui->toolButtonRightTop->setChecked(true);
	} else {
		qDebug() << "plot item get unknow axis set:x=" << mItem->xAxis() << ",y=" << mItem->yAxis();
	}
}

/**
 * @brief 设置标题
 * @param t
 */
void DAChartPlotItemSettingWidget::setItemTitle(const QString& t)
{
    ui->lineEditTitle->setText(t);
}

/**
 * @brief 标题
 * @return
 */
QString DAChartPlotItemSettingWidget::getItemTitle() const
{
    return ui->lineEditTitle->text();
}

void DAChartPlotItemSettingWidget::onItemTitleEditingFinished()
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	auto item = getPlotItem();
	item->setTitle(ui->lineEditTitle->text());
}

void DAChartPlotItemSettingWidget::onItemZValueChanged(double z)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	auto item = getPlotItem();
	item->setZ(z);
}

void DAChartPlotItemSettingWidget::onButtonGroupAxisClicked(QAbstractButton* btn)
{
	DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
	if (btn == ui->toolButtonLeftBottom) {
		mItem->setAxes(QwtPlot::xBottom, QwtPlot::yLeft);
	} else if (btn == ui->toolButtonLeftTop) {
		mItem->setAxes(QwtPlot::xTop, QwtPlot::yLeft);
	} else if (btn == ui->toolButtonRightBottom) {
		mItem->setAxes(QwtPlot::xBottom, QwtPlot::yRight);
	} else if (btn == ui->toolButtonRightTop) {
		mItem->setAxes(QwtPlot::xTop, QwtPlot::yRight);
	}
}
}
