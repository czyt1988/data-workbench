#include "DADialogChartGuide.h"
#include "ui_DADialogChartGuide.h"
#include "DADataManager.h"
#include "DAAbstractChartAddItemWidget.h"
#include <iterator>
#include <vector>
// qwt
#include "qwt_plot_curve.h"
namespace DA
{

DADialogChartGuide::DADialogChartGuide(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogChartGuide), _dataMgr(nullptr)
{
    ui->setupUi(this);
    init();
    connect(ui->comboBoxDataFrame, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogChartGuide::onComboBoxCurrentIndexChanged);
    connect(ui->listWidgetChartType, &QListWidget::itemChanged, this, &DADialogChartGuide::onListWidgetItemChanged);
}

DADialogChartGuide::~DADialogChartGuide()
{
    delete ui;
}

void DADialogChartGuide::init()
{
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/gui/chart-type/icon/chart-type/chart-curve.svg"), tr("curve"));
    item->setData(Qt::UserRole, (int)ChartCurve);
    ui->listWidgetChartType->addItem(item);
    item = new QListWidgetItem(QIcon(":/gui/chart-type/icon/chart-type/chart-scatter.svg"), tr("scatter"));
    item->setData(Qt::UserRole, (int)ChartScatter);
    ui->listWidgetChartType->addItem(item);
    //初始化
    ui->stackedWidget->setCurrentWidget(ui->pageCurve);
    ui->listWidgetChartType->setCurrentRow(0);
}
/**
 * @brief 设置datamanager,会把combox填入所有的dataframe
 * @param dmgr
 */
void DADialogChartGuide::setDataManager(DADataManager* dmgr)
{
    _dataMgr = dmgr;
    ui->pageCurve->setDataManager(dmgr);
    resetDataframeCombobox();
    updateCurrentPageData();
}

/**
 * @brief 设置当前的数据
 * @param d
 */
void DADialogChartGuide::setCurrentData(const DAData& d)
{
    _currentData = d;
    updateData();
    updateCurrentPageData();
}

DAData DADialogChartGuide::getCurrentData() const
{
    return _currentData;
}

/**
 * @brief 获取当前的绘图类型
 * @return
 */
DADialogChartGuide::ChartType DADialogChartGuide::getCurrentChartType() const
{
    QListWidgetItem* item = ui->listWidgetChartType->currentItem();
    if (item == nullptr) {
        return UnknowChartType;
    }
    return static_cast< ChartType >(item->data(Qt::UserRole).toInt());
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
    //一些基本属性的设置
    switch (getCurrentChartType()) {
    case ChartScatter: {
        QwtPlotCurve* c = static_cast< QwtPlotCurve* >(item);
        c->setStyle(QwtPlotCurve::Dots);
    } break;
    default:
        break;
    }
    return item;
}

void DADialogChartGuide::updateData()
{
    updateDataframeComboboxSelect();
    updateCurrentPageData();
}

/**
 * @brief 刷新dataframe combobox
 */
void DADialogChartGuide::resetDataframeCombobox()
{
    if (nullptr == _dataMgr) {
        return;
    }
    ui->comboBoxDataFrame->clear();
    int c = _dataMgr->getDataCount();
    for (int i = 0; i < c; ++i) {
        DAData d = _dataMgr->getData(i);
        if (d.isNull() || !d.isDataFrame()) {
            continue;
        }
        DAPyDataFrame df = d.toDataFrame();
        if (df.isNone()) {
            continue;
        }
        // id作为data
        ui->comboBoxDataFrame->addItem(d.getName(), d.id());
    }
    ui->comboBoxDataFrame->setCurrentIndex(-1);  //不选中
}

/**
 * @brief 更新combobox的选中状态，但不会触发currentIndexChanged信号
 */
void DADialogChartGuide::updateDataframeComboboxSelect()
{
    if (nullptr == _dataMgr) {
        return;
    }
    int index = _dataMgr->getDataIndex(_currentData);
    if (index < 0) {
        return;
    }
    QSignalBlocker b(ui->comboBoxDataFrame);
    Q_UNUSED(b);
    ui->comboBoxDataFrame->setCurrentIndex(index);
}

/**
 * @brief 刷新x，y两个列选择listwidget
 */
void DADialogChartGuide::updateCurrentPageData()
{
}

void DADialogChartGuide::onComboBoxCurrentIndexChanged(int i)
{
    if (nullptr == _dataMgr || i < 0) {
        return;
    }
    DAData d = _dataMgr->getData(i);
    setCurrentData(d);
    updateCurrentPageData();
}

void DADialogChartGuide::onListWidgetItemChanged(QListWidgetItem* item)
{
    ChartType ct = static_cast< ChartType >(item->data(Qt::UserRole).toInt());
    switch (ct) {
    case ChartCurve:
        ui->stackedWidget->setCurrentWidget(ui->pageCurve);
        break;
    case ChartScatter:
        ui->stackedWidget->setCurrentWidget(ui->pageCurve);
        break;
    default:
        break;
    }
}

}
