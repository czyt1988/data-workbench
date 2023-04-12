#include "DADialogDataframePlot.h"
#include "ui_DADialogDataframePlot.h"
#include "DADataManager.h"
#include <iterator>
#include <vector>
namespace DA
{

DADialogDataframePlot::DADialogDataframePlot(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataframePlot), _dataMgr(nullptr)
{
    ui->setupUi(this);
    init();
    connect(ui->comboBoxDataFrame, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogDataframePlot::onComboBoxCurrentIndexChanged);
    connect(ui->listWidgetChartType, &QListWidget::itemChanged, this, &DADialogDataframePlot::onListWidgetItemChanged);
}

DADialogDataframePlot::~DADialogDataframePlot()
{
    delete ui;
}

void DADialogDataframePlot::init()
{
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/gui/chart-type/icon/chart-type/chart-curve.svg"), tr("curve"));
    item->setData(Qt::UserRole, (int)ChartCurve);
    ui->listWidgetChartType->addItem(item);
    item = new QListWidgetItem(QIcon(":/gui/chart-type/icon/chart-type/chart-scatter.svg"), tr("scatter"));
    item->setData(Qt::UserRole, (int)ChartScatter);
    ui->listWidgetChartType->addItem(item);

    ui->stackedWidget->setCurrentWidget(ui->pageCurve);
    ui->listWidgetChartType->setCurrentRow(0);
}
/**
 * @brief 设置datamanager,会把combox填入所有的dataframe
 * @param dmgr
 */
void DADialogDataframePlot::setDataManager(DADataManager* dmgr)
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
void DADialogDataframePlot::setCurrentData(const DAData& d)
{
    _currentData = d;
    updateData();
    updateCurrentPageData();
}

DAData DADialogDataframePlot::getCurrentData() const
{
    return _currentData;
}

/**
 * @brief 获取为vector pointf
 * @return 成功返回true
 */
bool DADialogDataframePlot::getToVectorPointF(QVector< QPointF >& res)
{
    if (!_currentData.isDataFrame() || _currentData.isNull()) {
        return false;
    }
    return ui->pageCurve->getToVectorPointF(res);
}

/**
 * @brief 获取当前的绘图类型
 * @return
 */
DADialogDataframePlot::ChartType DADialogDataframePlot::getCurrentChartType() const
{
    QListWidgetItem* item = ui->listWidgetChartType->currentItem();
    if (item == nullptr) {
        return UnknowChartType;
    }
    return static_cast< ChartType >(item->data(Qt::UserRole).toInt());
}

void DADialogDataframePlot::updateData()
{
    updateDataframeComboboxSelect();
    updateCurrentPageData();
}

/**
 * @brief 刷新dataframe combobox
 */
void DADialogDataframePlot::resetDataframeCombobox()
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
void DADialogDataframePlot::updateDataframeComboboxSelect()
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
void DADialogDataframePlot::updateCurrentPageData()
{
}

void DADialogDataframePlot::onComboBoxCurrentIndexChanged(int i)
{
    if (nullptr == _dataMgr || i < 0) {
        return;
    }
    DAData d = _dataMgr->getData(i);
    setCurrentData(d);
    updateCurrentPageData();
}

void DADialogDataframePlot::onListWidgetItemChanged(QListWidgetItem* item)
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
