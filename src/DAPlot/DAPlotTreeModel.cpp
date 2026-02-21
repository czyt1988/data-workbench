#include "DAPlotTreeModel.h"
#include <QDebug>
#include <QPainter>
#include <QPixmap>
// qim
#include "QImFigureWidget.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlotItemNode.h"
#include "plot/QImPlotAxisInfo.h"

#include "DAStandardItemPlot.h"
#include "DAStandardItemPlotAxis.h"
#include "DAStandardItemPlotItem.h"
#define DAFigureTreeModel_Debug_Print 1

namespace DA
{

//----------------------------------------------------
//
//----------------------------------------------------

DAPlotTreeModel::DAPlotTreeModel(QObject* parent) : QStandardItemModel(parent), m_figure(nullptr)
{
}

DAPlotTreeModel::~DAPlotTreeModel()
{
    clearAllConnections();
}

QIM::QImFigureWidget* DAPlotTreeModel::figure() const
{
    return m_figure.data();
}

void DAPlotTreeModel::setFigure(QIM::QImFigureWidget* figure)
{
    if (m_figure == figure) {
        return;
    }

    // 清除所有现有连接
    clearAllConnections();

    m_figure = figure;

    if (m_figure) {
        // 连接figure信号
    }
    setupModel();
}

void DAPlotTreeModel::clearAllConnections()
{
    // 断开所有figure连接
    for (auto i = m_plotConnections.begin(); i != m_plotConnections.end(); ++i) {
        i.value().disconnectAll();
    }
    m_plotConnections.clear();
}

void DAPlotTreeModel::setupModel()
{
    clear();
    // 设置三列表头
    setHorizontalHeaderLabels(
        QStringList() << tr("element")   // cn:绘图元素
                      << tr("visible")   // cn:可见性
                      << tr("property")  // cn:属性
    );
    if (!m_figure) {
        return;
    }

    QStandardItem* rootItem          = invisibleRootItem();
    QList< QIM::QImPlotNode* > plots = m_figure->plotNodes();
    for (QIM::QImPlotNode* plot : plots) {
        addPlotNode(plot, rootItem);
    }
}

void DAPlotTreeModel::addPlotNode(QIM::QImPlotNode* plot, QStandardItem* parentItem)
{
    if (!plot) {
        return;
    }
    QStandardItem* plotItem = new DAStandardItemPlot(plot, DAStandardItemPlot::PlotText);
    // 可见性列 - 图层节点不需要可见性控制
    QStandardItem* visibilityItem = new DAStandardItemPlot(plot, DAStandardItemPlot::PlotVisible);
    // 颜色列 - 图层节点不需要颜色显示
    QStandardItem* propertyItem = new DAStandardItemPlot(plot, DAStandardItemPlot::PlotProperty);
    parentItem->appendRow(QList< QStandardItem* >() << plotItem << visibilityItem << propertyItem);

    auto con1 = connect(plot, &QIM::QImPlotNode::childNodeAdded, this, &DAPlotTreeModel::onPlotChildNodeAdded);
    auto con2 = connect(plot, &QIM::QImPlotNode::childNodeRemoved, this, &DAPlotTreeModel::onPlotChildNodeRemoved);
    m_plotConnections[ plot ].plotConnections.append(con1);
    m_plotConnections[ plot ].plotConnections.append(con2);
    addAllAxisInfos(plot, plotItem);

    addAllPlotItems(plot, plotItem);
}


void DAPlotTreeModel::addAllPlotItems(QIM::QImPlotNode* plot, QStandardItem* plotItem)
{
    QStandardItem* itemsFolder = new QStandardItem(tr("plot item"));  // cn:图元
    itemsFolder->setData(
        QVariant::fromValue(static_cast< int >(DAPlotTreeItemType::PlotItemFloder)),
        static_cast< int >(DAPlotTreeItemRole::RoleItemType)
    );
    itemsFolder->setEditable(false);
    m_plotItemFolderItem = itemsFolder;
    // itemsFolder->setIcon(QIcon(":/icons/items.png"));
    plotItem->appendRow(QList< QStandardItem* >() << itemsFolder << createEmptyItem() << createEmptyItem());

    // 添加所有图元
    const QList< QIM::QImPlotItemNode* >& items = plot->plotItemNodes();
    for (QIM::QImPlotItemNode* item : items) {
        if (item) {
            addPlotItemNode(item, itemsFolder);
        }
    }
}


void DAPlotTreeModel::addAllAxisInfos(QIM::QImPlotNode* plot, QStandardItem* plotItem)
{
    static QIcon s_axes_icon  = QIcon(":/DAFigure/icon/axes.svg");
    QStandardItem* axesFolder = new QStandardItem(tr("Axis"));  // cn:坐标轴
    axesFolder->setData(
        QVariant::fromValue(static_cast< int >(DAPlotTreeItemType::AxesFolder)),
        static_cast< int >(DAPlotTreeItemRole::RoleItemType)
    );
    axesFolder->setIcon(s_axes_icon);
    axesFolder->setEditable(false);
    m_axesFolderItem = axesFolder;
    plotItem->appendRow(QList< QStandardItem* >() << axesFolder << createEmptyItem() << createEmptyItem());
    // 添加坐标轴
    const int axisCnt = static_cast< int >(QIM::QImPlotAxisId::AxisCount);
    for (int axis = 0; axis < axisCnt; ++axis) {
        QIM::QImPlotAxisId id        = static_cast< QIM::QImPlotAxisId >(axis);
        QIM::QImPlotAxisInfo* axInfo = plot->axisInfo(id);
        if (!axInfo) {
            continue;
        }
        DAStandardItemPlotAxis* axisItem = new DAStandardItemPlotAxis(axInfo, DAStandardItemPlotAxis::PlotAxisText);
        DAStandardItemPlotAxis* axisVisible = new DAStandardItemPlotAxis(axInfo, DAStandardItemPlotAxis::PlotAxisVisible);
        DAStandardItemPlotAxis* axisProperty = new DAStandardItemPlotAxis(axInfo, DAStandardItemPlotAxis::PlotAxisProperty);
        axesFolder->appendRow(QList< QStandardItem* >() << axisItem << axisVisible << axisProperty);
        // 建立链接
        auto con = connect(axInfo, &QIM::QImPlotAxisInfo::labelChanged, this, &DAPlotTreeModel::onAxisLabelChanged);
        m_plotConnections[ plot ].axisConnections.append(con);
    }
}

/**
 * @brief 添加QwtPlotItem元素
 * @param item
 * @param parentItem
 */
void DAPlotTreeModel::addPlotItemNode(QIM::QImPlotItemNode* item, QStandardItem* parentItem)
{
    QStandardItem* itemNode = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemText);
    // 可见性列 - 图元节点需要可见性控制
    QStandardItem* itemVisibilityItem = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemVisible);
    // 颜色列 - 图元节点需要颜色显示
    QStandardItem* itemColorItem = new DAStandardItemPlotItem(item, DAStandardItemPlotItem::PlotItemProperty);
    parentItem->appendRow(QList< QStandardItem* >() << itemNode << itemVisibilityItem << itemColorItem);
}


void DAPlotTreeModel::removePlotNode(QIM::QImPlotNode* plot)
{
    QStandardItem* item = findTreeItem(plot);
    if (!item) {
        return;
    }
    removeRow(item->row());
    // 移除信号
    auto ite = m_plotConnections.find(plot);
    if (ite != m_plotConnections.end()) {
        ite.value().disconnectAll();
        m_plotConnections.erase(ite);
    }
}


void DAPlotTreeModel::removePlotItemNode(QIM::QImPlotItemNode* plotitem)
{
    if (!m_plotItemFolderItem) {
        return;
    }
    QStandardItem* item = findTreeItem(plotitem);
    if (!item) {
        return;
    }

    QStandardItem* p = item->parent();
    if (p) {
        int row = item->row();
        if (row >= 0) {
            p->removeRow(row);
        }
    }
}

QIcon DAPlotTreeModel::generateBrushIcon(const QBrush& b, const QSize& size) const
{
    QPixmap pixmap(size);
    QPainter p(&pixmap);
    p.fillRect(pixmap.rect(), b);
    return QIcon(pixmap);
}

void DAPlotTreeModel::onFigureCleared()
{
    setupModel();  // 完全重建
}

void DAPlotTreeModel::onPlotNodeAttached(QIM::QImPlotNode* plot, bool on)
{
    if (on) {
        QStandardItem* rootItem = invisibleRootItem();
        addPlotNode(plot, rootItem);
    } else {
        removePlotNode(plot);
    }
}

void DAPlotTreeModel::onPlotChildNodeAdded(QIM::QImAbstractNode* c)
{
    QIM::QImPlotItemNode* plotitemNode = qobject_cast< QIM::QImPlotItemNode* >(c);
    if (!plotitemNode || !m_plotItemFolderItem) {
        return;
    }
    addPlotItemNode(plotitemNode, m_plotItemFolderItem);
}

void DAPlotTreeModel::onPlotChildNodeRemoved(QIM::QImAbstractNode* c)
{
    QIM::QImPlotItemNode* plotitemNode = qobject_cast< QIM::QImPlotItemNode* >(c);
    if (!plotitemNode || !m_plotItemFolderItem) {
        return;
    }
    QStandardItem* plotItem = findTreeItem(plotitemNode);
    if (plotItem) {
        m_plotItemFolderItem->removeRow(plotItem->row());
    }
}

void DAPlotTreeModel::onAxisLabelChanged(const QString& label)
{
    QIM::QImPlotAxisInfo* axis = qobject_cast< QIM::QImPlotAxisInfo* >(sender());
    if (!axis) {
        return;
    }
    QStandardItem* axisItem = findTreeItem(axis);
    if (!axisItem) {
        return;
    }
    Q_EMIT dataChanged(axisItem->index(), axisItem->index());
}


QStandardItem* DAPlotTreeModel::createEmptyItem() const
{
    QStandardItem* item = new QStandardItem();
    item->setEditable(false);
    return item;
}


QIM::QImPlotNode* DAPlotTreeModel::plotFromItem(const QStandardItem* item) const
{
    if (checkItemType(DAPlotTreeItemType::Plot, item)) {
        return pointerFromItem< QIM::QImPlotNode* >(item);
    }
    return nullptr;
}

QIM::QImPlotAxisInfo* DAPlotTreeModel::axisFromItem(const QStandardItem* item) const
{
    if (checkItemType(DAPlotTreeItemType::Axis, item)) {
        return pointerFromItem< QIM::QImPlotAxisInfo* >(item);
    }
    return nullptr;
}


void DAPlotTreeModel::refresh()
{
    setupModel();
}

DAPlotTreeItemType DAPlotTreeModel::itemType(const QStandardItem* item)
{
    if (!item) {
        return DAPlotTreeItemType::Unknow;
    }
    QVariant v = item->data(static_cast< int >(DAPlotTreeItemRole::RoleItemType));
    if (!v.isValid()) {
        return DAPlotTreeItemType::Unknow;
    }
    return static_cast< DAPlotTreeItemType >(v.toInt());
}

bool DAPlotTreeModel::checkItemType(DAPlotTreeItemType type, const QStandardItem* item)
{
    return (type == itemType(item));
}


QStandardItem* DAPlotTreeModel::findTreeItem(QIM::QImPlotNode* plotNode) const
{
    return findTreeItem(DAPlotTreeItemType::Plot, plotNode, invisibleRootItem());
}

QStandardItem* DAPlotTreeModel::findTreeItem(QIM::QImPlotItemNode* plotItemNode) const
{
    return findTreeItem(DAPlotTreeItemType::PlotItem, plotItemNode, m_plotItemFolderItem);
}

QStandardItem* DAPlotTreeModel::findTreeItem(QIM::QImPlotAxisInfo* axisNode) const
{
    return findTreeItem(DAPlotTreeItemType::Axis, axisNode, m_axesFolderItem);
}

void DAPlotTreeModel::ConnectionsInfo::disconnectAll()
{
    for (const QMetaObject::Connection& con : std::as_const(plotConnections)) {
        disconnect(con);
    }
    for (const QMetaObject::Connection& con : std::as_const(axisConnections)) {
        disconnect(con);
    }
}


}  // End Of Namespace DA
