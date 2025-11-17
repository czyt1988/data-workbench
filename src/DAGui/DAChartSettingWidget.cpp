#include "DAChartSettingWidget.h"
#include "ui_DAChartSettingWidget.h"
#include <QPointer>
#include <QScrollArea>
#include "Models/DAFigureTreeModel.h"
#include "DAChartOperateWidget.h"
#include "DAChartPlotSettingWidget.h"
#include <QDebug>
#include "DASignalBlockers.hpp"

#ifndef DAChartSettingWidget_DEBUG_PRINT
#define DAChartSettingWidget_DEBUG_PRINT 1
#endif

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
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartSettingWidget::onComboBoxChartIndexChanged);
    connect(ui->comboBoxSelectItem,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartSettingWidget::onComboBoxItemIndexChanged);
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
        unbindChartOprateWidget(d_ptr->mChartOpt.data());
    }
    d_ptr->mChartOpt = opt;
    if (opt) {
        bindChartOprateWidget(opt);
    }
    // 设置当前的figure
    if (opt) {
        setFigure(opt->getCurrentFigure());
    }
}

/**
 * @brief 获取当前正在管理的@ref DAChartOperateWidget
 * @return 如果当前没有管理，返回nullptr
 */
DAChartOperateWidget* DAChartSettingWidget::getChartOprateWidget() const
{
    return d_ptr->mChartOpt.data();
}

/**
 * @brief 设置绘图窗口
 * @param fig
 */
void DAChartSettingWidget::setFigure(DAFigureWidget* fig)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::setFigure" << quintptr(fig);
#endif
    DAFigureWidget* oldfig = getFigure();
    // 避免递归绑定
    if (oldfig == fig) {
        return;
    }
    if (oldfig) {
        unbindFigure(oldfig);
    }
    ui->widgetFigureSetting->setFigure(fig);
    bindFigure(fig);
    // 更新chart combox
    resetChartsComboBox();
    // 更新chart
    if (fig) {
        setChart(fig->getCurrentChart());
    }
}

DAFigureWidget* DAChartSettingWidget::getFigure() const
{
    return ui->widgetFigureSetting->getFigure();
}

void DAChartSettingWidget::setChart(DAChartWidget* chart)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::setChart:" << quintptr(chart);
#endif
    DAChartWidget* oldChart = getChart();
    if (oldChart == chart) {
        return;
    }
    if (oldChart) {
        unbindChart(oldChart);
    }
    ui->widgetChartSetting->setChartWidget(chart);
    bindChart(chart);
    // 更新chart 和 items 的combox
    resetItemsComboBox(chart);
    // 这里一定要把信号锁起来，否则会递归调用
    DASignalBlockers b(ui->comboBoxSelectChart);
    if (!chart) {
        ui->comboBoxSelectChart->setCurrentIndex(-1);
        ui->comboBoxSelectItem->setCurrentIndex(-1);
        return;
    }
    // 找到索引
    int index = indexOfChart(chart);
    // 此函数会发射currentIndexChanged信号，->onComboBoxChartIndexChanged->setChart
    // 因此一定要把ui->comboBoxSelectChart的信号锁起，否则会递归调用
    if (ui->comboBoxSelectChart->currentIndex() != index) {
        ui->comboBoxSelectChart->setCurrentIndex(index);
    }
    // 把item也改变，否则会停留在之前的item
    if (ui->comboBoxSelectItem->count() > 0) {
        ui->comboBoxSelectItem->setCurrentIndex(0);  // 选中第一个
    }
}

void DAChartSettingWidget::setPlotItem(QwtPlotItem* item)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::setPlotItem" << quintptr(item);
#endif
    ui->widgetItemSetting->setPlotItem(item);
    // 更新combobox，信号已经锁起，避免重复触发comboboxchanged信号，导致递归调用
    QSignalBlocker b(ui->comboBoxSelectItem);

    if (!item) {
        ui->comboBoxSelectItem->setCurrentIndex(-1);
        return;
    }
    int index = indexOfItem(item);
    if (ui->comboBoxSelectItem->currentIndex() != index) {
        ui->comboBoxSelectItem->setCurrentIndex(index);
    }
}

DAChartWidget* DAChartSettingWidget::getChart() const
{
    return ui->widgetChartSetting->getChartWidget();
}

QwtPlotItem* DAChartSettingWidget::getPlotItem() const
{
    return ui->widgetItemSetting->getPlotItem();
}

void DAChartSettingWidget::bindFigure(DAFigureWidget* fig)
{
    if (fig) {
        connect(fig, &DAFigureWidget::chartAdded, this, &DAChartSettingWidget::onChartAdded);
        connect(fig, &DAFigureWidget::chartRemoved, this, &DAChartSettingWidget::onChartRemoved);
        connect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartSettingWidget::onCurrentChartChanged);
    }
}

/**
 * @brief 解绑figure相关信号
 * @param fig
 */
void DAChartSettingWidget::unbindFigure(DAFigureWidget* fig)
{
    if (fig) {
        disconnect(fig, &DAFigureWidget::chartAdded, this, &DAChartSettingWidget::onChartAdded);
        disconnect(fig, &DAFigureWidget::chartRemoved, this, &DAChartSettingWidget::onChartRemoved);
        disconnect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartSettingWidget::onCurrentChartChanged);
    }
}

/**
 * @brief 绑定绘图相关信号
 * @param chart
 */
void DAChartSettingWidget::bindChart(DAChartWidget* chart)
{
    if (chart) {
        connect(chart, &DAChartWidget::itemAttached, this, &DAChartSettingWidget::onItemAttached);
        connect(chart, &DAChartWidget::chartPropertyHasChanged, this, &DAChartSettingWidget::onChartPropertyHasChanged);
    }
}

/**
 * @brief 解绑绘图相关信号
 * @param chart
 */
void DAChartSettingWidget::unbindChart(DAChartWidget* chart)
{
    if (chart) {
        disconnect(chart, &DAChartWidget::itemAttached, this, &DAChartSettingWidget::onItemAttached);
        disconnect(chart, &DAChartWidget::chartPropertyHasChanged, this, &DAChartSettingWidget::onChartPropertyHasChanged);
    }
}

/**
 * @brief 获取chart在combobox的索引
 * @param chart
 * @return 如果没有，返回-1
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

/**
 * @brief 查找item在当前combobox的索引
 * @param item
 * @return 如果没有，返回-1
 */
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
 * @brief 获取当前的chart
 * @return
 */
DAChartWidget* DAChartSettingWidget::getCurrentSelectChart() const
{
    if (ui->comboBoxSelectChart->currentIndex() < 0) {
        return nullptr;
    }
    return ui->comboBoxSelectChart->currentData().value< DAChartWidget* >();
}

/**
 * @brief 获取当前选中的item
 * @return
 */
QwtPlotItem* DAChartSettingWidget::getCurrentItem() const
{
    if (ui->comboBoxSelectItem->currentIndex() < 0) {
        return nullptr;
    }
    return ui->comboBoxSelectItem->currentData().value< QwtPlotItem* >();
}

void DAChartSettingWidget::showFigureSettingWidget()
{
    if (ui->stackedWidget->currentWidget() == ui->widgetFigureSetting) {
        return;
    }
    ui->toolButtonFigure->setChecked(true);
    ui->comboBoxSelectChart->setVisible(false);
    ui->comboBoxSelectItem->setVisible(false);
    ui->stackedWidget->setCurrentWidget(ui->widgetFigureSetting);
}

void DAChartSettingWidget::showPlotSettingWidget()
{
    if (ui->stackedWidget->currentWidget() == ui->widgetChartSetting) {
        return;
    }
    ui->toolButtonChart->setChecked(true);
    ui->comboBoxSelectChart->setVisible(true);
    ui->comboBoxSelectItem->setVisible(false);
    ui->stackedWidget->setCurrentWidget(ui->widgetChartSetting);
}

void DAChartSettingWidget::showItemSettingWidget()
{
    if (ui->stackedWidget->currentWidget() == ui->widgetItemSetting) {
        return;
    }
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
    auto cnt = ui->comboBoxSelectChart->count();
    for (int i = 0; i < cnt; ++i) {
        DAChartWidget* saveChartPtr = ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
        if (saveChartPtr == chart) {
            ui->comboBoxSelectChart->removeItem(i);
            // 同时要查看item combox当前是否是
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

/**
 * @brief 重置ChartsComboBox
 */
void DAChartSettingWidget::resetChartsComboBox()
{
    // 避免触发onComboBoxChartIndexChanged
    DASignalBlockers block(ui->comboBoxSelectChart);
    DAFigureWidget* fig = getFigure();
    ui->comboBoxSelectChart->clear();
    if (!fig) {
        // null指针直接返回
        return;
    }
    const auto chartsList = fig->getCharts();
    const int cnt         = chartsList.size();
    for (int i = 0; i < cnt; ++i) {
        DAChartWidget* chart = chartsList[ i ];
        ui->comboBoxSelectChart->addItem(DAFigureTreeModel::chartTitle(chart, fig->figure()), QVariant::fromValue(chart));
    }
}

/**
 * @brief 重置ItemsComboBox
 */
void DAChartSettingWidget::resetItemsComboBox(DAChartWidget* chart)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::resetItemsComboBox";
#endif
    DASignalBlockers block(ui->comboBoxSelectItem);
    if (!chart) {
        // 为空时跳过
        ui->comboBoxSelectItem->clear();
        return;
    }
    const auto items = chart->itemList();
    if (ui->comboBoxSelectItem->count() > 0) {
        ui->comboBoxSelectItem->clear();
    }
    if (items.empty()) {
        return;
    }
    for (QwtPlotItem* item : items) {
        ui->comboBoxSelectItem->addItem(DAFigureTreeModel::chartItemName(item), QVariant::fromValue(item));
    }
}

/**
 * @brief 删除当前item复选框的一个QwtPlotItem,成功删除返回true
 * @param item
 * @return
 */
bool DAChartSettingWidget::removeItemFromItemComboBox(QwtPlotItem* item)
{
    const int cnt = ui->comboBoxSelectItem->count();
    for (int i = 0; i < cnt; ++i) {
        QwtPlotItem* plotitem = ui->comboBoxSelectItem->itemData(i).value< QwtPlotItem* >();
        if (plotitem == item) {
            ui->comboBoxSelectItem->removeItem(i);
            return true;
        }
    }
    return false;
}

/**
 * @brief 捕获绘图关闭，仅仅只处理最后一个绘图的关闭，其他2025
 * @param f
 */
void DAChartSettingWidget::onFigureCloseing(DAFigureWidget* f)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onFigureCloseing";
#endif
    Q_UNUSED(f);
    if (!d_ptr->mChartOpt) {
        return;
    }
    int c = d_ptr->mChartOpt->getFigureCount();
    if (c == 1) {
        // 这是删除最后一个tab标签，删除最后一个tab标签不会触发onCurrentFigureChanged
        setFigure(nullptr);
    }
}

void DAChartSettingWidget::onFigureCreated(DAFigureWidget* f)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onFigureCreated" << qintptr(f);
    ;
#endif
    if (f) {
        setFigure(f);
        showFigureSettingWidget();
    }
}

void DAChartSettingWidget::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onCurrentFigureChanged index=" << index;
#endif
    Q_UNUSED(index);
    if (f) {
        setFigure(f);
    }
}

void DAChartSettingWidget::onChartAdded(DAChartWidget* c)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onChartAdded";
#endif
    resetChartsComboBox();
    setChart(c);
    showPlotSettingWidget();
}

void DAChartSettingWidget::onChartRemoved(DAChartWidget* c)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onChartRemoved";
#endif
    unbindChart(c);
    removeChartFromComboBox(c);
}

void DAChartSettingWidget::onCurrentChartChanged(DAChartWidget* c)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onCurrentChartChanged";
#endif
    setChart(c);
}

void DAChartSettingWidget::onItemAttached(QwtPlotItem* plotItem, bool on)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onItemAttached ptr=" << qintptr(plotItem) << ",on=" << on;
#endif
    DAChartWidget* c = qobject_cast< DAChartWidget* >(sender());
    if (c) {
        setChartItemAttached(c, plotItem, on);
    }
}

void DAChartSettingWidget::setChartItemAttached(DAChartWidget* c, QwtPlotItem* plotItem, bool on)
{
    DAChartWidget* currentSelectChart = getCurrentSelectChart();
    if (currentSelectChart == c) {
        // 说明当前删除的chart item是当前选择的chart

        if (on) {
            // 有新增的item，把设置显示出来
            //  按照item类型显示
            showItemSettingWidget();
            resetItemsComboBox(c);
            setPlotItem(plotItem);
        } else {
            // 说明是删除
            removeItemFromItemComboBox(plotItem);
        }
    }
}

/**
 * @brief DAFigureSettingWidget::onComboBoxChartIndexChanged
 *
 * @note 注意，这里不要调用setCurrentChart函数
 * @param i
 */
void DAChartSettingWidget::onComboBoxChartIndexChanged(int i)
{
    DAChartWidget* chart = getChartByIndex(i);
    setChart(chart);
}

/**
 * @brief
 *
 * @note 注意，这里不要调用setCurrentItem函数
 * @param i
 */
void DAChartSettingWidget::onComboBoxItemIndexChanged(int i)
{
    QwtPlotItem* item = getPlotItemByIndex(i);
    setPlotItem(item);
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

/**
 * @brief 绘图的属性发生变化，刷新设置界面
 * @param chart
 */
void DAChartSettingWidget::onChartPropertyHasChanged(DAChartWidget* chart)
{
    if (getCurrentSelectChart() == chart) {
        updateChartUI();
    }
}

/**
 * @brief 绑定DAChartOperateWidget相关的信号槽
 * @param opt
 */
void DAChartSettingWidget::bindChartOprateWidget(DAChartOperateWidget* opt)
{
    if (opt) {
        connect(opt, &DAChartOperateWidget::figureRemoving, this, &DAChartSettingWidget::onFigureCloseing);
        connect(opt, &DAChartOperateWidget::figureCreated, this, &DAChartSettingWidget::onFigureCreated);
        connect(opt, &DAChartOperateWidget::currentFigureChanged, this, &DAChartSettingWidget::onCurrentFigureChanged);
    }
}

/**
 * @brief 解绑DAChartOperateWidget相关的信号槽
 * @param opt
 */
void DAChartSettingWidget::unbindChartOprateWidget(DAChartOperateWidget* opt)
{
    if (opt) {
        disconnect(opt, &DAChartOperateWidget::figureRemoving, this, &DAChartSettingWidget::onFigureCloseing);
        disconnect(opt, &DAChartOperateWidget::figureCreated, this, &DAChartSettingWidget::onFigureCreated);
        disconnect(opt, &DAChartOperateWidget::currentFigureChanged, this, &DAChartSettingWidget::onCurrentFigureChanged);
    }
}

void DAChartSettingWidget::updateChartUI()
{
    ui->widgetChartSetting->updateUI();
}

void DAChartSettingWidget::updateItemUI()
{
    // ui->widgetItemSetting->
}

DAChartWidget* DAChartSettingWidget::getChartByIndex(int i) const
{
    if (i < ui->comboBoxSelectChart->count() && i >= 0) {
        return ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
    }
    return nullptr;
}

QwtPlotItem* DAChartSettingWidget::getPlotItemByIndex(int i) const
{
    if (i < ui->comboBoxSelectItem->count() && i >= 0) {
        return ui->comboBoxSelectItem->itemData(i).value< QwtPlotItem* >();
    }
    return nullptr;
}

}  // end da
