#include "DAStandardItemPlotItem.h"
#include "DAPlotUtils.h"
#include "plot/QImPlotItemNode.h"
namespace DA
{

DAStandardItemPlotItem::DAStandardItemPlotItem(QIM::QImPlotItemNode* item, ItemType plotItemType)
    : QStandardItem(), m_plotItem(item), m_itemType(plotItemType)
{
    setEditable(false);
    // 设置节点类型角色
    setData(static_cast< int >(DAPlotTreeItemType::PlotItem), static_cast< int >(DAPlotTreeItemRole::RoleItemType));

    if (m_plotItem) {
        // 设置PlotItem指针
        setData(QVariant::fromValue(reinterpret_cast< quintptr >(item)), static_cast< int >(DAPlotTreeItemRole::RoleInnerPointer));
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
    case PlotItemProperty:
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
        QString t = m_plotItem->label();
        if (t.isEmpty()) {
            return QObject::tr("untitled plot item");
        }
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        DAPlotUtils::toIcon(m_plotItem);
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
        QColor c = m_plotItem->itemColor();
        if (c.isValid()) {
            return c;
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

}  // end DA
