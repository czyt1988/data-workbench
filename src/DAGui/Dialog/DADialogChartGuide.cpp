#include "DADialogChartGuide.h"
#include "ui_DADialogChartGuide.h"
#include <QHash>
#include <QListWidgetItem>
#include <QSet>
#include <utility>
// DA
#include "DADataManager.h"
#include "ChartAddItem/DAChartAddItemWidgetFactory.h"
#include "ChartAddItem/DAAbstractChartAddItemWidget.h"
#include "ChartAddItem/DAAbstractChart3DAddItemWidget.h"
#include "ChartAddItem/DAAbstractChartAddWidgetBase.h"

namespace DA
{
class DADialogChartGuide::PrivateData
{
    DA_DECLARE_PUBLIC(DADialogChartGuide)
public:
    explicit PrivateData(DADialogChartGuide* p);
    // 图表类型 → 已创建的页面（按需创建，类型用作 key 所以这里是 int）
    QHash< int, QWidget* > pages;
    // 已经赋过数据管理器的页面
    QSet< DAAbstractChartAddWidgetBase* > initializedWidgets;
};

DADialogChartGuide::PrivateData::PrivateData(DADialogChartGuide* p) : q_ptr(p)
{
}
//----------------------------------------------------
// DADialogChartGuide
//----------------------------------------------------

DADialogChartGuide::DADialogChartGuide(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADialogChartGuide)
{
    ui->setupUi(this);
    initListWidget();
    connect(ui->listWidgetChartType, &QListWidget::currentItemChanged, this, &DADialogChartGuide::onListWidgetCurrentItemChanged);
}

DADialogChartGuide::~DADialogChartGuide()
{
    delete ui;
}

/**
 * @brief 用注册表填充左侧类型列表（二维与三维，注册顺序即显示顺序）
 */
void DADialogChartGuide::initListWidget()
{
    const QList< DAChartAddItemWidgetDescriptor > descs = DAChartAddItemWidgetFactory::instance().allDescriptors();
    for (const DAChartAddItemWidgetDescriptor& desc : descs) {
        if (DAChartAddWidgetFamily::Stats == desc.family) {
            continue;
        }
        QListWidgetItem* item = new QListWidgetItem(desc.icon(), desc.text());
        item->setData(Qt::UserRole, static_cast< int >(desc.type));
        ui->listWidgetChartType->addItem(item);
    }
    ui->listWidgetChartType->setCurrentRow(0);
}

/**
 * @brief 设置datamanager
 * @param dmgr
 */
void DADialogChartGuide::setDataManager(DADataManager* dmgr)
{
    mDataMgr = dmgr;
    DA_D(d);
    d->initializedWidgets.clear();
    // 已创建的页面立即刷新
    for (QWidget* page : std::as_const(d->pages)) {
        if (auto* w = qobject_cast< DAAbstractChartAddWidgetBase* >(page)) {
            ensureWidgetDataManager(w);
        }
    }
    // 当前页可能在列表首次选中时已创建，这里确保它也拿到数据管理器
    if (QListWidgetItem* cur = ui->listWidgetChartType->currentItem()) {
        selectPage(static_cast< DA::DAChartTypes >(cur->data(Qt::UserRole).toInt()));
    }
}

/**
 * @brief 给页面设置数据管理器，同一页面只设置一次
 * @param w 页面
 */
void DADialogChartGuide::ensureWidgetDataManager(DAAbstractChartAddWidgetBase* w)
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

/**
 * @brief 按注册表创建页面并加入页面栈
 * @param type 图表类型
 * @return 页面指针，类型未注册返回 nullptr
 */
QWidget* DADialogChartGuide::ensurePage(DA::DAChartTypes type)
{
    DA_D(d);
    const DAChartAddItemWidgetDescriptor* desc = DAChartAddItemWidgetFactory::instance().descriptor(type);
    if (nullptr == desc || !desc->creator) {
        return nullptr;
    }
    // 允许多种类型共用同一个页面（如散点复用曲线页面），保证切换类型时已选数据不丢
    const DA::DAChartTypes pageType = (DA::DAChartTypes::Unknow == desc->pageType) ? type : desc->pageType;
    const int key                   = static_cast< int >(pageType);
    if (QWidget* page = d->pages.value(key, nullptr)) {
        return page;
    }
    QWidget* page = desc->creator();
    if (nullptr == page) {
        return nullptr;
    }
    ui->stackedWidget->addWidget(page);
    d->pages.insert(key, page);
    ensureWidgetDataManager(qobject_cast< DAAbstractChartAddWidgetBase* >(page));
    return page;
}

/**
 * @brief 切换到指定类型的页面
 * @param type 图表类型
 */
void DADialogChartGuide::selectPage(DA::DAChartTypes type)
{
    QWidget* page = ensurePage(type);
    if (nullptr == page) {
        return;
    }
    ui->stackedWidget->setCurrentWidget(page);
}

/**
 * @brief 获取当前的绘图类型
 * @return
 */
DA::DAChartTypes DADialogChartGuide::getCurrentChartType() const
{
    QListWidgetItem* item = ui->listWidgetChartType->currentItem();
    if (item == nullptr) {
        return DA::DAChartTypes::Unknow;
    }
    return static_cast< DA::DAChartTypes >(item->data(Qt::UserRole).toInt());
}

/**
 * @brief 获取绘图item
 * @return  如果没有返回nullptr
 */
QwtPlotItem* DADialogChartGuide::createPlotItem()
{
    DAAbstractChartAddItemWidget* w = getCurrentChartAddItemWidget();
    if (nullptr == w) {
        return nullptr;
    }
    QwtPlotItem* item = w->createPlotItem();
    if (nullptr == item) {
        return nullptr;
    }
    // 针对不同的类型设置item属性
    initSetPlotItem(item);
    return item;
}

/**
 * @brief 批量获取绘图item（如曲线的 1 个 x + 多个 y）
 * @return 无法创建时返回空列表
 */
QList< QwtPlotItem* > DADialogChartGuide::createPlotItems()
{
    DAAbstractChartAddItemWidget* w = getCurrentChartAddItemWidget();
    if (nullptr == w) {
        return {};
    }
    QList< QwtPlotItem* > items = w->createPlotItems();
    for (QwtPlotItem* item : std::as_const(items)) {
        initSetPlotItem(item);
    }
    return items;
}

/**
 * @brief 获取坐标轴标题推荐
 * @return 当前页面不提供时返回 false
 */
bool DADialogChartGuide::getAxisTitleHint(QString& xTitle, QString& yTitle) const
{
    DAAbstractChartAddItemWidget* w = getCurrentChartAddItemWidget();
    if (nullptr == w) {
        return false;
    }
    return w->getAxisTitleHint(xTitle, yTitle);
}

DAAbstractChartAddItemWidget* DADialogChartGuide::getCurrentChartAddItemWidget() const
{
    return qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->currentWidget());
}

/**
 * @brief 根据类型获取二维绘图指引窗口
 * @param chartType 图表类型
 * @return 类型未注册或不是二维类型时返回 nullptr
 */
DAAbstractChartAddItemWidget* DADialogChartGuide::getChartAddItemWidget(DAChartTypes chartType)
{
    return qobject_cast< DAAbstractChartAddItemWidget* >(ensurePage(chartType));
}

/**
 * @brief 根据当前绘图类型设置item属性
 * @param item 待修饰的绘图项
 *
 * 修饰动作来自注册表的 initPlotItem，例如散点图把曲线改为点状。
 */
void DADialogChartGuide::initSetPlotItem(QwtPlotItem* item)
{
    const DAChartAddItemWidgetDescriptor* desc = DAChartAddItemWidgetFactory::instance().descriptor(getCurrentChartType());
    if (desc && desc->initPlotItem) {
        desc->initPlotItem(item);
    }
}

/**
 * @brief 判断是否为3D图表类型
 * @param t 图表类型
 * @return 如果是3D类型返回true
 */
bool DADialogChartGuide::is3DChartType(DA::DAChartTypes t)
{
    return DAChartAddItemWidgetFactory::is3DType(t);
}

/**
 * @brief 创建3D绘图item
 * @return 如果无法创建返回nullptr
 */
Qwt3DPlotItem* DADialogChartGuide::create3DPlotItem()
{
    DAAbstractChart3DAddItemWidget* w = getCurrentChart3DAddItemWidget();
    if (nullptr == w) {
        return nullptr;
    }
    Qwt3DPlotItem* item = w->create3DPlotItem();
    if (nullptr == item) {
        return nullptr;
    }
    initSet3DPlotItem(item);
    return item;
}

/**
 * @brief 获取当前的3D绘图指引窗口
 * @return
 */
DAAbstractChart3DAddItemWidget* DADialogChartGuide::getCurrentChart3DAddItemWidget() const
{
    return qobject_cast< DAAbstractChart3DAddItemWidget* >(ui->stackedWidget->currentWidget());
}

/**
 * @brief 根据类型获取3D绘图指引窗口
 * @param chartType 图表类型
 * @return 类型未注册或不是三维类型时返回 nullptr
 */
DAAbstractChart3DAddItemWidget* DADialogChartGuide::getChartAdd3DItemWidget(DAChartTypes chartType)
{
    return qobject_cast< DAAbstractChart3DAddItemWidget* >(ensurePage(chartType));
}

/**
 * @brief 根据当前绘图类型设置3D item属性
 * @note 当前为占位实现，3D item 属性由后续的设置面板处理
 */
void DADialogChartGuide::initSet3DPlotItem(Qwt3DPlotItem* item)
{
    Q_UNUSED(item)
}

/**
 * @brief 设置当前的绘图类型
 * @param t
 */
void DADialogChartGuide::setCurrentChartType(DA::DAChartTypes t)
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
}

void DADialogChartGuide::onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);
    if (nullptr == current) {
        return;
    }
    selectPage(static_cast< DA::DAChartTypes >(current->data(Qt::UserRole).toInt()));
}

}  // end DA
