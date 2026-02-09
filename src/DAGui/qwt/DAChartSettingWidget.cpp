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
    // 此函数不适用widType==SettingPlotItems的情况
    void setComboboxFixSelectionArea(QComboBox* com, DAChartSettingWidget::SettingWidgetType widType);

public:
    QPointer< QwtPlot > mPlot;
    DAChartPlotSettingWidget* mPlotSettingWidget { nullptr };             /// 对应图表设置
    DAChartCanvasSettingWidget* mPlotCanvasSettingWidget { nullptr };     /// 对应绘图区域设置
    DAChartAxisSetWidget* mPlotScaleYLeftSettingWidget { nullptr };       /// 对应yleft设置
    DAChartAxisSetWidget* mPlotScaleXBottomSettingWidget { nullptr };     /// 对应xbottom设置
    DAChartAxisSetWidget* mPlotScaleYRightSettingWidget { nullptr };      /// 对应yright设置
    DAChartAxisSetWidget* mPlotScaleXTopSettingWidget { nullptr };        /// 对应xtop设置
    DAChartCommonItemsSettingWidget* mPlotItemSettingWidget { nullptr };  ///< 对应plotItem设置
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

void DAChartSettingWidget::PrivateData::setComboboxFixSelectionArea(QComboBox* com, DAChartSettingWidget::SettingWidgetType widType)
{
    const int comboboxCount = com->count();
    for (int i = 0; i < comboboxCount; ++i) {
        QVariant v = com->itemData(i);
        if (!v.isValid()) {
            continue;
        }
        int value = v.toInt();
        if (value == widType) {
            if (com->currentIndex() != i) {
                com->setCurrentIndex(i);
            }
        }
    }
}

//----------------------------------------------------
// DAFigureSettingWidget
//----------------------------------------------------

DAChartSettingWidget::DAChartSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartSettingWidget)
{
    ui->setupUi(this);  // ui中有信号绑定槽
    d_ptr->setupUi(ui->stackedWidget);

    connect(
        ui->comboBoxSelectItem, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAChartSettingWidget::onComboBoxItemIndexChanged
    );
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
    ui->comboBoxSelectItem->addItem(s_icon_plot_setting, tr("Chart Area"), SettingPlot);                 // cn:图表区
    ui->comboBoxSelectItem->addItem(s_icon_canvas_setting, tr("Canvas Area"), SettingCanvas);            // cn:绘图区
    ui->comboBoxSelectItem->addItem(s_icon_yleft_setting, tr("Y Left Scale"), SettingYLeftScale);        // cn:左Y轴
    ui->comboBoxSelectItem->addItem(s_icon_xbottom_setting, tr("X Bottom Scale"), SettingXBottomScale);  // cn:下X轴
    ui->comboBoxSelectItem->addItem(s_icon_yright_setting, tr("Y Right Scale"), SettingYRightScale);     // cn:右Y轴
    ui->comboBoxSelectItem->addItem(s_icon_xtop_setting, tr("X Top Scale"), SettingXTopScale);           // cn:上X轴
    // 下面是动态增加
    const QList< QwtPlotItem* > itemlist = d->mPlot->itemList();
    for (QwtPlotItem* item : itemlist) {
        appendPlotItemToComboBox(item);
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
 * @brief 显示对应设置窗口， 不会设置combobox的index
 * @param widType
 */
void DAChartSettingWidget::showSettingWidget(SettingWidgetType widType)
{
    switch (widType) {
    case SettingPlot:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotSettingWidget);
        break;
    case SettingCanvas:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotCanvasSettingWidget);
        break;
    case SettingYLeftScale:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleYLeftSettingWidget);
        break;
    case SettingYRightScale:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleYRightSettingWidget);
        break;
    case SettingXBottomScale:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleXBottomSettingWidget);
        break;
    case SettingXTopScale:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleXTopSettingWidget);
        break;
    case SettingPlotItems:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlotItemSettingWidget);
        break;
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
    if (!sel.plot) {
        return;
    }
    if (sel.plot != getPlot()) {
        setPlot(sel.plot);
    }
    switch (sel.selectionType) {
    case DAFigureElementSelection::SelectScaleWidget: {
        switch (sel.axisId) {
        case QwtAxis::YLeft:
            showScaleYLeftSetting();
            break;
        case QwtAxis::YRight:
            showScaleYRightSetting();
            break;
        case QwtAxis::XBottom:
            showScaleXBottomSetting();
            break;
        case QwtAxis::XTop:
            showScaleXTopSetting();
            break;
        default:
            break;
        }
    } break;
    case DAFigureElementSelection::SelectPlot: {
        // 选中绘图
        showPlotSetting();
        return;
    } break;
    case DAFigureElementSelection::SelectPlotItem: {
        showPlotItemSetting(sel.plotItem);
        return;
    } break;
    default:
        break;
    }
}

void DAChartSettingWidget::showPlotSetting()
{
    showSettingWidget(SettingPlot);
    // 更新combobox
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingPlot);
}

void DAChartSettingWidget::showCanvasSetting()
{
    showSettingWidget(SettingCanvas);
    // 更新combobox
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingCanvas);
}

void DAChartSettingWidget::showScaleYLeftSetting()
{
    showSettingWidget(SettingYLeftScale);
    // 更新combobox
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingYLeftScale);
}

void DAChartSettingWidget::showScaleYRightSetting()
{
    showSettingWidget(SettingYRightScale);
    // 更新combobox
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingYRightScale);
}

void DAChartSettingWidget::showScaleXBottomSetting()
{
    showSettingWidget(SettingXBottomScale);
    // 更新combobox
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingXBottomScale);
}

void DAChartSettingWidget::showScaleXTopSetting()
{
    showSettingWidget(SettingXTopScale);
    // 更新combobox
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingXTopScale);
}

/**
 * @brief 显示plotitem需要注意，如果传入的plotitem
 * @param item
 */
void DAChartSettingWidget::showPlotItemSetting(QwtPlotItem* item)
{
    setCurrentPlotItem(item);
}

DAChartPlotSettingWidget* DAChartSettingWidget::getChartPlotSettingWidget() const
{
    return d_ptr->mPlotSettingWidget;
}

DAChartCanvasSettingWidget* DAChartSettingWidget::getChartCanvasSettingWidget() const
{
    return d_ptr->mPlotCanvasSettingWidget;
}

DAChartAxisSetWidget* DAChartSettingWidget::getChartAxisSetWidget(int axisId) const
{
    switch (axisId) {
    case QwtAxis::YLeft:
        return d_ptr->mPlotScaleYLeftSettingWidget;
    case QwtAxis::YRight:
        return d_ptr->mPlotScaleYRightSettingWidget;
    case QwtAxis::XBottom:
        return d_ptr->mPlotScaleXBottomSettingWidget;
    case QwtAxis::XTop:
        return d_ptr->mPlotScaleXTopSettingWidget;
    }
    return nullptr;
}

DAChartCommonItemsSettingWidget* DAChartSettingWidget::getChartCommonItemsSettingWidget() const
{
    return d_ptr->mPlotItemSettingWidget;
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
 * @brief
 *
 * @note 注意，这里不要调用setCurrentItem函数
 * @param i
 */
void DAChartSettingWidget::onComboBoxItemIndexChanged(int index)
{
    if (index < SettingPlotItems) {
        // 固定页面
        QVariant v = ui->comboBoxSelectItem->itemData(index);
        if (!v.isValid()) {
            return;
        }
        SettingWidgetType widType = static_cast< SettingWidgetType >(v.toInt());
        showSettingWidget(widType);
    } else {
        // plotItem
        QwtPlotItem* item = getPlotItemFromComboBox(index);

        // 设置到窗口
        if (ui->stackedWidget->currentWidget() != d_ptr->mPlotItemSettingWidget) {
            ui->stackedWidget->setCurrentWidget(d_ptr->mPlotItemSettingWidget);
        }
        d_ptr->mPlotItemSettingWidget->setPlotItem(item);
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
    for (int i = SettingPlotItems; i < count; ++i) {
        QwtPlotItem* innerItem = getPlotItemFromComboBox(i);
        if (innerItem == item) {
            return i;
        }
    }
    return -1;
}

void DAChartSettingWidget::appendPlotItemToComboBox(const QwtPlotItem* item)
{
    ui->comboBoxSelectItem->addItem(DAChartUtil::plotItemIcon(item), DAChartUtil::plotItemName(item), SettingPlotItems);
    int lastIndex = ui->comboBoxSelectItem->count() - 1;
    ui->comboBoxSelectItem->setItemData(lastIndex, QVariant::fromValue(reinterpret_cast< quintptr >(item)), RolePlotItemPtr);
}

QwtPlotItem* DAChartSettingWidget::getPlotItemFromComboBox(int index) const
{
    QVariant v = ui->comboBoxSelectItem->itemData(index, RolePlotItemPtr);
    if (!v.isValid()) {
        return nullptr;
    }
    return reinterpret_cast< QwtPlotItem* >(v.value< quintptr >());
}

void DAChartSettingWidget::removePlotItemFromComboBox(const QwtPlotItem* item)
{
    int index = findComboBoxIndexFromPlotItem(item);
    if (index < 0) {
        return;
    }
    ui->comboBoxSelectItem->removeItem(index);
}

void DAChartSettingWidget::onItemAttached(QwtPlotItem* plotItem, bool on)
{
#if DAChartSettingWidget_DEBUG_PRINT
    qDebug() << "DAChartSettingWidget::onItemAttached ptr=" << qintptr(plotItem) << ",on=" << on;
#endif
    QwtPlot* plot = plotItem->plot();
    if (plot) {
        if (plot != getPlot()) {
            // 异常情况
            return;
        }
    }
    if (on) {
        // 新增
        appendPlotItemToComboBox(plotItem);
    } else {
        removePlotItemFromComboBox(plotItem);
    }

    // 设置窗口可以自动处理item的删除，不需要额外处理
}

}  // end da
