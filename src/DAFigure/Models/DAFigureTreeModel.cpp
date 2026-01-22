#include "DAFigureTreeModel.h"
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include "DAChartUtil.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_grid.h"
#include "qwt_text.h"
#include "qwt_color_map.h"
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
#define DAFigureTreeModel_Debug_Print 1

namespace DA
{

//----------------------------------------------------
//
//----------------------------------------------------

DAFigureTreeModel::DAFigureTreeModel(QObject* parent) : QStandardItemModel(parent), m_figure(nullptr)
{
}

DAFigureTreeModel::~DAFigureTreeModel()
{
    clearAllConnections();
}

void DAFigureTreeModel::setFigure(QwtFigure* figure)
{
    if (m_figure == figure) {
        return;
    }

    // 清除所有现有连接
    clearAllConnections();

    m_figure = figure;
    m_plotItems.clear();
    m_plotItemItems.clear();

    if (m_figure) {
        // 连接figure信号
        m_figureConnections << connect(m_figure, &QwtFigure::axesAdded, this, &DAFigureTreeModel::onAxesAdded);
        m_figureConnections << connect(m_figure, &QwtFigure::axesRemoved, this, &DAFigureTreeModel::onAxesRemoved);
        m_figureConnections << connect(m_figure, &QwtFigure::figureCleared, this, &DAFigureTreeModel::onFigureCleared);
        m_figureConnections << connect(m_figure, &QwtFigure::currentAxesChanged, this, &DAFigureTreeModel::onCurrentAxesChanged);
    }

    setupModel();
}

void DAFigureTreeModel::clearAllConnections()
{
    // 断开所有figure连接
    for (const QMetaObject::Connection& conn : m_figureConnections) {
        disconnect(conn);
    }
    m_figureConnections.clear();

    // 断开所有plot连接
    for (auto it = m_plotConnections.begin(); it != m_plotConnections.end(); ++it) {
        for (const QMetaObject::Connection& conn : it.value()) {
            disconnect(conn);
        }
    }
    m_plotConnections.clear();
}

void DAFigureTreeModel::setupModel()
{
    clear();
    // 设置三列表头
    setHorizontalHeaderLabels(
        QStringList() << tr("element")   // cn:绘图元素
                      << tr("visible")   // cn:可见性
                      << tr("property")  // cn:属性
    );
    m_plotItems.clear();
    m_plotItemItems.clear();

    if (!m_figure)
        return;

    QStandardItem* rootItem = invisibleRootItem();

    QList< QwtPlot* > plots = m_figure->allAxes();
    for (QwtPlot* plot : plots) {
        addPlotToModel(plot, rootItem);
    }
}

void DAFigureTreeModel::addPlotToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    if (!plot || m_plotItems.contains(plot)) {
        return;
    }

    // 创建绘图节点 - 三列
    static QIcon s_plot_icon = QIcon(":/DAFigure/icon/chart.svg");
    QStandardItem* plotItem  = new QStandardItem(tr("chart"));  // cn:绘图
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
    QList< QwtPlot* > parasites = plot->parasitePlots();
    for (QwtPlot* parasite : parasites) {
        addLayerToModel(parasite, plotItem);
    }
}

void DAFigureTreeModel::addLayerToModel(QwtPlot* plot, QStandardItem* parentItem)
{
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
    m_plotConnections[ plot ] = plotConnections;
    m_plotItems[ plot ]       = layerItem;
}

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
    QStandardItem* itemNode = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemText);
    // 可见性列 - 图元节点需要可见性控制
    QStandardItem* itemVisibilityItem = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemVisible);
    // 颜色列 - 图元节点需要颜色显示
    QStandardItem* itemColorItem = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemColor);
    parentItem->appendRow(QList< QStandardItem* >() << itemNode << itemVisibilityItem << itemColorItem);
    m_plotItemItems[ item ] = itemNode;
}

void DAFigureTreeModel::removePlotItem(QwtPlotItem* item, QStandardItem* parentItem)
{
    QStandardItem* itemNode = m_plotItemItems.value(item);
    if (itemNode) {
        parentItem->removeRow(itemNode->row());
        m_plotItemItems.remove(item);
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

QString DAFigureTreeModel::generatePlotItemName(QwtPlotItem* item) const
{
    return DAChartUtil::plotItemName(item);
}

QIcon DAFigureTreeModel::generatePlotItemIcon(QwtPlotItem* item) const
{
    return DAChartUtil::plotItemIcon(item);
}

QIcon DAFigureTreeModel::generateBrushIcon(const QBrush& b) const
{
    QPixmap pixmap(22, 22);
    QPainter p(&pixmap);
    p.fillRect(pixmap.rect(), b);
    return QIcon(pixmap);
}

void DAFigureTreeModel::onAxesAdded(QwtPlot* plot)
{
    if (plot && !m_plotItems.contains(plot)) {
        addPlotToModel(plot, invisibleRootItem());
    }
}

void DAFigureTreeModel::onAxesRemoved(QwtPlot* plot)
{
    removePlotFromModel(plot);
}

void DAFigureTreeModel::onFigureCleared()
{
    setupModel();  // 完全重建
}

void DAFigureTreeModel::onCurrentAxesChanged(QwtPlot* plot)
{
    updateAxesPropertyItem();
}

void DAFigureTreeModel::onItemAttached(QwtPlotItem* item, bool on)
{
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
        if (!m_plotItemItems.contains(item)) {
            addPlotItem(item, itemsFolder);
        }
    } else {
        removePlotItem(item, itemsFolder);
    }
    Q_EMIT chartItemAttached(item, on);
}

void DAFigureTreeModel::removePlotFromModel(QwtPlot* plot)
{
    QStandardItem* plotItem = m_plotItems.value(plot);
    if (plotItem) {
        // 断开该plot的所有连接
        if (m_plotConnections.contains(plot)) {
            const auto& connects = m_plotConnections[ plot ];
            for (const QMetaObject::Connection& conn : connects) {
                disconnect(conn);
            }
            m_plotConnections.remove(plot);
        }

        // 移除所有相关的图元记录
        const QwtPlotItemList& items = plot->itemList();
        for (QwtPlotItem* item : items) {
            m_plotItemItems.remove(item);
        }

        // 移除寄生绘图的记录和连接
        if (plot->isHostPlot()) {
            QList< QwtPlot* > parasites = plot->parasitePlots();
            for (QwtPlot* parasite : parasites) {
                m_plotItems.remove(parasite);
                const QwtPlotItemList& parasiteItems = parasite->itemList();
                for (QwtPlotItem* item : parasiteItems) {
                    m_plotItemItems.remove(item);
                }

                // 断开寄生绘图的连接
                if (m_plotConnections.contains(parasite)) {
                    for (const QMetaObject::Connection& conn : std::as_const(m_plotConnections[ parasite ])) {
                        disconnect(conn);
                    }
                    m_plotConnections.remove(parasite);
                }
            }
        }

        invisibleRootItem()->removeRow(plotItem->row());
        m_plotItems.remove(plot);
    }
}

QStandardItem* DAFigureTreeModel::createEmptyItem() const
{
    QStandardItem* item = new QStandardItem();
    item->setEditable(false);
    return item;
}

QStandardItem* DAFigureTreeModel::createAxesPropertyItem(QwtPlot* plot) const
{
    QStandardItem* item = new QStandardItem();
    item->setEditable(false);
    if (!m_figure) {
        return item;
    }
    static QIcon iconSelectedCurrentChart = QIcon(":/DAFigure/icon/select-current-chart.svg");
    if (m_figure->currentAxes() == plot) {
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
        if (plot == m_figure->currentAxes()) {
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

QwtPlot* DAFigureTreeModel::plotFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtPlot >(item, RolePlot);
}

QwtPlot* DAFigureTreeModel::plotFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtPlot >(index, RolePlot);
}

QwtScaleWidget* DAFigureTreeModel::scaleFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtScaleWidget >(item, RoleScale);
}

QwtScaleWidget* DAFigureTreeModel::scaleFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtScaleWidget >(index, RoleScale);
}

QwtPlotItem* DAFigureTreeModel::plotItemFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtPlotItem >(item, RolePlotItem);
}

QwtPlotItem* DAFigureTreeModel::plotItemFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtPlotItem >(index, RolePlotItem);
}

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

void DAFigureTreeModel::refresh()
{
    setupModel();
}

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

QStandardItem* DAFigureTreeModel::findPlotItem(QwtPlot* plot) const
{
    return m_plotItems.value(plot, nullptr);
}

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

}  // End Of Namespace DA
