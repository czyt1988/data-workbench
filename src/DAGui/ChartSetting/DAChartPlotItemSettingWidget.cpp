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
void DAChartPlotItemSettingWidget::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    QSignalBlocker b1(ui->lineEditTitle), b2(ui->doubleSpinBoxZ);
    ui->lineEditTitle->setText(item->title().text());
    ui->doubleSpinBoxZ->setValue(item->z());
    updateAxis(item);
}

void DAChartPlotItemSettingWidget::applySetting(QwtPlotItem* item)
{
    QPair< int, int > axisids = getAxisIDs();
    item->setAxes(static_cast< QwtAxisId >(axisids.first), static_cast< QwtAxisId >(axisids.second));
    item->setTitle(ui->lineEditTitle->text());
    item->setZ(ui->doubleSpinBoxZ->value());
    replot();
}

/**
 * @brief 更新坐标轴的设置
 */
void DAChartPlotItemSettingWidget::updateAxis(const QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
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
        qDebug() << "plot item get unknow axis set:x=" << item->xAxis() << ",y=" << item->yAxis();
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

/**
 * @brief 获取axis信息
 * @return
 */
QPair< int, int > DAChartPlotItemSettingWidget::getAxisIDs() const
{
    if (ui->toolButtonLeftBottom->isChecked()) {
        return qMakePair(QwtPlot::xBottom, QwtPlot::yLeft);
    } else if (ui->toolButtonLeftTop->isChecked()) {
        return qMakePair(QwtPlot::xTop, QwtPlot::yLeft);
    } else if (ui->toolButtonRightBottom->isChecked()) {
        return qMakePair(QwtPlot::xBottom, QwtPlot::yRight);
    } else if (ui->toolButtonRightTop->isChecked()) {
        return qMakePair(QwtPlot::xTop, QwtPlot::yRight);
    }
    return qMakePair(QwtPlot::xBottom, QwtPlot::yLeft);
}

void DAChartPlotItemSettingWidget::onItemTitleEditingFinished()
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    auto item = getPlotItem();
    item->setTitle(ui->lineEditTitle->text());
    replot();
}

void DAChartPlotItemSettingWidget::onItemZValueChanged(double z)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    auto item = getPlotItem();
    item->setZ(z);
    replot();
}

void DAChartPlotItemSettingWidget::onButtonGroupAxisClicked(QAbstractButton* btn)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    auto item = getPlotItem();
    if (btn == ui->toolButtonLeftBottom) {
        item->setAxes(QwtPlot::xBottom, QwtPlot::yLeft);
    } else if (btn == ui->toolButtonLeftTop) {
        item->setAxes(QwtPlot::xTop, QwtPlot::yLeft);
    } else if (btn == ui->toolButtonRightBottom) {
        item->setAxes(QwtPlot::xBottom, QwtPlot::yRight);
    } else if (btn == ui->toolButtonRightTop) {
        item->setAxes(QwtPlot::xTop, QwtPlot::yRight);
    }
    replot();
}
}
