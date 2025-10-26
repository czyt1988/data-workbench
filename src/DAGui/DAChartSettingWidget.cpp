#include "DAChartSettingWidget.h"
#include "ui_DAChartSettingWidget.h"
#include <QPointer>
#include <QScrollArea>
#include "Models/DAFigureTreeModel.h"
#include "DAChartOperateWidget.h"
#include "DAChartPlotSettingWidget.h"
#include "DADebug.hpp"
#include "DASignalBlockers.hpp"

DA_DEBUG_PRINT(DAChartSettingWidget, true)

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
            QOverload< int >::of(&QComboBox::activated),
            this,
            &DAChartSettingWidget::onComboBoxChartActivated);
    connect(ui->comboBoxSelectItem,
            QOverload< int >::of(&QComboBox::activated),
            this,
            &DAChartSettingWidget::onComboBoxItemActived);
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
        disconnect(d_ptr->mChartOpt, &DAChartOperateWidget::figureRemoving, this, &DAChartSettingWidget::onFigureCloseing);
        disconnect(d_ptr->mChartOpt, &DAChartOperateWidget::figureCreated, this, &DAChartSettingWidget::onFigureCreated);
        disconnect(d_ptr->mChartOpt,
                   &DAChartOperateWidget::currentFigureChanged,
                   this,
                   &DAChartSettingWidget::onCurrentFigureChanged);
    }
    d_ptr->mChartOpt = opt;
    if (opt) {
        connect(opt, &DAChartOperateWidget::figureRemoving, this, &DAChartSettingWidget::onFigureCloseing);
        connect(opt, &DAChartOperateWidget::figureCreated, this, &DAChartSettingWidget::onFigureCreated);
        connect(opt, &DAChartOperateWidget::currentFigureChanged, this, &DAChartSettingWidget::onCurrentFigureChanged);
    }
    // 设置当前的figure
    if (opt) {
        setFigure(opt->getCurrentFigure());
    }
}

/**
 * @brief 设置绘图窗口
 * @param fig
 */
void DAChartSettingWidget::setFigure(DAFigureWidget* fig)
{
    DADebug() << "setFigure1";
    DAFigureWidget* oldfig = ui->widgetFigureSetting->getFigure();
    if (oldfig) {
        unbindFigure(oldfig);
    }
    bindFigure(fig);
    // 更新chart
    if (fig) {
        setChart(fig->getCurrentChart());
    }
}

void DAChartSettingWidget::setChart(DAChartWidget* chart)
{
    DADebug() << "setChart";
    DASignalBlockers b(ui->comboBoxSelectChart, ui->comboBoxSelectItem);
    if (chart == nullptr) {
        ui->widgetChartSetting->setChartWidget(nullptr);
        ui->comboBoxSelectChart->clear();
        ui->comboBoxSelectItem->clear();
    } else {
        // 这里会触发indexChanged信号
        int index = indexOfChart(chart);
        if (index == -1) {
            ui->widgetChartSetting->setChartWidget(nullptr);
            ui->comboBoxSelectChart->clear();
            ui->comboBoxSelectItem->clear();
            return;
        }
        ui->comboBoxSelectChart->setCurrentIndex(index);
        // 重置itemcombobox
        resetItemsComboBox(chart);
        ui->widgetChartSetting->setChartWidget(chart);
    }
}

DAFigureWidget* DAChartSettingWidget::getFigure() const
{
    return ui->widgetFigureSetting->getFigure();
}

DAChartWidget* DAChartSettingWidget::getChart() const
{
    return ui->widgetChartSetting->getChartWidget();
}

QwtPlotItem* DAChartSettingWidget::getItem() const
{
    return ui->widgetItemSetting->getPlotItem();
}

void DAChartSettingWidget::bindFigure(DAFigureWidget* fig)
{
    DADebug() << "bindFigure";
    DAFigureWidget* oldfig = ui->widgetFigureSetting->getFigure();
    if (oldfig == fig) {
        return;
    }
    if (fig) {
        connect(fig, &DAFigureWidget::chartAdded, this, &DAChartSettingWidget::onChartAdded);
        connect(fig, &DAFigureWidget::chartRemoved, this, &DAChartSettingWidget::onChartRemoved);
        connect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartSettingWidget::onCurrentChartChanged);
    }
}

void DAChartSettingWidget::unbindFigure(DAFigureWidget* fig)
{
    DADebug() << "unbindFigure ptr=" << qintptr(fig);
    if (fig) {
        disconnect(fig, &DAFigureWidget::chartAdded, this, &DAChartSettingWidget::onChartAdded);
        disconnect(fig, &DAFigureWidget::chartRemoved, this, &DAChartSettingWidget::onChartRemoved);
        disconnect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartSettingWidget::onCurrentChartChanged);
        // 更新chart信息
        const auto charts = fig->getCharts();
        for (auto chart : charts) {
            // 解除绑定chart的信号槽
            unbindChart(chart);
        }
    }
}

void DAChartSettingWidget::bindChart(DAChartWidget* chart)
{
    DADebug() << "bindChart ptr=" << qintptr(chart);
    if (chart) {
        connect(chart, &DAChartWidget::itemAttached, this, &DAChartSettingWidget::onItemAttached);
        connect(chart, &DAChartWidget::chartPropertyHasChanged, this, &DAChartSettingWidget::onChartPropertyHasChanged);
    }
}

void DAChartSettingWidget::unbindChart(DAChartWidget* chart)
{
    DADebug() << "unbindChart ptr=" << qintptr(chart);
    if (chart) {
        disconnect(chart, &DAChartWidget::itemAttached, this, &DAChartSettingWidget::onItemAttached);
        disconnect(chart, &DAChartWidget::chartPropertyHasChanged, this, &DAChartSettingWidget::onChartPropertyHasChanged);
    }
}

/**
 * @brief 获取chart在combobox的索引
 * @param chart
 * @return
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
 * @brief 通过设置窗口，设置当前的chart
 * @param chart
 */
void DAChartSettingWidget::setCurrentChart(DAChartWidget* chart)
{
    if (!chart) {
        return;
    }
    // 找到索引
    auto fig      = getFigure();
    auto chartfig = chart->getFigure();
    if (fig && fig != chartfig) {
        qWarning() << tr("The assigned plot does not belong to the figure managed by the current figure window.");  // cn:设置的绘图不属于当前绘图窗口管理的图表。
        return;
    }

    fig->setCurrentChart(chart);
}

/**
 * @brief 获取当前的chart
 * @return
 */
DAChartWidget* DAChartSettingWidget::getCurrentChart() const
{
    if (ui->comboBoxSelectChart->currentIndex() < 0) {
        return nullptr;
    }

    return ui->comboBoxSelectChart->currentData().value< DAChartWidget* >();
}

/**
 * @brief 设置当前的item
 * @param item
 */
void DAChartSettingWidget::setCurrentItem(QwtPlotItem* item)
{
    DADebug() << "setCurrentItem ptr=" << qintptr(item);
    // 这里不会触发combobox的 任何信号
    setItemUI(item);
}

QwtPlotItem* DAChartSettingWidget::getCurrentItem() const
{
    if (ui->comboBoxSelectItem->currentIndex() < 0) {
        return nullptr;
    }
    return ui->comboBoxSelectItem->currentData().value< QwtPlotItem* >();
}

void DAChartSettingWidget::showFigureSettingWidget()
{
    DADebug() << "showFigureSettingWidget";
    ui->toolButtonFigure->setChecked(true);
    ui->comboBoxSelectChart->setVisible(false);
    ui->comboBoxSelectItem->setVisible(false);
    ui->stackedWidget->setCurrentWidget(ui->widgetFigureSetting);
}

void DAChartSettingWidget::showPlotSettingWidget()
{
    DADebug() << "showPlotSettingWidget";
    ui->toolButtonChart->setChecked(true);
    ui->comboBoxSelectChart->setVisible(true);
    ui->comboBoxSelectItem->setVisible(false);
    ui->stackedWidget->setCurrentWidget(ui->widgetChartSetting);
}

void DAChartSettingWidget::showItemSettingWidget()
{
    DADebug() << "showItemSettingWidget";
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
    DADebug() << "removeChartFromComboBox";
    auto c = ui->comboBoxSelectChart->count();
    for (int i = 0; i < c; ++i) {
        DAChartWidget* saveChartPtr = ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
        if (saveChartPtr == chart) {
            ui->comboBoxSelectChart->removeItem(i);
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

void DAChartSettingWidget::resetChartsComboBox()
{
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
        ui->comboBoxSelectChart->addItem(DAChartWidgetStandardItem::getChartTitle(fig, chart), QVariant::fromValue(chart));
    }

    updateChartUI();
}

void DAChartSettingWidget::setItemsComboBox(const QList< QwtPlotItem* >& its)
{
    DADebug() << "setItemsComboBox";
    if (ui->comboBoxSelectItem->count() > 0) {
        ui->comboBoxSelectItem->clear();
    }
    if (its.empty()) {
        return;
    }
    QVariant curData     = ui->comboBoxSelectItem->currentData();
    QwtPlotItem* oldItem = nullptr;
    if (curData.isValid()) {
        oldItem = curData.value< QwtPlotItem* >();
    }
    for (QwtPlotItem* item : its) {
        ui->comboBoxSelectItem->addItem(DAChartItemStandardItem::getItemName(item), QVariant::fromValue(item));
    }
    if (ui->comboBoxSelectItem->count() > 0) {
        ui->comboBoxSelectItem->setCurrentIndex(0);
    }
    // 默认选中
    if (oldItem) {
        setCurrentItem(oldItem);
    } else if (its.size() > 0) {
        setCurrentItem(its.first());
    }
}

void DAChartSettingWidget::resetItemsComboBox(DAChartWidget* chart)
{
    DADebug() << "resetItemsComboBox";
    const auto items = chart->itemList();
    setItemsComboBox(items);
}

void DAChartSettingWidget::onFigureCloseing(DAFigureWidget* f)
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

void DAChartSettingWidget::onFigureCreated(DAFigureWidget* f)
{
    DADebug() << "onFigureCreated ptr=" << qintptr(f);
    if (f) {
        setFigure(f);
        showFigureSettingWidget();
    }
}

void DAChartSettingWidget::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
    DADebug() << "onCurrentFigureChanged ptr=" << qintptr(f) << ",index=" << index;
    Q_UNUSED(index);
    if (f) {
        setFigure(f);
    }
}

void DAChartSettingWidget::onChartAdded(DAChartWidget* c)
{
    bindChart(c);

    resetChartsComboBox();
    // setCurrentChart(c);//这个函数无必要，addchart会默认把current设置为add的那个chart
    showPlotSettingWidget();
}

void DAChartSettingWidget::onChartRemoved(DAChartWidget* c)
{
    // 刷新chart
    unbindChart(c);
    removeChartFromComboBox(c);
}

void DAChartSettingWidget::onCurrentChartChanged(DAChartWidget* c)
{
    DADebug() << "onCurrentChartChanged";
    setChart(c);
}

void DAChartSettingWidget::onItemAttached(QwtPlotItem* plotItem, bool on)
{
    DADebug() << "onItemAttached ptr=" << qintptr(plotItem) << ",on=" << on;
    DAChartWidget* c = qobject_cast< DAChartWidget* >(sender());
    if (c) {
        onChartItemAttached(c, plotItem, on);
    }
}

void DAChartSettingWidget::onChartItemAttached(DAChartWidget* c, QwtPlotItem* plotItem, bool on)
{
    if (c) {
        resetItemsComboBox(c);
        if (on) {
            // 有新增的item，把设置显示出来
            //  按照item类型显示
            showItemSettingWidget();
            setCurrentChart(c);
            setCurrentItem(plotItem);
        }
    }
}

/**
 * @brief DAFigureSettingWidget::onComboBoxChartIndexChanged
 *
 * @note 注意，这里不要调用setCurrentChart函数
 * @param i
 */
void DAChartSettingWidget::onComboBoxChartActivated(int i)
{
    DADebug() << "onComboBoxChartActivated(" << i << ")";
    DAChartWidget* chart = getChartByIndex(i);
    setCurrentChart(chart);
    // 这里不显示chart对应的stacked widget
}

/**
 * @brief
 *
 * @note 注意，这里不要调用setCurrentItem函数
 * @param i
 */
void DAChartSettingWidget::onComboBoxItemActived(int i)
{
    DADebug() << "onComboBoxItemActived(" << i << ")";
    if (i < 0) {
        setCurrentItem(nullptr);
    } else {
        QwtPlotItem* item = ui->comboBoxSelectItem->itemData(i).value< QwtPlotItem* >();
        setCurrentItem(item);
        showItemSettingWidget();
    }
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
    if (getCurrentChart() == chart) {
        updateChartUI();
    }
}

void DAChartSettingWidget::setChartUI(DAChartWidget* chart)
{
}

void DAChartSettingWidget::updateChartUI()
{
    DADebug() << "updateChartUI";
    setChartUI(getCurrentChart());
}

void DAChartSettingWidget::setItemUI(QwtPlotItem* item)
{
    DADebug() << "setItemUI ptr=" << qintptr(item);
    QSignalBlocker b(ui->comboBoxSelectItem);
    if (item == nullptr) {
        ui->widgetItemSetting->setPlotItem(nullptr);
        ui->comboBoxSelectItem->setCurrentIndex(-1);
    } else {
        auto index = indexOfItem(item);
        ui->comboBoxSelectItem->setCurrentIndex(index);
        ui->widgetItemSetting->setPlotItem(item);
    }
}

void DAChartSettingWidget::updateItemUI()
{
    DADebug() << "updateItemUI";
    setItemUI(getCurrentItem());
}

DAChartWidget* DAChartSettingWidget::getChartByIndex(int i) const
{
    if (i < ui->comboBoxSelectChart->count() && i >= 0) {
        return ui->comboBoxSelectChart->itemData(i).value< DAChartWidget* >();
    }
    return nullptr;
}

}  // end da
