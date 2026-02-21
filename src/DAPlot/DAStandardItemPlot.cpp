#include "DAStandardItemPlot.h"
#include "plot/QImPlotNode.h"
namespace DA
{
DAStandardItemPlot::DAStandardItemPlot(QIM::QImPlotNode* plot, ItemType plotType)
    : QStandardItem(), m_plot(plot), m_itemType(plotType)
{
    setData(static_cast< int >(DAPlotTreeItemType::Plot), static_cast< int >(DAPlotTreeItemRole::RoleItemType));
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), static_cast< int >(DAPlotTreeItemRole::RoleInnerPointer));
    setEditable(false);
}

DAStandardItemPlot::~DAStandardItemPlot()
{
}

QVariant DAStandardItemPlot::data(int role) const
{
    if (!isValid()) {
        return QStandardItem::data(role);
    }

    switch (m_itemType) {
    case PlotText:
        return handleItemTextType(role);
    case PlotVisible:
        return handleItemVisibleType(role);
    case PlotProperty:
        return handlePlotPropertyType(role);
    default:
        break;
    }

    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot::handleItemTextType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    static QIcon s_plot_icon = QIcon(":/DAFigure/icon/layout.svg");
    switch (role) {
    case Qt::DisplayRole: {
        return m_plot->title();
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        return s_plot_icon;
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot::handleItemVisibleType(int role) const
{
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot::handlePlotPropertyType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        if (!m_plot) {
            return QVariant();
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

}  // end DA
