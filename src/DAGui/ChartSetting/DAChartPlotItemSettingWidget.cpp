#include "DAChartPlotItemSettingWidget.h"
#include "ui_DAChartPlotItemSettingWidget.h"
#include <QDebug>
#include "qwt_plot_item.h"
#include "qwt_text.h"
#include "qwt_plot.h"
#define DAChartPlotItemSettingWidget_CheckItem()                                                                       \
    do {                                                                                                               \
        if (nullptr == mItem) {                                                                                        \
            qDebug() << "DAChartPlotItemSettingWidget's plot item is null";                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

namespace DA
{

DAChartPlotItemSettingWidget::DAChartPlotItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartPlotItemSettingWidget)
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
    mItem = item;
    // 如果item有plot，则把plot设置进来，plot可以知道item是否被delete
    QwtPlot* oldPlot = mPlot.data();
    QwtPlot* newPlot = nullptr;
    if (item) {
        newPlot = item->plot();
    }
    if (oldPlot == newPlot) {
        return;
    }
    if (oldPlot) {
        disconnect(oldPlot, &QwtPlot::itemAttached, this, &DAChartPlotItemSettingWidget::onPlotItemAttached);
    }
    if (newPlot) {
        connect(mPlot.data(), &QwtPlot::itemAttached, this, &DAChartPlotItemSettingWidget::onPlotItemAttached);
    }
}

/**
 * @brief 获取item
 * @return
 */
QwtPlotItem* DAChartPlotItemSettingWidget::getPlotItem() const
{
    return mItem;
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
void DAChartPlotItemSettingWidget::updateUI()
{
    DAChartPlotItemSettingWidget_CheckItem();
    QSignalBlocker b1(ui->lineEditTitle), b2(ui->doubleSpinBoxZ);
    ui->lineEditTitle->setText(mItem->title().text());
    ui->doubleSpinBoxZ->setValue(mItem->z());
    updateAxis();
}

/**
 * @brief 更新坐标轴的设置
 */
void DAChartPlotItemSettingWidget::updateAxis()
{
    DAChartPlotItemSettingWidget_CheckItem();
    QSignalBlocker b(ui->buttonGroupAxis);
    Q_UNUSED(b);
    if (QwtPlot::yLeft == mItem->yAxis() && QwtPlot::xBottom == mItem->xAxis()) {
        ui->toolButtonLeftBottom->setChecked(true);
    } else if (QwtPlot::yLeft == mItem->yAxis() && QwtPlot::xTop == mItem->xAxis()) {
        ui->toolButtonLeftTop->setChecked(true);
    } else if (QwtPlot::yRight == mItem->yAxis() && QwtPlot::xBottom == mItem->xAxis()) {
        ui->toolButtonRightBottom->setChecked(true);
    } else if (QwtPlot::yRight == mItem->yAxis() && QwtPlot::xTop == mItem->xAxis()) {
        ui->toolButtonRightTop->setChecked(true);
    } else {
        qDebug() << "plot item get unknow axis set:x=" << mItem->xAxis() << ",y=" << mItem->yAxis();
    }
}

void DAChartPlotItemSettingWidget::onItemTitleEditingFinished()
{
    DAChartPlotItemSettingWidget_CheckItem();
    mItem->setTitle(ui->lineEditTitle->text());
}

void DAChartPlotItemSettingWidget::onItemZValueChanged(double z)
{
    DAChartPlotItemSettingWidget_CheckItem();
    mItem->setZ(z);
}

void DAChartPlotItemSettingWidget::onPlotItemAttached(QwtPlotItem* plotItem, bool on)
{
    if (!on && plotItem == mItem) {
        // item脱离plot，有可能会被delete
    }
}

void DAChartPlotItemSettingWidget::onButtonGroupAxisClicked(QAbstractButton* btn)
{
    DAChartPlotItemSettingWidget_CheckItem();
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
