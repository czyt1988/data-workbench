#include "DAStandardItemPlotItem.h"
#include "qwt_plot_item.h"
#include "DAFigureTreeModel.h"
#include "DAChartUtil.h"
namespace DA
{

DAStandardItemPlotItem::DAStandardItemPlotItem(QwtPlotItem* item, ItemType plotItemType)
    : QStandardItem(), m_plotItem(item), m_itemType(plotItemType)
{
    // 设置节点类型角色
    setData(DAFigureTreeModel::NodeTypePlotItem, DAFigureTreeModel::RoleNodeType);

    if (m_plotItem) {
        // 设置Plot和PlotItem指针
        setData(QVariant::fromValue(reinterpret_cast< quintptr >(item->plot())), DAFigureTreeModel::RolePlot);
        setData(QVariant::fromValue(reinterpret_cast< quintptr >(item)), DAFigureTreeModel::RolePlotItem);
    }

    if (PlotItemVisible == plotItemType) {
        setCheckable(true);
    }
}

DAStandardItemPlotItem::~DAStandardItemPlotItem()
{

}

QVariant DAStandardItemPlotItem::data(int role) const
{
    if (!m_plotItem) {
        return QStandardItem::data(role);
    }

    switch (m_itemType) {
    case PlotItemText:
        return handleItemTextType(role);
    case PlotItemVisible:
        return handleItemVisibleType(role);
    case PlotItemColor:
        return handleItemColorType(role);
    default:
        break;
    }

    return QStandardItem::data(role);
}

/**
 * @brief 处理文字显示
 * @param role
 * @return
 */
QVariant DAStandardItemPlotItem::handleItemTextType(int role) const
{
    if (!m_plotItem) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            return m->plotItemName(m_plotItem);
        }
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            return m->plotItemIcon(m_plotItem);
        }
        return QVariant();
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

/**
 * @brief 处理可见性显示
 * @param role
 * @return
 */
QVariant DAStandardItemPlotItem::handleItemVisibleType(int role) const
{
    if (!m_plotItem) {
        return QVariant();
    }
    static QIcon s_icon_not_visible(":/DAFigure/icon/chartitem-invisible.svg");
    static QIcon s_icon_visible(":/DAFigure/icon/chartitem-visible.svg");

    switch (role) {
    case Qt::DisplayRole: {
        return QVariant();
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        if (m_plotItem->isVisible()) {
            return s_icon_visible;
        } else {
            return s_icon_not_visible;
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

/**
 * @brief 处理颜色显示
 * @param role
 * @return
 */
QVariant DAStandardItemPlotItem::handleItemColorType(int role) const
{
    if (!m_plotItem) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        return QVariant();
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            QBrush brush = DAChartUtil::getPlotItemBrush(m_plotItem);
            return m->brushIcon(brush);
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

}//end DA
