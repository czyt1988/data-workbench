#include "DADialogStatsChartGuide.h"
#include "ui_DADialogStatsChartGuide.h"
#include <QHash>
#include <QListWidgetItem>
#include <QSet>
#include <utility>
// DA
#include "DADataManager.h"
#include "ChartAddItem/DAChartAddItemWidgetFactory.h"
#include "ChartAddItem/DAAbstractStatsChartAddWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"

namespace DA
{

class DADialogStatsChartGuide::PrivateData
{
    DA_DECLARE_PUBLIC(DADialogStatsChartGuide)
public:
    explicit PrivateData(DADialogStatsChartGuide* p);
    // 图表类型 → 已创建的页面（按需创建，类型用作 key 所以这里是 int）
    QHash< int, QWidget* > pages;
    // 已经赋过数据管理器的页面
    QSet< DAAbstractStatsChartAddWidget* > initializedWidgets;
};

DADialogStatsChartGuide::PrivateData::PrivateData(DADialogStatsChartGuide* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DADialogStatsChartGuide
//----------------------------------------------------

DADialogStatsChartGuide::DADialogStatsChartGuide(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADialogStatsChartGuide)
{
    ui->setupUi(this);
    initListWidget();
    // 信号连接
    connect(ui->listWidgetChartType, &QListWidget::currentItemChanged, this, &DADialogStatsChartGuide::onListWidgetCurrentItemChanged);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DADialogStatsChartGuide::onAccepted);
}

DADialogStatsChartGuide::~DADialogStatsChartGuide()
{
    delete ui;
}

/**
 * @brief 用注册表填充左侧类型列表（只取统计家族）
 */
void DADialogStatsChartGuide::initListWidget()
{
    const QList< DAChartAddItemWidgetDescriptor > descs =
        DAChartAddItemWidgetFactory::instance().descriptors(DAChartAddWidgetFamily::Stats);
    for (const DAChartAddItemWidgetDescriptor& desc : descs) {
        QListWidgetItem* item = new QListWidgetItem(desc.icon(), desc.text());
        item->setData(Qt::UserRole, static_cast< int >(desc.type));
        ui->listWidgetChartType->addItem(item);
    }
    // 默认选中第一项
    ui->listWidgetChartType->setCurrentRow(0);
}

/**
 * @brief 按注册表创建页面并加入页面栈
 * @param type 图表类型
 * @return 页面指针；未注册或不是统计类型时返回 nullptr
 */
QWidget* DADialogStatsChartGuide::ensurePage(DA::DAChartTypes type)
{
    DA_D(d);
    const int key = static_cast< int >(type);
    if (QWidget* page = d->pages.value(key, nullptr)) {
        return page;
    }
    const DAChartAddItemWidgetDescriptor* desc = DAChartAddItemWidgetFactory::instance().descriptor(type);
    if (nullptr == desc || DAChartAddWidgetFamily::Stats != desc->family || !desc->creator) {
        return nullptr;
    }
    QWidget* page = desc->creator();
    if (nullptr == page) {
        return nullptr;
    }
    ui->stackedWidget->addWidget(page);
    d->pages.insert(key, page);
    if (auto* w = qobject_cast< DAAbstractStatsChartAddWidget* >(page)) {
        // 信号签名一致，直接转发为对话框的同名信号
        connect(w, &DAAbstractStatsChartAddWidget::plotRequested, this, &DADialogStatsChartGuide::plotRequested);
        ensureWidgetDataManager(w);
        propagateFigureChart(w);
    }
    return page;
}

/**
 * @brief 切换到指定类型的页面
 * @param type 图表类型
 */
void DADialogStatsChartGuide::selectPage(DA::DAChartTypes type)
{
    QWidget* page = ensurePage(type);
    if (nullptr == page) {
        return;
    }
    ui->stackedWidget->setCurrentWidget(page);
}

void DADialogStatsChartGuide::ensureWidgetDataManager(DAAbstractStatsChartAddWidget* w)
{
    DA_D(d);
    if (nullptr == w || nullptr == mDataMgr) {
        return;
    }
    if (d->initializedWidgets.contains(w)) {
        return;
    }
    w->setDataManager(mDataMgr);
    d->initializedWidgets.insert(w);
}

void DADialogStatsChartGuide::propagateFigureChart(DAAbstractStatsChartAddWidget* w)
{
    if (w) {
        w->setFigureWidget(mFigureWidget);
        w->setChartWidget(mChartWidget);
    }
}

void DADialogStatsChartGuide::setDataManager(DADataManager* dmgr)
{
    mDataMgr = dmgr;
    DA_D(d);
    d->initializedWidgets.clear();
    for (QWidget* page : std::as_const(d->pages)) {
        if (auto* w = qobject_cast< DAAbstractStatsChartAddWidget* >(page)) {
            ensureWidgetDataManager(w);
        }
    }
    // 当前页可能在列表首次选中时已创建，这里确保它也拿到数据管理器
    if (QListWidgetItem* cur = ui->listWidgetChartType->currentItem()) {
        selectPage(static_cast< DA::DAChartTypes >(cur->data(Qt::UserRole).toInt()));
    }
}

void DADialogStatsChartGuide::setFigureWidget(DAFigureWidget* fig)
{
    mFigureWidget = fig;
    DA_D(d);
    for (QWidget* page : std::as_const(d->pages)) {
        if (auto* w = qobject_cast< DAAbstractStatsChartAddWidget* >(page)) {
            w->setFigureWidget(fig);
        }
    }
}

void DADialogStatsChartGuide::setChartWidget(DAChartWidget* chart)
{
    mChartWidget = chart;
    DA_D(d);
    for (QWidget* page : std::as_const(d->pages)) {
        if (auto* w = qobject_cast< DAAbstractStatsChartAddWidget* >(page)) {
            w->setChartWidget(chart);
        }
    }
}

DA::DAChartTypes DADialogStatsChartGuide::getCurrentChartType() const
{
    QListWidgetItem* item = ui->listWidgetChartType->currentItem();
    if (item == nullptr) {
        return DA::DAChartTypes::Unknow;
    }
    return static_cast< DA::DAChartTypes >(item->data(Qt::UserRole).toInt());
}

void DADialogStatsChartGuide::setCurrentChartType(DA::DAChartTypes t)
{
    int c = ui->listWidgetChartType->count();
    for (int i = 0; i < c; ++i) {
        auto item = ui->listWidgetChartType->item(i);
        int v     = item->data(Qt::UserRole).toInt();
        if (v == static_cast< int >(t)) {
            ui->listWidgetChartType->setCurrentItem(item);
            break;
        }
    }
    // 确保当前 widget 始终获取到 dataManager/figure/chart（同类型重复调用时
    // currentItemChanged 不触发，需要手动传播）
    if (DAAbstractStatsChartAddWidget* w = getCurrentStatsChartAddWidget()) {
        ensureWidgetDataManager(w);
        propagateFigureChart(w);
    }
}

DAAbstractStatsChartAddWidget* DADialogStatsChartGuide::getCurrentStatsChartAddWidget() const
{
    return qobject_cast< DAAbstractStatsChartAddWidget* >(ui->stackedWidget->currentWidget());
}

/**
 * @brief 根据类型获取统计绘图设置 widget
 * @param chartType 图表类型
 * @return 未注册或不是统计类型时返回 nullptr
 */
DAAbstractStatsChartAddWidget* DADialogStatsChartGuide::getStatsChartAddWidget(DAChartTypes chartType)
{
    return qobject_cast< DAAbstractStatsChartAddWidget* >(ensurePage(chartType));
}

void DADialogStatsChartGuide::onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);
    if (nullptr == current) {
        return;
    }
    selectPage(static_cast< DA::DAChartTypes >(current->data(Qt::UserRole).toInt()));
}

void DADialogStatsChartGuide::onAccepted()
{
    DAAbstractStatsChartAddWidget* w = getCurrentStatsChartAddWidget();
    if (nullptr == w) {
        return;
    }
    // 调用 widget 的确认逻辑：校验数据、构建参数并发射 plotRequested
    w->onButtonBoxAccepted();
}

}  // namespace DA
