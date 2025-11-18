#include "DAChartSettingWidget.h"
#include "ui_DAChartSettingWidget.h"
#include <QPointer>
#include <QScrollArea>
#include "Models/DAFigureTreeModel.h"
#include "DAChartPlotSettingWidget.h"
#include "DAChartCanvasSettingWidget.h"
#include "DAChartAxisSetWidget.h"
#include "DAChartCommonItemsSettingWidget.h"
#include <QDebug>
#include "DASignalBlockers.hpp"
#include "DAChartUtil.h"

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
    DAChartWidget* plotToChart(QwtPlot* plot) const;
    void setupUi(QStackedWidget* stackWidget);

public:
    QPointer< QwtPlot > mPlot;
    DAChartPlotSettingWidget* mPlotSettingWidget { nullptr };             /// 对应图表设置
    DAChartCanvasSettingWidget* mPlotCanvasSettingWidget { nullptr };     /// 对应绘图区域设置
    DAChartAxisSetWidget* mPlotScaleYLeftSettingWidget { nullptr };       /// 对应yleft设置
    DAChartAxisSetWidget* mPlotScaleXBottomSettingWidget { nullptr };     /// 对应xbottom设置
    DAChartAxisSetWidget* mPlotScaleYRightSettingWidget { nullptr };      /// 对应yright设置
    DAChartAxisSetWidget* mPlotScaleXTopSettingWidget { nullptr };        /// 对应xtop设置
    DAChartCommonItemsSettingWidget* mPlotItemSettingWidget { nullptr };  ///< 对应plotItem设置
    const int fix_combobox_count { 6 };  ///< 6个固定的内容，combobox从这里开始查找
};

DAChartSettingWidget::PrivateData::PrivateData(DAChartSettingWidget* p) : q_ptr(p)
{
}

DAChartWidget* DAChartSettingWidget::PrivateData::plotToChart(QwtPlot* plot) const
{
    if (!plot) {
        return nullptr;
    }
    DAChartWidget* chart = nullptr;
    if (plot->isHostPlot()) {
        chart = qobject_cast< DAChartWidget* >(plot);
    } else {
        chart = qobject_cast< DAChartWidget* >(plot->hostPlot());
    }
    return chart;
}

void DAChartSettingWidget::PrivateData::setupUi(QStackedWidget* stackWidget)
{
    mPlotSettingWidget             = new DAChartPlotSettingWidget(stackWidget);
    mPlotCanvasSettingWidget       = new DAChartCanvasSettingWidget(stackWidget);
    mPlotScaleYLeftSettingWidget   = new DAChartAxisSetWidget(stackWidget);
    mPlotScaleXBottomSettingWidget = new DAChartAxisSetWidget(stackWidget);
    mPlotScaleYRightSettingWidget  = new DAChartAxisSetWidget(stackWidget);
    mPlotScaleXTopSettingWidget    = new DAChartAxisSetWidget(stackWidget);
    mPlotItemSettingWidget         = new DAChartCommonItemsSettingWidget(stackWidget);
    stackWidget->addWidget(mPlotSettingWidget);
    stackWidget->addWidget(mPlotCanvasSettingWidget);
    stackWidget->addWidget(mPlotScaleYLeftSettingWidget);
    stackWidget->addWidget(mPlotScaleXBottomSettingWidget);
    stackWidget->addWidget(mPlotScaleYRightSettingWidget);
    stackWidget->addWidget(mPlotScaleXTopSettingWidget);
    stackWidget->addWidget(mPlotItemSettingWidget);
}

//----------------------------------------------------
// DAFigureSettingWidget
//----------------------------------------------------

DAChartSettingWidget::DAChartSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartSettingWidget)
{
    ui->setupUi(this);  // ui中有信号绑定槽
    d_ptr->setupUi(ui->stackedWidget);

    connect(ui->comboBoxSelectItem,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartSettingWidget::onComboBoxItemIndexChanged);
}

DAChartSettingWidget::~DAChartSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置绘图
 * @param plot
 */
void DAChartSettingWidget::setPlot(QwtPlot* plot)
{
    DA_D(d);
    if (d->mPlot == plot) {
        return;
    }
    if (d->mPlot) {
        // 断开和此窗口的所有连接
        disconnect(d->mPlot.data(), nullptr, this, nullptr);
    }
    d->mPlot = plot;
    updateUI();
    if (plot) {
        // 关联信号
        connect(plot, &QwtPlot::destroyed, this, &DAChartSettingWidget::onPlotDestroyed);
        connect(plot, &QwtPlot::itemAttached, this, &DAChartSettingWidget::onItemAttached);
    }
    d->mPlotSettingWidget->setPlot(plot);
    d->mPlotCanvasSettingWidget->setPlot(plot);
    d->mPlotScaleYLeftSettingWidget->setPlot(plot, QwtAxis::YLeft);
    d->mPlotScaleXBottomSettingWidget->setPlot(plot, QwtAxis::XBottom);
    d->mPlotScaleYRightSettingWidget->setPlot(plot, QwtAxis::YRight);
    d->mPlotScaleXTopSettingWidget->setPlot(plot, QwtAxis::XTop);
}

/**
 * @brief 获取当前管理的绘图指针
 * @return
 */
QwtPlot* DAChartSettingWidget::getPlot() const
{
    DA_DC(d);
    return d->mPlot.data();
}

/**
 * @brief 更新界面
 */
void DAChartSettingWidget::updateUI()
{
    // 1. 更新combobox
    resetComboBox();
}

void DAChartSettingWidget::resetComboBox()
{
    DA_D(d);
    // 刷新combobox
    ui->comboBoxSelectItem->clear();
    if (d->mPlot.isNull()) {
        return;
    }
    // 插入固定内容
    static QIcon s_icon_plot_setting    = QIcon(":/DAGui/icon/figure-setting.svg");
    static QIcon s_icon_canvas_setting  = QIcon(":/DAGui/icon/chart.svg");
    static QIcon s_icon_yleft_setting   = QIcon(":/DAGui/ChartSetting/icon/axisYLeft.svg");
    static QIcon s_icon_yright_setting  = QIcon(":/DAGui/ChartSetting/icon/axisYRight.svg");
    static QIcon s_icon_xbottom_setting = QIcon(":/DAGui/ChartSetting/icon/axisXBottom.svg");
    static QIcon s_icon_xtop_setting    = QIcon(":/DAGui/ChartSetting/icon/axisXTop.svg");
    // 图表区（plot）、绘图区（canvas）、4个坐标轴、plotitems
    ui->comboBoxSelectItem->addItem(s_icon_plot_setting, tr("Chart Area"), PlotArea);                 // cn:图表区
    ui->comboBoxSelectItem->addItem(s_icon_canvas_setting, tr("Canvas Area"), CanvasArea);            // cn:绘图区
    ui->comboBoxSelectItem->addItem(s_icon_yleft_setting, tr("Y Left Scale"), YLeftScaleArea);        // cn:左Y轴
    ui->comboBoxSelectItem->addItem(s_icon_xbottom_setting, tr("X Bottom Scale"), XBottomScaleArea);  // cn:下X轴
    ui->comboBoxSelectItem->addItem(s_icon_yright_setting, tr("Y Right Scale"), YRightScaleArea);     // cn:右Y轴
    ui->comboBoxSelectItem->addItem(s_icon_yright_setting, tr("X Top Scale"), XTopScaleArea);         // cn:上X轴
    // 下面是动态增加
    const QList< QwtPlotItem* > itemlist = d->mPlot->itemList();
    for (QwtPlotItem* i : itemlist) {
        ui->comboBoxSelectItem->addItem(DAChartUtil::plotItemIcon(i), DAChartUtil::plotItemName(i), PlotItemsArea);
        int lastIndex = ui->comboBoxSelectItem->count() - 1;
        ui->comboBoxSelectItem->setItemData(lastIndex, QVariant::fromValue(reinterpret_cast< quintptr >(i)), RolePlotItemPtr);
    }
}

void DAChartSettingWidget::setCurrentPlotItem(QwtPlotItem* item)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::setPlotItem" << quintptr(item);
#endif
    // 1. 查找到combobox中是否存在这个item
    int index = findComboBoxIndexFromPlotItem(item);
    if (index < 0) {
        // 没找到，说明当前的plot不是这个item，切换到对应plot
        QwtPlot* plot = item->plot();
        if (!plot) {
            return;
        }
        // 设置plot会更新combobox
        setPlot(plot);
        // 再次查找
        index = findComboBoxIndexFromPlotItem(item);
        if (index < 0) {
            // 异常
            return;
        }
    }
    // 这里index一定大于0
    //  2. 查看当前combobox是否选中这个index
    if (ui->comboBoxSelectItem->currentIndex() != index) {
        QSignalBlocker b(ui->comboBoxSelectItem);  // 避免触发currentIndexChanged
        ui->comboBoxSelectItem->setCurrentIndex(index);
    }

    // 3.设置到窗口中
    if (ui->stackedWidget->currentWidget() != d_ptr->mPlotItemSettingWidget) {
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotItemSettingWidget);
    }
    d_ptr->mPlotItemSettingWidget->setPlotItem(item);
}

QwtPlotItem* DAChartSettingWidget::getCurrentPlotItem() const
{
    return d_ptr->mPlotItemSettingWidget->getPlotItem();
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

/**
 * @brief 设置选中内容
 *
 * DAFigureElementSelection一般有绘图结构树传递过来
 * @param sel
 */
void DAChartSettingWidget::setSelection(const DAFigureElementSelection& sel)
{
    switch (sel.selectionType) {
    case DAFigureElementSelection::SelectScaleWidget:
    case DAFigureElementSelection::SelectPlot: {
        // 选中绘图
        setFigure(sel.figureWidget);
        DAChartWidget* chart = d_ptr->plotToChart(sel.plot);
        if (chart) {
            setChart(chart);
            showPlotSettingWidget();
        }
        return;
    } break;
    case DAFigureElementSelection::SelectPlotItem: {
        setFigure(sel.figureWidget);
        DAChartWidget* chart = d_ptr->plotToChart(sel.plot);
        if (chart) {
            setChart(chart);
        }
        setPlotItem(sel.plotItem);
        showItemSettingWidget();
        return;
    } break;
    default:
        break;
    }
}

void DAChartSettingWidget::showPlotItemSetting(QwtPlotItem* item)
{
    setPlotItem(item);
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
 * @brief 通过plotitem，从combobox找到对应的索引
 * @param item
 * @return 找不到返回-1
 */
int DAChartSettingWidget::findComboBoxIndexFromPlotItem(const QwtPlotItem* item) const
{
    const int count = ui->comboBoxSelectItem->count();
    for (int i = d_ptr->fix_combobox_count; i < count; ++i) {
        QVariant v = ui->comboBoxSelectItem->itemData(i, RolePlotItemPtr);
        if (!v.isValid()) {
            continue;
        }
        QwtPlotItem* innerItem = reinterpret_cast< QwtPlotItem* >(v.value< quintptr >());
        if (innerItem == item) {
            return i;
        }
    }
    return -1;
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
        ui->comboBoxSelectChart->addItem(DAFigureTreeModel::plotTitle(chart, fig->figure()), QVariant::fromValue(chart));
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
        ui->comboBoxSelectItem->addItem(DAFigureTreeModel::plotItemName(item), QVariant::fromValue(item));
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
 * @brief 绘图删除
 * @param obj
 */
void DAChartSettingWidget::onPlotDestroyed(QObject* obj)
{
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
