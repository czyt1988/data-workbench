#include "DAStandardItemPlotItem.h"
#include "qwt_plot_item.h"
#include "DAFigureTreeModel.h"
#include "DAChartUtil.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param item 关联的QwtPlotItem指针
 * @param plotItemType 项目类型
 */
DAStandardItemPlotItem::DAStandardItemPlotItem(QwtPlotItem* item, ItemType plotItemType)
    : QStandardItem(), mPlotItem(item), mItemType(plotItemType)
{
    setEditable(false);
    // 设置节点类型角色
    setData(DAFigureTreeModel::NodeTypePlotItem, DAFigureTreeModel::RoleNodeType);

    if (mPlotItem) {
        // 设置Plot和PlotItem指针
        setData(QVariant::fromValue(reinterpret_cast< quintptr >(item->plot())), DAFigureTreeModel::RolePlot);
        setData(QVariant::fromValue(reinterpret_cast< quintptr >(item)), DAFigureTreeModel::RolePlotItem);
    }
}

/**
 * @brief 析构函数
 */
DAStandardItemPlotItem::~DAStandardItemPlotItem()
{
}

/**
 * @brief 获取项目数据
 * @param role 数据角色
 * @return 对应角色的数据
 */
QVariant DAStandardItemPlotItem::data(int role) const
{
    if (!mPlotItem) {
        return QStandardItem::data(role);
    }

    switch (mItemType) {
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
    if (!mPlotItem) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            return m->generatePlotItemName(mPlotItem);
        }
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            return m->generatePlotItemIcon(mPlotItem);
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
    if (!mPlotItem) {
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
        if (mPlotItem->isVisible()) {
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
    if (!mPlotItem) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        return QVariant();
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            QBrush brush = DAChartUtil::getPlotItemBrush(mPlotItem);
            return m->generateBrushIcon(brush);
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

}  // end DA
