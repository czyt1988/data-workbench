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

/**
 * @brief 设置 DAFigureWidget，缓存指针后调用 setFigure()
 * @param figWidget
 */
void DAFigureTreeModel::setFigureWidget(DA::DAFigureWidget* figWidget)
{
    if (m_figureWidget == figWidget) {
        return;
    }
    m_figureWidget = figWidget;
    setFigure(figWidget ? figWidget->figure() : nullptr);
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
    // 清理 3D 状态
    m_plot3DItems.clear();
    m_plot3DItemItems.clear();
    m_plot3DConnections.clear();

    if (m_figure) {
        // 连接figure信号
        m_figureConnections << connect(m_figure, &QwtFigure::axesAdded, this, &DAFigureTreeModel::onAxesAdded);
        m_figureConnections << connect(m_figure, &QwtFigure::axesRemoved, this, &DAFigureTreeModel::onAxesRemoved);
        m_figureConnections << connect(m_figure, &QwtFigure::figureCleared, this, &DAFigureTreeModel::onFigureCleared);
        m_figureConnections << connect(
            m_figure, &QwtFigure::currentAxesChanged, this, &DAFigureTreeModel::onCurrentAxesChanged);
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

    // 断开所有3D chart信号连接
    for (auto it = m_plot3DConnections.constBegin(); it != m_plot3DConnections.constEnd(); ++it) {
        const QList< QMetaObject::Connection >& conns = it.value();
        for (const QMetaObject::Connection& conn : conns) {
            disconnect(conn);
        }
    }
    m_plot3DConnections.clear();
}

void DAFigureTreeModel::setupModel()
{
    clear();
    // 设置三列表头
    setHorizontalHeaderLabels(QStringList() << tr("element")   // cn:绘图元素
                                            << tr("visible")   // cn:可见性
                                            << tr("property")  // cn:属性
    );
    m_plotItems.clear();
    m_plotItemItems.clear();
    m_plot3DItems.clear();
    m_plot3DItemItems.clear();

    if (!m_figure)
        return;

    QStandardItem* rootItem = invisibleRootItem();

    // 遍历 2D chart（现有逻辑不变）
    const QList< QwtPlot* > plots = m_figure->allAxes();
    for (QwtPlot* plot : plots) {
        addPlotToModel(plot, rootItem);
    }

    // 遍历 3D chart
    if (m_figureWidget) {
        const QList< DAChart3DWidget* > charts3D = m_figureWidget->get3DCharts();
        for (DAChart3DWidget* chart3D : charts3D) {
            add3DChartToModel(chart3D, rootItem);
        }
        // 连接 DAFigureWidget 的 3D chart 增删信号
        m_figureConnections << connect(m_figureWidget.data(), &DAFigureWidget::chart3DAdded,
                                       this, &DAFigureTreeModel::on3DChartAdded);
        m_figureConnections << connect(m_figureWidget.data(), &DAFigureWidget::chart3DRemoved,
                                       this, &DAFigureTreeModel::on3DChartRemoved);
    }
}

void DAFigureTreeModel::addPlotToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    if (!plot || m_plotItems.contains(plot)) {
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
            const QList< QwtPlot* > parasites = plot->parasitePlots();
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

QModelIndex DAFigureTreeModel::indexFromPlotItem(QwtPlotItem* item) const
{
    QStandardItem* stdItem = m_plotItemItems.value(item, nullptr);
    if (!stdItem) {
        return QModelIndex();
    }
    return indexFromItem(stdItem);
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

/**
 * @brief 通知指定plotItem的可见性列刷新
 *
 * 通过发出dataChanged信号让视图重新请求可见性列的data(),
 * 从而刷新可见性图标的显示
 * @param item 需要刷新的plotItem
 */
void DAFigureTreeModel::notifyPlotItemVisibilityChanged(QwtPlotItem* item)
{
    if (!item) {
        return;
    }
    QStandardItem* itemNode = m_plotItemItems.value(item, nullptr);
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
    if (!plot) {
        return;
    }
    QStandardItem* layerItem = m_plotItems.value(plot, nullptr);
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
    if (!item) {
        return;
    }
    QStandardItem* itemNode = m_plotItemItems.value(item, nullptr);
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
    if (!plot) {
        return;
    }
    QStandardItem* layerItem = m_plotItems.value(plot, nullptr);
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
    if (!chart || m_plot3DItems.contains(chart)) {
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
    m_plot3DConnections[ chart ] = conns;
    m_plot3DItems[ chart ]       = layerItem;
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
    QStandardItem* itemNode = new DAStandardItemPlot3DItem(item, DAStandardItemPlot3DItem::Plot3DItemText);
    QStandardItem* itemVisibleItem = new DAStandardItemPlot3DItem(item, DAStandardItemPlot3DItem::Plot3DItemVisible);
    QStandardItem* itemColorItem   = new DAStandardItemPlot3DItem(item, DAStandardItemPlot3DItem::Plot3DItemColor);
    parentItem->appendRow(QList< QStandardItem* >() << itemNode << itemVisibleItem << itemColorItem);
    m_plot3DItemItems[ item ] = itemNode;
}

/**
 * @brief 从树中移除3D plot item节点
 * @param item 3D plot item指针
 * @param parentItem 父节点
 */
void DAFigureTreeModel::remove3DPlotItem(Qwt3DPlotItem* item, QStandardItem* parentItem)
{
    QStandardItem* itemNode = m_plot3DItemItems.value(item);
    if (itemNode) {
        parentItem->removeRow(itemNode->row());
        m_plot3DItemItems.remove(item);
    }
}

/**
 * @brief 3D item 挂载/卸载信号处理
 * @param item 3D plot item指针
 * @param on true=attach，false=detach
 */
void DAFigureTreeModel::on3DItemAttached(Qwt3DPlotItem* item, bool on)
{
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
        if (!m_plot3DItemItems.contains(item)) {
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
    if (chart && !m_plot3DItems.contains(chart)) {
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
    // m_plot3DItems 存储的是 layer 节点（NodeTypePlot3D），不是 folder 节点
    QStandardItem* layerItem = m_plot3DItems.value(chart, nullptr);
    if (!layerItem) {
        return;
    }

    // 断开该 3D chart 的所有连接
    if (m_plot3DConnections.contains(chart)) {
        const auto& conns = m_plot3DConnections[ chart ];
        for (const QMetaObject::Connection& conn : conns) {
            disconnect(conn);
        }
        m_plot3DConnections.remove(chart);
    }

    // 移除所有相关的 3D plot item 记录
    const QList< Qwt3DPlotItem* >& items = chart->itemList();
    for (Qwt3DPlotItem* item : items) {
        m_plot3DItemItems.remove(item);
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
    m_plot3DItems.remove(chart);
}

/**
 * @brief 查找3D chart对应的layer节点
 * @param chart 3D chart指针
 * @return layer节点指针，未找到返回nullptr
 */
QStandardItem* DAFigureTreeModel::find3DChartItem(DAChart3DWidget* chart) const
{
    return m_plot3DItems.value(chart, nullptr);
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
    if (!item) {
        return;
    }
    QStandardItem* itemNode = m_plot3DItemItems.value(item, nullptr);
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
    if (!item) {
        return;
    }
    QStandardItem* itemNode = m_plot3DItemItems.value(item, nullptr);
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
    if (!chart) {
        return;
    }
    QStandardItem* layerItem = m_plot3DItems.value(chart, nullptr);
    if (!layerItem) {
        return;
    }
    // m_plot3DItems 存储的是 layer 节点（NodeTypePlot3D），其 data() 动态查询 chart 标题
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
    if (!item) {
        return;
    }
    QStandardItem* itemNode = m_plot3DItemItems.value(item, nullptr);
    if (!itemNode) {
        return;
    }
    QStandardItem* parentItem = itemNode->parent();
    if (parentItem) {
        parentItem->removeRow(itemNode->row());
    }
    m_plot3DItemItems.remove(item);
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
    QStandardItem* stdItem = m_plot3DItemItems.value(item, nullptr);
    if (!stdItem) {
        return QModelIndex();
    }
    return indexFromItem(stdItem);
}

}  // End Of Namespace DA
