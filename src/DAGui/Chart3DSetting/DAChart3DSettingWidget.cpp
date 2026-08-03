#include "DAChart3DSettingWidget.h"
#include "ui_DAChart3DSettingWidget.h"
#include <QPointer>
#include <QScrollArea>
#include <QDebug>
#include <QSignalBlocker>
#include "DAChart3DCommonItemsSettingWidget.h"
#include "DAChart3DItemSettingPanelFactory.h"
#include "DAChart3DWidget.h"

// 07计划完成后取消以下注释：
// #include "DAChart3DPlotSettingPanel.h"
// #include "DAChart3DCoordSysSettingPanel.h"
// #include "DAChart3DAxisSettingPanel.h"
// #include "DAChart3DColorLegendSettingPanel.h"

namespace DA
{
class DAChart3DSettingWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChart3DSettingWidget)
public:
    PrivateData(DAChart3DSettingWidget* p);
    void setupUi(QStackedWidget* stackWidget);
    void setComboboxFixSelectionArea(QComboBox* com, DAChart3DSettingWidget::SettingWidgetType widType);

public:
    QPointer< Qwt3DPlot > mPlot3D;
    // 固定子面板（07计划实现，本阶段为占位nullptr）
    QWidget* mChart3DSettingWidget { nullptr };      // TODO: 07计划替换为DAChart3DPlotSettingPanel*
    QWidget* mCoordSysSettingWidget { nullptr };     // TODO: 07计划替换为DAChart3DCoordSysSettingPanel*
    QWidget* mXAxisSettingWidget { nullptr };        // TODO: 07计划替换为DAChart3DAxisSettingPanel*
    QWidget* mYAxisSettingWidget { nullptr };        // TODO: 07计划替换为DAChart3DAxisSettingPanel*
    QWidget* mZAxisSettingWidget { nullptr };        // TODO: 07计划替换为DAChart3DAxisSettingPanel*
    QWidget* mColorLegendSettingWidget { nullptr };  // TODO: 07计划替换为DAChart3DColorLegendSettingPanel*
    DAChart3DCommonItemsSettingWidget* mPlot3DItemSettingWidget { nullptr };
};

DAChart3DSettingWidget::PrivateData::PrivateData(DAChart3DSettingWidget* p) : q_ptr(p)
{
}

/**
 * @brief 创建各固定子面板并添加到stackedWidget
 *
 * 本计划阶段：固定面板使用占位QWidget，待07计划替换为具体面板。
 */
void DAChart3DSettingWidget::PrivateData::setupUi(QStackedWidget* stackWidget)
{
    // 07计划完成后替换为：
    // mChart3DSettingWidget    = new DAChart3DPlotSettingPanel(stackWidget);
    // mCoordSysSettingWidget   = new DAChart3DCoordSysSettingPanel(stackWidget);
    // mXAxisSettingWidget      = new DAChart3DAxisSettingPanel(AXIS::X1, stackWidget);
    // mYAxisSettingWidget      = new DAChart3DAxisSettingPanel(AXIS::Y1, stackWidget);
    // mZAxisSettingWidget      = new DAChart3DAxisSettingPanel(AXIS::Z1, stackWidget);
    // mColorLegendSettingWidget = new DAChart3DColorLegendSettingPanel(stackWidget);

    // 本阶段占位：
    mChart3DSettingWidget       = new QWidget(stackWidget);
    mCoordSysSettingWidget      = new QWidget(stackWidget);
    mXAxisSettingWidget         = new QWidget(stackWidget);
    mYAxisSettingWidget         = new QWidget(stackWidget);
    mZAxisSettingWidget         = new QWidget(stackWidget);
    mColorLegendSettingWidget   = new QWidget(stackWidget);

    mPlot3DItemSettingWidget = new DAChart3DCommonItemsSettingWidget(stackWidget);

    stackWidget->addWidget(mChart3DSettingWidget);
    stackWidget->addWidget(mCoordSysSettingWidget);
    stackWidget->addWidget(mXAxisSettingWidget);
    stackWidget->addWidget(mYAxisSettingWidget);
    stackWidget->addWidget(mZAxisSettingWidget);
    stackWidget->addWidget(mColorLegendSettingWidget);
    stackWidget->addWidget(mPlot3DItemSettingWidget);
}

void DAChart3DSettingWidget::PrivateData::setComboboxFixSelectionArea(
    QComboBox* com, DAChart3DSettingWidget::SettingWidgetType widType)
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
// DAChart3DSettingWidget
//----------------------------------------------------

DAChart3DSettingWidget::DAChart3DSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChart3DSettingWidget)
{
    ui->setupUi(this);

    // 注册所有已知3D面板类型（只需调用一次）
    DAChart3DItemSettingPanelFactory::instance().registerAllKnown3DPanels();

    d_ptr->setupUi(ui->stackedWidget);

    connect(ui->comboBoxSelectItem,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChart3DSettingWidget::onComboBoxItemIndexChanged);
}

DAChart3DSettingWidget::~DAChart3DSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置3D绘图
 *
 * 通过qobject_cast判断plot是否为DAChart3DWidget，如果是则连接
 * plot3DItemAttached信号以在item attach/detach时同步combobox。
 */
void DAChart3DSettingWidget::setPlot3D(Qwt3DPlot* plot)
{
    DA_D(d);
    if (d->mPlot3D == plot) {
        return;
    }
    if (d->mPlot3D) {
        disconnect(d->mPlot3D.data(), nullptr, this, nullptr);
    }
    d->mPlot3D = plot;
    updateUI();
    if (plot) {
        // 连接DAChart3DWidget::plot3DItemAttached信号以同步combobox
        DAChart3DWidget* chart3DWidget = qobject_cast< DAChart3DWidget* >(plot);
        if (chart3DWidget) {
            connect(chart3DWidget, &DAChart3DWidget::plot3DItemAttached,
                    this, &DAChart3DSettingWidget::onPlot3DItemAttached);
        }
    }
    // TODO: 07计划完成后，对固定面板调用setTarget(plot)
}

/**
 * @brief 获取当前管理的3D绘图指针
 */
Qwt3DPlot* DAChart3DSettingWidget::getPlot3D() const
{
    DA_DC(d);
    return d->mPlot3D.data();
}

/**
 * @brief 更新界面
 */
void DAChart3DSettingWidget::updateUI()
{
    resetComboBox();
}

/**
 * @brief 重置combobox
 */
void DAChart3DSettingWidget::resetComboBox()
{
    DA_D(d);
    ui->comboBoxSelectItem->clear();
    if (d->mPlot3D.isNull()) {
        return;
    }
    // 插入固定内容
    // TODO: 07计划添加专用图标
    ui->comboBoxSelectItem->addItem(tr("3D Chart Area"), Setting3DChart);          // cn:3D图表区
    ui->comboBoxSelectItem->addItem(tr("Coordinate System"), Setting3DCoordSys);  // cn:坐标系
    ui->comboBoxSelectItem->addItem(tr("X Axis"), Setting3DXAxis);                 // cn:X轴
    ui->comboBoxSelectItem->addItem(tr("Y Axis"), Setting3DYAxis);                 // cn:Y轴
    ui->comboBoxSelectItem->addItem(tr("Z Axis"), Setting3DZAxis);                 // cn:Z轴
    ui->comboBoxSelectItem->addItem(tr("Color Legend"), Setting3DColorLegend);     // cn:颜色图例
    // 动态条目
    const QList< Qwt3DPlotItem* > itemlist = d->mPlot3D->itemList();
    for (Qwt3DPlotItem* item : itemlist) {
        appendPlot3DItemToComboBox(item);
    }
}

/**
 * @brief 设置当前选中的3D plotitem
 */
void DAChart3DSettingWidget::setCurrentPlot3DItem(Qwt3DPlotItem* item)
{
    if (!item) {
        return;
    }
    // 1. 查找combobox中是否存在这个item
    int index = findComboBoxIndexFromPlot3DItem(item);
    if (index < 0) {
        Qwt3DPlot* plot = item->plot();
        if (!plot) {
            return;
        }
        setPlot3D(plot);
        index = findComboBoxIndexFromPlot3DItem(item);
        if (index < 0) {
            return;
        }
    }
    // 2. 切换combobox
    if (ui->comboBoxSelectItem->currentIndex() != index) {
        QSignalBlocker b(ui->comboBoxSelectItem);
        ui->comboBoxSelectItem->setCurrentIndex(index);
    }
    // 3. 切换到items设置页面并设置item
    if (ui->stackedWidget->currentWidget() != d_ptr->mPlot3DItemSettingWidget) {
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlot3DItemSettingWidget);
    }
    d_ptr->mPlot3DItemSettingWidget->setPlot3DItem(item);
}

Qwt3DPlotItem* DAChart3DSettingWidget::getCurrentPlot3DItem() const
{
    return d_ptr->mPlot3DItemSettingWidget->getPlot3DItem();
}

/**
 * @brief 显示对应设置窗口
 */
void DAChart3DSettingWidget::showSettingWidget(SettingWidgetType widType)
{
    switch (widType) {
    case Setting3DChart:
        ui->stackedWidget->setCurrentWidget(d_ptr->mChart3DSettingWidget);
        break;
    case Setting3DCoordSys:
        ui->stackedWidget->setCurrentWidget(d_ptr->mCoordSysSettingWidget);
        break;
    case Setting3DXAxis:
        ui->stackedWidget->setCurrentWidget(d_ptr->mXAxisSettingWidget);
        break;
    case Setting3DYAxis:
        ui->stackedWidget->setCurrentWidget(d_ptr->mYAxisSettingWidget);
        break;
    case Setting3DZAxis:
        ui->stackedWidget->setCurrentWidget(d_ptr->mZAxisSettingWidget);
        break;
    case Setting3DColorLegend:
        ui->stackedWidget->setCurrentWidget(d_ptr->mColorLegendSettingWidget);
        break;
    case Setting3DPlotItems:
        ui->stackedWidget->setCurrentWidget(d_ptr->mPlot3DItemSettingWidget);
        break;
    }
}

/**
 * @brief 设置选中内容
 *
 * DAFigureElementSelection由绘图结构树传递。
 * 根据选择类型路由到对应设置页面。
 */
void DAChart3DSettingWidget::setSelection(const DAFigureElementSelection& sel)
{
    if (!sel.plot3D) {
        return;
    }
    if (sel.plot3D != getPlot3D()) {
        setPlot3D(sel.plot3D);
    }
    switch (sel.selectionType) {
    case DAFigureElementSelection::SelectPlot3D:
        show3DChartSetting();
        return;
    case DAFigureElementSelection::SelectPlot3DItem:
        showPlot3DItemSetting(sel.plot3DItem);
        return;
    default:
        break;
    }
}

void DAChart3DSettingWidget::show3DChartSetting()
{
    showSettingWidget(Setting3DChart);
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, Setting3DChart);
}

void DAChart3DSettingWidget::show3DCoordSysSetting()
{
    showSettingWidget(Setting3DCoordSys);
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, Setting3DCoordSys);
}

void DAChart3DSettingWidget::show3DXAxisSetting()
{
    showSettingWidget(Setting3DXAxis);
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, Setting3DXAxis);
}

void DAChart3DSettingWidget::show3DYAxisSetting()
{
    showSettingWidget(Setting3DYAxis);
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, Setting3DYAxis);
}

void DAChart3DSettingWidget::show3DZAxisSetting()
{
    showSettingWidget(Setting3DZAxis);
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, Setting3DZAxis);
}

void DAChart3DSettingWidget::show3DColorLegendSetting()
{
    showSettingWidget(Setting3DColorLegend);
    QSignalBlocker b(ui->comboBoxSelectItem);
    d_ptr->setComboboxFixSelectionArea(ui->comboBoxSelectItem, Setting3DColorLegend);
}

void DAChart3DSettingWidget::showPlot3DItemSetting(Qwt3DPlotItem* item)
{
    setCurrentPlot3DItem(item);
}

DAChart3DCommonItemsSettingWidget* DAChart3DSettingWidget::getChart3DCommonItemsSettingWidget() const
{
    return d_ptr->mPlot3DItemSettingWidget;
}

void DAChart3DSettingWidget::changeEvent(QEvent* e)
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
 * @brief combobox切换槽
 */
void DAChart3DSettingWidget::onComboBoxItemIndexChanged(int index)
{
    if (index < Setting3DPlotItems) {
        // 固定页面
        QVariant v = ui->comboBoxSelectItem->itemData(index);
        if (!v.isValid()) {
            return;
        }
        SettingWidgetType widType = static_cast< SettingWidgetType >(v.toInt());
        showSettingWidget(widType);
    } else {
        // 3D plotItem
        Qwt3DPlotItem* item = getPlot3DItemFromComboBox(index);
        if (ui->stackedWidget->currentWidget() != d_ptr->mPlot3DItemSettingWidget) {
            ui->stackedWidget->setCurrentWidget(d_ptr->mPlot3DItemSettingWidget);
        }
        d_ptr->mPlot3DItemSettingWidget->setPlot3DItem(item);
    }
}

/**
 * @brief 通过plotitem查找combobox索引
 */
int DAChart3DSettingWidget::findComboBoxIndexFromPlot3DItem(const Qwt3DPlotItem* item) const
{
    const int count = ui->comboBoxSelectItem->count();
    for (int i = Setting3DPlotItems; i < count; ++i) {
        Qwt3DPlotItem* innerItem = getPlot3DItemFromComboBox(i);
        if (innerItem == item) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 添加plotitem到combobox
 */
void DAChart3DSettingWidget::appendPlot3DItemToComboBox(const Qwt3DPlotItem* item)
{
    QString name = item->title();
    if (name.isEmpty()) {
        name = tr("Unnamed Item");  // cn:未命名项
    }
    ui->comboBoxSelectItem->addItem(name, Setting3DPlotItems);
    int lastIndex = ui->comboBoxSelectItem->count() - 1;
    ui->comboBoxSelectItem->setItemData(lastIndex, QVariant::fromValue(reinterpret_cast< quintptr >(item)), RolePlot3DItemPtr);
}

Qwt3DPlotItem* DAChart3DSettingWidget::getPlot3DItemFromComboBox(int index) const
{
    QVariant v = ui->comboBoxSelectItem->itemData(index, RolePlot3DItemPtr);
    if (!v.isValid()) {
        return nullptr;
    }
    return reinterpret_cast< Qwt3DPlotItem* >(v.value< quintptr >());
}

void DAChart3DSettingWidget::removePlot3DItemFromComboBox(const Qwt3DPlotItem* item)
{
    int index = findComboBoxIndexFromPlot3DItem(item);
    if (index < 0) {
        return;
    }
    ui->comboBoxSelectItem->removeItem(index);
}

/**
 * @brief 3D plot的item挂载/卸载
 *
 * 连接DAChart3DWidget::plot3DItemAttached信号，在item attach时
 * 添加到combobox并自动选中，在detach时从combobox移除。
 * @param item 发生挂载/卸载的item
 * @param on true表示attach，false表示detach
 */
void DAChart3DSettingWidget::onPlot3DItemAttached(Qwt3DPlotItem* item, bool on)
{
    Qwt3DPlot* plot = item->plot();
    if (plot) {
        if (plot != getPlot3D()) {
            // 异常情况：item 不属于当前管理的 plot
            return;
        }
    }
    if (on) {
        // 新增
        appendPlot3DItemToComboBox(item);
        // 自动选中新添加的plotItem并显示其设置面板
        setCurrentPlot3DItem(item);
    } else {
        removePlot3DItemFromComboBox(item);
    }
}

}  // namespace DA
