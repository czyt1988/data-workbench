#include "DAPlotSettingWidget.h"
#include "ui_DAPlotSettingWidget.h"
#include <QPointer>
#include <QScrollArea>
#include <QDebug>
#include "DASignalBlockers.hpp"

// qim
#include "DAPlotTreeModel.h"
#include "plot/QImPlotNode.h"

#ifndef DAPlotSettingWidget_DEBUG_PRINT
#define DAPlotSettingWidget_DEBUG_PRINT 1
#endif

namespace DA
{
class DAPlotSettingWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAPlotSettingWidget)
public:
    PrivateData(DAPlotSettingWidget* p);
    void setupUi(QStackedWidget* stackWidget);
    // 此函数不适用widType==SettingPlotItems的情况
    void setComboboxFixSelectionArea(QComboBox* com, DAPlotSettingWidget::SettingWidgetType widType);

public:
    QPointer< QIM::QImPlotNode > mPlot;
    // DAChartPlotSettingWidget* mPlotSettingWidget { nullptr };             /// 对应图表设置
    // DAChartCanvasSettingWidget* mPlotCanvasSettingWidget { nullptr };     /// 对应绘图区域设置
    // DAChartAxisSetWidget* mPlotScaleYLeftSettingWidget { nullptr };       /// 对应yleft设置
    // DAChartAxisSetWidget* mPlotScaleXBottomSettingWidget { nullptr };     /// 对应xbottom设置
    // DAChartAxisSetWidget* mPlotScaleYRightSettingWidget { nullptr };      /// 对应yright设置
    // DAChartAxisSetWidget* mPlotScaleXTopSettingWidget { nullptr };        /// 对应xtop设置
    // DAChartCommonItemsSettingWidget* mPlotItemSettingWidget { nullptr };  ///< 对应plotItem设置
};

DAPlotSettingWidget::PrivateData::PrivateData(DAPlotSettingWidget* p) : q_ptr(p)
{
}

void DAPlotSettingWidget::PrivateData::setupUi(QStackedWidget* stackWidget)
{
    // mPlotSettingWidget             = new DAChartPlotSettingWidget(stackWidget);
    // mPlotCanvasSettingWidget       = new DAChartCanvasSettingWidget(stackWidget);
    // mPlotScaleYLeftSettingWidget   = new DAChartAxisSetWidget(stackWidget);
    // mPlotScaleXBottomSettingWidget = new DAChartAxisSetWidget(stackWidget);
    // mPlotScaleYRightSettingWidget  = new DAChartAxisSetWidget(stackWidget);
    // mPlotScaleXTopSettingWidget    = new DAChartAxisSetWidget(stackWidget);
    // mPlotItemSettingWidget         = new DAChartCommonItemsSettingWidget(stackWidget);
    // stackWidget->addWidget(mPlotSettingWidget);
    // stackWidget->addWidget(mPlotCanvasSettingWidget);
    // stackWidget->addWidget(mPlotScaleYLeftSettingWidget);
    // stackWidget->addWidget(mPlotScaleXBottomSettingWidget);
    // stackWidget->addWidget(mPlotScaleYRightSettingWidget);
    // stackWidget->addWidget(mPlotScaleXTopSettingWidget);
    // stackWidget->addWidget(mPlotItemSettingWidget);
}

void DAPlotSettingWidget::PrivateData::setComboboxFixSelectionArea(QComboBox* com, DAPlotSettingWidget::SettingWidgetType widType)
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

DAPlotSettingWidget::DAPlotSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAPlotSettingWidget)
{
    ui->setupUi(this);  // ui中有信号绑定槽
    d_ptr->setupUi(ui->stackedWidget);

    connect(
        ui->comboBoxSelectItem, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAPlotSettingWidget::onComboBoxItemIndexChanged
    );
}

DAPlotSettingWidget::~DAPlotSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置绘图
 * @param plot
 */
void DAPlotSettingWidget::setPlotNode(QIM::QImPlotNode* plot)
{
    DA_D(d);
    if (d->mPlot == plot) {
        return;
    }
    if (d->mPlot) {
        // 断开和此窗口的所有连接
    }
    d->mPlot = plot;
    updateUI();
    if (plot) {
        // 关联信号
    }
}

/**
 * @brief 获取当前管理的绘图指针
 * @return
 */
QIM::QImPlotNode* DAPlotSettingWidget::getPlotNode() const
{
    DA_DC(d);
    return d->mPlot.data();
}

/**
 * @brief 更新界面
 */
void DAPlotSettingWidget::updateUI()
{
    // 1. 更新combobox
    resetComboBox();
}

void DAPlotSettingWidget::resetComboBox()
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
    // // 图表区（plot）、绘图区（canvas）、4个坐标轴、plotitems
    // ui->comboBoxSelectItem->addItem(s_icon_plot_setting, tr("Chart Area"), SettingPlot);                 // cn:图表区
    // ui->comboBoxSelectItem->addItem(s_icon_canvas_setting, tr("Canvas Area"), SettingCanvas);            // cn:绘图区
    // ui->comboBoxSelectItem->addItem(s_icon_yleft_setting, tr("Y Left Scale"), SettingYLeftScale);        // cn:左Y轴
    // ui->comboBoxSelectItem->addItem(s_icon_xbottom_setting, tr("X Bottom Scale"), SettingXBottomScale);  // cn:下X轴
    // ui->comboBoxSelectItem->addItem(s_icon_yright_setting, tr("Y Right Scale"), SettingYRightScale);     // cn:右Y轴
    // ui->comboBoxSelectItem->addItem(s_icon_xtop_setting, tr("X Top Scale"), SettingXTopScale);           // cn:上X轴
    // // 下面是动态增加
    // const QList< QwtPlotItem* > itemlist = d->mPlot->itemList();
    // for (QwtPlotItem* item : itemlist) {
    //     appendPlotItemToComboBox(item);
    // }
}


/**
 * @brief 显示对应设置窗口， 不会设置combobox的index
 * @param widType
 */
void DAPlotSettingWidget::showSettingWidget(SettingWidgetType widType)
{
    // switch (widType) {
    // case SettingPlot:
    //     ui->stackedWidget->setCurrentWidget(d_ptr->mPlotSettingWidget);
    //     break;
    // case SettingCanvas:
    //     ui->stackedWidget->setCurrentWidget(d_ptr->mPlotCanvasSettingWidget);
    //     break;
    // case SettingYLeftScale:
    //     ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleYLeftSettingWidget);
    //     break;
    // case SettingYRightScale:
    //     ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleYRightSettingWidget);
    //     break;
    // case SettingXBottomScale:
    //     ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleXBottomSettingWidget);
    //     break;
    // case SettingXTopScale:
    //     ui->stackedWidget->setCurrentWidget(d_ptr->mPlotScaleXTopSettingWidget);
    //     break;
    // case SettingPlotItems:
    //     ui->stackedWidget->setCurrentWidget(d_ptr->mPlotItemSettingWidget);
    //     break;
    // }
}

/**
 * @brief 设置选中内容
 *
 * DAFigureElementSelection一般有绘图结构树传递过来
 * @param sel
 */
void DAPlotSettingWidget::setSelection(const DAFigureElementSelection& sel)
{
    // if (!sel.plot) {
    //     return;
    // }
    // if (sel.plot != getPlot()) {
    //     setPlot(sel.plot);
    // }
    // switch (sel.selectionType) {
    // case DAFigureElementSelection::SelectScaleWidget: {
    //     switch (sel.axisId) {
    //     case QwtAxis::YLeft:
    //         showScaleYLeftSetting();
    //         break;
    //     case QwtAxis::YRight:
    //         showScaleYRightSetting();
    //         break;
    //     case QwtAxis::XBottom:
    //         showScaleXBottomSetting();
    //         break;
    //     case QwtAxis::XTop:
    //         showScaleXTopSetting();
    //         break;
    //     default:
    //         break;
    //     }
    // } break;
    // case DAFigureElementSelection::SelectPlot: {
    //     // 选中绘图
    //     showPlotSetting();
    //     return;
    // } break;
    // case DAFigureElementSelection::SelectPlotItem: {
    //     showPlotItemSetting(sel.plotItem);
    //     return;
    // } break;
    // default:
    //     break;
    // }
}

// void DAPlotSettingWidget::showPlotSetting()
// {
//     showSettingWidget(SettingPlot);
//     // 更新combobox
//     QSignalBlocker b(ui->comboBoxSelectItem);
//     d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingPlot);
// }

// void DAPlotSettingWidget::showCanvasSetting()
// {
//     showSettingWidget(SettingCanvas);
//     // 更新combobox
//     QSignalBlocker b(ui->comboBoxSelectItem);
//     d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingCanvas);
// }

// void DAPlotSettingWidget::showScaleYLeftSetting()
// {
//     showSettingWidget(SettingYLeftScale);
//     // 更新combobox
//     QSignalBlocker b(ui->comboBoxSelectItem);
//     d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingYLeftScale);
// }

// void DAPlotSettingWidget::showScaleYRightSetting()
// {
//     showSettingWidget(SettingYRightScale);
//     // 更新combobox
//     QSignalBlocker b(ui->comboBoxSelectItem);
//     d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingYRightScale);
// }

// void DAPlotSettingWidget::showScaleXBottomSetting()
// {
//     showSettingWidget(SettingXBottomScale);
//     // 更新combobox
//     QSignalBlocker b(ui->comboBoxSelectItem);
//     d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingXBottomScale);
// }

// void DAPlotSettingWidget::showScaleXTopSetting()
// {
//     showSettingWidget(SettingXTopScale);
//     // 更新combobox
//     QSignalBlocker b(ui->comboBoxSelectItem);
//     d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, SettingXTopScale);
// }

// /**
//  * @brief 显示plotitem需要注意，如果传入的plotitem
//  * @param item
//  */
// void DAPlotSettingWidget::showPlotItemSetting(QwtPlotItem* item)
// {
//     setCurrentPlotItem(item);
// }


void DAPlotSettingWidget::changeEvent(QEvent* e)
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
void DAPlotSettingWidget::onComboBoxItemIndexChanged(int index)
{
    // if (index < SettingPlotItems) {
    //     // 固定页面
    //     QVariant v = ui->comboBoxSelectItem->itemData(index);
    //     if (!v.isValid()) {
    //         return;
    //     }
    //     SettingWidgetType widType = static_cast< SettingWidgetType >(v.toInt());
    //     showSettingWidget(widType);
    // } else {
    //     // plotItem
    //     QwtPlotItem* item = getPlotItemFromComboBox(index);

    //     // 设置到窗口
    //     if (ui->stackedWidget->currentWidget() != d_ptr->mPlotItemSettingWidget) {
    //         ui->stackedWidget->setCurrentWidget(d_ptr->mPlotItemSettingWidget);
    //     }
    //     d_ptr->mPlotItemSettingWidget->setPlotItem(item);
    // }
}

// /**
//  * @brief 通过plotitem，从combobox找到对应的索引
//  * @param item
//  * @return 找不到返回-1
//  */
// int DAPlotSettingWidget::findComboBoxIndexFromPlotItem(const QwtPlotItem* item) const
// {
//     const int count = ui->comboBoxSelectItem->count();
//     for (int i = SettingPlotItems; i < count; ++i) {
//         QwtPlotItem* innerItem = getPlotItemFromComboBox(i);
//         if (innerItem == item) {
//             return i;
//         }
//     }
//     return -1;
// }

// void DAPlotSettingWidget::appendPlotItemToComboBox(const QwtPlotItem* item)
// {
//     ui->comboBoxSelectItem->addItem(DAChartUtil::plotItemIcon(item), DAChartUtil::plotItemName(item), SettingPlotItems);
//     int lastIndex = ui->comboBoxSelectItem->count() - 1;
//     ui->comboBoxSelectItem->setItemData(lastIndex, QVariant::fromValue(reinterpret_cast< quintptr >(item)), RolePlotItemPtr);
// }

// QwtPlotItem* DAPlotSettingWidget::getPlotItemFromComboBox(int index) const
// {
//     QVariant v = ui->comboBoxSelectItem->itemData(index, RolePlotItemPtr);
//     if (!v.isValid()) {
//         return nullptr;
//     }
//     return reinterpret_cast< QwtPlotItem* >(v.value< quintptr >());
// }

// void DAPlotSettingWidget::removePlotItemFromComboBox(const QwtPlotItem* item)
// {
//     int index = findComboBoxIndexFromPlotItem(item);
//     if (index < 0) {
//         return;
//     }
//     ui->comboBoxSelectItem->removeItem(index);
// }

// void DAPlotSettingWidget::onItemAttached(QwtPlotItem* plotItem, bool on)
// {
// #if DAPlotSettingWidget_DEBUG_PRINT
//     qDebug() << "DAPlotSettingWidget::onItemAttached ptr=" << qintptr(plotItem) << ",on=" << on;
// #endif
//     QwtPlot* plot = plotItem->plot();
//     if (plot) {
//         if (plot != getPlot()) {
//             // 异常情况
//             return;
//         }
//     }
//     if (on) {
//         // 新增
//         appendPlotItemToComboBox(plotItem);
//     } else {
//         removePlotItemFromComboBox(plotItem);
//     }

//     // 设置窗口可以自动处理item的删除，不需要额外处理
// }

}  // end da
