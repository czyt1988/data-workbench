#include "DAFigureTreeModel.h"
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include "DAChartUtil.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_grid.h"
#include "qwt_text.h"
#include "qwt_colormap.h"
#include "qwt_column_symbol.h"
#include "qwt_plot_textlabel.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_graphic.h"
#include "qwt_symbol.h"
#include "DAChartUtil.h"

#include "DAStandardItemPlot.h"
#include "DAStandardItemPlotScale.h"
#include "DAStandardItemPlotItem.h"
#include "DAStandardItemPlot3D.h"
#include "DAStandardItemPlot3DItem.h"
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"
#define DAFigureTreeModel_Debug_Print 1

namespace DA
{

class DAFigureTreeModel::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureTreeModel)
public:
    PrivateData(DAFigureTreeModel* p);
    QwtFigure* mFigure;
    QHash< QwtPlot*, QStandardItem* > mPlotItems;
    QHash< QwtPlotItem*, QStandardItem* > mPlotItemItems;
    // 3D 成员变量
    QHash< DAChart3DWidget*, QStandardItem* > mPlot3DItems;       ///< 3D chart -> layer 树节点
    QHash< Qwt3DPlotItem*, QStandardItem* > mPlot3DItemItems;    ///< 3D plot item -> 树节点
    QHash< DAChart3DWidget*, QList< QMetaObject::Connection > > mPlot3DConnections;  ///< 3D chart 信号连接
    QPointer< DAFigureWidget > mFigureWidget;  ///< DAFigureWidget 缓存
    // 连接管理
    QList< QMetaObject::Connection > mFigureConnections;
    QHash< QwtPlot*, QList< QMetaObject::Connection > > mPlotConnections;
};

/**
 * @brief PrivateData 构造函数
 * @param p 父对象 DAFigureTreeModel 指针
 */
DAFigureTreeModel::PrivateData::PrivateData(DAFigureTreeModel* p) : q_ptr(p)
{
}


//----------------------------------------------------
//
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param parent 父 QObject 指针
 */
DAFigureTreeModel::DAFigureTreeModel(QObject* parent) : QStandardItemModel(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAFigureTreeModel::~DAFigureTreeModel()
{
    clearAllConnections();
}

/**
 * @brief 获取关联的QwtFigure
 * @return QwtFigure指针
 */
QwtFigure* DAFigureTreeModel::figure() const
{
    DA_DC(d);
    return d->mFigure;
}

/**
 * @brief 设置 DAFigureWidget，缓存指针后调用 setFigure()
 * @param figWidget
 */
void DAFigureTreeModel::setFigureWidget(DA::DAFigureWidget* figWidget)
{
    DA_D(d);
    if (d->mFigureWidget == figWidget) {
        return;
    }
    d->mFigureWidget = figWidget;
    setFigure(figWidget ? figWidget->figure() : nullptr);
}

/**
 * @brief 设置关联的 QwtFigure
 * @param figure QwtFigure 指针
 */
void DAFigureTreeModel::setFigure(QwtFigure* figure)
{
    DA_D(d);
    if (d->mFigure == figure) {
        return;
    }

    // 清除所有现有连接
    clearAllConnections();

    d->mFigure = figure;
    d->mPlotItems.clear();
    d->mPlotItemItems.clear();
    // 清理 3D 状态
    d->mPlot3DItems.clear();
    d->mPlot3DItemItems.clear();
    d->mPlot3DConnections.clear();

    if (d->mFigure) {
        // 连接figure信号
        d->mFigureConnections << connect(d->mFigure, &QwtFigure::axesAdded, this, &DAFigureTreeModel::onAxesAdded);
        d->mFigureConnections << connect(d->mFigure, &QwtFigure::axesRemoved, this, &DAFigureTreeModel::onAxesRemoved);
        d->mFigureConnections << connect(d->mFigure, &QwtFigure::figureCleared, this, &DAFigureTreeModel::onFigureCleared);
        d->mFigureConnections << connect(
            d->mFigure, &QwtFigure::currentAxesChanged, this, &DAFigureTreeModel::onCurrentAxesChanged);
    }

    setupModel();
}

/**
 * @brief 断开所有信号连接
 *
 * 清理 figure、plot 以及 3D chart 的所有信号连接
 */
void DAFigureTreeModel::clearAllConnections()
{
    DA_D(d);
    // 断开所有figure连接
    for (const QMetaObject::Connection& conn : d->mFigureConnections) {
        disconnect(conn);
    }
    d->mFigureConnections.clear();

    // 断开所有plot连接
    for (auto it = d->mPlotConnections.begin(); it != d->mPlotConnections.end(); ++it) {
        for (const QMetaObject::Connection& conn : it.value()) {
            disconnect(conn);
        }
    }
    d->mPlotConnections.clear();

    // 断开所有3D chart信号连接
    for (auto it = d->mPlot3DConnections.constBegin(); it != d->mPlot3DConnections.constEnd(); ++it) {
        const QList< QMetaObject::Connection >& conns = it.value();
        for (const QMetaObject::Connection& conn : conns) {
            disconnect(conn);
        }
    }
    d->mPlot3DConnections.clear();
}

/**
 * @brief 构建树模型
 *
 * 清空模型后重新填充所有 2D 和 3D chart 的树节点
 */
void DAFigureTreeModel::setupModel()
{
    DA_D(d);
    clear();
    // 设置三列表头
    setHorizontalHeaderLabels(QStringList() << tr("element")   // cn:绘图元素
                                            << tr("visible")   // cn:可见性
                                            << tr("property")  // cn:属性
    );
    d->mPlotItems.clear();
    d->mPlotItemItems.clear();
    d->mPlot3DItems.clear();
    d->mPlot3DItemItems.clear();

    if (!d->mFigure)
        return;

    QStandardItem* rootItem = invisibleRootItem();

    // 遍历 2D chart（现有逻辑不变）
    const QList< QwtPlot* > plots = d->mFigure->allAxes();
    for (QwtPlot* plot : plots) {
        addPlotToModel(plot, rootItem);
    }

    // 遍历 3D chart
    if (d->mFigureWidget) {
        const QList< DAChart3DWidget* > charts3D = d->mFigureWidget->get3DCharts();
        for (DAChart3DWidget* chart3D : charts3D) {
            add3DChartToModel(chart3D, rootItem);
        }
        // 连接 DAFigureWidget 的 3D chart 增删信号
        d->mFigureConnections << connect(d->mFigureWidget.data(), &DAFigureWidget::chart3DAdded,
                                       this, &DAFigureTreeModel::on3DChartAdded);
        d->mFigureConnections << connect(d->mFigureWidget.data(), &DAFigureWidget::chart3DRemoved,
                                       this, &DAFigureTreeModel::on3DChartRemoved);
    }
}

/**
 * @brief 将 2D 绘图添加到树模型
 * @param plot QwtPlot 指针
 * @param parentItem 父节点
 */
void DAFigureTreeModel::addPlotToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    DA_D(d);
    if (!plot || d->mPlotItems.contains(plot)) {
        return;
    }

    // 创建绘图节点 - 三列
    static QIcon s_plot_icon = QIcon(":/DAFigure/icon/chart.svg");
    // 优先使用plot的title作为显示文字，为空时回退到默认"chart"
    QString chartText = plot->title().text();
    if (chartText.isEmpty()) {
        chartText = tr("chart");  // cn:绘图
    }
    QStandardItem* plotItem = new QStandardItem(chartText);
    plotItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
    plotItem->setData(NodeTypePlotFolder, RoleNodeType);
    plotItem->setIcon(s_plot_icon);
    plotItem->setEditable(false);

    // 可见性列 - 绘图节点不需要可见性控制
    QStandardItem* visibilityItem = createEmptyItem();

    // 颜色列 - 绘图节点不需要颜色显示
    QStandardItem* propertyItem = createAxesPropertyItem(plot);

    parentItem->appendRow(QList< QStandardItem* >() << plotItem << visibilityItem << propertyItem);

    // 添加宿主图层
    addLayerToModel(plot, plotItem);

    // 添加寄生图层
    const QList< QwtPlot* > parasites = plot->parasitePlots();
    for (QwtPlot* parasite : parasites) {
        addLayerToModel(parasite, plotItem);
    }
}

/**
 * @brief 添加图层节点到树模型
 * @param plot QwtPlot 指针
 * @param parentItem 父节点
 */
void DAFigureTreeModel::addLayerToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    DA_D(d);
    if (!plot) {
        return;
    }

    QStandardItem* layerItem = new DAStandardItemPlot(plot, DAStandardItemPlot::PlotText);
    // 可见性列 - 图层节点不需要可见性控制
    QStandardItem* visibilityItem = new DAStandardItemPlot(plot, DAStandardItemPlot::PlotVisible);
    // 颜色列 - 图层节点不需要颜色显示
    QStandardItem* propertyItem = new DAStandardItemPlot(plot, DAStandardItemPlot::PlotProperty);
    parentItem->appendRow(QList< QStandardItem* >() << layerItem << visibilityItem << propertyItem);

    addAxesToLayer(plot, layerItem);
    addPlotItemsToLayer(plot, layerItem);

    // 连接plot的信号
    QList< QMetaObject::Connection > plotConnections;
    plotConnections << connect(plot, &QwtPlot::itemAttached, this, &DAFigureTreeModel::onItemAttached);
    d->mPlotConnections[ plot ] = plotConnections;
    d->mPlotItems[ plot ]       = layerItem;
}

/**
 * @brief 添加坐标轴文件夹节点到图层
 * @param plot QwtPlot 指针
 * @param layerItem 图层节点
 */
void DAFigureTreeModel::addAxesToLayer(QwtPlot* plot, QStandardItem* layerItem)
{
    static QIcon s_axes_icon  = QIcon(":/DAFigure/icon/axes.svg");
    QStandardItem* axesFolder = new QStandardItem(tr("Axis"));  // cn:坐标轴
    axesFolder->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
    axesFolder->setData(NodeTypeAxesFolder, RoleNodeType);
    axesFolder->setIcon(s_axes_icon);
    axesFolder->setEditable(false);

    layerItem->appendRow(QList< QStandardItem* >() << axesFolder << createEmptyItem() << createEmptyItem());

    for (int axis = 0; axis < QwtAxis::AxisPositions; ++axis) {
        DAStandardItemPlotScale* axisItem = new DAStandardItemPlotScale(plot, axis, DAStandardItemPlotScale::PlotScaleText);
        DAStandardItemPlotScale* axisVisible =
            new DAStandardItemPlotScale(plot, axis, DAStandardItemPlotScale::PlotScaleVisible);
        DAStandardItemPlotScale* axisProperty =
            new DAStandardItemPlotScale(plot, axis, DAStandardItemPlotScale::PlotScaleProperty);
        axesFolder->appendRow(QList< QStandardItem* >() << axisItem << axisVisible << axisProperty);
    }
}

/**
 * @brief 添加图元文件夹节点及所有图元到图层
 * @param plot QwtPlot 指针
 * @param layerItem 图层节点
 */
void DAFigureTreeModel::addPlotItemsToLayer(QwtPlot* plot, QStandardItem* layerItem)
{
    QStandardItem* itemsFolder = new QStandardItem(tr("plot item"));  // cn:图元
    itemsFolder->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
    itemsFolder->setData(NodeTypeItemsFolder, RoleNodeType);
    itemsFolder->setEditable(false);
    // itemsFolder->setIcon(QIcon(":/icons/items.png"));
    layerItem->appendRow(QList< QStandardItem* >() << itemsFolder << createEmptyItem() << createEmptyItem());

    // 添加所有图元
    const QwtPlotItemList& items = plot->itemList();
    for (QwtPlotItem* item : items) {
        if (item) {
            addPlotItem(item, itemsFolder);
        }
    }
}

/**
 * @brief 添加QwtPlotItem元素
 * @param item
 * @param parentItem
 */
void DAFigureTreeModel::addPlotItem(QwtPlotItem* item, QStandardItem* parentItem)
{
    DA_D(d);
    QStandardItem* itemNode = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemText);
    // 可见性列 - 图元节点需要可见性控制
    QStandardItem* itemVisibilityItem = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemVisible);
    // 颜色列 - 图元节点需要颜色显示
    QStandardItem* itemColorItem = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemColor);
    parentItem->appendRow(QList< QStandardItem* >() << itemNode << itemVisibilityItem << itemColorItem);
    d->mPlotItemItems[ item ] = itemNode;
}

/**
 * @brief 从树中移除图元节点
 * @param item QwtPlotItem 指针
 * @param parentItem 父节点
 */
void DAFigureTreeModel::removePlotItem(QwtPlotItem* item, QStandardItem* parentItem)
{
    DA_D(d);
    QStandardItem* itemNode = d->mPlotItemItems.value(item);
    if (itemNode) {
        parentItem->removeRow(itemNode->row());
        d->mPlotItemItems.remove(item);
    }
}

/**
 * @brief 用于生成绘图对应的文字
 *
 * 如果想改变文字内容，可重写此函数
 * @param plot 绘图指针
 * @param fig figure指针
 * @return
 */
QString DAFigureTreeModel::generatePlotTitleText(QwtPlot* plot) const
{
    return DAChartUtil::plotTitle(plot, figure());
}

/**
 * @brief 生成图元的显示名称
 * @param item QwtPlotItem 指针
 * @return 图元名称字符串
 */
QString DAFigureTreeModel::generatePlotItemName(QwtPlotItem* item) const
{
    return DAChartUtil::plotItemName(item);
}

/**
 * @brief 生成图元的图标
 * @param item QwtPlotItem 指针
 * @return 图标
 */
QIcon DAFigureTreeModel::generatePlotItemIcon(QwtPlotItem* item) const
{
    return DAChartUtil::plotItemIcon(item);
}

/**
 * @brief 生成画刷的图标
 * @param b 画刷
 * @return 图标
 */
QIcon DAFigureTreeModel::generateBrushIcon(const QBrush& b) const
{
    QPixmap pixmap(22, 22);
    QPainter p(&pixmap);
    p.fillRect(pixmap.rect(), b);
    return QIcon(pixmap);
}

/**
 * @brief 坐标轴添加信号处理
 * @param plot 新增坐标轴所在的 QwtPlot 指针
 */
void DAFigureTreeModel::onAxesAdded(QwtPlot* plot)
{
    DA_D(d);
    if (plot && !d->mPlotItems.contains(plot)) {
        addPlotToModel(plot, invisibleRootItem());
    }
}

/**
 * @brief 坐标轴移除信号处理
 * @param plot 被移除坐标轴所在的 QwtPlot 指针
 */
void DAFigureTreeModel::onAxesRemoved(QwtPlot* plot)
{
    removePlotFromModel(plot);
}

/**
 * @brief figure 清空信号处理
 */
void DAFigureTreeModel::onFigureCleared()
{
    setupModel();  // 完全重建
}

/**
 * @brief 当前坐标轴变化信号处理
 * @param plot 当前选中的 QwtPlot 指针
 */
void DAFigureTreeModel::onCurrentAxesChanged(QwtPlot* plot)
{
    updateAxesPropertyItem();
}

/**
 * @brief 图元挂载/卸载信号处理
 * @param item QwtPlotItem 指针
 * @param on true=挂载，false=卸载
 */
void DAFigureTreeModel::onItemAttached(QwtPlotItem* item, bool on)
{
    DA_D(d);
    QwtPlot* plot = qobject_cast< QwtPlot* >(sender());
    if (!plot) {
        return;
    }

    QStandardItem* plotItem = findPlotItem(plot);
    if (!plotItem) {
        return;
    }

    QStandardItem* itemsFolder = findItemsFolderForPlot(plotItem, plot);
    if (!itemsFolder) {
        return;
    }

    if (on) {
        if (!d->mPlotItemItems.contains(item)) {
            addPlotItem(item, itemsFolder);
        }
    } else {
        removePlotItem(item, itemsFolder);
    }
    Q_EMIT chartItemAttached(item, on);
}

/**
 * @brief 从树模型中移除绘图节点
 *
 * 断开该 plot 的所有信号连接，清理相关图元记录，移除寄生绘图记录，
 * 最后从树中删除对应节点
 * @param plot 要移除的 QwtPlot 指针
 */
void DAFigureTreeModel::removePlotFromModel(QwtPlot* plot)
{
    DA_D(d);
    QStandardItem* plotItem = d->mPlotItems.value(plot);
    if (plotItem) {
        // 断开该plot的所有连接
        if (d->mPlotConnections.contains(plot)) {
            const auto& connects = d->mPlotConnections[ plot ];
            for (const QMetaObject::Connection& conn : connects) {
                disconnect(conn);
            }
            d->mPlotConnections.remove(plot);
        }

        // 移除所有相关的图元记录
        const QwtPlotItemList& items = plot->itemList();
        for (QwtPlotItem* item : items) {
            d->mPlotItemItems.remove(item);
        }

        // 移除寄生绘图的记录和连接
        if (plot->isHostPlot()) {
            const QList< QwtPlot* > parasites = plot->parasitePlots();
            for (QwtPlot* parasite : parasites) {
                d->mPlotItems.remove(parasite);
                const QwtPlotItemList& parasiteItems = parasite->itemList();
                for (QwtPlotItem* item : parasiteItems) {
                    d->mPlotItemItems.remove(item);
                }

                // 断开寄生绘图的连接
                if (d->mPlotConnections.contains(parasite)) {
                    for (const QMetaObject::Connection& conn : std::as_const(d->mPlotConnections[ parasite ])) {
                        disconnect(conn);
                    }
                    d->mPlotConnections.remove(parasite);
                }
            }
        }

        invisibleRootItem()->removeRow(plotItem->row());
        d->mPlotItems.remove(plot);
    }
}

/**
 * @brief 创建空白的不可编辑 Item
 * @return 新建的 QStandardItem 指针
 */
QStandardItem* DAFigureTreeModel::createEmptyItem() const
{
    QStandardItem* item = new QStandardItem();
    item->setEditable(false);
    return item;
}

/**
 * @brief 创建坐标轴属性列 Item
 *
 * 如果该 plot 是当前坐标轴，则设置选中图标
 * @param plot QwtPlot 指针
 * @return 新建的 QStandardItem 指针
 */
QStandardItem* DAFigureTreeModel::createAxesPropertyItem(QwtPlot* plot) const
{
    DA_DC(d);
    QStandardItem* item = new QStandardItem();
    item->setEditable(false);
    if (!d->mFigure) {
        return item;
    }
    static QIcon iconSelectedCurrentChart = QIcon(":/DAFigure/icon/select-current-chart.svg");
    if (d->mFigure->currentAxes() == plot) {
        item->setIcon(iconSelectedCurrentChart);
    }
    // 这里把绘图的指针存入
    item->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), DAFigureTreeModel::RolePlot);
    return item;
}

/**
 * @brief 更新坐标系的当前属性
 */
void DAFigureTreeModel::updateAxesPropertyItem()
{
    DA_D(d);
    // 遍历一级节点的第三列
    const int cnt                         = rowCount();
    static QIcon iconSelectedCurrentChart = QIcon(":/DAFigure/icon/select-current-chart.svg");
    for (int r = 0; r < cnt; ++r) {
        QStandardItem* plotPropertyItem = item(r, 2);
        if (!plotPropertyItem) {
            continue;
        }
        QwtPlot* plot = plotFromItem(plotPropertyItem);
        if (!plot) {
            continue;
        }
        if (plot == d->mFigure->currentAxes()) {
            if (plotPropertyItem->icon().isNull()) {
                plotPropertyItem->setIcon(iconSelectedCurrentChart);
            }
        } else {
            if (!plotPropertyItem->icon().isNull()) {
                plotPropertyItem->setIcon(QIcon());
            }
        }
    }
}

/**
 * @brief 从 QStandardItem 获取关联的 QwtPlot 指针
 * @param item 树节点
 * @return QwtPlot 指针，未关联时为 nullptr
 */
QwtPlot* DAFigureTreeModel::plotFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtPlot >(item, RolePlot);
}

/**
 * @brief 从 QModelIndex 获取关联的 QwtPlot 指针
 * @param index 模型索引
 * @return QwtPlot 指针，未关联时为 nullptr
 */
QwtPlot* DAFigureTreeModel::plotFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtPlot >(index, RolePlot);
}

/**
 * @brief 从 QStandardItem 获取关联的 QwtScaleWidget 指针
 * @param item 树节点
 * @return QwtScaleWidget 指针，未关联时为 nullptr
 */
QwtScaleWidget* DAFigureTreeModel::scaleFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtScaleWidget >(item, RoleScale);
}

/**
 * @brief 从 QModelIndex 获取关联的 QwtScaleWidget 指针
 * @param index 模型索引
 * @return QwtScaleWidget 指针，未关联时为 nullptr
 */
QwtScaleWidget* DAFigureTreeModel::scaleFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtScaleWidget >(index, RoleScale);
}

/**
 * @brief 从 QStandardItem 获取关联的 QwtPlotItem 指针
 * @param item 树节点
 * @return QwtPlotItem 指针，未关联时为 nullptr
 */
QwtPlotItem* DAFigureTreeModel::plotItemFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtPlotItem >(item, RolePlotItem);
}

/**
 * @brief 从 QModelIndex 获取关联的 QwtPlotItem 指针
 * @param index 模型索引
 * @return QwtPlotItem 指针，未关联时为 nullptr
 */
QwtPlotItem* DAFigureTreeModel::plotItemFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtPlotItem >(index, RolePlotItem);
}

/**
 * @brief 从 QStandardItem 获取关联的坐标轴 ID
 * @param item 树节点
 * @return 坐标轴 ID，未关联时返回 QwtAxis::AxisPositions
 */
QwtAxisId DAFigureTreeModel::axisIdFromItem(const QStandardItem* item) const
{
    if (!item) {
        return QwtAxis::AxisPositions;
    }
    QVariant v = item->data(RoleAxisId);
    if (!v.isValid()) {
        return QwtAxis::AxisPositions;
    }
    return v.toInt();
}

/**
 * @brief 从 QModelIndex 获取关联的坐标轴 ID
 * @param index 模型索引
 * @return 坐标轴 ID，未关联时返回 QwtAxis::AxisPositions
 */
QwtAxisId DAFigureTreeModel::axisIdFromItem(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QwtAxis::AxisPositions;
    }

    QStandardItem* item = itemFromIndex(index);
    if (!item) {
        return QwtAxis::AxisPositions;
    }
    return axisIdFromItem(item);
}

/**
 * @brief 刷新模型，重建整个树
 */
void DAFigureTreeModel::refresh()
{
    setupModel();
}

/**
 * @brief 获取树节点的类型
 * @param item 树节点
 * @return 节点类型枚举值
 */
DAFigureTreeModel::NodeType DAFigureTreeModel::itemType(QStandardItem* item) const
{
    if (!item) {
        return NodeTypeUnknow;
    }
    QVariant v = item->data(RoleNodeType);
    if (!v.isValid()) {
        return NodeTypeUnknow;
    }
    return static_cast< NodeType >(v.toInt());
}

/**
 * @brief 返回指定模型索引的 ItemFlags
 *
 * 根据 2D/3D 节点类型设置拖拽和接收拖放标志
 * @param index 模型索引
 * @return ItemFlags
 */
Qt::ItemFlags DAFigureTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QStandardItemModel::flags(index);
    if (!index.isValid()) {
        return f;
    }
    // 节点类型信息存储在column 0
    QModelIndex col0 = index.sibling(index.row(), 0);
    QStandardItem* item = itemFromIndex(col0);
    if (!item) {
        return f;
    }
    int nodeType = item->data(RoleNodeType).toInt();
    // 只有PlotItem的第0列可拖出
    if (index.column() == 0 && (nodeType == NodeTypePlotItem || nodeType == NodeTypePlot3DItem)) {
        f |= Qt::ItemIsDragEnabled;
    }
    // 以下节点类型可接收拖放（2D + 3D）
    switch (nodeType) {
    case NodeTypePlotFolder:
    case NodeTypePlot:
    case NodeTypeItemsFolder:
    case NodeTypePlotItem:
    case NodeTypePlot3DFolder:
    case NodeTypePlot3D:
    case NodeTypePlot3DItemsFolder:
    case NodeTypePlot3DItem:
        f |= Qt::ItemIsDropEnabled;
        break;
    default:
        break;
    }
    return f;
}

/**
 * @brief 根据 QwtPlotItem 获取对应的模型索引
 * @param item QwtPlotItem 指针
 * @return 模型索引，未找到时返回无效索引
 */
QModelIndex DAFigureTreeModel::indexFromPlotItem(QwtPlotItem* item) const
{
    DA_DC(d);
    QStandardItem* stdItem = d->mPlotItemItems.value(item, nullptr);
    if (!stdItem) {
        return QModelIndex();
    }
    return indexFromItem(stdItem);
}

/**
 * @brief 查找 plot 对应的树节点
 * @param plot QwtPlot 指针
 * @return 树节点指针，未找到返回 nullptr
 */
QStandardItem* DAFigureTreeModel::findPlotItem(QwtPlot* plot) const
{
    DA_DC(d);
    return d->mPlotItems.value(plot, nullptr);
}

/**
 * @brief 查找 plot 节点下的图元文件夹节点
 * @param plotItem 绘图节点
 * @param plot QwtPlot 指针
 * @return 图元文件夹节点指针，未找到返回 nullptr
 */
QStandardItem* DAFigureTreeModel::findItemsFolderForPlot(QStandardItem* plotItem, QwtPlot* plot) const
{
    // plotItem 下面挂两个文件夹，一个坐标轴，一个item
    for (int i = 0; i < plotItem->rowCount(); ++i) {
        QStandardItem* folderItem = plotItem->child(i);
        if (itemType(folderItem) == NodeTypeItemsFolder) {
            return folderItem;
        }
    }
    return nullptr;
}

/**
 * @brief 通知指定plotItem的可见性列刷新
 *
 * 通过发出dataChanged信号让视图重新请求可见性列的data(),
 * 从而刷新可见性图标的显示
 * @param item 需要刷新的plotItem
 */
void DAFigureTreeModel::notifyPlotItemVisibilityChanged(QwtPlotItem* item)
{
    DA_D(d);
    if (!item) {
        return;
    }
    QStandardItem* itemNode = d->mPlotItemItems.value(item, nullptr);
    if (!itemNode || !itemNode->parent()) {
        return;
    }
    int row = itemNode->row();
    QStandardItem* visibleItem = itemNode->parent()->child(row, 1);
    if (visibleItem) {
        QModelIndex idx = indexFromItem(visibleItem);
        Q_EMIT dataChanged(idx, idx, { Qt::DecorationRole });
    }
}

/**
 * @brief 通知指定坐标轴的可见性列刷新
 * @param plot 坐标轴所在的plot
 * @param axisId 坐标轴ID
 */
void DAFigureTreeModel::notifyAxisVisibilityChanged(QwtPlot* plot, QwtAxisId axisId)
{
    DA_D(d);
    if (!plot) {
        return;
    }
    QStandardItem* layerItem = d->mPlotItems.value(plot, nullptr);
    if (!layerItem) {
        return;
    }
    // 在layer下查找AxesFolder
    for (int i = 0; i < layerItem->rowCount(); ++i) {
        QStandardItem* folderItem = layerItem->child(i);
        if (itemType(folderItem) == NodeTypeAxesFolder) {
            // 在AxesFolder下查找对应的axis
            for (int j = 0; j < folderItem->rowCount(); ++j) {
                QStandardItem* axisItem = folderItem->child(j);
                if (axisIdFromItem(axisItem) == axisId) {
                    QStandardItem* visibleItem = folderItem->child(j, 1);
                    if (visibleItem) {
                        QModelIndex idx = indexFromItem(visibleItem);
                        Q_EMIT dataChanged(idx, idx, { Qt::DecorationRole });
                    }
                    return;
                }
            }
        }
    }
}

/**
 * @brief 通知指定plotItem的文字列刷新（用于重命名后）
 * @param item 需要刷新的plotItem
 */
void DAFigureTreeModel::notifyPlotItemTextChanged(QwtPlotItem* item)
{
    DA_D(d);
    if (!item) {
        return;
    }
    QStandardItem* itemNode = d->mPlotItemItems.value(item, nullptr);
    if (!itemNode || !itemNode->parent()) {
        return;
    }
    int row = itemNode->row();
    // 刷新第一列(名称列)
    QStandardItem* textItem = itemNode->parent()->child(row, 0);
    if (textItem) {
        QModelIndex idx = indexFromItem(textItem);
        Q_EMIT dataChanged(idx, idx, { Qt::DisplayRole, Qt::DecorationRole });
    }
}

/**
 * @brief 通知指定坐标轴的文字列刷新（用于重命名后）
 * @param plot 坐标轴所在的plot
 * @param axisId 坐标轴ID
 */
void DAFigureTreeModel::notifyAxisTextChanged(QwtPlot* plot, QwtAxisId axisId)
{
    DA_D(d);
    if (!plot) {
        return;
    }
    QStandardItem* layerItem = d->mPlotItems.value(plot, nullptr);
    if (!layerItem) {
        return;
    }
    for (int i = 0; i < layerItem->rowCount(); ++i) {
        QStandardItem* folderItem = layerItem->child(i);
        if (itemType(folderItem) == NodeTypeAxesFolder) {
            for (int j = 0; j < folderItem->rowCount(); ++j) {
                QStandardItem* axisItem = folderItem->child(j);
                if (axisIdFromItem(axisItem) == axisId) {
                    QStandardItem* textItem = folderItem->child(j, 0);
                    if (textItem) {
                        QModelIndex idx = indexFromItem(textItem);
                        Q_EMIT dataChanged(idx, idx, { Qt::DisplayRole, Qt::DecorationRole });
                    }
                    return;
                }
            }
        }
    }
}

/**
 * @brief 通知指定chart节点的文字列刷新（用于重命名后）
 * @param plot chart对应的plot
 */
void DAFigureTreeModel::notifyPlotFolderTextChanged(QwtPlot* plot)
{
    if (!plot) {
        return;
    }
    // chart节点是顶层节点，遍历顶层节点查找对应的plot
    const int cnt = rowCount();
    for (int r = 0; r < cnt; ++r) {
        QStandardItem* plotFolderItem = item(r, 0);
        if (!plotFolderItem) {
            continue;
        }
        if (plotFromItem(plotFolderItem) == plot) {
            QModelIndex idx = indexFromItem(plotFolderItem);
            Q_EMIT dataChanged(idx, idx, { Qt::DisplayRole });
            return;
        }
    }
}

// ================================================================
// 3D 相关方法实现
// ================================================================

/**
 * @brief 添加3D chart到树模型
 * @param chart 3D chart指针
 * @param parentItem 父节点（invisibleRootItem）
 */
void DAFigureTreeModel::add3DChartToModel(DAChart3DWidget* chart, QStandardItem* parentItem)
{
    DA_D(d);
    if (!chart || d->mPlot3DItems.contains(chart)) {
        return;
    }

    // 创建 3D chart 文件夹节点
    static QIcon s_chart3d_icon = QIcon(":/DAFigure/icon/chart.svg");
    QString chartText = chart->getChart3DTitle();
    if (chartText.isEmpty()) {
        chartText = tr("3D Chart");  // cn:3D绘图
    }
    QStandardItem* chartFolderItem = new QStandardItem(chartText);
    chartFolderItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(chart)), RolePlot3D);
    chartFolderItem->setData(NodeTypePlot3DFolder, RoleNodeType);
    chartFolderItem->setIcon(s_chart3d_icon);
    chartFolderItem->setEditable(false);

    QStandardItem* visibilityItem = createEmptyItem();
    QStandardItem* propertyItem    = createEmptyItem();

    parentItem->appendRow(QList< QStandardItem* >() << chartFolderItem << visibilityItem << propertyItem);

    // 添加 layout 层级
    add3DLayerToModel(chart, chartFolderItem);
}

/**
 * @brief 添加3D chart的layout层级（轴文件夹 + item文件夹）
 * @param chart 3D chart指针
 * @param parentItem 父节点（folder节点）
 */
void DAFigureTreeModel::add3DLayerToModel(DAChart3DWidget* chart, QStandardItem* parentItem)
{
    DA_D(d);
    if (!chart) {
        return;
    }

    // 创建 layout 节点
    QStandardItem* layerItem = new DAStandardItemPlot3D(chart, DAStandardItemPlot3D::Plot3DText);
    QStandardItem* visibilityItem = new DAStandardItemPlot3D(chart, DAStandardItemPlot3D::Plot3DVisible);
    QStandardItem* propertyItem   = new DAStandardItemPlot3D(chart, DAStandardItemPlot3D::Plot3DProperty);
    parentItem->appendRow(QList< QStandardItem* >() << layerItem << visibilityItem << propertyItem);

    // 添加 3D 轴
    add3DAxesToLayer(chart, layerItem);

    // 添加 3D plot items
    add3DPlotItemsToLayer(chart, layerItem);

    // 连接 DAChart3DWidget 的 item 变化信号
    QList< QMetaObject::Connection > conns;
    conns << connect(chart, &DAChart3DWidget::plot3DItemAttached,
                     this, &DAFigureTreeModel::on3DItemAttached);
    d->mPlot3DConnections[ chart ] = conns;
    d->mPlot3DItems[ chart ]       = layerItem;
}

/**
 * @brief 添加3D chart的轴文件夹（只显示X/Y/Z 3根主轴）
 * @param chart 3D chart指针
 * @param layerItem layer节点
 */
void DAFigureTreeModel::add3DAxesToLayer(DAChart3DWidget* chart, QStandardItem* layerItem)
{
    static QIcon s_axes_icon = QIcon(":/DAFigure/icon/axes.svg");
    QStandardItem* axesFolder = new QStandardItem(tr("Axis"));  // cn:坐标轴
    axesFolder->setData(QVariant::fromValue(reinterpret_cast< quintptr >(chart)), RolePlot3D);
    axesFolder->setData(NodeTypePlot3DAxesFolder, RoleNodeType);
    axesFolder->setIcon(s_axes_icon);
    axesFolder->setEditable(false);

    layerItem->appendRow(QList< QStandardItem* >() << axesFolder << createEmptyItem() << createEmptyItem());

    // 3D chart 只显示 3 根主轴：X1(0), Y1(1), Z1(2)
    static const struct { int axisId; const char* label; } mainAxes[] = {
        { X1, "X" },  // cn:X轴
        { Y1, "Y" },  // cn:Y轴
        { Z1, "Z" }   // cn:Z轴
    };
    for (const auto& ax : mainAxes) {
        // 优先读取用户已设置的轴标签，为空时 fallback 到默认名称
        QString axisLabel = chart->get3DAxisLabel(static_cast< AXIS >(ax.axisId));
        if (axisLabel.isEmpty()) {
            axisLabel = tr("Axis %1").arg(ax.label);  // cn:%1轴
        }
        QStandardItem* axisItem = new QStandardItem(axisLabel);
        axisItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(chart)), RolePlot3D);
        axisItem->setData(NodeTypePlot3DAxis, RoleNodeType);
        axisItem->setData(ax.axisId, RoleAxis3DId);
        axisItem->setIcon(s_axes_icon);
        axisItem->setEditable(false);

        QStandardItem* axisVisible = createEmptyItem();
        QStandardItem* axisProp    = createEmptyItem();
        axesFolder->appendRow(QList< QStandardItem* >() << axisItem << axisVisible << axisProp);
    }
}

/**
 * @brief 添加3D chart的item文件夹及所有3D plot items
 * @param chart 3D chart指针
 * @param layerItem layer节点
 */
void DAFigureTreeModel::add3DPlotItemsToLayer(DAChart3DWidget* chart, QStandardItem* layerItem)
{
    QStandardItem* itemsFolder = new QStandardItem(tr("plot item"));  // cn:图元
    itemsFolder->setData(QVariant::fromValue(reinterpret_cast< quintptr >(chart)), RolePlot3D);
    itemsFolder->setData(NodeTypePlot3DItemsFolder, RoleNodeType);
    itemsFolder->setEditable(false);
    layerItem->appendRow(QList< QStandardItem* >() << itemsFolder << createEmptyItem() << createEmptyItem());

    // 添加所有 3D plot items
    const QList< Qwt3DPlotItem* >& items = chart->itemList();
    for (Qwt3DPlotItem* item : items) {
        if (item) {
            add3DPlotItem(item, itemsFolder);
        }
    }
}

/**
 * @brief 添加单个3D plot item节点
 * @param item 3D plot item指针
 * @param parentItem 父节点（items文件夹）
 */
void DAFigureTreeModel::add3DPlotItem(Qwt3DPlotItem* item, QStandardItem* parentItem)
{
    DA_D(d);
    QStandardItem* itemNode = new DAStandardItemPlot3DItem(item, DAStandardItemPlot3DItem::Plot3DItemText);
    QStandardItem* itemVisibleItem = new DAStandardItemPlot3DItem(item, DAStandardItemPlot3DItem::Plot3DItemVisible);
    QStandardItem* itemColorItem   = new DAStandardItemPlot3DItem(item, DAStandardItemPlot3DItem::Plot3DItemColor);
    parentItem->appendRow(QList< QStandardItem* >() << itemNode << itemVisibleItem << itemColorItem);
    d->mPlot3DItemItems[ item ] = itemNode;
}

/**
 * @brief 从树中移除3D plot item节点
 * @param item 3D plot item指针
 * @param parentItem 父节点
 */
void DAFigureTreeModel::remove3DPlotItem(Qwt3DPlotItem* item, QStandardItem* parentItem)
{
    DA_D(d);
    QStandardItem* itemNode = d->mPlot3DItemItems.value(item);
    if (itemNode) {
        parentItem->removeRow(itemNode->row());
        d->mPlot3DItemItems.remove(item);
    }
}

/**
 * @brief 3D item 挂载/卸载信号处理
 * @param item 3D plot item指针
 * @param on true=attach，false=detach
 */
void DAFigureTreeModel::on3DItemAttached(Qwt3DPlotItem* item, bool on)
{
    DA_D(d);
    DAChart3DWidget* chart = qobject_cast< DAChart3DWidget* >(sender());
    if (!chart || !item) {
        return;
    }

    QStandardItem* chartItem = find3DChartItem(chart);
    if (!chartItem) {
        return;
    }

    QStandardItem* itemsFolder = find3DItemsFolderForChart(chartItem);
    if (!itemsFolder) {
        return;
    }

    if (on) {
        if (!d->mPlot3DItemItems.contains(item)) {
            add3DPlotItem(item, itemsFolder);
        }
    } else {
        remove3DPlotItem(item, itemsFolder);
    }
    Q_EMIT chart3DItemAttached(item, on);
}

/**
 * @brief 3D chart 添加信号处理
 * @param chart 新增的3D chart指针
 */
void DAFigureTreeModel::on3DChartAdded(DA::DAChart3DWidget* chart)
{
    DA_D(d);
    if (chart && !d->mPlot3DItems.contains(chart)) {
        add3DChartToModel(chart, invisibleRootItem());
    }
}

/**
 * @brief 3D chart 移除信号处理
 * @param chart 移除的3D chart指针
 */
void DAFigureTreeModel::on3DChartRemoved(DA::DAChart3DWidget* chart)
{
    remove3DChartFromModel(chart);
}

/**
 * @brief 从树模型中移除整个3D chart（folder + layer + 子节点）
 * @param chart 3D chart指针
 */
void DAFigureTreeModel::remove3DChartFromModel(DAChart3DWidget* chart)
{
    DA_D(d);
    // mPlot3DItems 存储的是 layer 节点（NodeTypePlot3D），不是 folder 节点
    QStandardItem* layerItem = d->mPlot3DItems.value(chart, nullptr);
    if (!layerItem) {
        return;
    }

    // 断开该 3D chart 的所有连接
    if (d->mPlot3DConnections.contains(chart)) {
        const auto& conns = d->mPlot3DConnections[ chart ];
        for (const QMetaObject::Connection& conn : conns) {
            disconnect(conn);
        }
        d->mPlot3DConnections.remove(chart);
    }

    // 移除所有相关的 3D plot item 记录
    const QList< Qwt3DPlotItem* >& items = chart->itemList();
    for (Qwt3DPlotItem* item : items) {
        d->mPlot3DItemItems.remove(item);
    }

    // 从树中移除整个 3D chart folder 节点（包含 layer 及其所有子节点）
    // layerItem 是 NodeTypePlot3D（layer），其 parent 是 NodeTypePlot3DFolder（folder）
    QStandardItem* folderItem = layerItem->parent();
    if (folderItem) {
        // folder 节点的 parent 为 invisibleRootItem（顶层节点时返回 nullptr）
        QStandardItem* grandParent = folderItem->parent();
        if (grandParent) {
            grandParent->removeRow(folderItem->row());
        } else {
            invisibleRootItem()->removeRow(folderItem->row());
        }
    } else {
        // 防御性：layer 节点本身是顶层节点（不应发生）
        invisibleRootItem()->removeRow(layerItem->row());
    }
    d->mPlot3DItems.remove(chart);
}

/**
 * @brief 查找3D chart对应的layer节点
 * @param chart 3D chart指针
 * @return layer节点指针，未找到返回nullptr
 */
QStandardItem* DAFigureTreeModel::find3DChartItem(DAChart3DWidget* chart) const
{
    DA_DC(d);
    return d->mPlot3DItems.value(chart, nullptr);
}

/**
 * @brief 查找3D chart对应的item文件夹节点
 * @param chartItem layer节点（NodeTypePlot3D）
 * @return item文件夹节点指针，未找到返回nullptr
 */
QStandardItem* DAFigureTreeModel::find3DItemsFolderForChart(QStandardItem* chartItem) const
{
    if (!chartItem) {
        return nullptr;
    }
    // chartItem 是 NodeTypePlot3D（layer 节点），其直接子节点包含
    // NodeTypePlot3DAxesFolder 和 NodeTypePlot3DItemsFolder
    for (int i = 0; i < chartItem->rowCount(); ++i) {
        QStandardItem* child = chartItem->child(i);
        if (itemType(child) == NodeTypePlot3DItemsFolder) {
            return child;
        }
    }
    return nullptr;
}

/**
 * @brief 生成3D plot item的显示名称
 * @param item 3D plot item指针
 * @return 名称字符串
 */
QString DAFigureTreeModel::generate3DPlotItemName(Qwt3DPlotItem* item) const
{
    if (!item) {
        return QString();
    }
    QString name = item->title();
    if (name.isEmpty()) {
        name = tr("3D Item");  // cn:3D图元
    }
    return name;
}

/**
 * @brief 生成3D plot item的图标（根据rtti返回不同图标）
 * @param item 3D plot item指针
 * @return 图标
 */
QIcon DAFigureTreeModel::generate3DPlotItemIcon(Qwt3DPlotItem* item) const
{
    if (!item) {
        return QIcon();
    }
    static QIcon s_icon_surface(":/DAFigure/icon/chart.svg");
    static QIcon s_icon_bar(":/DAFigure/icon/chart.svg");
    static QIcon s_icon_line(":/DAFigure/icon/chart.svg");
    static QIcon s_icon_default(":/DAFigure/icon/chart.svg");

    int rtti = item->rtti();
    switch (rtti) {
    case Rtti_Plot3DSurface:
        return s_icon_surface;
    case Rtti_Plot3DBar:
        return s_icon_bar;
    case Rtti_Plot3DLine:
        return s_icon_line;
    default:
        return s_icon_default;
    }
}

/**
 * @brief 通知指定3D plotItem的可见性列刷新
 * @param item 3D plot item指针
 */
void DAFigureTreeModel::notify3DPlotItemVisibilityChanged(Qwt3DPlotItem* item)
{
    DA_D(d);
    if (!item) {
        return;
    }
    QStandardItem* itemNode = d->mPlot3DItemItems.value(item, nullptr);
    if (!itemNode || !itemNode->parent()) {
        return;
    }
    int row = itemNode->row();
    QStandardItem* visibleItem = itemNode->parent()->child(row, 1);
    if (visibleItem) {
        QModelIndex idx = indexFromItem(visibleItem);
        Q_EMIT dataChanged(idx, idx, { Qt::DecorationRole });
    }
}

/**
 * @brief 通知指定3D plotItem的文字列刷新
 * @param item 3D plot item指针
 */
void DAFigureTreeModel::notify3DPlotItemTextChanged(Qwt3DPlotItem* item)
{
    DA_D(d);
    if (!item) {
        return;
    }
    QStandardItem* itemNode = d->mPlot3DItemItems.value(item, nullptr);
    if (!itemNode || !itemNode->parent()) {
        return;
    }
    int row = itemNode->row();
    QStandardItem* textItem = itemNode->parent()->child(row, 0);
    if (textItem) {
        QModelIndex idx = indexFromItem(textItem);
        Q_EMIT dataChanged(idx, idx, { Qt::DisplayRole, Qt::DecorationRole });
    }
}

/**
 * @brief 通知指定3D chart节点的文字列刷新
 * @param chart 3D chart指针
 */
void DAFigureTreeModel::notify3DPlot3DTextChanged(DAChart3DWidget* chart)
{
    DA_D(d);
    if (!chart) {
        return;
    }
    QStandardItem* layerItem = d->mPlot3DItems.value(chart, nullptr);
    if (!layerItem) {
        return;
    }
    // mPlot3DItems 存储的是 layer 节点（NodeTypePlot3D），其 data() 动态查询 chart 标题
    QModelIndex idx = indexFromItem(layerItem);
    Q_EMIT dataChanged(idx, idx, { Qt::DisplayRole });
}

/**
 * @brief 显式从树中移除指定3D plot item节点
 *
 * 供 onContextMenuDeleteTriggered() 在 detach() + delete 之前调用，
 * 确保树和 hash 一致，避免信号处理时序依赖
 * @param item 3D plot item指针
 */
void DAFigureTreeModel::remove3DPlotItemFromTree(Qwt3DPlotItem* item)
{
    DA_D(d);
    if (!item) {
        return;
    }
    QStandardItem* itemNode = d->mPlot3DItemItems.value(item, nullptr);
    if (!itemNode) {
        return;
    }
    QStandardItem* parentItem = itemNode->parent();
    if (parentItem) {
        parentItem->removeRow(itemNode->row());
    }
    d->mPlot3DItemItems.remove(item);
}

/**
 * @brief 返回3D chart指针
 * @param item 树节点
 * @return 3D chart指针
 */
DAChart3DWidget* DAFigureTreeModel::plot3DFromItem(const QStandardItem* item) const
{
    return pointerFromItem< DAChart3DWidget >(item, RolePlot3D);
}

/**
 * @brief 返回3D chart指针
 * @param index 模型索引
 * @return 3D chart指针
 */
DAChart3DWidget* DAFigureTreeModel::plot3DFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< DAChart3DWidget >(index, RolePlot3D);
}

/**
 * @brief 返回3D plot item指针
 * @param item 树节点
 * @return 3D plot item指针
 */
Qwt3DPlotItem* DAFigureTreeModel::plot3DItemFromItem(const QStandardItem* item) const
{
    return pointerFromItem< Qwt3DPlotItem >(item, RolePlot3DItem);
}

/**
 * @brief 返回3D plot item指针
 * @param index 模型索引
 * @return 3D plot item指针
 */
Qwt3DPlotItem* DAFigureTreeModel::plot3DItemFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< Qwt3DPlotItem >(index, RolePlot3DItem);
}

/**
 * @brief 返回3D plot item对应的QModelIndex
 * @param item 3D plot item指针
 * @return 模型索引
 */
QModelIndex DAFigureTreeModel::indexFrom3DPlotItem(Qwt3DPlotItem* item) const
{
    DA_DC(d);
    QStandardItem* stdItem = d->mPlot3DItemItems.value(item, nullptr);
    if (!stdItem) {
        return QModelIndex();
    }
    return indexFromItem(stdItem);
}

}  // End Of Namespace DA
