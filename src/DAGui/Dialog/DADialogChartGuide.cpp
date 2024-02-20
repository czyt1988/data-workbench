#include "DADialogChartGuide.h"
#include "ui_DADialogChartGuide.h"
#include "DADataManager.h"
#include "DAAbstractChartAddItemWidget.h"
#include "DAChartAddCurveWidget.h"
#include <iterator>
#include <vector>
// qwt
#include "qwt_plot_curve.h"
namespace DA
{

DADialogChartGuide::DADialogChartGuide(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogChartGuide)
{
    ui->setupUi(this);
    init();

    connect(ui->listWidgetChartType, &QListWidget::currentItemChanged, this, &DADialogChartGuide::onListWidgetCurrentItemChanged);
}

DADialogChartGuide::~DADialogChartGuide()
{
    delete ui;
}

void DADialogChartGuide::init()
{
    QListWidgetItem* item = nullptr;
    // curve
    item = new QListWidgetItem(QIcon(":/gui/chart-type/icon/chart-type/chart-curve.svg"), tr("curve"));
    item->setData(Qt::UserRole, static_cast< int >(DA::ChartTypes::Curve));
    ui->listWidgetChartType->addItem(item);
    // scatter
    item = new QListWidgetItem(QIcon(":/gui/chart-type/icon/chart-type/chart-scatter.svg"), tr("scatter"));
    item->setData(Qt::UserRole, static_cast< int >(DA::ChartTypes::Scatter));
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
    }
}

DAData DADialogChartGuide::getCurrentData() const
{
    QWidget* w = ui->stackedWidget->currentWidget();
    if (DAChartAddCurveWidget* c = qobject_cast< DAChartAddCurveWidget* >(w)) {
        return c->getCurrentData();
    }
    return DAData();
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
    DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->currentWidget());
    if (!w) {
        return;
    }
    w->updateData();
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
    default:
        break;
    }
}
}
