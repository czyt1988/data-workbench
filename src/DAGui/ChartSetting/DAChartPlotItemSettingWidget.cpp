#include "DAChartPlotItemSettingWidget.h"
#include "ui_DAChartPlotItemSettingWidget.h"
#include <QActionGroup>
#include <QDebug>
#include "qwt_plot_item.h"
#include "qwt_text.h"
#include "qwt_plot.h"
#define DAChartPlotItemSettingWidget_CheckItem()                                                                       \
    do {                                                                                                               \
        if (nullptr == _item) {                                                                                        \
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
    _actionGroupAxis = new QActionGroup(this);
    _actionAxisLeftBottom = _actionGroupAxis->addAction(QIcon(":/gui/chart-setting/icon/axisLeftBottom.svg"), tr("Left Bottom"));
    _actionAxisLeftBottom->setCheckable(true);
    _actionAxisLeftTop = _actionGroupAxis->addAction(QIcon(":/gui/chart-setting/icon/axisLeftTop.svg"), tr("Left Top"));
    _actionAxisLeftTop->setCheckable(true);
    _actionAxisRightBottom = _actionGroupAxis->addAction(QIcon(":/gui/chart-setting/icon/axisRightBottom.svg"), tr("Right Bottom"));
    _actionAxisRightBottom->setCheckable(true);
    _actionAxisRightTop = _actionGroupAxis->addAction(QIcon(":/gui/chart-setting/icon/axisRightTop.svg"), tr("Right Top"));
    _actionAxisRightTop->setCheckable(true);
    ui->toolButtonLeftBottom->setDefaultAction(_actionAxisLeftBottom);
    ui->toolButtonLeftTop->setDefaultAction(_actionAxisLeftTop);
    ui->toolButtonRightBottom->setDefaultAction(_actionAxisRightBottom);
    ui->toolButtonRightTop->setDefaultAction(_actionAxisRightTop);
    connect(_actionGroupAxis, &QActionGroup::triggered, this, &DAChartPlotItemSettingWidget::onActionGroupAxisTriggered);
    connect(ui->lineEditTitle, &QLineEdit::editingFinished, this, &DAChartPlotItemSettingWidget::onItemTitleEditingFinished);
    connect(ui->doubleSpinBoxZ, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, &DAChartPlotItemSettingWidget::onItemZValueChanged);
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
    _item = item;
    //如果item有plot，则把plot设置进来，plot可以知道item是否被delete
    QwtPlot* oldPlot = _plot.data();
    _plot            = item->plot();
    if (_plot) {
        connect(_plot.data(), &QwtPlot::itemAttached, this, &DAChartPlotItemSettingWidget::onPlotItemAttached);
    } else if (oldPlot) {
        disconnect(oldPlot, &QwtPlot::itemAttached, this, &DAChartPlotItemSettingWidget::onPlotItemAttached);
    }
}

/**
 * @brief 获取item
 * @return
 */
QwtPlotItem* DAChartPlotItemSettingWidget::getPlotItem() const
{
    return _item;
}

/**
 * @brief 清除所有信息和内容
 */
void DAChartPlotItemSettingWidget::clear()
{
    _item = nullptr;
    _plot = nullptr;
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
    ui->lineEditTitle->setText(_item->title().text());
    ui->doubleSpinBoxZ->setValue(_item->z());
    updateAxis();
}

/**
 * @brief 更新坐标轴的设置
 */
void DAChartPlotItemSettingWidget::updateAxis()
{
    DAChartPlotItemSettingWidget_CheckItem();
    QSignalBlocker b1(_actionGroupAxis);
    if (QwtPlot::yLeft == _item->yAxis() && QwtPlot::xBottom == _item->xAxis()) {
        _actionAxisLeftBottom->setChecked(true);
    } else if (QwtPlot::yLeft == _item->yAxis() && QwtPlot::xTop == _item->xAxis()) {
        _actionAxisLeftTop->setChecked(true);
    } else if (QwtPlot::yRight == _item->yAxis() && QwtPlot::xBottom == _item->xAxis()) {
        _actionAxisRightBottom->setChecked(true);
    } else if (QwtPlot::yRight == _item->yAxis() && QwtPlot::xTop == _item->xAxis()) {
        _actionAxisRightTop->setChecked(true);
    } else {
        qDebug() << "plot item get unknow axis set:x=" << _item->xAxis() << ",y=" << _item->yAxis();
    }
}

void DAChartPlotItemSettingWidget::onItemTitleEditingFinished()
{
    DAChartPlotItemSettingWidget_CheckItem();
    _item->setTitle(ui->lineEditTitle->text());
}

void DAChartPlotItemSettingWidget::onItemZValueChanged(double z)
{
    DAChartPlotItemSettingWidget_CheckItem();
    _item->setZ(z);
}

void DAChartPlotItemSettingWidget::onPlotItemAttached(QwtPlotItem* plotItem, bool on)
{
    if (!on && plotItem == _item) {
        // item脱离plot，有可能会被delete
    }
}

void DAChartPlotItemSettingWidget::onActionGroupAxisTriggered(QAction* act)
{
    DAChartPlotItemSettingWidget_CheckItem();
    if (act = _actionAxisLeftBottom) {
        _item->setAxes(QwtPlot::xBottom, QwtPlot::yLeft);
    } else if (act == _actionAxisLeftTop) {
        _item->setAxes(QwtPlot::xTop, QwtPlot::yLeft);
    } else if (act == _actionAxisRightBottom) {
        _item->setAxes(QwtPlot::xBottom, QwtPlot::yRight);
    } else if (act == _actionAxisRightTop) {
        _item->setAxes(QwtPlot::xTop, QwtPlot::yRight);
    }
}
}
